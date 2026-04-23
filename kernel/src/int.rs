use crate::gdt;
use crate::memory;
use crate::sched;
use crate::vfs::File;
use crate::vfs::pipe;
use acpi::platform::InterruptModel;
use alloc::sync::Arc;
use conquer_once::spin::OnceCell;
use core::arch::naked_asm;
use core::ptr::NonNull;
use lazy_static::lazy_static;
use spinning_top::Spinlock;
use x86_64::VirtAddr;
use x86_64::instructions::port::Port;
use x86_64::instructions::port::PortReadOnly;
use x86_64::instructions::port::PortWriteOnly;
use x86_64::structures::idt::InterruptDescriptorTable;
use x86_64::structures::idt::InterruptStackFrame;
use x86_64::structures::idt::PageFaultErrorCode;
use x86_64::structures::paging::OffsetPageTable;

lazy_static! {
    static ref IDT: InterruptDescriptorTable = {
        let mut idt = InterruptDescriptorTable::new();
        idt.divide_error.set_handler_fn(divide_error_handler);
        idt.debug.set_handler_fn(debug_handler);
        idt.non_maskable_interrupt
            .set_handler_fn(non_maskable_interrupt_handler);
        idt.breakpoint.set_handler_fn(breakpoint_handler);
        idt.overflow.set_handler_fn(overflow_handler);
        idt.bound_range_exceeded
            .set_handler_fn(bound_range_exceeded_handler);
        idt.invalid_opcode.set_handler_fn(invalid_opcode_handler);
        idt.device_not_available
            .set_handler_fn(device_not_available_handler);
        unsafe {
            idt.double_fault
                .set_handler_fn(double_fault_handler)
                .set_stack_index(gdt::DOUBLE_FAULT_IST_INDEX);
        }
        idt.invalid_tss.set_handler_fn(invalid_tss_handler);
        idt.segment_not_present
            .set_handler_fn(segment_not_present_handler);
        idt.stack_segment_fault
            .set_handler_fn(stack_segment_fault_handler);
        idt.general_protection_fault
            .set_handler_fn(general_protection_fault_handler);
        idt.page_fault.set_handler_fn(page_fault_handler);
        idt.x87_floating_point
            .set_handler_fn(x87_floating_point_handler);
        idt.alignment_check.set_handler_fn(alignment_check_handler);
        idt.machine_check.set_handler_fn(machine_check_handler);
        idt.simd_floating_point
            .set_handler_fn(simd_floating_point_handler);
        idt.virtualization.set_handler_fn(virtualization_handler);
        idt.cp_protection_exception
            .set_handler_fn(cp_protection_handler);
        idt.hv_injection_exception
            .set_handler_fn(hv_injection_handler);
        idt.vmm_communication_exception
            .set_handler_fn(vmm_communication_handler);
        idt.security_exception.set_handler_fn(security_handler);
        unsafe {
            idt[InterruptIndex::Timer.as_u8()].set_handler_addr(VirtAddr::from_ptr(
                timer_handler as *const extern "C" fn() -> (),
            ));
        }
        idt[InterruptIndex::Keyboard.as_u8()].set_handler_fn(keyboard_handler);
        idt[InterruptIndex::LapicError.as_u8()].set_handler_fn(lapic_error_handler);
        idt[InterruptIndex::Spurious.as_u8()].set_handler_fn(spurious_interrupt_handler);
        idt
    };
}

pub static INPUT_PIPE: OnceCell<pipe::Pipe> = OnceCell::uninit();

pub fn init() {
    IDT.load();
}

static LAPIC: OnceCell<Spinlock<&mut [u32; 252]>> = OnceCell::uninit();

