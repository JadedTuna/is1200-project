#ifndef _SERIAL_H
#define _SERIAL_H

char TX_BUFFER[4096];
volatile int TX_BUFFER_IND;
int TX_BUFFER_FREE;

void uart1tx_int_set(char enabled);
void serial_init(void);
void serial_write(const char *s);
void serial_flush(void);
void serial_printf(const char *format, ...);

#endif /* _SERIAL_H */