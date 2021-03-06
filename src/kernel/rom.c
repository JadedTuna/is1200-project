#include "rom.h"

/* eeprom control byte (address + R) set to write */
#define _ROM_I2CADDR_W 0xA0
/* eeprom control byte (address + R) set to read */
#define _ROM_I2CADDR_R 0xA1

/**/
#define _ITWOC_SETUP_DELAY(delay)                    \
    do {                                             \
        int __priv;                                  \
        for (__priv = (delay); __priv > 0; __priv--) \
            ;                                        \
    } while (0)

/* i2c control register bits*/
#define _ENABLE BIT_AT(15)
#define __UNIMPLEMENTED BIT_AT(14)
#define _STOP_IN_IDLE BIT_AT(13)
#define _RELEASE_CLK BIT_AT(12)
#define _IPMI_ENABLE BIT_AT(11)
#define _10BITSADDR BIT_AT(10)
#define _SLEW_DIS BIT_AT(9)
#define _SM_LVLS BIT_AT(8)
#define _GCALL_EN BIT_AT(7)
#define _CLKSTRETCH_EN BIT_AT(6)
#define _ACK_DATA BIT_AT(5)
#define _ACK_EN BIT_AT(4)
#define _RCV_EN BIT_AT(3)
#define _STOP_EN BIT_AT(2)
#define _RESTART_EN BIT_AT(1)
#define _START_EN BIT_AT(0)

/* i2c status register bits*/
#define _ACKSTAT BIT_AT(15)
#define _TRSTAT BIT_AT(14)
#define _STOP_IN_IDLE BIT_AT(13)
#define _RELEASE_CLK BIT_AT(12)
#define _IPMI_ENABLE BIT_AT(11)
#define _10BITSADDR BIT_AT(10)
#define _SLEW_DIS BIT_AT(9)
#define _SM_LVLS BIT_AT(8)
#define _GCALL_EN BIT_AT(7)
#define _CLKSTRETCH_EN BIT_AT(6)
#define _ACK_DATA BIT_AT(5)
#define _ACK_EN BIT_AT(4)
#define _RCV_EN BIT_AT(3)
#define _STOP_EN BIT_AT(2)
#define _RESTART_EN BIT_AT(1)
#define _START_EN BIT_AT(0)

/* i2c interrupt flag bit */
#define _INTFLAG_BIT BIT_AT(31)
#define _BUSFLAG_BIT BIT_AT(29)

typedef rom_address_t address_t;

/* waits for the i2c master event interrupt flag to be set and clears it
 *  USE CAREFULLY, THIS CAN DEADLOCK
 */
static inline void _itwoc_wait_clear(void) {
    while (!(IFS(0) & _INTFLAG_BIT)) {
        IFSCLR(0) = _BUSFLAG_BIT;
    }
    IFSCLR(0) = _INTFLAG_BIT;
}

/* send a single byte down the i2c line; return ack status (0 is ACK, anything else is NACK)*/
static inline int _itwoc_transact_byte(byte b) {
    I2C1TRN = b;
    _itwoc_wait_clear();
    return I2C1STAT & _ACKSTAT;
}

static inline void _itwoc_send_stop(void) {
    I2C1CONSET = _STOP_EN;
    _itwoc_wait_clear();
}

static inline void _itwoc_send_ack(void) {
    I2C1CONSET = _ACK_EN;
    _itwoc_wait_clear();
}

static inline byte _itwoc_receive_byte(void) {
    I2C1CONSET = _RCV_EN; // prepare to receive
    _itwoc_wait_clear();  // wait for end of transmission
    return I2C1RCV;
}

void _rom_start_R(void) {
    do {
        I2C1CONSET = _START_EN;                     // send start
        _itwoc_wait_clear();                        // wait for it
    } while (_itwoc_transact_byte(_ROM_I2CADDR_R)); // send address and restart if NACK
}

int _rom_start_and_send_address_W(address_t addr) {
    do {
        I2C1CONSET = _START_EN;                     // send start
        _itwoc_wait_clear();                        // wait for it
    } while (_itwoc_transact_byte(_ROM_I2CADDR_W)); // send address and restart if NACK

    if (_itwoc_transact_byte(addr >> 8)) // send high byte & error if NACK
        return -1;
    if (_itwoc_transact_byte(addr & 0xf)) // send low byte & error if NACK
        return -1;
    return 0;
}

void itwoc_setup(void) {
    I2C1ADD = 3;
    I2C1BRG = 0x184;
    I2C1CON = _ENABLE;
    _ITWOC_SETUP_DELAY(10);
    IFSCLR(0) = _INTFLAG_BIT | _BUSFLAG_BIT;
    _ITWOC_SETUP_DELAY(60);
}

int rom_write_byte(address_t addr, byte b) {
    int ret = 0;
    if (_rom_start_and_send_address_W(addr))
        ret = -1;
    else if (_itwoc_transact_byte(b))
        ret = -1;
    _itwoc_send_stop();
    return ret;
}

int rom_write_page(address_t addr, byte buffer[], int len) {
    if (len > 64)
        len = 64; // we may want to error just to be clear that we didn't really write the buffer
    if (_rom_start_and_send_address_W(addr)) {
        _itwoc_send_stop();
        return -1;
    }
    int i;
    for (i = 0; i < len; i++) {
        if (_itwoc_transact_byte(buffer[i])) {
            break;
        }
    }
    _itwoc_send_stop();
    return i;
}

byte rom_read_current_byte(void) {
    _rom_start_R();
    byte ret = _itwoc_receive_byte();
    _itwoc_send_stop();
    return ret;
}

int rom_read_byte(address_t addr) {
    if (_rom_start_and_send_address_W(addr)) {
        return -1;
    }
    return rom_read_current_byte();
}

int rom_read_sequential(address_t addr, byte buffer[], int len) {
    /* we may want to check for wraparound at address 0x7fff */
    if (_rom_start_and_send_address_W(addr)) {
        return -1;
    }
    _rom_start_R();
    int i;
    for (i = 0; i < len; i++) {
        buffer[i] = _itwoc_receive_byte();
        _itwoc_send_ack();
    }
    _itwoc_send_stop();
    return i;
}
