__attribute__((noreturn))
void _start(void) {
  for (;;) {
    asm volatile("hlt");
  }
}
