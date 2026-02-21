#ifndef _ACPI_H
#define _ACPI_H

void acpi_init(void *rsdp);
void send_eoi(void);

#endif // _ACPI_H
