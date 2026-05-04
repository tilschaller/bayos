use flate2;
use sha2::{Digest, Sha256};
use std::{
    env, fs,
    io::{self, BufReader, Read, Write},
    path::{Path, PathBuf},
    process::Command,
};

enum ArchiveFormat {
    TarXz,
    TarGz,
}

struct Source {
    name: &'static str,
    url: &'static str,
    sha256: &'static str,
    dir_name: &'static str,
    format: ArchiveFormat,
}

const SOURCES: &[Source] = &[
    Source {
        name: "gcc",
        url: "https://ftp.fu-berlin.de/unix/languages/gcc/releases/gcc-15.2.0/gcc-15.2.0.tar.xz",
        sha256: "438fd996826b0c82485a29da03a72d71d6e3541a83ec702df4271f6fe025d24e",
        dir_name: "gcc-15.2.0",
        format: ArchiveFormat::TarXz,
    },
    Source {
        name: "binutils",
        url: "https://ftp.fu-berlin.de/unix/gnu/binutils/binutils-2.46.0.tar.xz",
        sha256: "d75a94f4d73e7a4086f7513e67e439e8fcdcbb726ffe63f4661744e6256b2cf2",
        dir_name: "binutils-2.46.0",
        format: ArchiveFormat::TarXz,
    },
    Source {
        name: "mlibc",
        url: "https://github.com/managarm/mlibc/archive/refs/tags/v6.3.1.tar.gz",
        sha256: "9236025b32636b2e79cdd00b5914b5e89c3423da257c2db24253e0cac1679444",
        dir_name: "mlibc-6.3.1",
        format: ArchiveFormat::TarGz,
    },
];

