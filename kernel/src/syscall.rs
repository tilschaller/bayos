use x86_64::registers::model_specific::{Efer, LStar, Star, SFMask};
use x86_64::registers::rflags::RFlags;
use x86_64::VirtAddr;

pub fn init() {
    unsafe {
        Efer::write_raw(Efer::read_raw() | 1);
    }

    LStar::write(VirtAddr::from_ptr(syscall_handler as *const fn() -> !));

    unsafe {
        Star::write_raw(0x18, 0x8);
    } 

    SFMask::write(RFlags::DIRECTION_FLAG);
}

use core::arch::naked_asm;

#[unsafe(naked)]
extern "C" fn syscall_entry() -> ! {
    naked_asm!(
        "push rax",
        "push rcx",
        "push rdx",
        "push rbx",
        "push rbp",
        "push rsi",
        "push rdi",

        "call {syscall_handler}",

        "pop rdi",
        "pop rsi",
        "pop rbp",
        "pop rbx",
        "pop rdx",
        "pop rcx",
        "pop rax",

        "sysret",
        syscall_handler = sym syscall_handler,
    );
}

extern "C" fn syscall_handler() {
    log::info!("in syscall");
}
