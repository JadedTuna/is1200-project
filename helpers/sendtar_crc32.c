#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/stat.h>
const uint32_t BLOCK_SIZE = 512;

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
    uint32_t alloc_size;
    stat(path, &st);
    *size = st.st_size;
    alloc_size = *size;

    FILE *fp = fopen(path, "r");
    assert(fp);

    if (alloc_size % BLOCK_SIZE)
    	alloc_size += BLOCK_SIZE - (alloc_size % BLOCK_SIZE);
    alloc_size = *size;

    *data = calloc(alloc_size, 1);
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
	(void)argc;

    setbuf(stdout, NULL);
    assert(readfile(argv[1], &data, &size));

    fwrite(&size, 4, 1, stdout);

    uint32_t offset;
    uint32_t chunk_size;
    for (offset = 0; offset < size; offset += BLOCK_SIZE) {
        chunk_size = size - offset < BLOCK_SIZE
            ? (size - offset) : BLOCK_SIZE;
    	crcwrite(data + offset, chunk_size);
    }
}