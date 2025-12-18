__attribute__((noreturn))
void _start(unsigned long long memory_map, unsigned long long video_mode) {
  (void)memory_map;
  (void)video_mode;

  for (;;) {
    asm volatile("hlt");
  }
}
