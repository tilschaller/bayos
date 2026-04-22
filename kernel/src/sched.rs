use alloc::boxed::Box;
use alloc::vec::Vec;
use conquer_once::spin::OnceCell;
use spinning_top::{RwSpinlock, Spinlock};
use x86_64::PrivilegeLevel;
use x86_64::VirtAddr;
use x86_64::registers::rflags::RFlags;
use x86_64::structures::gdt::SegmentSelector;
use x86_64::structures::idt::InterruptStackFrameValue;

#[derive(PartialEq)]
pub enum Status {
    READY,
    RUNNING,
    DEAD,
}

pub struct Process {
    _id: u64,
    status: Status,
    rsp: u64,
    _kernel_stack: Option<Box<[u8; 0x4000]>>,
}

pub static PROCESS_LIST: OnceCell<Spinlock<Vec<Process>>> = OnceCell::uninit();
pub static CURRENT_PROCESS: OnceCell<RwSpinlock<usize>> = OnceCell::uninit();

pub fn init() {
    PROCESS_LIST.init_once(|| {
        Spinlock::new({
            let mut vec = Vec::new();
            vec.push(Process {
                _id: 0,
                status: Status::RUNNING,
                rsp: 0,
                _kernel_stack: None,
            });
            vec
        })
    });
    CURRENT_PROCESS.init_once(|| RwSpinlock::new(0));
}

pub fn add_process(entry: fn() -> !) {
    x86_64::instructions::interrupts::without_interrupts(|| {
        let mut proc_list = PROCESS_LIST.get().unwrap().lock();

        let cpu_flags = x86_64::registers::rflags::read() | RFlags::INTERRUPT_FLAG;
        let kernel_stack: Box<[u8; 0x4000]> = Box::new([0; 0x4000]);

        let stack_frame = unsafe {
            core::mem::transmute::<*const u8, *mut InterruptStackFrameValue>(
                kernel_stack.as_ptr().wrapping_add(120),
            )
        };

        unsafe {
            (*stack_frame).instruction_pointer = VirtAddr::from_ptr(entry as *const fn() -> !);
            (*stack_frame).code_segment = SegmentSelector::new(1, PrivilegeLevel::Ring0);
            (*stack_frame).cpu_flags = cpu_flags;
            (*stack_frame).stack_pointer =
                VirtAddr::from_ptr(kernel_stack.as_ptr().wrapping_add(kernel_stack.len()));
            (*stack_frame).stack_segment = SegmentSelector::new(2, PrivilegeLevel::Ring0);
        }

        proc_list.push(Process {
            _id: 1,
            status: Status::READY,
            rsp: kernel_stack.as_ptr() as u64,
            _kernel_stack: Some(kernel_stack),
        })
    });
}

// delete the current process
pub fn delete_process(index: usize) {
    x86_64::instructions::interrupts::without_interrupts(|| {
        let mut proc_list = PROCESS_LIST.get().unwrap().lock();

        proc_list.remove(index);
    });
}

pub fn mark_current_as_dead() {
    x86_64::instructions::interrupts::without_interrupts(|| {
        let mut proc_list = PROCESS_LIST.get().unwrap().lock();
        let index = CURRENT_PROCESS.get().unwrap().read();

        proc_list[*index].status = Status::DEAD;
    });
}

pub extern "C" fn schedule(rsp: u64) -> u64 {
    let mut index = CURRENT_PROCESS.get().unwrap().write();
    let mut proc_list = PROCESS_LIST.get().unwrap().lock();

    proc_list[*index].rsp = rsp;

    if proc_list[*index].status != Status::DEAD {
        proc_list[*index].status = Status::READY;
    } else {
        delete_process(*index);
    }

    loop {
        *index += 1;
        if *index >= proc_list.len() {
            *index = 0;
        }
        if proc_list[*index].status == Status::READY {
            proc_list[*index].status = Status::RUNNING;
            break;
        }
    }

    return proc_list[*index].rsp;
}
