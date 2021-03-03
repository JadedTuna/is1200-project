#include "interrupts.h"
#include "serial.h"

#include <pic32mx.h>
#include <stdio.h>

void farcall(unsigned int *);

int RAMcode[] = {
    0x3c02bf88, // lui	v0,0xbf88
    0x34426110, // ori	v0,v0,0x6110
    0x24030037, // li	v1,55
    0xac430000, // sw	v1,0(v0)
    0x03e00008, // jr	ra
    0x00000000  // nop
};
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

void exception_handler(int exc) {
    // Show error code in case something fails later down the line
    PORTE = exc;
    int last_addr = 0, sp = 0;
    __asm__("mfc0	%0, $14, 0" : "=r"(last_addr));
    __asm__("addi	%0, $sp, 0" : "=r"(sp));
    serial_printf("exception occurred: 0x%x\r\n", exc & 0xff);
    serial_printf("last address: 0x%x\r\n", last_addr);
    serial_printf("sp: 0x%x\r\n", sp);
    serial_printf("BMXCON: 0x%x\r\n", BMXCON);
    serial_printf("BMXDKPBA: 0x%x\r\n", BMXDKPBA);
    serial_printf("BMXDUDBA: 0x%x\r\n", BMXDUDBA);
    serial_printf("BMXDUPBA: 0x%x\r\n", BMXDUPBA);
    for (;;) {
        // Blink between 0xff and the error code
        simple_delay(1000);
        PORTE = 0xff;
        simple_delay(1000);
        PORTE = exc;
    }
}

int main(void) {
    setup_interrupts();

    TRISE &= ~0xff;
    PORTE = 0;
    TRISF |= 1;

    serial_init();
    enable_interrupts();
    int ebase = 0;
    __asm__("mfc0	%0, $15, 1" : "=r"(ebase));
    serial_printf(
        "0x%x\r\n"
        "0x%x\r\n",
        INTCON,
        ebase);

    int sp = 0;
    __asm__("addi	%0, $sp, 0" : "=r"(sp));
    serial_printf("main: sp = 0x%x\r\n", sp);

    /* 6 KB of user memory; 4 KB data, 2 KB prog */
    serial_write("Setting BMXDKPBA\r\n");
    BMXDKPBA = 14 * 1024;
    serial_write("Setting BMXDUDBA\r\n");
    BMXDUDBA = 26 * 1024;
    serial_write("Setting BMXDUPBA\r\n");
    BMXDUPBA = 30 * 1024;
    PORTE = 0x3c;

    serial_printf_async("BMXCON: 0x%x\r\n", BMXCON);
    serial_printf_async("BMXDKPBA: 0x%x\r\n", BMXDKPBA);
    serial_printf_async("BMXDUDBA: 0x%x\r\n", BMXDUDBA);
    serial_printf_async("BMXDUPBA: 0x%x\r\n", BMXDUPBA);
    serial_printf_async("BMXDRMSZ: 0x%x\r\n", BMXDRMSZ);
    // serial_printf_async("BMXPUPBA: 0x%x\r\n", BMXPUPBA);
    // serial_printf_async("BMXPFMSZ: 0x%x\r\n", BMXPFMSZ);
    // serial_printf_async("BMXBOOTSZ: 0x%x\r\n", BMXBOOTSZ);
    serial_printf_async("main: 0x%x\r\n", main);

    PORTE = 0xaa;
    // return;

    serial_write_async("loading user code\r\n");
    unsigned i;
    unsigned *userprog = (unsigned *)(0x7F000000 + BMXDUPBA);
    for (i = 0; i < sizeof(RAMcode) / sizeof(int); i++) {
        serial_printf_async("0x%x\r\n", RAMcode[i]);
        serial_printf_async("Writing to 0x%x\r\n", userprog + i);
        serial_flush();
        userprog[i] = RAMcode[i];
    }
    serial_flush();

    // return;
    serial_write("before farcall\r\n");
    farcall(userprog);
    serial_write_async("after farcall\r\n");

    return 0;
}
