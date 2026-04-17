#![no_std]
#![no_main]
#![feature(abi_x86_interrupt)]
#![feature(ptr_cast_array)] // this is used only once and can be removed

extern crate alloc;

pub mod acpi;
pub mod allocator;
pub mod framebuffer;
pub mod gdt;
pub mod int;
pub mod memory;
pub mod sched;
pub mod syscall;

use ::acpi::{AcpiTables, platform::AcpiPlatform};
use alloc::sync::Arc;
use bootloader_api::{BootInfo, BootloaderConfig, config::Mapping, entry_point};
use spinning_top::Spinlock;
use x86_64::VirtAddr;

pub static BOOTLOADER_CONFIG: BootloaderConfig = {
    let mut config = BootloaderConfig::new_default();
    config.mappings.dynamic_range_start = Some(0xffff_8000_0000_0000);
    config.mappings.physical_memory = Some(Mapping::Dynamic);
    config.mappings.kernel_base = Mapping::FixedAddress(0xffff_ffff_8000_0000);
    config
};

entry_point!(kernel_main, config = &BOOTLOADER_CONFIG);

fn kernel_main(boot_info: &'static mut BootInfo) -> ! {
    let fb = boot_info.framebuffer.take().unwrap();
    let logger = framebuffer::LOGGER.get_or_init(move || {
        let info = fb.info();
        framebuffer::Logger::new(fb.into_buffer(), info)
    });
    log::set_logger(logger).expect("logger already set");
    log::set_max_level(log::LevelFilter::Info);
    log::info!("BAYOS");

    gdt::init();
    log::info!("GDT [OK]");

    int::init();
    log::info!("interrupts [OK]");

    let phys_memory_offset = VirtAddr::new(
        boot_info
            .physical_memory_offset
            .into_option()
            .expect("physical memory was not mapped"),
    );
    let mut mapper = unsafe { memory::init(phys_memory_offset) };
    let mut frame_allocator =
        unsafe { memory::BootInfoFrameAllocator::init(&boot_info.memory_regions) };
    log::info!("frame allocator [OK]");

    allocator::init_heap(&mut mapper, &mut frame_allocator).expect("heap initialization failed");
    log::info!("heap [OK]");

    let mapper = Arc::new(Spinlock::new(mapper));

    let acpi_handler = acpi::AcpiHandlerStruct::new(mapper.clone());
    let acpi_tables = unsafe {
        AcpiTables::from_rsdp(
            acpi_handler.clone(),
            boot_info
                .rsdp_addr
                .take()
                .expect("bootloader found no rsdp address") as usize,
        )
        .expect("could not create acpi tables from rsdp")
    };
    let acpi_platform = AcpiPlatform::new(acpi_tables, acpi_handler.clone())
        .expect("could not collect information about platform using acpi");
    int::init_apic(acpi_platform.interrupt_model, mapper.clone());
    log::info!("ACPI [OK]");

    sched::init();
    log::info!("scheduler [OK]");

    let _frame_allocator = memory::BitmapAllocator::init(frame_allocator);
    
    syscall::init();
    log::info!("syscalls [OK]");

    log::info!("starting scheduler!");

    x86_64::instructions::interrupts::enable();
    
    sched::add_process(hcf);

    loop {
        x86_64::instructions::hlt();
    }
}

#[panic_handler]
fn panic(info: &core::panic::PanicInfo) -> ! {
    x86_64::instructions::interrupts::disable();
    log::error!("{:?}", info);
    hcf();
}

fn hcf() -> ! {
    x86_64::instructions::interrupts::disable();
    loop {
        x86_64::instructions::hlt();
    }
}
