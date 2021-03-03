#ifndef _SERIAL_H
#define _SERIAL_H

#include <stdint.h>
#include <stddef.h>

extern char TX_BUFFER[4096];
extern volatile size_t TX_BUFFER_IND;
extern size_t TX_BUFFER_FREE;

void uart1tx_int_set(char enabled);
void serial_init(void);

void serial_write(const char *s);
void serial_nwrite(const char *s, size_t size);
int serial_printf(const char *format, ...);

void serial_write_async(const char *s);
void serial_nwrite_async(const char *s, size_t size);
int serial_printf_async(const char *format, ...);
void serial_flush(void);

#endif /* _SERIAL_H */
