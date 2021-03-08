#include "display.h"
#include "eeprom.h"
#include "elf.h"
#include "serial.h"
#include "ustar.h"

#include <pic32mx.h>
#include <string.h>

typedef struct {
    uint16_t addr;
    char name[17];
} file_entry_t;
file_entry_t *filelist;
size_t filelist_size;

#define FILE_ENTRY_COUNT 3  // How many files should be shown at once
int oct2bin(const char *data, size_t size);

static void sleepms(int ms) {
    for (; ms > 0; ms--) {
        int i;
        for (i = 6000; i; i--)
            ;
    }
}

static void draw_filelist(size_t start, size_t selected) {
    size_t i, j;
    for (i = start, j = 0; j < FILE_ENTRY_COUNT; i++, j++) {
        // Draw a file entry
        if (i == selected) {
            // Make it inverted
            display_filled_rect(1, j * 11 + 1, 126, 8);
            display_text_inverted(0, j * 11, filelist[i].name);
        } else {
            display_rect(0, j * 11, 128, 10);
            display_text(0, j * 11, filelist[i].name);
        }
    }
}

static size_t get_file_count(void) {
    size_t count, tmpsize;
    uint16_t addr;
    UStar_Fhdr fhdr;
    // Count files
    // TODO: get this into a separate ustar_ function
    for (addr = 0, count = 0; addr <= EEPROM_SIZE - USTAR_BLOCK_SIZE; count++) {
        eeprom_read(addr, &fhdr, sizeof(UStar_Fhdr));

        if (memcmp(fhdr.ustar, "ustar", 5))
            // Not filesystem anymore
            break;

        // Move over header
        addr += USTAR_BLOCK_SIZE;
        tmpsize = oct2bin(fhdr.filesize, sizeof(fhdr.filesize) - 1);
        // Move over file data
        addr += tmpsize;
        // Complete a block
        if (tmpsize % USTAR_BLOCK_SIZE)
            addr += USTAR_BLOCK_SIZE - (tmpsize % USTAR_BLOCK_SIZE);
    }
    return count;
}

static void populate_filelist(void) {
    size_t i, j, tmpsize;
    uint16_t addr;
    UStar_Fhdr fhdr;
    // Count files
    // TODO: get this into a separate ustar_ function
    serial_printf("filelist ptr: 0x%x\r\n", filelist);
    for (addr = 0, i = 0; addr <= EEPROM_SIZE - USTAR_BLOCK_SIZE; i++) {
        serial_printf("reading 0x%x\r\n", addr);
        eeprom_read(addr, &fhdr, sizeof(UStar_Fhdr));
        serial_write("read\r\n");

        if (memcmp(fhdr.ustar, "ustar", 5))
            // Not filesystem anymore
            break;

        serial_write("checked ustar\r\n");
        // Save file address
        filelist[i].addr = addr;

        // Move over header
        addr += USTAR_BLOCK_SIZE;
        tmpsize = oct2bin(fhdr.filesize, sizeof(fhdr.filesize) - 1);
        serial_write("calc tempsize\r\n");

        serial_write("copy addr\r\n");
        // Copy first 16 characters of the filename
        for (j = 0; j < 16; j++)
            filelist[i].name[j] = fhdr.filename[j];
        filelist[i].name[16] = '\0';
        serial_write("copy name\r\n");

        // Move over file data
        addr += tmpsize;
        serial_write("skip data\r\n");
        // Complete a block
        if (tmpsize % USTAR_BLOCK_SIZE)
            addr += USTAR_BLOCK_SIZE - (tmpsize % USTAR_BLOCK_SIZE);
        serial_write("finish block\r\n");
    }
}

void graphical_shell(void) {
    size_t first_visible, selected;
    size_t i;

    display_init();

    // Show intro
    display_text(0, 0, "BTN4: UP");
    display_text(0, 8, "BTN3: DOWN");
    display_text(0, 16, "BTN2: EXIT");
    display_text(0, 24, "BTN1: OPEN/START");
    display_update();

    // Wait for BTN1 press
    // TODO: user mode has io_buttons, why can't kernel too?
    for (;;) {
        if ((PORTF >> 1) & 1)
            break;
    }

    display_clear();
    display_text(0, 0, "Loading files...");
    display_update();

    serial_write("getting file count\r\n");
    filelist_size = get_file_count();
    serial_write("got file count\r\n");

    serial_write("alloc filelist\r\n");
    file_entry_t local_filelist[filelist_size];
    serial_write("alloc filelist ok\r\n");
    for (i = 0; i < filelist_size; i++)
        memset(&local_filelist[i], 0, sizeof(file_entry_t));
    serial_write("zero filelist ok\r\n");

    filelist = local_filelist;
    serial_printf("file count: %d\r\n", filelist_size);
    populate_filelist();
    serial_write("filelist populated\r\n");

    display_clear();
    draw_filelist(0, 0);
    display_update();

    // Main loop
    for (first_visible = 0, selected = 0;;) {
        uint8_t state = 0;
        // BTN1
        state |= (PORTF >> 1) & 1;
        // BTN2-4
        state |= ((PORTD >> 5) & 0b111) << 1;

        if (state & 0b10)
            // BTN2
            break;
        if (state & 0b100) {
            // BTN3 - down
            if (++selected >= filelist_size)
                selected--;
            else {
                if (selected - first_visible >= FILE_ENTRY_COUNT)
                    first_visible = selected - FILE_ENTRY_COUNT + 1;
                display_clear();
                draw_filelist(first_visible, selected);
                display_update();
                sleepms(300);
            }
        } else if (state & 0b1000) {
            // BTN4 - up
            if (selected) {
                if (selected == first_visible)
                    first_visible--;
                selected--;
                display_clear();
                draw_filelist(first_visible, selected);
                display_update();
                sleepms(300);
            }
        } else if (state & 0b1) {
            // BTN1
            display_clear();
            display_update();
            UStar_Fhdr fhdr;
            eeprom_read(filelist[selected].addr, &fhdr, sizeof(UStar_Fhdr));
            serial_printf("exec(%s)\r\n", fhdr.filename);
            // Shutdown the display
            // User code will initialize it again if needed
            display_shutdown();

            // Run the user program
            ELF_Status code = elf_run(fhdr.filename);

            // Turn display back on
            // This also clears it
            display_init();

            switch (code) {
            case ELF_OK: {
                display_text(0, 0, "ELF exec OK");
                break;
            }
            case ELF_NOT_FOUND: {
                display_text(0, 0, "File not found");
                break;
            }
            case ELF_INVALID_FORMAT: {
                display_text(0, 0, "Invalid format");
                break;
            }
            case ELF_UNSUPPORTED: {
                display_text(0, 0, "Unsupported ELF");
                break;
            }
            case ELF_CODE_SEGMENT_TOO_BIG: {
                display_text(0, 0, "CODESEG too big");
                break;
            }
            case ELF_DATA_SEGMENT_TOO_BIG: {
                display_text(0, 0, "DATASEG too big");
                break;
            }
            default: {
                display_text(0, 0, "Unknown result");
                break;
            }
            }
            display_update();

            // Make sure result is seen
            sleepms(3000);
            display_clear();
            draw_filelist(first_visible, selected);
            display_update();
        }
    }

    // End display
    display_shutdown();
}
