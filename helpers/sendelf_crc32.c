/**
 * Send a prepared ELF file over to the chipKIT.
 * Must be called as a file sender over from picocom.
 *
 * Works on hope and asserts.
 * Oh, and there's no error checking. Better pass a correct
 * ELF file!
 */
#include "../src/include/elf.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

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

int readfile(const char *path, uint8_t **data, uint32_t *size) {
    struct stat st;
    stat(path, &st);
    *size = st.st_size;

    FILE *fp = fopen(path, "r");
    assert(fp);

    *data = malloc(*size);
    assert(*data);

    assert(fread(*data, *size, 1, fp) == 1);

    fclose(fp);

    return 1;
}

void crcwrite(void *data, size_t size) {
    uint32_t crc = crc32(data, size);
    fwrite(&crc, sizeof(crc), 1, stdout);
    fwrite(data, size, 1, stdout);
}

int main(int argc, char *argv[]) {
    uint8_t *data;
    uint32_t size;
    uint32_t crc;
    Elf32_Ehdr *elf_header;
    Elf32_Phdr *progheaders;

    (void)argc;

    setbuf(stdout, NULL);
    assert(readfile(argv[1], &data, &size));

    elf_header = (Elf32_Ehdr *)data;
    // Send ELF header
    crcwrite(elf_header, sizeof(Elf32_Ehdr));

    progheaders = (Elf32_Phdr *)(data + elf_header->e_ehsize);

    // Send code and data segments
    int i;
    for (i = 2; i < elf_header->e_phnum; i++) {
        Elf32_Phdr *progheader = &progheaders[i];
        // Write program header CRC
        crc = crc32((uint8_t *)progheader, sizeof(Elf32_Phdr));
        fwrite(&crc, 4, 1, stdout);
        // Write program header
        fwrite(progheader, sizeof(Elf32_Phdr), 1, stdout);

        uint8_t *segment = data + progheader->p_offset;
        // Write segment CRC
        crc = crc32(segment, progheader->p_filesz);
        fwrite(&crc, 4, 1, stdout);

        // Write segment
        fwrite(segment, 1, progheader->p_filesz, stdout);
    }

    return 0;
}