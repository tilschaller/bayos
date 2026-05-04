use std::env;
use std::path::PathBuf;
use std::process::Command;

fn main() {
    let manifest_dir = env::var("CARGO_MANIFEST_DIR").unwrap();
    
    let compiler_bin = PathBuf::from(&manifest_dir)
        .join("cross/tools/usr/bin");

    let current_path = env::var("PATH").unwrap_or_default();
    let new_path = format!("{}:{}", compiler_bin.display(), current_path);

    let init_c = PathBuf::from(&manifest_dir).join("init.c");
    let output = PathBuf::from(&manifest_dir).join("init");

    let status = Command::new("x86_64-bayos-gcc")
        .env("PATH", &new_path)
        .args([
            init_c.to_str().unwrap(),
            "-o",
            output.to_str().unwrap(),
        ])
        .status()
        .expect("Failed to run x86_64-bayos-gcc — is it present in cross/tools/usr/bin?");

    if !status.success() {
        panic!("x86_64-bayos-gcc failed to compile init.c");
    }

    println!("cargo:rerun-if-changed=init.c");
    println!("cargo:rerun-if-changed=cross/tools/user/bin/x86_64-bayos-gcc");
    println!("cargo:rerun-if-changed=cross/sysroot/usr/lib/libc.a");
}
