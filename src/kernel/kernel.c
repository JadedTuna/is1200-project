#include "common.h"
#include "eeprom.h"
#include "elf.h"
#include "i2c.h"
#include "interrupts.h"
#include "rom.h"
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

void rom_test(void) {
    int b;
    itwoc_setup();
    byte buffer[] = "this is text much texty very text haha";
    rom_address_t addr = 0x012;

    serial_printf("should see: %c at %x\r\n", *buffer, addr);
    if (rom_write_byte(addr, buffer[0]))
        serial_write("writing single byte errored\r\n");

    b = rom_read_byte(addr);
    if (b < 0)
        serial_printf("reading single bit errored: %d\r\n", b);

    serial_printf("byte at same address: %c\r\n", b);
    addr += 128;
    rom_write_page(addr, buffer + 5, sizeof(buffer) - 5);
    serial_printf("now saving at %x: %s\r\n", addr, buffer + 5);
    byte readbuffer[100] = { 0 };
    rom_read_sequential(addr, readbuffer, sizeof(buffer) - 5);
    serial_printf("has read: %s", readbuffer);
    serial_write("test complete\r\n");
}

void rom_test_wd(void) {
    i2c_setup();

    char buf[64] = { 0 };

    int i;
    for (i = 0; i < 64; i++)
        buf[i] = i;

    serial_write("input 8 characters: ");
    serial_read(buf, 8);
    PORTE = 0x0F;

    buf[9] = 0;
    serial_printf("Got: %s\r\n", buf);
    serial_printf("wrote %d bytes\r\n", eeprom_write(0, buf, 8));
    PORTE = 10;
    simple_delay(1000);
    serial_printf("read %d bytes\r\n", eeprom_read(0, buf, 8));
    serial_hexdump(buf, 8);
    serial_write("trying again\r\n");
    for (i = 0; i < 64; i++)
        buf[i] = 0;
    serial_hexdump(buf, 8);

    serial_printf("read %d bytes\r\n", eeprom_read(0, buf, 8));
    serial_hexdump(buf, 8);
}

int main(void) {
    setup_interrupts();
    TRISE &= ~0xFF;
    PORTE = 0;
    TRISF |= 1;

    // Clear PBCLK divisor
    OSCCONCLR = PIC32_OSCCON_PBDIV_MASK;
    // Set PBCLK to 40MHz (divisor 2)
    OSCCONSET = PIC32_OSCCON_PBDIV_2;

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

    rom_test_wd();
    for (;;)
        ;
    // while (1)
    // rom_test();

    uint32_t *entry_point = elf_load_program_serial();

    serial_write("Doing farcall\r\n");
    farcall(entry_point);
    serial_write("Back in kernel!\r\n");

    for (;;)
        ;
    return 0;
}