pub fn init_apic(interrupt_model: InterruptModel, mapper: Arc<Spinlock<OffsetPageTable<'static>>>) {
    // disable the legacy PIC
    let mut pic_1_cmd_port: PortWriteOnly<u8> = PortWriteOnly::new(0x20);
    let mut pic_1_data_port: PortWriteOnly<u8> = PortWriteOnly::new(0x21);
    let mut pic_2_cmd_port: PortWriteOnly<u8> = PortWriteOnly::new(0xA0);
    let mut pic_2_data_port: PortWriteOnly<u8> = PortWriteOnly::new(0xA1);

    unsafe {
        pic_1_cmd_port.write(0x11);
        pic_2_cmd_port.write(0x11);

        // remap the pic
        // so spurious interrupt arrives at 0xFF
        pic_1_data_port.write(0xF8);
        pic_2_data_port.write(0xF0);

        pic_1_data_port.write(0x4);
        pic_2_data_port.write(0x2);

        pic_1_data_port.write(0x1);
        pic_2_data_port.write(0x1);

        pic_1_data_port.write(0xff);
        pic_2_data_port.write(0xff);
    }

    let apic = match interrupt_model {
        InterruptModel::Apic(apic) => apic,
        _ => panic!("Interrupt Model is not APIC"),
    };

    let local_apic_address: NonNull<u32> =
        memory::get_virtual_address(mapper.clone(), apic.local_apic_address as usize);
    let mut local_apic_non_null = local_apic_address.cast_array::<252>();
    LAPIC.init_once(|| Spinlock::new(unsafe { local_apic_non_null.as_mut() }));

    let mut lapic = LAPIC.try_get().unwrap().lock();

    // initialize lapic to well known state
    lapic[56] = 0xFFFFFFFF;
    lapic[52] = (lapic[52] & 0xFFFFFF) | 0x1;
    lapic[200] = 0x10000;
    lapic[208] = (4 << 8) | (1 << 16);
    lapic[212] = 0x10000;
    lapic[216] = 0x10000;
    lapic[32] = 0x0;

    // set lapic internal error vector
    lapic[220] = InterruptIndex::LapicError.as_u32() | 0x100;
    // set spurious interrupt vector
    lapic[60] = InterruptIndex::Spurious.as_u32() | 0x100;

    // enable lapic timer as described here
    // https://wiki.osdev.org/APIC_Timer#Enabling_APIC_Timer
    lapic[200] = InterruptIndex::Timer.as_u32();
    lapic[248] = 0x3;

    // configure the pit
    let mut pit_61: Port<u8> = Port::new(0x61);
    let mut pit_43: PortWriteOnly<u8> = PortWriteOnly::new(0x43);
    let mut pit_42: PortWriteOnly<u8> = PortWriteOnly::new(0x42);
    let mut pit_60: PortReadOnly<u8> = PortReadOnly::new(0x60);
    unsafe {
        let val = pit_61.read();
        pit_61.write((val & 0xfd) | 1);
        pit_43.write(0b10110010);
        pit_42.write(0x9b);
        // cause a small delay
        pit_60.read();
        pit_42.write(0x2e);
    }

    lapic[200] |= 0x10000;

    lapic[224] = 0xffffffff;

    // sleep using pit
    unsafe {
        let val = pit_61.read() & 0xfe;
        pit_61.write(val);
        pit_61.write(val | 1);
    }

    // wait until pit is finished
    while unsafe { pit_61.read() } & 0x20 == 0 {}
    // stop lapic timer
    lapic[200] = 0x10000;

    let ticks_in_10ms = 0xffffffff - lapic[228];

    // set apic timer periodic
    lapic[200] = InterruptIndex::Timer.as_u32() | 0x20000;
    lapic[248] = 0x3;
    lapic[224] = ticks_in_10ms;

    let local_apic_id = lapic[8];

    for io_apic in apic.io_apics {
        let mut ioregsel_non_null =
            memory::get_virtual_address(mapper.clone(), io_apic.address as usize);
        let ioregsel: &mut u16 = unsafe { ioregsel_non_null.as_mut() };
        let mut ioregwin_non_null =
            memory::get_virtual_address(mapper.clone(), (io_apic.address + 16) as usize);
        let ioregwin: &mut u32 = unsafe { ioregwin_non_null.as_mut() };

        *ioregsel = 0x1;
        let num_of_irqs: u8 = (((*ioregwin >> 16) & 0xFF) - 1).try_into().unwrap();

        // mask all irqs in ioapic
        for i in 0..num_of_irqs {
            *ioregsel = 0x10 + (i as u16) * 2;
            *ioregwin = 1 << 16;
            *ioregsel = 0x11 + (i as u16) * 2;
            *ioregwin = 0x0;
        }

        if io_apic.global_system_interrupt_base == 0 {
            *ioregsel = 0x12;
            *ioregwin = InterruptIndex::Keyboard.as_u32();
            *ioregsel = 0x13;
            *ioregwin = local_apic_id << 24;
        }
    }

    INPUT_PIPE
        .try_init_once(|| pipe::Pipe::new(128))
        .unwrap();
}

