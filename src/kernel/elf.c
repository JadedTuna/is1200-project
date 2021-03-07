#include "elf.h"

#include "eeprom.h"
#include "serial.h"
#include "ustar.h"

#include <string.h>

void simple_delay(int);
void farcall(uint8_t *);

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

int check_crc(uint32_t crc1, uint32_t crc2) {
    serial_printf("crc1: 0x%x, crc2: 0x%x\r\n", crc1, crc2);
    if (crc1 != crc2) {
        serial_write("CRC mismatch!\r\n");
        return 0;
        for (;;) {
            serial_write("\rCRC mismatch!");
            simple_delay(500);
            serial_write("\r             ");
            simple_delay(500);
        }
    }
    return 1;
}

/**
 * Read with simple error checking
 */
int crcread(void *buffer, size_t size) {
    uint32_t crc, ccrc;

    serial_read(&crc, sizeof(crc));
    serial_read(buffer, size);
    ccrc = crc32(buffer, size);
    return check_crc(crc, ccrc);
}

// Sketchy way of getting an ELF into memory thru UART
uint8_t *elf_load_program_serial(void) {
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
        if (progheader.p_filesz != progheader.p_memsz) {
            serial_printf("Segment needs to be zeroed: p_filesz: 0x%x, p_memsz: 0x%x\r\n",
                progheader.p_filesz, progheader.p_memsz);
            memset(segment + progheader.p_filesz, 0, progheader.p_memsz - progheader.p_filesz);
        }
    }

    return (uint8_t *)elf_header.e_entry;
}

/**
 * Load an ELF program from EEPROM.
 *
 * @param fileaddr File EEPROM address.
 * @param size File size.
 * @param entry Entry point, set only if file load is successful.
 * @return Result code.
 */
ELF_Status elf_load_eeprom(uint16_t fileaddr, uint8_t **entry) {
    Elf32_Ehdr ehdr;

    eeprom_read(fileaddr, &ehdr, sizeof(ehdr));
    if (ehdr.e_magic[0] != ELF_MAGIC_0 || ehdr.e_magic[1] != ELF_MAGIC_1
        || ehdr.e_magic[2] != ELF_MAGIC_2 || ehdr.e_magic[3] != ELF_MAGIC_3) {
        /* Invalid ELF header */
        return ELF_INVALID_FORMAT;
    }

    if (ehdr.e_bits != ELF_LSB || ehdr.e_endian != ELF_32 || ehdr.e_type != ET_EXEC
        || ehdr.e_machine != EM_MIPS || ehdr.e_version != EV_CURRENT) {
        /* Unsupported ELF file */
        return ELF_UNSUPPORTED;
    }

    /* Load relevant segments */
    Elf32_Phdr phdr;
    // Program headers offset
    uint16_t rd_addr = fileaddr + ehdr.e_phoff;
    int i;
    /**
     * Sketchy way. It assumes first two segments are sections
     * .MIPS.abiflags and .ram_exchange_data, and the following
     * two are .text (+ friends) and .data.
     */
    for (i = 2; i < ehdr.e_phnum; i++) {
        eeprom_read(rd_addr + sizeof(Elf32_Phdr) * i, &phdr, sizeof(Elf32_Phdr));
        serial_printf(
            "Loading segment #%d at 0x%x, size 0x%x (memsz 0x%x)\r\n",
            i,
            phdr.p_vaddr,
            phdr.p_filesz,
            phdr.p_memsz);
        /* Load segment into memory (if it isn't 0 length, like .bss) */
        if (phdr.p_filesz)
            eeprom_read(fileaddr + phdr.p_offset, (uint8_t *)phdr.p_vaddr, phdr.p_filesz);
        /* Zero out rest of the segment if file & mem sizes mismatch. */
        if (phdr.p_filesz < phdr.p_memsz) {
            memset((uint8_t *)phdr.p_vaddr + phdr.p_filesz, 0, phdr.p_memsz - phdr.p_filesz);
        }
    }

    *entry = (uint8_t *)ehdr.e_entry;

    return ELF_OK;
}

/**
 * Run a program from the EEPROM
 *
 * @param filename Program to execute.
 * @return Status code.
 */
ELF_Status elf_run(const char *filename) {
    uint16_t addr;
    size_t size;
    uint32_t ra;
    __asm__("addi %0, $ra, 0" : "=r"(ra));
    serial_printf("ra: 0x%08x\r\n", ra);
    if (ustar_find_file(filename, &addr, &size) == USTAR_FOUND) {
        serial_printf("Found %s at 0x%x (size %d)\r\n", filename, addr, size);
    } else {
        serial_printf("%s not found\r\n", filename);
        return ELF_NOT_FOUND;
    }
    (void)size;

    uint8_t *entry;
    switch (elf_load_eeprom(addr, &entry)) {
    case ELF_OK: {
        serial_write("ELF load successful.\r\n");
        serial_printf("Entry point: 0x%x\r\n", entry);
        serial_write("Doing farcall\r\n");
        farcall(entry);
        // void (*program_entry)(void) = (void *)entry;
        // (*program_entry)();
        __asm__("addi %0, $ra, 0" : "=r"(ra));
        serial_printf("ra: 0x%08x\r\n", ra);
        serial_write("Back in kernel!\r\n");
        break;
    }
    case ELF_INVALID_FORMAT: {
        serial_write("Invalid ELF file.\r\n");
        break;
    }
    case ELF_UNSUPPORTED: {
        serial_write("Unsupported ELF file.\r\n");
        break;
    }
    case ELF_CODE_SEGMENT_TOO_BIG: {
        serial_write("ELF code segment is too big.\r\n");
        break;
    }
    case ELF_DATA_SEGMENT_TOO_BIG: {
        serial_write("ELF data segment is too big.\r\n");
        break;
    }
    default: {
        serial_write("Unknown error while loading ELF file.\r\n");
        break;
    }
    }

    serial_write("about to return\r\n");

    return ELF_OK;
}