fn main() {
    println!("cargo:rerun-if-changed=build.rs");
    println!("cargo:rerun-if-changed=binutils.patch");
    println!("cargo:rerun-if-changed=gcc.patch");
    println!("cargo:rerun-if-changed=mlibc.patch");

    let manifest_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());
    let cache_dir = manifest_dir.join("cache");
    fs::create_dir_all(&cache_dir).expect("failed to create cache dir");

    for src in SOURCES {
        let tarball = cache_dir.join(match src.format {
            ArchiveFormat::TarXz => format!("{}.tar.xz", src.name),
            ArchiveFormat::TarGz => format!("{}.tar.gz", src.name),
        });
        let src_dir = cache_dir.join(src.dir_name);

        if tarball.exists() {
            println!("cargo:warning=[{}] tarball cached, verifying...", src.name);
            if verify_sha256(&tarball, src.sha256) {
                println!("cargo:warning=[{}] checksum OK", src.name);
            } else {
                println!(
                    "cargo:warning=[{}] checksum MISMATCH - redownloading",
                    src.name
                );
                fs::remove_file(&tarball).unwrap();
                download(src.url, &tarball);
                assert_sha256(&tarball, src.sha256, src.name);
            }
        } else {
            println!("cargo:warning=[{}] downloading from {}", src.name, src.url);
            download(src.url, &tarball);
            assert_sha256(&tarball, src.sha256, src.name);
        }

        if src_dir.exists() {
            println!("cargo:warning=[{}] source dir already extracted", src.name);
        } else {
            println!("cargo:warning=[{}] extracting…", src.name);
            extract_archive(&tarball, &cache_dir, &src.format);
            println!("cargo:warning=[{}] extracted to {:?}", src.name, src_dir);

            let patch_file = manifest_dir.join(format!("{}.patch", src.name));
            if patch_file.exists() {
                apply_patch(&patch_file, &src_dir);
            } else {
                println!("cargo:warning=[{}] patch file not found", src.name);
            }
        }
    }

    // next we create the folders
    let sysroot_dir = manifest_dir.join("sysroot");
    let tools_dir = manifest_dir.join("tools");
    fs::create_dir_all(&sysroot_dir).expect("failed to create sysroot dir");
    fs::create_dir_all(&tools_dir).expect("failed to create tools dir");

    // install the headers
    let mlibc_dir = cache_dir.join("mlibc-6.3.1");
    let status = Command::new("meson")
        .args([
            "setup",
            "--cross-file=mlibc-bayos.txt",
            "--prefix=/usr",
            "-Dheaders_only=true",
            "headers-build",
        ])
        .current_dir(&mlibc_dir)
        .status()
        .expect("meson setup failed");
    assert!(status.success(), "meson setup failed");

    let status = Command::new("ninja")
        .args(["-C", "headers-build", "install"])
        .current_dir(&mlibc_dir)
        .env("DESTDIR", &sysroot_dir)
        .status()
        .expect("ninja install failed");
    assert!(status.success(), "ninja install failed");

    // compile binutils
    let binutils_dir = cache_dir.join("binutils-2.46.0").join("build");
    fs::create_dir_all(&binutils_dir).expect("failed to create binutils build dir");

    let status = Command::new("../configure")
        .args([
            "--target=x86_64-bayos",
            "--prefix=/usr",
            &format!("--with-sysroot={}", sysroot_dir.display()),
            "--disable-werror",
            "--enable-default-execstack=no",
        ])
        .current_dir(&binutils_dir)
        .status()
        .expect("configuring bintutils failed");
    assert!(status.success(), "configuring binutils failed");

    let nproc = std::thread::available_parallelism()
        .map(|n| n.get())
        .unwrap_or(1);

    let status = Command::new("make")
        .arg(format!("-j{}", nproc))
        .current_dir(&binutils_dir)
        .status()
        .expect("make failed");
    assert!(status.success(), "make failed");

    let status = Command::new("make")
        .arg("install")
        .env("DESTDIR", &tools_dir)
        .current_dir(&binutils_dir)
        .status()
        .expect("make install failed");
    assert!(status.success(), "make install failed");

    let gcc_dir = cache_dir.join("gcc-15.2.0").join("build");
    fs::create_dir_all(&gcc_dir).expect("failed to create gcc build dir");
    let status = Command::new("../configure")
        .args([
            "--target=x86_64-bayos",
            "--prefix=/usr",
            &format!("--with-sysroot={}", sysroot_dir.display()),
            "--enable-languages=c,c++",
            "--enable-threads=posix",
            "--disable-multilib",
            "--enable-shared",
            "--enable-host-shared",
        ])
        .current_dir(&gcc_dir)
        .env(
            "PATH",
            format!(
                "{}/usr/bin:{}",
                tools_dir.display(),
                std::env::var("PATH").unwrap_or_default()
            ),
        )
        .status()
        .expect("gcc configure failed");
    assert!(status.success(), "gcc configure failed");

    let status = Command::new("make")
        .args([&format!("-j{}", nproc), "all-gcc", "all-target-libgcc"])
        .current_dir(&gcc_dir)
        .env(
            "PATH",
            format!(
                "{}/usr/bin:{}",
                tools_dir.display(),
                std::env::var("PATH").unwrap_or_default()
            ),
        )
        .status()
        .expect("make gcc failed");
    assert!(status.success(), "make gcc failed");
    let status = Command::new("make")
        .args(["install-gcc", "install-target-libgcc"])
        .current_dir(&gcc_dir)
        .env("DESTDIR", &tools_dir)
        .env(
            "PATH",
            format!(
                "{}/usr/bin:{}",
                tools_dir.display(),
                std::env::var("PATH").unwrap_or_default()
            ),
        )
        .status()
        .expect("make install gcc failed");
    assert!(status.success(), "make install gcc failed");

    let status = Command::new("meson")
        .args([
            "setup",
            "--cross-file=mlibc-bayos.txt",
            "--prefix=/usr",
            "-Ddefault_library=static",
            "-Dno_headers=true",
            "build",
        ])
        .current_dir(&mlibc_dir)
        .env(
            "PATH",
            format!(
                "{}/usr/bin:{}",
                tools_dir.display(),
                std::env::var("PATH").unwrap_or_default()
            ),
        )
        .status()
        .expect("meson setup mlibc failed");
    assert!(status.success(), "meson setup mlibc failed");

    let status = Command::new("ninja")
        .args(["-C", "build", "install"])
        .current_dir(&mlibc_dir)
        .env("DESTDIR", &sysroot_dir)
        .env(
            "PATH",
            format!(
                "{}/usr/bin:{}",
                tools_dir.display(),
                std::env::var("PATH").unwrap_or_default()
            ),
        )
        .status()
        .expect("ninja install mlibc failed");
    assert!(status.success(), "ninja install mlibc failed");
}