fn send_eoi() {
    let mut lapic = LAPIC.try_get().unwrap().lock();
    lapic[44] = 0x0;
}

extern "x86-interrupt" fn divide_error_handler(stack_frame: InterruptStackFrame) {
    panic!("EXCEPTION: DIVIDE ERROR\n{:#?}", stack_frame);
}

extern "x86-interrupt" fn debug_handler(stack_frame: InterruptStackFrame) {
    log::warn!("EXCEPTION: DEBUG\n{:#?}", stack_frame);
}

extern "x86-interrupt" fn non_maskable_interrupt_handler(stack_frame: InterruptStackFrame) {
    panic!("EXCEPTION: NON MASKABLE INTERRUPT\n{:#?}", stack_frame);
}

extern "x86-interrupt" fn breakpoint_handler(stack_frame: InterruptStackFrame) {
    log::warn!("EXCEPTION: BREAKPOINT\n{:#?}", stack_frame);
}

extern "x86-interrupt" fn overflow_handler(stack_frame: InterruptStackFrame) {
    panic!("EXCEPTION: OVERFLOW\n{:#?}", stack_frame);
}

extern "x86-interrupt" fn bound_range_exceeded_handler(stack_frame: InterruptStackFrame) {
    panic!("EXCEPTION: BOUND RANGE EXCEEDED\n{:#?}", stack_frame);
}

extern "x86-interrupt" fn invalid_opcode_handler(stack_frame: InterruptStackFrame) {
    panic!("EXCEPTION: INVALID OPCODE\n{:#?}", stack_frame);
}

extern "x86-interrupt" fn device_not_available_handler(stack_frame: InterruptStackFrame) {
    panic!("EXCEPTION: DEVICE NOT AVAILABLE\n{:#?}", stack_frame);
}

extern "x86-interrupt" fn double_fault_handler(
    stack_frame: InterruptStackFrame,
    _error_code: u64,
) -> ! {
    panic!("EXCEPTION: DOUBLE FAULT\n{:#?}", stack_frame);
}

extern "x86-interrupt" fn invalid_tss_handler(stack_frame: InterruptStackFrame, _error_code: u64) {
    panic!("EXCEPTION: INVALID TSS\n{:#?}", stack_frame);
}

extern "x86-interrupt" fn segment_not_present_handler(
    stack_frame: InterruptStackFrame,
    error_code: u64,
) {
    panic!(
        "EXCEPTION: SEGMENT NOT PRESENT\nERROR_CODE: {}\n{:#?}",
        error_code, stack_frame
    );
}

extern "x86-interrupt" fn stack_segment_fault_handler(
    stack_frame: InterruptStackFrame,
    _error_code: u64,
) {
    panic!("EXCEPTION: STACK SEGMENT FAULT\n{:#?}", stack_frame);
}

extern "x86-interrupt" fn general_protection_fault_handler(
    stack_frame: InterruptStackFrame,
    error_code: u64,
) {
    panic!(
        "EXCEPTION: GENERAL PROTECTION FAULT\nERROR_CODE: {:x}\n{:#?}",
        error_code, stack_frame
    );
}

extern "x86-interrupt" fn page_fault_handler(
    stack_frame: InterruptStackFrame,
    error_code: PageFaultErrorCode,
) {
    panic!(
        "EXCEPTION: PAGE FAULT\nERROR_CODE: {:#?}\n{:#?}",
        error_code, stack_frame
    );
}

extern "x86-interrupt" fn x87_floating_point_handler(stack_frame: InterruptStackFrame) {
    panic!("EXCEPTION: X87 FLOATING POINT\n{:#?}", stack_frame);
}

extern "x86-interrupt" fn alignment_check_handler(
    stack_frame: InterruptStackFrame,
    _error_code: u64,
) {
    panic!("EXCEPTION: ALIGMENT CHECK\n{:#?}", stack_frame);
}

