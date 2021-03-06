#include "elf.h"

#include "serial.h"

void simple_delay(int);

uint32_t crc32(void *data, size_t size) {
    int i;
    uint32_t byte, crc, mask;
    uint8_t *dataptr = (uint8_t *)data;

    // Init CRC to all 1's
    crc = 0xFFFFFFFF;
    while (size--) {
        byte = *dataptr++;
        crc ^= byte;
        for (i = 0; i < 8; i++) {
            mask = -(crc & 1);
            // 0xEDB88320 is reversed polynomial
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
    }

    // Return inverted CRC
    return ~crc;
}

void check_crc(uint32_t crc1, uint32_t crc2) {
    serial_printf("crc1: 0x%x, crc2: 0x%x\r\n", crc1, crc2);
    if (crc1 != crc2) {
        for (;;) {
            serial_write("\rCRC mismatch!");
            simple_delay(500);
            serial_write("\r             ");
            simple_delay(500);
        }
    }
}

/**
 * Read with simple error checking
 */
void crcread(void *buffer, size_t size) {
    uint32_t crc, ccrc;

    serial_read(&crc, sizeof(crc));
    serial_read(buffer, size);
    ccrc = crc32(buffer, size);
    check_crc(crc, ccrc);
}

// Sketchy way of getting an ELF into memory thru UART
uint32_t *elf_load_program_serial(void) {
    Elf32_Phdr progheader;
    Elf32_Ehdr elf_header;

    // Load ELF header
    crcread(&elf_header, sizeof(Elf32_Ehdr));

    serial_printf("entry point: 0x%x\r\n", elf_header.e_entry);

    // Load two segments: code and data
    // TODO: zero out rest of segment if needed
    // TODO: make sure segments fit into memory...
    // TODO: find relevant segments by checking their vaddrs
    int segnum;
    for (segnum = 0; segnum < 2; segnum++) {
        // Load program header
        crcread(&progheader, sizeof(Elf32_Phdr));

        uint8_t *segment = (uint8_t *)progheader.p_vaddr;

        // Load segment
        crcread(segment, progheader.p_filesz);
    }

    return (uint32_t *)elf_header.e_entry;
}
