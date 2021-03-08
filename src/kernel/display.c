#include "display.h"

#include "serial.h"

#include <pic32mx.h>
#include <string.h>

uint8_t DPY_BUFFER[DPY_BUFFER_SIZE];
int DPY_DID_INIT = 0;

/**
 * Used in other places, but for the sake of clarity.
 */
static void sleepms(int ms) {
    for (; ms > 0; ms--) {
        int i;
        for (i = 6000; i; i--)
            ;
    }
}

/**
 * Configure SPI2 in master mode with idle clock HIGH.
 * Used for display communication.
 */
static void spi2_init(void) {
    /* Configure SPI */
    serial_write("Initializing SPI2 in master mode\r\n");
    /* Section 23. SPI. 23.3.2.1 */
    // Disable interrupts for SPI2
    IECCLR(1) = 0b111 << 5;
    // Reset config
    SPI2CON = 0;
    // Clear input buffer
    SPI2BUF = 0;
    // Set baud rate
    // Fsck = PBCLK/(2*(SPIxBRG + 1))
    // This gives 4MHz clock with PBCLK = 40MHz
    // Max clock for display comms is 10MHz
    SPI2BRG = 4;
    // Clear receive overflow bit
    SPI2STATCLR = PIC32_SPISTAT_SPIROV;
    // Configure SPI
    // Idle clock is HIGH, MODE is Master
    /* (Idle clock) Basic I/O Shield -> Display Example */
    SPI2CONSET = PIC32_SPICON_CKP | PIC32_SPICON_MSTEN;
    // Enable SPI
    SPI2CONSET = PIC32_SPICON_ON;
    serial_write("SPI2 configured and enabled.\r\n");
}

/**
 * Initialize pins used for display control.
 * Control display & chip power, chip data/cmd and chip reset.
 */
static void display_pins_init(void) {
    /* Configure pins */
    serial_write("Configuring pins.\r\n");
    // RF4-6 to HIGH
    PORTFSET = RF_SLM_DATA | RF_SLM_VBAT | RF_SLM_VDD;
    // RF4-6 are outputs
    TRISFCLR = RF_SLM_DATA | RF_SLM_VBAT | RF_SLM_VDD;

    // RG9 to HIGH
    PORTGSET = RG_SLM_RESET;
    // RG9 is output
    TRISGCLR = RG_SLM_RESET;

    serial_write("Pins configured.\r\n");
}

/**
 * Initialize the display by turning on and configuring the Solomon chip.
 */
static void solomon_init(void) {
    /* Initialize display */
    serial_write("Initializing display.\r\n");
    // We will be sending commands, clear data/cmd bit
    PORTFCLR = RF_SLM_DATA;

    // Turn Vdd on and wait for power to come up.
    PORTFCLR = RF_SLM_VDD;
    sleepms(1);

    // Turn display off
    display_send_byte(0xAE);

    // Bring reset low, wait for the driver to reset, turn reset high
    PORTGCLR = RG_SLM_RESET;
    sleepms(1);
    PORTGSET = RG_SLM_RESET;

    // TODO: explain
    display_send_byte(0x8D);
    display_send_byte(0x14);

    display_send_byte(0xD9);
    display_send_byte(0xF1);

    // Turn VBAT on and wait 100ms
    PORTFCLR = RF_SLM_VBAT;
    sleepms(100);

    // Invert display, to put origin in top-left corner
    // TODO: explain
    display_send_byte(0xA1);
    display_send_byte(0xC8);

    // Select sequential COM configuration.
    // Display memory is then non-interleaved.
    display_send_byte(0xDA);
    display_send_byte(0x20);

    // Turn display on
    display_send_byte(0xAF);

    serial_write("Display on.\r\n");
}

/**
 * Initialize display by configuring SPI2, pins and display driver chip.
 */
void display_init(void) {
    if (display_did_init())
        // Don't init twice
        return;
    /* Zero out buffer */
    int i;
    for (i = 0; i < DPY_BUFFER_SIZE; i++)
        DPY_BUFFER[i] = 0;

    /* Initialize everything else */
    spi2_init();
    display_pins_init();
    solomon_init();

    DPY_DID_INIT = 1;
}

/**
 * Check whether the display is initialized.
 *
 * @return Display init status.
 */
