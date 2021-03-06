#ifndef _SERIAL_H
#define _SERIAL_H

#include <stddef.h>
#include <stdint.h>

#define RX_BUFFER_SIZE 512
typedef struct {
    uint8_t buffer[RX_BUFFER_SIZE];
    volatile uint32_t head;
    volatile uint32_t tail;
} ring_buffer_t;
extern ring_buffer_t rx_buffer, tx_buffer;

void uart1rx_int_set(char enabled);
void serial_init(void);

void serial_write(const char *s);
void serial_nwrite(const char *s, size_t size);
int serial_printf(const char *format, ...);

void serial_hexdump(const void *data, size_t size);

size_t serial_read_string(void *buffer, size_t size);
void serial_read(void *buffer, size_t size);

#endif /* _SERIAL_H */
