#include "common.h"
#include "eeprom.h"
#include "elf.h"
#include "i2c.h"
#include "interrupts.h"
#include "rom.h"
#include "serial.h"
#include "ustar.h"

#include <pic32mx.h>
#include <stdio.h>
#include <stdlib.h>

void farcall(uint8_t *);
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
    uint8_t v0;

    // In case this is a syscall
    __asm__("addi   %0, $a0, 0" : "=r"(srvnum));
    __asm__("addi   %0, $a1, 0" : "=r"(a0));
    __asm__("addi   %0, $a2, 0" : "=r"(a1));
    __asm__("addi   %0, $a3, 0" : "=r"(a2));
    __asm__("addi   %0, $v0, 0" : "=r"(v0));

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
        serial_printf("$v0: 0x%x\r\n", v0);
        serial_printf("BMXCON: 0x%x\r\n", BMXCON);
        serial_printf("BMXDKPBA: 0x%x\r\n", BMXDKPBA);
        serial_printf("BMXDUDBA: 0x%x\r\n", BMXDUDBA);
        serial_printf("BMXDUPBA: 0x%x\r\n", BMXDUPBA);
        // FIXME: 0x80006800 should be a variable
        serial_hexdump((uint32_t *)sp, 0x80006800 - sp);
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

    serial_write("Exiting exception handler.\r\n");
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

void flash_ustar(void) {
    uint32_t size;
    uint32_t eeprom_addr = 0;
    uint8_t buffer[512] = { 0 };

    // Read FS size
    serial_read(&size, 4);
    serial_printf("size: %lu\r\n", size);
    int i = 0;
    size_t chunk_size = sizeof(buffer);
    while (eeprom_addr < size) {
        chunk_size = size - eeprom_addr < sizeof(buffer) ? (size - eeprom_addr) : sizeof(buffer);

        uint32_t block_crc;
        serial_printf("reading block %d\r\n", i++);
        if (!crcread(buffer, chunk_size)) {
            serial_hexdump(buffer, sizeof(buffer));
            break;
        }
        serial_write("done reading\r\n");
        block_crc = crc32(buffer, chunk_size);
        serial_printf("writing to eeprom at 0x%x\r\n", eeprom_addr);
        eeprom_write(eeprom_addr, buffer, chunk_size);
        // Make sure it was written properly
        serial_write("reading from eeprom\r\n");
        serial_printf("read %lu bytes\r\n", eeprom_read(eeprom_addr, buffer, chunk_size));
        if (!check_crc(block_crc, crc32(buffer, chunk_size))) {
            serial_hexdump(buffer, sizeof(buffer));
            break;
        }

        eeprom_addr += sizeof(buffer);
    }

    serial_write("Done!\r\n");
}

void init(void) {
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
    i2c_setup();

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
}

int main(void) {
    uint32_t sp;
    __asm__("addi   %0, $sp, 0" : "=r"(sp));
    init();

    serial_printf("sp: 0x%08x\r\n", sp);

    serial_write("WARNING: if the chipKIT is reset during EEPROM operations, "
                 "a power cycle is needed to properly reset the I2C controller.\r\n");
    serial_write("WELCOME to kernel land.\r\n");
    serial_write("Make sure you are using the correct file transfer program! "
                 "sendelf_crc32 for uploading a program, sendtar_crc32 for "
                 "flashing a filesystem\r\nThis is set up in the Makefile.\r\n");
    for (;;) {
        serial_write("Menu selection:\r\n"
                     "[F] flash filesystem\r\n"
                     "[U] upload program\r\n"
                     "[R] run program.elf from the filesystem\r\nYour selection: ");
        char choice;
        serial_read(&choice, 1);
        serial_write("\r\n");
        if (choice == 'F' || choice == 'f') {
            serial_write("Will flash a filesystem.\r\n");
            flash_ustar();
        } else if (choice == 'U' || choice == 'u') {
            serial_write("Will receive and execute a file.\r\n");
            uint8_t *entry_point = elf_load_program_serial();

            serial_write("Doing farcall\r\n");
            farcall(entry_point);
            serial_write("Back in kernel!\r\n");
        } else if (choice == 'R' || choice == 'r') {
            serial_write("Will run program.elf\r\n");
            serial_write("Good chance it will work but fail to return back here.\r\n");
            elf_run("program.elf");
            serial_write("back\r\n");
        } else {
            serial_write("Unknown command.\r\n");
        }
        serial_write("out of if\r\n");
    }

    for (;;)
        ;
    return 0;
}