inline int display_did_init(void) {
    return DPY_DID_INIT;
}

/**
 * Perform a correct display shutdown.
 */
void display_shutdown(void) {
    // Display off comand
    PORTFCLR = RF_SLM_DATA;
    display_send_byte(0xAE);
    // just in case
    sleepms(10);

    // Turn VBAT off and wait 100ms
    PORTFSET = RF_SLM_VBAT;
    sleepms(100);

    // Turn Vdd off
    PORTFSET = RF_SLM_VDD;

    // Not initialized anymore
    DPY_DID_INIT = 0;
}

/**
 * Send a single byte to display over SPI2.
 *
 * @param byte Data/command byte.
 * @return Response byte.
 */
uint8_t display_send_byte(uint8_t byte) {
    // Wait until transmit buffer is empty
    while (!(SPI2STAT & PIC32_SPISTAT_SPITBE))
        ;

    // Send data/command byte
    SPI2BUF = byte;

    // Wait until receive buffer is full
    // This is our confirmation of received command
    while (!(SPI2STAT & PIC32_SPISTAT_SPIRBF))
        ;
    // Return received byte & clear the buffer
    return SPI2BUF;
}

/**
 * Update the display, sending over data from local buffer.
 */
void display_update(void) {
    // page addr
    size_t i, j;
    for (i = 0; i < DPY_PAGES; i++) {
        PORTFCLR = RF_SLM_DATA;
        display_send_byte(0x22);
        display_send_byte(i);

        // start at left column
        display_send_byte(0x00);

        display_send_byte(0x10);
        // display_send_byte(0x00);

        PORTFSET = RF_SLM_DATA;
        for (j = 0; j < DPY_COLS; j++)
            display_send_byte(j[DPY_BUFFER + (i * DPY_COLS)]);
    }
}

/**
 * Clear the display.
 */
void display_clear(void) {
    int i;
    for (i = 0; i < DPY_BUFFER_SIZE; i++)
        DPY_BUFFER[i] = 0;
}

/**
 * Draw a hollow rectangle (with wraparound and down).
 */
void display_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    uint8_t row, column;
    // DPY_BUFFER[0] = 0xFF;
    // Draw left side
    column = x;
    for (row = y; row < y + h; row++)
        DPY_BUFFER[(row / 8) * DPY_COLS + column] |= 1 << (row % 8);
    // Draw right side
    column = x + w - 1;
    for (row = y; row < y + h; row++)
        DPY_BUFFER[(row / 8) * DPY_COLS + column] |= 1 << (row % 8);

    // Draw top side
    row = y;
    for (column = x + 1; column < x + w - 1; column++)
        DPY_BUFFER[(row / 8) * DPY_COLS + column] |= 1 << (row % 8);

    // Draw bottom side
    row = y + h - 1;
    for (column = x + 1; column < x + w - 1; column++)
        DPY_BUFFER[(row / 8) * DPY_COLS + column] |= 1 << (row % 8);
}

void display_filled_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    uint8_t row, column;

    // Draw from left to right, column by column
    for (column = x; column < x + w; column++) {
        for (row = y; row < y + h; row++) {
            DPY_BUFFER[(row / 8) * DPY_COLS + column] |= 1 << (row % 8);
        }
    }
}

void display_ntext(uint8_t x, uint8_t y, const char *text, size_t size, int invert_color) {
    size_t i, c, r, offset;
    uint8_t row, column;
    uint8_t coldata, bit;

    for (i = 0; i < size; i++) {
        // Render each character
        for (c = 0; c < 8; c++) {
            // Render each column
            column = x + c + i * 8;
            coldata = font[text[i] * 8 + c];
            for (row = y, r = 0; row < y + 8; row++, r++) {
                // Render each row bit
                bit = (coldata >> r) & 1;
                offset = (row / 8) * DPY_COLS + column;
                if (invert_color)
                    DPY_BUFFER[offset] &= ~(bit << (row % 8));
                else
                    DPY_BUFFER[offset] |= bit << (row % 8);
            }
        }
    }
}

void display_text(uint8_t x, uint8_t y, const char *text) {
    display_ntext(x, y, text, strlen(text), 0);
}

void display_text_inverted(uint8_t x, uint8_t y, const char *text) {
    display_ntext(x, y, text, strlen(text), 1);
}
