#ifndef _USTAR_H
#define _USTAR_H

#include <stdint.h>
#include <stddef.h>

#define USTAR_BLOCK_SIZE 512

typedef union {
	struct {
		/* Pre-POSIX.1-1988 */
		char filename[100];
		char filemode[8];
		char uid[8];
		char gid[8];
		char filesize[12];
		char modtime[12];
		char checksum[8];
		/* UStar */
		char type;
		char linkname[100];
		char ustar[6];
		char version[2];
		char owner[32];
		char group[32];
		char major[8];
		char minor[8];
		char prefix[155];
	};

	/* Make sure this is 512 bytes long (size of a block) */
	char block[512];
} UStar_Fhdr;

enum {
	USTAR_NOT_FOUND,
	USTAR_FOUND
};

int ustar_find_file(const char *filename, uint16_t *addr, size_t *size);

#endif