#include "serial.h"

#include "interrupts.h"

#include <pic32mx.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

char TX_BUFFER[4096] = { 0 };
volatile size_t TX_BUFFER_IND = 0;
size_t TX_BUFFER_FREE = sizeof(TX_BUFFER);

void uart1tx_int_set(char enabled) {
    if (enabled) {
        IPCSET(6) = 0b00100;               // set priority
        IECSET(0) = (1 << PIC32_IRQ_U1TX); // enable UART1 TX interrupt
    } else {
        IPCCLR(6) = 0b11111;             // clear priority
        IECCLR(0) = 1 << PIC32_IRQ_U1TX; // disable UART1 TX interrupt
    }
}

/**
 * Initialize serial interface.
 */
void serial_init(void) {
    // UART1 setup
    U1BRG = PIC32_BRG_BAUD(80 * 1000 * 1000, 9600); // set baud rate to 9600
    U1STA = 0;
    U1STASET = PIC32_USTA_UTXISEL_1;

    // Configure UART1
    U1MODE = 0;
    // Enable UART1
    U1MODESET = PIC32_UMODE_ON;

    // Enable TX and RX
    U1STASET = PIC32_USTA_UTXEN | PIC32_USTA_URXEN;

    // Configure UART1 interrupts
    uart1tx_int_set(0);
}

/**
 * Non-blocking serial write for NULL-terminated strings.
 *
 * @param s String to write.
 */
void serial_write_async(const char *s) {
    serial_nwrite_async(s, strlen(s));
}

/**
 * Non-blocking serial write.
 *
 * @param s Data buffer.
 * @param size Number of bytes to write.
 */
void serial_nwrite_async(const char *s, size_t size) {
    while (size) {
        // Write a chunk
        // Wait until there is at least a single byte available
        while (!(volatile int)TX_BUFFER_FREE)
            ;

        // Critical section
        disable_interrupts();

        // Copy a chunk into buffer
        int min_len = size > TX_BUFFER_FREE ? TX_BUFFER_FREE : size;
        strncpy(
            TX_BUFFER + (sizeof(TX_BUFFER) - TX_BUFFER_FREE), // first available byte
            s,
            min_len);
        s += min_len;
        size -= min_len;

        TX_BUFFER_FREE -= min_len;
        if ((IEC(0) & (1 << PIC32_IRQ_U1TX)) == 0)
            uart1tx_int_set(1);

        enable_interrupts();
    }
}

/**
 * Block until output buffer is empty.
 */
void serial_flush(void) {
    while ((volatile int)TX_BUFFER_FREE != sizeof(TX_BUFFER))
        ;
}

/**
 * Non-blocking serial printf.
 *
 * @param format Format string.
 * @param ... Arguments.
 * @return Number of bytes written.
 */
int serial_printf_async(const char *format, ...) {
    char buffer[4096] = { 0 };
    va_list ap;
    int rv;

    va_start(ap, format);
    rv = vsnprintf(buffer, ~(size_t)0, format, ap);
    va_end(ap);
    serial_write_async(buffer);

    return rv;
}

/**
 * Blocking serial write for NULL-terminated strings.
 *
 * @param s String to write.
 */
void serial_write(const char *s) {
    serial_nwrite(s, strlen(s));
}

/**
 * Blocking serial write.
 *
 * @param s Data buffer.
 * @param size Number of bytes to write.
 */
void serial_nwrite(const char *s, size_t size) {
    size_t i;

    disable_interrupts();
    uart1tx_int_set(0);
    for (i = 0; i < size; i++) {
        while (!(U1STA & PIC32_USTA_TRMT))
            ;
        U1TXREG = s[i];
    }
    // FIXME: this should not blindly enable the IRQ
    // and instead save its status earlier and load here
    uart1tx_int_set(1);
    enable_interrupts();
}

/**
 * Blocking serial printf.
 *
 * @param format Format string.
 * @param ... Arguments.
 * @return Number of bytes written.
 */
int serial_printf(const char *format, ...) {
    char buffer[4096] = { 0 };
    va_list ap;
    int rv;

    va_start(ap, format);
    rv = vsnprintf(buffer, ~(size_t)0, format, ap);
    va_end(ap);
    serial_write(buffer);

    return rv;
}