fn download(url: &str, dest: &Path) {
    let resp = ureq::get(url)
        .call()
        .unwrap_or_else(|e| panic!("HTTP GET {url} failed: {e}"));

    assert!(
        resp.status() == 200,
        "unexpected HTTP status {} for {url}",
        resp.status()
    );

    let mut file =
        fs::File::create(dest).unwrap_or_else(|e| panic!("cannot create {:?}: {e}", dest));

    let mut reader = resp.into_body().into_reader();
    let mut buf = [0u8; 65536];
    let mut total = 0u64;

    loop {
        let n = reader.read(&mut buf).expect("read error during download");
        if n == 0 {
            break;
        }
        file.write_all(&buf[..n])
            .expect("write error during download");
        total += n as u64;
        // progress visible in `cargo build -vv`
        if total % (1024 * 1024 * 50) < 65536 {
            println!("cargo:warning=  … {:.0} MB", total as f64 / 1_048_576.0);
        }
    }

    file.flush().unwrap();
    println!(
        "cargo:warning=  download complete ({:.1} MB)",
        total as f64 / 1_048_576.0
    );
}

fn sha256_of(path: &Path) -> String {
    let f = fs::File::open(path).unwrap_or_else(|e| panic!("cannot open {:?}: {e}", path));
    let mut reader = BufReader::new(f);
    let mut hasher = Sha256::new();
    let mut buf = [0u8; 65536];
    loop {
        let n = reader.read(&mut buf).expect("read error");
        if n == 0 {
            break;
        }
        hasher.update(&buf[..n]);
    }
    hex::encode(hasher.finalize())
}

fn verify_sha256(path: &Path, expected: &str) -> bool {
    // skip verification when placeholder is still in place
    if expected.starts_with("REPLACE_WITH") {
        return true;
    }
    sha256_of(path) == expected
}

fn assert_sha256(path: &Path, expected: &str, label: &str) {
    if expected.starts_with("REPLACE_WITH") {
        // print the real digest so the developer can fill it in
        println!(
            "cargo:warning=[{}] SHA-256 (paste into build.rs): {}",
            label,
            sha256_of(path)
        );
        return;
    }
    let got = sha256_of(path);
    assert!(
        got == expected,
        "[{label}] SHA-256 mismatch!\n  expected: {expected}\n  got:      {got}"
    );
}

fn extract_archive(tarball: &Path, dest_dir: &Path, format: &ArchiveFormat) {
    let file = fs::File::open(tarball)
        .unwrap_or_else(|e| panic!("cannot open tarball {:?}: {e}", tarball));

    let buf = BufReader::new(file);

    match format {
        ArchiveFormat::TarXz => {
            let xz = xz2::read::XzDecoder::new(buf);
            unpack_tar(tar::Archive::new(xz), dest_dir);
        }
        ArchiveFormat::TarGz => {
            let gz = flate2::read::GzDecoder::new(buf);
            unpack_tar(tar::Archive::new(gz), dest_dir);
        }
    }
}

fn unpack_tar<R: Read>(mut ar: tar::Archive<R>, dest_dir: &Path) {
    ar.set_preserve_permissions(true);
    ar.set_overwrite(true);
    ar.unpack(dest_dir)
        .unwrap_or_else(|e| panic!("extraction into {:?} failed: {e}", dest_dir));
}

fn apply_patch(patch_file: &Path, src_dir: &Path) {
    println!(
        "cargo:warning=applying patch {:?} to {:?}",
        patch_file, src_dir
    );

    let output = Command::new("patch")
        .args([
            "-p1",       // strip one leading path component (standard for git patches)
            "--forward", // skip already-applied hunks instead of erroring
            "--batch",   // never prompt interactively
            "-i",
            patch_file.to_str().unwrap(),
        ])
        .current_dir(src_dir) // run inside the source tree
        .output()
        .expect("failed to run `patch` — is it installed?");

    // always print stdout/stderr so failures are visible in `cargo build -vv`
    if !output.stdout.is_empty() {
        for line in String::from_utf8_lossy(&output.stdout).lines() {
            println!("cargo:warning=[patch] {}", line);
        }
    }
    if !output.stderr.is_empty() {
        for line in String::from_utf8_lossy(&output.stderr).lines() {
            println!("cargo:warning=[patch] {}", line);
        }
    }

    assert!(
        output.status.success(),
        "patch failed for {:?} — see warnings above",
        patch_file
    );

    println!(
        "cargo:warning=[patch] {:?} applied successfully",
        patch_file
    );
}
