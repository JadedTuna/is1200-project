#include "eeprom.h"

#include "i2c.h"

#include <pic32mx.h>

size_t eeprom_write_page(uint16_t addr, const void *data, size_t size) {
    size_t i;
    const uint8_t *dataptr = (uint8_t *)data;

    // TODO: error?
    if (size > 64)
        size = 64;

    /* Initiate write */
    do {
        i2c_begin();
    } while (!i2c_send(I2C_EEPROM_ADDRESS << 1));

    /* Send address */
    i2c_send(addr >> 8);
    i2c_send(addr & 0xFF);

    /* Send data */
    for (i = 0; i < size; i++) {
        if (!i2c_send(dataptr[i])) {
            /* got a NACK */
            break;
        }
    }

    /* Finish write */
    i2c_end();

    return i;
}

size_t eeprom_write(uint16_t addr, const void *data, size_t size) {
    const uint8_t *dataptr = (uint8_t *)data;

    if (addr + size > 0x7FFF) {
        /* Can't write that far! */
        // FIXME
    } else if (addr > 0x7FFF) {
        /* Can't write that far! */
        // FIXME
    }

    size_t chunk_size, written = 0;
    while (size) {
        chunk_size = size < 64 ? size : 64;
        written += eeprom_write_page(addr, dataptr, chunk_size);

        addr += chunk_size;
        dataptr += chunk_size;
        size -= chunk_size;
    }

    return written;
}

size_t eeprom_read(uint16_t addr, void *buffer, size_t size) {
    uint8_t *bufptr = (uint8_t *)buffer;

    /* Initiate write */
    PORTE = 11;
    do {
        i2c_begin();
    } while (!i2c_send(I2C_EEPROM_ADDRESS << 1));
    PORTE = 12;

    /* Send address */
    i2c_send(addr >> 8);
    PORTE = 13;
    i2c_send(addr & 0xFF);
    PORTE = 14;

    /* Finish write */
    i2c_end();
    PORTE = 15;

    /* Initiate write */
    do {
        i2c_begin();
    } while (!i2c_send((I2C_EEPROM_ADDRESS << 1) | 1));
    PORTE = 16;

    size_t i;
    PORTE = 17;
    for (i = 0; i < size; i++) {
        PORTE = i;
        uint8_t byte = i2c_recv();
        PORTE = i | (1 << 7);
        if (i + 1 < size)
            i2c_ack();
        PORTE = i | (2 << 7);
        bufptr[i] = byte;
    }

    i2c_nack();
    i2c_end();
    PORTE = 18;

    return i;
}