extern "x86-interrupt" fn machine_check_handler(stack_frame: InterruptStackFrame) -> ! {
    panic!("EXCEPTION: MACHINE CHECK\n{:#?}", stack_frame);
}

extern "x86-interrupt" fn simd_floating_point_handler(stack_frame: InterruptStackFrame) {
    panic!("EXCEPTION: SIMD FLOATING POINT\n{:#?}", stack_frame);
}

extern "x86-interrupt" fn virtualization_handler(stack_frame: InterruptStackFrame) {
    panic!("EXCEPTION: VIRTUALIZATION\n{:#?}", stack_frame);
}

extern "x86-interrupt" fn cp_protection_handler(
    stack_frame: InterruptStackFrame,
    _error_code: u64,
) {
    panic!("EXCEPTION: CP PROTECTION\n{:#?}", stack_frame);
}

extern "x86-interrupt" fn hv_injection_handler(stack_frame: InterruptStackFrame) {
    panic!("EXCEPTION: HV INJECTION\n{:#?}", stack_frame);
}

extern "x86-interrupt" fn vmm_communication_handler(
    stack_frame: InterruptStackFrame,
    _error_code: u64,
) {
    panic!("EXCEPTION: VMM COMMUNICATION\n{:#?}", stack_frame);
}

extern "x86-interrupt" fn security_handler(stack_frame: InterruptStackFrame, _error_code: u64) {
    panic!("EXCEPTION: SECURITY\n{:#?}", stack_frame);
}

#[allow(dead_code)]
#[unsafe(naked)]
extern "C" fn timer_handler() -> () {
    naked_asm!(
        "push rax",
        "push rcx",
        "push rdx",
        "push rbx",
        "push rbp",
        "push rsi",
        "push rdi",
        "push r8",
        "push r9",
        "push r10",
        "push r11",
        "push r12",
        "push r13",
        "push r14",
        "push r15",

        "call {eoi}",

        "mov rdi, rsp",

        "call {schedule}",

        "mov rsp, rax",

        "pop r15",
        "pop r14",
        "pop r13",
        "pop r12",
        "pop r11",
        "pop r10",
        "pop r9",
        "pop r8",
        "pop rdi",
        "pop rsi",
        "pop rbp",
        "pop rbx",
        "pop rdx",
        "pop rcx",
        "pop rax",

        "iretq",
        eoi = sym send_eoi,
        schedule = sym sched::schedule,
    );
}

extern "x86-interrupt" fn keyboard_handler(_stack_frame: InterruptStackFrame) {
    send_eoi();

    use pc_keyboard::{DecodedKey, HandleControl, Keyboard, ScancodeSet1, layouts};
    use x86_64::instructions::port::Port;

    lazy_static! {
        static ref KEYBOARD: Spinlock<Keyboard<layouts::De105Key, ScancodeSet1>> =
            Spinlock::new(Keyboard::new(
                ScancodeSet1::new(),
                layouts::De105Key,
                HandleControl::Ignore
            ));
    }

    let mut keyboard = KEYBOARD.lock();
    let mut port = Port::new(0x60);

    let scancode: u8 = unsafe { port.read() };
    if let Ok(Some(key_event)) = keyboard.add_byte(scancode) {
        if let Some(key) = keyboard.process_keyevent(key_event) {
            let _ = match key {
                DecodedKey::Unicode(character) => {
                    if let Some(c) = character.as_ascii() {
                        let pipe = INPUT_PIPE.try_get().unwrap();

                        pipe.write(0, 1, &c.to_u8() as *const u8);
                    }
                }
                DecodedKey::RawKey(_) => (),
            };
        }
    }
}

extern "x86-interrupt" fn lapic_error_handler(_stack_frame: InterruptStackFrame) {
    log::error!("lapic internal");
}

extern "x86-interrupt" fn spurious_interrupt_handler(_stack_frame: InterruptStackFrame) {
    log::info!("spurious interrupt");
}

#[derive(Debug, Clone, Copy)]
#[repr(u8)]
pub enum InterruptIndex {
    Timer = 32,
    Keyboard = 33,
    LapicError = 34,
    Spurious = 0xFF,
}

impl InterruptIndex {
    fn as_u8(self) -> u8 {
        self as u8
    }

    fn as_u32(self) -> u32 {
        self as u32
    }
}
