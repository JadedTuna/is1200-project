
#ifndef _ROM_H
#define _ROM_H

#include "common.h"

#include <pic32mx.h>
#include <stdint.h>

typedef uint16_t rom_address_t;

/* get the i2c module ready for use
 */
void itwoc_setup(void);

/* write given byte to given rom address, return -1 for error and 0 otherwise
 */
int rom_write_byte(rom_address_t addr, byte b);

/* write data in buffer to addr, up to 64 bytes or exhaustion of buffer,
 * return bytes successfully written (ACKed), or -1 for error
 */
int rom_write_page(rom_address_t addr, byte buffer[], int len);

/* read byte currently pointed by the internal rom address pointer
 * (probably should not be used directly)
 */
byte rom_read_current_byte(void);

/* get byte at given rom address, or -1 for error
 */
int rom_read_byte(rom_address_t addr);

/* read `len` bytes starting from given address into buffer (buffer must be big enough!!),
 * returns number of bytes succesfully read, or -1 for error
 */
int rom_read_sequential(rom_address_t addr, byte buffer[], int len);

#endif
