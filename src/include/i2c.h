#ifndef _I2C_H
#define _I2C_H

#include <stddef.h>
#include <stdint.h>

void i2c_setup(void);
void i2c_flush(void);
void i2c_begin(void);
void i2c_end(void);
int i2c_send(uint8_t byte);
uint8_t i2c_recv(void);
void i2c_ack();
void i2c_nack();

#endif /* _I2C_H */