use crate::memory;
use acpi::{Handle, Handler, PciAddress, PhysicalMapping, aml::AmlError};
use alloc::sync::Arc;
use spinning_top::Spinlock;
use x86_64::structures::paging::OffsetPageTable;

#[derive(Clone)]
pub struct AcpiHandlerStruct {
    mapper: Arc<Spinlock<OffsetPageTable<'static>>>,
}

impl AcpiHandlerStruct {
    pub const fn new(mapper: Arc<Spinlock<OffsetPageTable<'static>>>) -> Self {
        AcpiHandlerStruct { mapper }
    }
}

impl Handler for AcpiHandlerStruct {
    unsafe fn map_physical_region<T>(
        &self,
        physical_address: usize,
        size: usize,
    ) -> PhysicalMapping<Self, T> {
        // all of the physical memory is already mapped
        PhysicalMapping {
            physical_start: physical_address,
            // not really pretty
            virtual_start: memory::get_virtual_address(self.mapper.clone(), physical_address),
            region_length: size,
            mapped_length: size,
            handler: { self.clone() },
        }
    }

    fn unmap_physical_region<T>(_region: &PhysicalMapping<Self, T>) {
        // doesnt need to be unmapped,
        // all of the physical memory is mapped at all time anyways
    }

    fn read_u8(&self, _address: usize) -> u8 {
        todo!()
    }

    fn read_u16(&self, _address: usize) -> u16 {
        todo!()
    }

    fn read_u32(&self, _address: usize) -> u32 {
        todo!()
    }

    fn read_u64(&self, _address: usize) -> u64 {
        todo!()
    }

    fn write_u8(&self, _address: usize, _value: u8) {
        todo!()
    }

    fn write_u16(&self, _address: usize, _value: u16) {
        todo!()
    }

    fn write_u32(&self, _address: usize, _value: u32) {
        todo!()
    }

    fn write_u64(&self, _address: usize, _value: u64) {
        todo!()
    }

    fn read_io_u8(&self, _address: u16) -> u8 {
        todo!()
    }

    fn read_io_u16(&self, _address: u16) -> u16 {
        todo!()
    }

    fn read_io_u32(&self, _address: u16) -> u32 {
        todo!()
    }

    fn write_io_u8(&self, _port: u16, _value: u8) {
        todo!()
    }

    fn write_io_u16(&self, _port: u16, _value: u16) {
        todo!()
    }

    fn write_io_u32(&self, _port: u16, _value: u32) {
        todo!()
    }

    fn read_pci_u8(&self, _address: PciAddress, _offset: u16) -> u8 {
        todo!()
    }

    fn read_pci_u16(&self, _address: PciAddress, _offset: u16) -> u16 {
        todo!()
    }

    fn read_pci_u32(&self, _address: PciAddress, _offset: u16) -> u32 {
        todo!()
    }

    fn write_pci_u8(&self, _address: PciAddress, _offset: u16, _value: u8) {
        todo!()
    }

    fn write_pci_u16(&self, _address: PciAddress, _offset: u16, _value: u16) {
        todo!()
    }

    fn write_pci_u32(&self, _address: PciAddress, _offset: u16, _value: u32) {
        todo!()
    }

    fn nanos_since_boot(&self) -> u64 {
        todo!()
    }

    fn stall(&self, _microseconds: u64) {
        todo!()
    }

    fn sleep(&self, _milliseconds: u64) {
        todo!()
    }

    fn create_mutex(&self) -> Handle {
        todo!()
    }

    fn acquire(&self, _mutex: Handle, _timeout: u16) -> Result<(), AmlError> {
        todo!()
    }

    fn release(&self, _mutex: Handle) {
        todo!()
    }
}
