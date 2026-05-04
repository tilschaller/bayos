use crate::framebuffer::LOGGER;
use crate::int::INPUT_PIPE;
use crate::vfs::File;
use core::fmt::Write;
use core::slice::from_raw_parts;
use core::str::from_utf8;
use x86_64::VirtAddr;
use x86_64::registers::model_specific::{Efer, LStar, Star, FsBase};

pub fn init() {
    unsafe {
        Efer::write_raw(Efer::read_raw() | 1);
    }

    LStar::write(VirtAddr::from_ptr(syscall_entry as *const fn()));

    unsafe {
        Star::write_raw(0x16, 0x8);
    }

    FsBase::write(VirtAddr::new(0x1fb000));
}

use core::arch::naked_asm;

#[unsafe(naked)]
extern "C" fn syscall_entry() {
    // NOTE: we are using the user stack
    // it would be better to switch a
    // kernel stack beforehand
    naked_asm!(
        // preserve the same registers as linux
        "push rbp",
        "push r15",
        "push r14",
        "push r13",
        "push r12",
        // r11 contains rflags
        "push r11",
        // rcx contains rip
        "push rcx",

        // set up the registers for call to syscall_handler
        // 1. Syscall number: rax -> rdi
        // 2. Argument 0    : rdi -> rsi
        // 3. Argument 1    : rsi -> rdx
        // 4: Argument 2    : rdx -> rcx
        // 5: Argument 3    : r10 -> r8
        // 6: Argument 4    : r8  -> r9
        // 7: Argument 5    : r9  -> on the stack
        // this has to be done in reverse order,
        // otherwise we would overwrite the registers
        // before we can switch them to them
        "push r9",
        "mov r9, r8",
        "mov r8, r10",
        "mov rcx, rdx",
        "mov rdx, rsi",
        "mov rsi, rdi",
        "mov rdi, rax",
        // call the actual syscall handler
        "call {syscall_handler}",
        // clean up the seventh argument
        "pop rax",

        // restore the registers
        "pop rcx",
        "pop r11",
        "pop r12",
        "pop r13",
        "pop r14",
        "pop r15",
        "pop rbp",

        "sysretq",
        syscall_handler = sym syscall_handler,
    );
}

extern "C" fn syscall_handler(
    index: u64,
    arg0: u64,
    arg1: u64,
    arg2: u64,
    _arg3: u64,
    _arg4: u64,
    _arg5: u64,
) {
    match index {
        0 => read_syscall(arg0, arg1 as *mut u8, arg2 as usize),
        1 => write_syscall(arg0, arg1 as *const u8, arg2 as usize),
        _ => log::info!("unknown syscall {:x}", index),
    }
}

fn read_syscall(fd: u64, buf: *mut u8, count: usize) {
    match fd {
        0 => {
            let pipe = INPUT_PIPE.try_get().unwrap();
            pipe.read(0, count, buf);
        }
        _ => (),
    }
}

fn write_syscall(fd: u64, buf: *const u8, count: usize) {
    match fd {
        1 => {
            let buf = unsafe { from_raw_parts(buf, count) };
            let mut writer = LOGGER.get().unwrap().into_inner().lock();
            if let Ok(buffer) = from_utf8(buf) {
                let _ = write!(writer, "{}", buffer);
            }
        }
        _ => (),
    }
}
