#ifndef _ELF_H
#define _ELF_H
#include <stdint.h>
#include <stddef.h>

/* These are ELF definitions */
typedef uint16_t Elf32_Half; // Unsigned half int
typedef uint32_t Elf32_Off;  // Unsigned offset
typedef uint32_t Elf32_Addr; // Unsigned address
typedef uint32_t Elf32_Word; // Unsigned int
typedef int32_t Elf32_Sword; // Signed int

#define ELF_MAGIC_0 0x7F
#define ELF_MAGIC_1 'E'
#define ELF_MAGIC_2 'L'
#define ELF_MAGIC_3 'F'

#define ELF_LSB     1       // Little Endian
#define ELF_32      1       // 32-bit
#define EM_MIPS     0x08    // MIPS architecture
#define EV_CURRENT  0x01    // Current version is 1

/* Unknown, relocatable, executable */
typedef enum {
    ET_NONE = 0,
    ET_REL,
    ET_EXEC
} Elf_Type;

typedef struct {
    uint8_t e_magic[4];
    uint8_t e_bits;
    uint8_t e_endian;
    uint8_t e_hversion;
    uint8_t e_abi;
    uint8_t e_padding[8];
    Elf32_Half e_type;
    Elf32_Half e_machine;
    Elf32_Word e_version;
    Elf32_Word e_entry;
    Elf32_Off e_phoff;
    Elf32_Off e_shoff;
    Elf32_Word e_flags;
    Elf32_Half e_ehsize;
    Elf32_Half e_phentsize;
    Elf32_Half e_phnum;
    Elf32_Half e_shentsize;
    Elf32_Half e_shnum;
    Elf32_Half e_shstridx;
} Elf32_Ehdr;

typedef struct {
    Elf32_Word sh_name;
    Elf32_Word sh_type;
    Elf32_Word sh_flags;
    Elf32_Addr sh_addr;
    Elf32_Off sh_offset;
    Elf32_Word sh_size;
    Elf32_Word sh_link;
    Elf32_Word sh_info;
    Elf32_Word sh_addralign;
    Elf32_Word sh_entsize;
} Elf32_Shdr;

typedef struct {
    Elf32_Word p_type;
    Elf32_Off p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
} Elf32_Phdr;

typedef enum {
    ELF_OK,
    ELF_NOT_FOUND,
    ELF_INVALID_FORMAT,
    ELF_UNSUPPORTED,
    ELF_CODE_SEGMENT_TOO_BIG,
    ELF_DATA_SEGMENT_TOO_BIG,
} ELF_Status;

/* Here come function definitions */
uint32_t crc32(void *data, size_t size);
int check_crc(uint32_t crc1, uint32_t crc2);
int crcread(void *buffer, size_t size);
uint8_t *elf_load_program_serial(void);
ELF_Status elf_load_eeprom(uint16_t fileaddr, uint8_t **entry);
ELF_Status elf_run(const char *filename);

#endif /* _ELF_H */
