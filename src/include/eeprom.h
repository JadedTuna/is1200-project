#ifndef _EEPROM_H
#define _EEPROM_H

#define I2C_EEPROM_ADDRESS 0b1010000
#define EEPROM_SIZE 0x7FFF
#include "eeprom.h"
#include "i2c.h"

#include <stddef.h>
#include <stdint.h>

size_t eeprom_write(uint16_t addr, const void *data, size_t size);
size_t eeprom_read(uint16_t addr, void *buffer, size_t size);

#endif /* _EEPROM_H */
