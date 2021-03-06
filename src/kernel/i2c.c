#include "i2c.h"

#include <pic32mx.h>

/**
 * Setup registers for communication over I2C1
 */
void i2c_setup(void) {
    volatile uint8_t clear;

    // Initialize
    I2C1CON = 0;
    I2C1STAT = 0;
    // Set baud rate
    // Pre-calculated, from 61116F
    // Formula: ((1/(2*Fsck) - Tpgd) * PBCLK) - 2
    // 0xC2 - 100 kHz, 0x2C - 400 kHz
    I2C1BRG = 0xC2;
    // Turn on
    I2C1CONSET = PIC32_I2CCON_ON;
    // Clear anything from the receive register
    // This seems to help sometimes
    clear = I2C1RCV;
}

/**
 * Wait until transmission is complete or a condition has been initiated.
 */
void i2c_flush(void) {
    /* Can't use this too much. */
    // Wait until TRSTAT is clear (end of master transmission)
    // or master mode bits are all unset (5 low)
    // Technically, only 4 low bits need to be checked, since
    // 5th is ACKDT, whether ACK or NACK is to be sent (0 is ACK)
    // Futhermore, they are the ones cleared by module
    while ((I2C1STAT & PIC32_I2CSTAT_TRSTAT) || (I2C1CON & 0b11111))
        ;
}

/**
 * Begin I2C transaction.
 */
void i2c_begin(void) {
    i2c_flush();
    I2C1CONSET = PIC32_I2CCON_SEN; // Start Condition
    i2c_flush();
}

/**
 * End I2C transaction.
 */
void i2c_end(void) {
    i2c_flush();
    I2C1CONSET = PIC32_I2CCON_PEN; // Stop Condition
    i2c_flush();
}

/**
 * Send a single byte over I2C.
 *
 * @param byte Byte to send.
 * @return 1 if ACK, 0 if NACK.
 */
int i2c_send(uint8_t byte) {
    i2c_flush();
    I2C1TRN = byte;
    i2c_flush();

    return !(I2C1STAT & PIC32_I2CSTAT_ACKSTAT);
}

/**
 * Receive a single byte over I2c.
 *
 * @return Received byte.
 */
uint8_t i2c_recv(void) {
    i2c_flush();
    // Ready to receive
    I2C1CONSET = PIC32_I2CCON_RCEN;
    i2c_flush();
    // Inform that we have consumed the byte
    I2C1STATCLR = PIC32_I2CSTAT_I2COV;
    return I2C1RCV;
}

/**
 * Send an ACK.
 */
void i2c_ack() {
    i2c_flush();
    I2C1CONCLR = PIC32_I2CCON_ACKDT;
    I2C1CONSET = PIC32_I2CCON_ACKEN;
    i2c_flush();
}

/**
 * Send a NACK.
 */
void i2c_nack() {
    i2c_flush();
    I2C1CONSET = PIC32_I2CCON_ACKDT;
    I2C1CONSET = PIC32_I2CCON_ACKEN;
    i2c_flush();
}
