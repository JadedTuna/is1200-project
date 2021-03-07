#ifdef COMPILE_TESTS

    #include "eeprom.h"
    #include "elf.h"
    #include "i2c.h"
    #include "rom.h"
    #include "serial.h"

void simple_delay(int cycles);

void rom_test(void) {
    int b;
    itwoc_setup();
    byte buffer[] = "this is text much texty very text haha";
    rom_address_t addr = 0x012;

    serial_printf("should see: %c at %x\r\n", *buffer, addr);
    if (rom_write_byte(addr, buffer[0]))
        serial_write("writing single byte errored\r\n");

    b = rom_read_byte(addr);
    if (b < 0)
        serial_printf("reading single bit errored: %d\r\n", b);

    serial_printf("byte at same address: %c\r\n", b);
    addr += 128;
    rom_write_page(addr, buffer + 5, sizeof(buffer) - 5);
    serial_printf("now saving at %x: %s\r\n", addr, buffer + 5);
    byte readbuffer[100] = { 0 };
    rom_read_sequential(addr, readbuffer, sizeof(buffer) - 5);
    serial_printf("has read: %s", readbuffer);
    serial_write("test complete\r\n");
}

void rom_test_wd(void) {
    i2c_setup();

    char buf[64] = { 0 };

    int i;
    for (i = 0; i < 64; i++)
        buf[i] = i;

    serial_write("input 8 characters: ");
    serial_read(buf, 8);
    PORTE = 0x0F;

    buf[9] = 0;
    serial_printf("Got: %s\r\n", buf);
    serial_printf("wrote %d bytes\r\n", eeprom_write(0, buf, 8));
    PORTE = 10;
    simple_delay(1000);
    serial_printf("read %d bytes\r\n", eeprom_read(0, buf, 8));
    serial_hexdump(buf, 8);
    serial_write("trying again\r\n");
    for (i = 0; i < 64; i++)
        buf[i] = 0;
    serial_hexdump(buf, 8);

    serial_printf("read %d bytes\r\n", eeprom_read(0, buf, 8));
    serial_hexdump(buf, 8);
}

void rom_test_wd2(void) {
    i2c_setup();

    char buffer[256];
    int i;
    for (i = 0; i <= 0xFF; i++)
        buffer[i] = i;

    uint32_t crc = crc32(buffer, sizeof(buffer));
    serial_write("writing\r\n");
    eeprom_write(0, buffer, 256);
    serial_write("reading\r\n");
    eeprom_read(0, buffer, 256);
    serial_hexdump(buffer, 256);
    uint32_t crc2 = crc32(buffer, sizeof(buffer));
    check_crc(crc, crc2);
}

#endif /* COMPILE_TESTS */
