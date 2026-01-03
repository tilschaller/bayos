#include <fail.h>
#include <stdio.h>

__attribute__((noreturn))
void hcf() {
        printf("Halting core");
        for (;;) {
                asm volatile("cli; hlt");
        }
}