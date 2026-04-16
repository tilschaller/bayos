use alloc::sync::Arc;
use bootloader_api::info::{MemoryRegionKind, MemoryRegions};
use core::ptr::NonNull;
use spinning_top::Spinlock;
use x86_64::{
    PhysAddr, VirtAddr,
    structures::paging::{FrameAllocator, FrameDeallocator, OffsetPageTable, PageTable, PhysFrame, Size4KiB},
};
use bitvec::prelude::*;

pub unsafe fn init(physical_memory_offset: VirtAddr) -> OffsetPageTable<'static> {
    unsafe {
        let level_4_table = active_level_4_table(physical_memory_offset);
        OffsetPageTable::new(level_4_table, physical_memory_offset)
    }
}

pub fn get_virtual_address<T>(
    mapper: Arc<Spinlock<OffsetPageTable<'static>>>,
    physical_address: usize,
) -> NonNull<T> {
    NonNull::new(
        (mapper.lock().phys_offset().as_mut_ptr::<T>() as *mut u8).wrapping_add(physical_address)
            as *mut T,
    )
    .unwrap()
}

unsafe fn active_level_4_table(physical_memory_offset: VirtAddr) -> &'static mut PageTable {
    use x86_64::registers::control::Cr3;

    let (level_4_table_frame, _) = Cr3::read();

    let phys = level_4_table_frame.start_address();
    let virt = physical_memory_offset + phys.as_u64();
    let page_table_ptr: *mut PageTable = virt.as_mut_ptr();

    unsafe { &mut *page_table_ptr }
}

pub struct BootInfoFrameAllocator {
    memory_map: &'static MemoryRegions,
    next: usize,
}

impl BootInfoFrameAllocator {
    pub unsafe fn init(memory_map: &'static MemoryRegions) -> Self {
        BootInfoFrameAllocator {
            memory_map,
            next: 0,
        }
    }

    pub fn frames(&self) -> usize {
        self.usable_frames().count()
    }

    fn usable_frames(&self) -> impl Iterator<Item = PhysFrame> {
        let regions = self.memory_map.iter();
        let usable_regions = regions.filter(|r| r.kind == MemoryRegionKind::Usable);
        let addr_ranges = usable_regions.map(|r| r.start..r.end);
        let frame_addresses = addr_ranges.flat_map(|r| r.step_by(4096));
        frame_addresses.map(|addr| PhysFrame::containing_address(PhysAddr::new(addr)))
    }
}

unsafe impl FrameAllocator<Size4KiB> for BootInfoFrameAllocator {
    fn allocate_frame(&mut self) -> Option<PhysFrame> {
        let frame = self.usable_frames().nth(self.next);
        self.next += 1;
        frame
    }
}

impl FrameDeallocator<Size4KiB> for BootInfoFrameAllocator {
    unsafe fn deallocate_frame(&mut self, _frame: PhysFrame) {
        panic!("Cant deallocate frame using this allocator");
    }
}

pub struct BitmapAllocator {
    memory_map: &'static MemoryRegions,
    bitmap: BitVec<u8, Lsb0>,
}

impl BitmapAllocator {
    pub fn init(old: BootInfoFrameAllocator) -> Self {
        let bitmap_len = old.frames();
        log::info!("creating bitmap allocator with {:x} frames", bitmap_len);

        let mut bitvec = BitVec::repeat(false, bitmap_len);

        for i in 0..old.next {
            bitvec.set(i, true);
        }

        Self {
            memory_map: old.memory_map,
            bitmap: bitvec,
        }
    }

    fn usable_frames(&self) -> impl Iterator<Item = PhysFrame> {
        let regions = self.memory_map.iter();
        let usable_regions = regions.filter(|r| r.kind == MemoryRegionKind::Usable);
        let addr_ranges = usable_regions.map(|r| r.start..r.end);
        let frame_addresses = addr_ranges.flat_map(|r| r.step_by(4096));
        frame_addresses.map(|addr| PhysFrame::containing_address(PhysAddr::new(addr)))
    }
}

unsafe impl FrameAllocator<Size4KiB> for BitmapAllocator {
    fn allocate_frame(&mut self) -> Option<PhysFrame> {
        let index = match self.bitmap.first_zero() {
            Some(i) => i,
            None => return None,
        };
        self.bitmap.set(index, true);
        self.usable_frames().nth(index)
    }
}

impl FrameDeallocator<Size4KiB> for BitmapAllocator {
    unsafe fn deallocate_frame(&mut self, frame: PhysFrame) {
        let index = self.usable_frames().enumerate().find_map(|(i, cmp)| (cmp == frame).then_some(i)).expect("Frame not in memory_map");
        self.bitmap.set(index, false);
    }
}
