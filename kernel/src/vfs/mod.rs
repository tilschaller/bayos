pub trait File {
    // offset: offset into file to read from
    // size: number of bytes to read
    // buffer: buffer to write into
    fn read(&self, offset: usize, size: usize, buffer: *mut u8);
    // offset: offset into file to write to
    // size: number of bytes to write
    // buffer: buffer to read from
    fn write(&mut self, offset: usize, size: usize, buffer: *const u8);
}

pub mod pipe;
