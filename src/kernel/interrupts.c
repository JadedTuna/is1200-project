#include "interrupts.h"

#include "serial.h"

#include <pic32mx.h>

void setup_interrupts(void) {
    disable_interrupts();

    /* Switch to Multi Vector Mode */
    int cause = 0;
    __asm__("mfc0	%0, $13, 0" : "=r"(cause));
    cause |= 0x00800000;
    __asm__("mtc0	%0, $13, 0" : "=r"(cause));
    __asm__("ehb");
    INTCONSET = PIC32_INTCON_MVEC;
    enable_interrupts();
}

void default_isr(register /* otherwise it corrupts the stack */ int interrupt) {
    int vector = INTSTAT & 0b11111;
    if (interrupt && 0) {
        PORTE = interrupt;
        serial_printf(
            "\r\n"
            "\tinterrupt: 0x%x\r\n"
            "\tvector: 0x%x\r\n"
            "\tIFS(0): 0x%x\r\n",
            interrupt,
            vector,
            IFS(0));
        for (;;)
            ;
    }

    // Unknown interrupt
    if (!(IFS(0) & (1 << PIC32_IRQ_U1TX)) && !(IFS(0) & (1 << PIC32_IRQ_U1RX))) {
        serial_write("another interrupt!\r\n");
        serial_printf("0x%X\r\n", IFS(0));
        PORTE = 0x1010;
        for (;;)
            ;
    }

    // TX
    if (IFS(0) & (1 << PIC32_IRQ_U1TX)) {
        // Async writes are gone (for now?), just reset the flag
        IFSCLR(0) = 1 << PIC32_IRQ_U1TX;
    }

    // RX
    if (IFS(0) & (1 << PIC32_IRQ_U1RX)) {
        uint8_t byte;
        uint32_t ind;
        while (U1STA & PIC32_USTA_URXDA) {
            // Keep reading bytes while there is data available
            byte = U1RXREG;
            ind = (rx_buffer.head + 1) % RX_BUFFER_SIZE;
            // Don't overwrite the current tail position.
            if (ind != rx_buffer.tail) {
                rx_buffer.buffer[rx_buffer.head] = byte;
                rx_buffer.head = ind;
            }
        }
        // Clear interrupt
        IFSCLR(0) = 1 << PIC32_IRQ_U1RX;
    }
}
