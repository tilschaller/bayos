#include <fail.h>

__attribute__((noreturn))
void hcf() {
        for (;;) {
                asm volatile("cli; hlt");
        }
}