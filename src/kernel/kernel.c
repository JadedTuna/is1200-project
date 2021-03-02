#include "interrupts.h"
#include "serial.h"

#include <pic32mx.h>
#include <stdio.h>

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
    serial_priority_printf("exception occurred: 0x%x\r\n", exc & 0xff);
    serial_priority_printf("last address: 0x%x\r\n", last_addr);
    serial_priority_printf("sp: 0x%x\r\n", sp);
    serial_priority_printf("BMXCON: 0x%x\r\n", BMXCON);
    serial_priority_printf("BMXDKPBA: 0x%x\r\n", BMXDKPBA);
    serial_priority_printf("BMXDUDBA: 0x%x\r\n", BMXDUDBA);
    serial_priority_printf("BMXDUPBA: 0x%x\r\n", BMXDUPBA);
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
    serial_priority_printf(
        "0x%x\r\n"
        "0x%x\r\n",
        INTCON,
        ebase);

    int sp = 0;
    __asm__("addi	%0, $sp, 0" : "=r"(sp));
    serial_priority_printf("main: sp = 0x%x\r\n", sp);

    /* 6 KB of user memory; 4 KB data, 2 KB prog */
    serial_priority_write("Setting BMXDKPBA\r\n");
    BMXDKPBA = 14 * 1024;
    serial_priority_write("Setting BMXDUDBA\r\n");
    BMXDUDBA = 26 * 1024;
    serial_priority_write("Setting BMXDUPBA\r\n");
    BMXDUPBA = 30 * 1024;
    PORTE = 0x3c;

    serial_printf("BMXCON: 0x%x\r\n", BMXCON);
    serial_printf("BMXDKPBA: 0x%x\r\n", BMXDKPBA);
    serial_printf("BMXDUDBA: 0x%x\r\n", BMXDUDBA);
    serial_printf("BMXDUPBA: 0x%x\r\n", BMXDUPBA);
    serial_printf("BMXDRMSZ: 0x%x\r\n", BMXDRMSZ);
    // serial_printf("BMXPUPBA: 0x%x\r\n", BMXPUPBA);
    // serial_printf("BMXPFMSZ: 0x%x\r\n", BMXPFMSZ);
    // serial_printf("BMXBOOTSZ: 0x%x\r\n", BMXBOOTSZ);
    serial_printf("main: 0x%x\r\n", main);

    PORTE = 0xaa;
    // return;

    serial_write("loading user code\r\n");
    int i;
    unsigned *userprog = (unsigned *)(0x7F000000 + BMXDUPBA);
    for (i = 0; i < sizeof(RAMcode) / sizeof(int); i++) {
        serial_printf("0x%x\r\n", RAMcode[i]);
        serial_printf("Writing to 0x%x\r\n", userprog + i);
        serial_flush();
        userprog[i] = RAMcode[i];
    }
    serial_flush();

    // return;
    serial_priority_write("before farcall\r\n");
    farcall(userprog);
    serial_write("after farcall\r\n");

    return 0;
}
