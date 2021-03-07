#include "serial.h"

#include "interrupts.h"

#include <pic32mx.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

ring_buffer_t rx_buffer;

void uart1rx_int_set(char enabled) {
    if (enabled) {
        IPCSET(6) = 0b00100;               // set priority
        IECSET(0) = (1 << PIC32_IRQ_U1RX); // enable UART1 RX interrupt
    } else {
        IPCCLR(6) = 0b11111;             // clear priority
        IECCLR(0) = 1 << PIC32_IRQ_U1RX; // disable UART1 RX interrupt
    }
}

/**
 * Initialize serial interface.
 */
void serial_init(void) {
    rx_buffer.head = rx_buffer.tail = 0;

    // UART1 setup
    U1BRG = PIC32_BRG_BAUD(80 / 2 * 1000 * 1000, 9600); // set baud rate to 9600
    U1STA = 0;
    U1STASET = PIC32_USTA_UTXISEL_1;

    // Configure UART1
    U1MODE = 0;
    // Enable UART1
    U1MODESET = PIC32_UMODE_ON;

    // Enable TX and RX
    U1STASET = PIC32_USTA_UTXEN | PIC32_USTA_URXEN;

    // Configure UART1 interrupts
    uart1rx_int_set(1);
}

/**
 * Blocking serial write for NULL-terminated strings.
 *
 * @param s String to write.
 */
void serial_write(const char *s) { serial_nwrite(s, strlen(s)); }

/**
 * Blocking serial write.
 *
 * @param s Data buffer.
 * @param size Number of bytes to write.
 */
void serial_nwrite(const char *s, size_t size) {
    size_t i;

    // disable_interrupts();
    // uart1tx_int_set(0);
    for (i = 0; i < size; i++) {
        while (!(U1STA & PIC32_USTA_TRMT))
            ;
        U1TXREG = s[i];
    }
    // FIXME: this should not blindly enable the IRQ
    // and instead save its status earlier and load here
    // uart1tx_int_set(1);
    // enable_interrupts();
    IFSCLR(0) = 1 << 28;
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

/**
 * Write a hexdump of the specified memory region.
 * Same format as hexdump -C on Linux
 *
 * @param data Start of memory region.
 * @param size Size of the memory region.
 */
void serial_hexdump(const void *data, size_t size) {
    size_t i, idx;
    uint8_t c;
    char hexbuf[16];
    const char *buffer = (char *)data;

    serial_printf("%08x  ", buffer);

    for (i = 0, idx = 0; i < size; i++) {
        idx = i & 0xF;
        c = ((const uint8_t *)buffer)[i];

        if (idx == 8) {
            serial_write(" ");
        } else if (i && !idx) {
            serial_write("|");
            serial_nwrite(hexbuf, 8);
            serial_nwrite(hexbuf + 8, 8);
            serial_write("|\r\n");
            serial_printf("%08x  ", ((const uint8_t *)buffer) + i);
        }

        serial_printf("%02x ", c);

        if (c < 0x20 || c > 0x7e)
            hexbuf[idx] = '.';
        else
            hexbuf[idx] = c;
    }

    for (++idx; idx < 16; idx++) {
        hexbuf[idx] = ' ';
        if (idx == 8)
            serial_write(" ");
        serial_write("   ");
    }
    serial_write("|");
    serial_nwrite(hexbuf, 8);
    serial_nwrite(hexbuf + 8, 8);
    serial_write("|\r\n");
}

/**
 * Write a 32-bit number as binary.
 *
 * @param num Number to display.
 */
void serial_writebin(register uint32_t num) {
    char buf[34];
    size_t i;
    for (i = 0; i < 32; i++)
        buf[i] = ((num >> (31 - i)) & 1) + '0';
    buf[32] = '\r';
    buf[33] = '\n';

    for (i = 0; i < sizeof(buf); i++) {
        while (!(U1STA & PIC32_USTA_TRMT))
            ;
        U1TXREG = buf[i];
    }
}

/**
 * Read up to `size` bytes until \0.
 *
 * @param buffer Buffer to read into.
 * @param size Read up to this many bytes.
 * @return Number of bytes read.
 */
size_t serial_read_string(void *buffer, size_t size) {
    char c;
    size_t num_read = 0;
    uint8_t *bufptr = (uint8_t *)buffer;
    do {
        // Wait until there is data available
        while (rx_buffer.head == rx_buffer.tail)
            ;
        // Read next byte and advance tail
        c = rx_buffer.buffer[rx_buffer.tail++];
        bufptr[num_read++] = c;
        // Keep it in check
        rx_buffer.tail %= RX_BUFFER_SIZE;
    } while (c && num_read < size);

    return num_read;
}

/**
 * Read `size` bytes in blocking mode.
 *
 * @param buffer Buffer to read into.
 * @param size Number of bytes to read.
 * @return Number of bytes read.
 */
void serial_read(void *buffer, size_t size) {
    uint8_t *bufptr = buffer;
    do {
        // Wait until there is data available
        while (rx_buffer.head == rx_buffer.tail)
            ;
        // Read next byte and advance tail
        *bufptr++ = rx_buffer.buffer[rx_buffer.tail++];
        // Keep it in check
        rx_buffer.tail %= RX_BUFFER_SIZE;
    } while (--size);
}
