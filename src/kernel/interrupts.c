#include <pic32mx.h>
#include "../include/serial.h"

inline void enable_interrupts(void) { __asm__("ei"); }
inline void disable_interrupts(void) { __asm__("di"); __asm__("ehb"); }

void setup_interrupts(void) {
	disable_interrupts();

	/* Switch to Multi Vector Mode */
	int cause = 0;
	__asm__("mfc0	%0, $13, 0" : "=r"(cause));
	cause |= 0x00800000;
	__asm__("mtc0	%0, $13, 0": "=r"(cause));
	__asm__("ehb");
	INTCONSET = PIC32_INTCON_MVEC;
	enable_interrupts();
}

void default_isr(register /* otherwise it corrupts the stack */ int interrupt) {
	// __asm__("addi	%0, $k1, 0" : "=r"(interrupt));
	int irq = INTSTAT & 0b11111;
	if (interrupt) {
		// PORTE = interrupt;
		// serial_priority_printf("\r\n"
		// 	"\tinterrupt: 0x%x\r\n"
		// 	"\tirq: 0x%x\r\n"
		// 	"\tIFS(0): 0x%x\r\n", interrupt, irq, IFS(0));
		// for (;;);
	}
	if (IFS(0) & (1 << PIC32_IRQ_U1TX)) {
		// TX
		if (TX_BUFFER_IND >= (sizeof(TX_BUFFER) - TX_BUFFER_FREE)) {
			// Done sending
			disable_interrupts();
			TX_BUFFER_IND = 0;
			TX_BUFFER_FREE = sizeof(TX_BUFFER);
			uart1tx_int_set(0);
			enable_interrupts();
		} else if (sizeof(TX_BUFFER) != TX_BUFFER_FREE) {
			IFSCLR(0) = 1 << PIC32_IRQ_U1TX;
			U1TXREG = TX_BUFFER[TX_BUFFER_IND++];
		}
		// PORTE = 0x80 | TX_BUFFER_IND;
	} else {
		PORTE = 0x80;
		serial_write("another interrupt\r\n");
	}
}
