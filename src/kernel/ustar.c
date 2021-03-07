#include "ustar.h"

#include "eeprom.h"
#include "serial.h"

#include <string.h>

/**
 * Convert an ASCII octal number into an integer.
 *
 * @param data ASCII octal number string.
 * @param size Size of the input string.
 * @return Number in integer format.
 */
int oct2bin(const char *data, size_t size) {
    int n = 0;
    while (size--) {
        n *= 8;
        n += *data++ - '0';
    }

    return n;
}

/**
 * Find a file in the EEPROM containing a UStar filesystem (tar archive).
 * addr and size will not be modified unless a file is found.
 *
 * @param filename Filename.
 * @param addr File address on EEPROM (if found).
 * @param size File size on EEPROM (if found).
 */
int ustar_find_file(const char *filename, uint16_t *addr, size_t *size) {
    UStar_Fhdr header;
    uint16_t curaddr;
    size_t tmpsize;
    for (curaddr = 0; curaddr <= EEPROM_SIZE - USTAR_BLOCK_SIZE;) {
        serial_printf("0x%x\r\n", curaddr);
        size_t rd = eeprom_read(curaddr, &header, USTAR_BLOCK_SIZE);
        serial_printf("Read %lu bytes\r\n", rd);
        // Make sure we are still in the filesystem
        if (memcmp(header.ustar, "ustar", 5))
            return USTAR_NOT_FOUND;

        curaddr += USTAR_BLOCK_SIZE;
        // FIXME: limit filename size to 99 (+ \0)
        tmpsize = oct2bin(header.filesize, sizeof(header.filesize) - 1);
        if (!memcmp(header.filename, filename, strlen(filename) + 1)) {
            // File found
            *addr = curaddr;
            *size = tmpsize;
            return USTAR_FOUND;
        }

        curaddr += tmpsize;
        if (tmpsize % USTAR_BLOCK_SIZE)
            curaddr += USTAR_BLOCK_SIZE - (tmpsize % USTAR_BLOCK_SIZE);
    }

    return USTAR_NOT_FOUND;
}
