use super::File;
use core::slice::{from_raw_parts, from_raw_parts_mut};
use crossbeam_queue::ArrayQueue;

pub struct Pipe {
    array: ArrayQueue<u8>,
}

impl Pipe {
    pub fn new(size: usize) -> Self {
        Self {
            array: ArrayQueue::<u8>::new(size),
        }
    }
}

impl File for Pipe {
    fn read(&self, _offset: usize, size: usize, buffer: *mut u8) {
        let arr = unsafe { from_raw_parts_mut(buffer, size) };

        for i in 0..size {
            loop {
                let c = self.array.pop();
                match c {
                    Some(c) => {
                        arr[i] = c;
                        break;
                    }
                    None => continue,
                }
            }
        }
    }

    fn write(&mut self, _offset: usize, size: usize, buffer: *const u8) {
        let arr = unsafe { from_raw_parts(buffer, size) };

        for i in 0..size {
            self.array.push(arr[i]).expect("Pipe is full");
        }
    }
}
