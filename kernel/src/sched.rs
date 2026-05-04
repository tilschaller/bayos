use crate::elf;
use crate::memory::BitmapAllocator;
use alloc::boxed::Box;
use alloc::sync::Arc;
use alloc::vec::Vec;
use conquer_once::spin::OnceCell;
use core::ops::DerefMut;
use core::slice::from_raw_parts_mut;
use spinning_top::{RwSpinlock, Spinlock};
use x86_64::PrivilegeLevel;
use x86_64::VirtAddr;
use x86_64::registers::rflags::RFlags;
use x86_64::structures::gdt::SegmentSelector;
use x86_64::structures::idt::InterruptStackFrameValue;
use x86_64::structures::paging::FrameAllocator;
use x86_64::structures::paging::Mapper;
use x86_64::structures::paging::OffsetPageTable;
use x86_64::structures::paging::Page;
use x86_64::structures::paging::PageTableFlags;
use x86_64::structures::paging::Size4KiB;
use x86_64::structures::paging::mapper::MapToError;

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

// adds a user process to the address space of mapper
// using frame allocator passed
// efi is the program
pub fn add_user_process(
    efi: u64,
    mapper: Arc<Spinlock<OffsetPageTable<'static>>>,
    frame_allocator: Arc<Spinlock<BitmapAllocator>>,
) -> Result<(), MapToError<Size4KiB>> {
    let mut mapper = mapper.lock();
    let mut frame_allocator = frame_allocator.lock();

    let elf_header = efi as *mut elf::Header;
    let elf_programs = unsafe {
        from_raw_parts_mut(
            (efi + (*elf_header).e_phoff) as *mut elf::Program,
            (*elf_header).e_phnum as usize,
        )
    };

    for p in elf_programs {
        if p.p_type == 1 {
            let page_range = {
                let start = VirtAddr::new(p.p_vaddr);
                let end = VirtAddr::new(p.p_vaddr + p.p_memsz - 1u64);
                let start_page = Page::containing_address(start);
                let end_page = Page::containing_address(end);
                Page::range_inclusive(start_page, end_page)
            };

            for page in page_range {
                let frame = frame_allocator
                    .allocate_frame()
                    .ok_or(MapToError::FrameAllocationFailed)?;
                let flags = PageTableFlags::PRESENT
                    | PageTableFlags::WRITABLE
                    | PageTableFlags::USER_ACCESSIBLE;
                unsafe {
                    mapper
                        .map_to(page, frame, flags, frame_allocator.deref_mut())?
                        .flush()
                };
            }

            unsafe {
                core::ptr::copy_nonoverlapping(
                    (efi + p.p_offset) as *const u8,
                    p.p_vaddr as *mut u8,
                    p.p_memsz as usize,
                );

if p.p_memsz > p.p_filesz {
        core::ptr::write_bytes(
            (p.p_vaddr + p.p_filesz) as *mut u8,
            0,
            (p.p_memsz - p.p_filesz) as usize,
        );
    }
            };
        }
    }

    let page_range = {
        let start = VirtAddr::new(0x200000 - 0x5000);
        let end = VirtAddr::new(0x200000 - 1);
        let start_page = Page::containing_address(start);
        let end_page = Page::containing_address(end);
        Page::range_inclusive(start_page, end_page)
    };

    for page in page_range {
        let frame = frame_allocator
            .allocate_frame()
            .ok_or(MapToError::FrameAllocationFailed)?;
        let flags =
            PageTableFlags::PRESENT | PageTableFlags::WRITABLE | PageTableFlags::USER_ACCESSIBLE;
        unsafe {
            mapper
                .map_to(page, frame, flags, frame_allocator.deref_mut())?
                .flush()
        };
    }

    let cpu_flags = x86_64::registers::rflags::read() | RFlags::INTERRUPT_FLAG;

    let stack: u64 = 0x200000 - 0x4000;
    let stack_frame = (stack + 120 + 8) as *mut InterruptStackFrameValue;
    let tcb = (0x200000 - 0x5000) as *mut u64;

    unsafe {
        (*stack_frame).instruction_pointer = VirtAddr::new((*elf_header).e_entry);
        (*stack_frame).code_segment = SegmentSelector::new(6, PrivilegeLevel::Ring3);
        (*stack_frame).cpu_flags = cpu_flags;
        (*stack_frame).stack_pointer = VirtAddr::new(0x200000);
        (*stack_frame).stack_segment = SegmentSelector::new(5, PrivilegeLevel::Ring3);
        *tcb = 0x200000 - 0x5000;
    }

    x86_64::instructions::interrupts::without_interrupts(|| {
        let mut proc_list = PROCESS_LIST.get().unwrap().lock();

        proc_list.push(Process {
            _id: 2,
            status: Status::READY,
            rsp: stack + 8,
            _kernel_stack: None,
        });
    });

    Ok(())
}

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
