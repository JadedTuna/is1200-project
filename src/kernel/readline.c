#include "serial.h"

size_t serial_readline(char *buffer, size_t size) {
    char c = 0;
    size_t i = 0, j;
    size_t maxind = 0;

    do {
        c = serial_getc();

        if (0x1b == c && (c = serial_getc()) == '[') {
            // This is an escape sequence
            c = serial_getc();
            switch (c) {
            case 'D': {
                // Left arrow
                if (i) {
                    serial_write("\x1b[D");
                    i--;
                }
                break;
            }
            case 'C': {
                // Right arrow
                if (i < maxind) {
                    serial_write("\x1b[C");
                    i++;
                }
                break;
            }
            }
            continue;
        }

        // Backspace
        if (0x08 == c || 0x7F == c) {
            if (i) {
                // Overwrite previous character with space and move there
                serial_write("\x1b[D \x1b[D");
                i--;
                // Move characters in memory
                for (j = i; j < maxind; j++) {
                    buffer[j] = buffer[j + 1];
                }
                // Move characters on the screen
                serial_nwrite(buffer + i, maxind - i - 1);
                serial_putc(' ');
                // Move back to where we were
                for (j = i; j < maxind; j++)
                    serial_write("\x1b[D");
                maxind--;
            }
        }

        if (c >= 0x20 && c < 0x7F && i < size - 1) {
            // Printable ascii
            if (i == maxind) {
                // End of line
                serial_putc(c);
                buffer[i++] = c;
                maxind = i;
            } else if (i < maxind) {
                // Somewhere mid-line
                if (++maxind < size) {
                    // Can add characters
                    // Move chars to the right
                    for (j = maxind; j > i; j--)
                        buffer[j] = buffer[j - 1];
                    // Put current char
                    buffer[i] = c;
                    // Redraw the rest of the line
                    serial_nwrite(buffer + i, maxind - i);
                    // Get back to where we were but forward by one
                    for (j = i; j < maxind - 1; j++)
                        serial_write("\x1b[D");

                    i++;
                } else {
                    // Too many characters already
                    maxind--;
                }
            }
        }
    } while (0x0D != c);
    serial_write("\r\n");
    // serial_printf("i: %d, maxind: %d\r\n", i, maxind);
    buffer[maxind] = '\0';

    return maxind;
}
