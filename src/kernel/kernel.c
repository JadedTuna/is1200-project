#include "elf.h"
#include "interrupts.h"
#include "serial.h"

#include <pic32mx.h>
#include <stdio.h>
#include <stdlib.h>

void farcall(uint32_t *);
void kernel_syscall(uint32_t srvnum, uint32_t a0, uint32_t a1, uint32_t a2);

FILE *const stdout = 0; // FIXME: this really shouldn't be needed

void _nmi_handler() {
    // TODO: stub
    // maybe can debug like in exception_handler?
    PORTE = 0x55;
    for (;;)
        ;
}

void _on_reset() { }
void _on_bootstrap() { }

void simple_delay(int cycles) {
    for (; cycles > 0; cycles--) {
        int i;
        for (i = 6000; i; i--)
            ;
    }
}

void general_exception() {
    PORTE = 0xFF;
    uint32_t srvnum, a0, a1, a2;
    uint32_t cause;
    uint8_t exc_code;
    // In case this is a syscall
    __asm__("addi   %0, $a0, 0" : "=r"(srvnum));
    __asm__("addi   %0, $a1, 0" : "=r"(a0));
    __asm__("addi   %0, $a2, 0" : "=r"(a1));
    __asm__("addi   %0, $a3, 0" : "=r"(a2));

    // Get the exception code
    __asm__("mfc0   %0, $13, 0" : "=r"(cause));
    __asm__("ehb");

    exc_code = (cause & 0xFF) >> 2;
    serial_printf("general_exception(0x%x)\r\n", exc_code);

    switch (exc_code) {
    case 0x08: {
        kernel_syscall(srvnum, a0, a1, a2);
        break;
    }
    default: {
        // Show error code in case something fails later down the line
        PORTE = exc_code;
        uint32_t last_addr = 0, sp = 0;
        __asm__("mfc0   %0, $14, 0" : "=r"(last_addr));
        __asm__("ehb");
        __asm__("addi   %0, $sp, 0" : "=r"(sp));
        serial_printf("exception occurred: 0x%x\r\n", exc_code & 0xff);
        serial_printf("last address: 0x%x\r\n", last_addr);
        serial_printf("$sp: 0x%x\r\n", sp);
        serial_printf("BMXCON: 0x%x\r\n", BMXCON);
        serial_printf("BMXDKPBA: 0x%x\r\n", BMXDKPBA);
        serial_printf("BMXDUDBA: 0x%x\r\n", BMXDUDBA);
        serial_printf("BMXDUPBA: 0x%x\r\n", BMXDUPBA);
        for (;;) {
            // Blink between 0xff and the error code
            simple_delay(1000);
            PORTE = 0xff;
            simple_delay(1000);
            PORTE = exc_code;
        }
        break;
    }
    }
}

void kernel_syscall(uint32_t srvnum, uint32_t a0, uint32_t a1, uint32_t a2) {
    /*
        FIXME: address validation, don't let user execute syscalls on addresses
        they don't have access to.
    */

    switch (srvnum) {
    case 4: {
        // serial_write
        serial_write((char *)a0);
        serial_write("\r\n");
        break;
    }
    default: {
        serial_printf("Unknown syscall: 0x%x\r\n", srvnum);
        serial_printf("a0: 0x%x\r\na1: 0x%x\r\na2: 0x%x\r\n", a0, a1, a2);
        for (;;)
            ;
        break;
    }
    }
}

int main(void) {
    setup_interrupts();
    TRISE &= ~0xFF;
    PORTE = 0;
    TRISF |= 1;

    serial_init();
    enable_interrupts();

    /* 6 KB of user memory; 4 KB data, 2 KB prog */
    serial_write("Setting BMXDKPBA\r\n");
    BMXDKPBA = 14 * 1024;
    serial_write("Setting BMXDUDBA\r\n");
    BMXDUDBA = 26 * 1024;
    serial_write("Setting BMXDUPBA\r\n");
    BMXDUPBA = 30 * 1024;

    serial_printf(
        "User data: %d bytes, user program: %d bytes\r\n",
        BMXDUPBA - BMXDUDBA,
        BMXDRMSZ - BMXDUPBA);

    uint32_t *entry_point = elf_load_program_serial();

    serial_write("Doing farcall\r\n");
    farcall(entry_point);
    serial_write("Back in kernel!\r\n");

    for (;;)
        ;
    return 0;
}
