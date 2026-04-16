use alloc::boxed::Box;
use alloc::vec::Vec;
use conquer_once::spin::OnceCell;
use spinning_top::{RwSpinlock, Spinlock};
use x86_64::PrivilegeLevel;
use x86_64::VirtAddr;
use x86_64::registers::rflags::RFlags;
use x86_64::structures::gdt::SegmentSelector;
use x86_64::structures::idt::InterruptStackFrame;

#[derive(PartialEq)]
pub enum Status {
    READY,
    RUNNING,
    DEAD,
}

pub struct Process {
    id: u64,
    status: Status,
    pub ctx: InterruptStackFrame,
    kernel_stack: Option<Box<[u8; 0x4000]>>,
}

pub static PROCESS_LIST: OnceCell<Spinlock<Vec<Process>>> = OnceCell::uninit();
pub static CURRENT_PROCESS: OnceCell<RwSpinlock<usize>> = OnceCell::uninit();

pub fn init() {
    PROCESS_LIST.init_once(|| {
        Spinlock::new({
            let mut vec = Vec::new();
            vec.push(Process {
                id: 0,
                status: Status::RUNNING,
                ctx: InterruptStackFrame::new(
                    VirtAddr::zero(),
                    SegmentSelector(0),
                    RFlags::empty(),
                    VirtAddr::zero(),
                    SegmentSelector(0),
                ),
                kernel_stack: None,
            });
            vec
        })
    });
    CURRENT_PROCESS.init_once(|| RwSpinlock::new(0));
}

pub fn add_process(entry: fn() -> !) {
    x86_64::instructions::interrupts::without_interrupts(|| {
        let mut proc_list = PROCESS_LIST.try_get().unwrap().lock();

        let cpu_flags = proc_list[0].ctx.cpu_flags;
        let kernel_stack: Box<[u8; 0x4000]> = Box::new([0; 0x4000]);

        proc_list.push(Process {
            id: 1,
            status: Status::READY,
            ctx: InterruptStackFrame::new(
                VirtAddr::from_ptr(entry as *const fn() -> !),
                SegmentSelector::new(0x8, PrivilegeLevel::Ring0),
                cpu_flags,
                VirtAddr::from_ptr(kernel_stack.as_ptr()),
                SegmentSelector::new(0x10, PrivilegeLevel::Ring0),
            ),
            kernel_stack: Some(kernel_stack),
        });
    });
}

// dont call this directly
pub fn delete_process(index: usize) {
    let mut proc_list = PROCESS_LIST.get().unwrap().lock();

    proc_list.remove(index);
}

pub fn schedule() {
    let mut index = CURRENT_PROCESS.get().unwrap().write();

    let mut proc_list = PROCESS_LIST.get().unwrap().lock();

    if proc_list[*index].status != Status::DEAD {
        proc_list[*index].status = Status::READY;
    } else {
        delete_process(*index);
    }

    loop {
        *index += 1;
        if *index == proc_list.len() {
            *index = 0;
        }
        if proc_list[*index].status == Status::READY {
            proc_list[*index].status = Status::RUNNING;
            break;
        }
    }
}
