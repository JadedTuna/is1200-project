#ifndef _PROG_DISPLAY_H
#define _PROG_DISPLAY_H

#include <stddef.h>
#include <stdint.h>

#define RF_SLM_DATA (1 << 4)
#define RF_SLM_VBAT (1 << 5)
#define RF_SLM_VDD (1 << 6)
#define RG_SLM_RESET (1 << 9)

#define DPY_BUFFER_SIZE 512
#define DPY_PAGES 4
#define DPY_COLS 128
#define DPY_ROWS 32

extern uint8_t DPY_BUFFER[DPY_BUFFER_SIZE];
extern const uint8_t const font[128 * 8];

void display_init(void);
uint8_t display_send_byte(uint8_t byte);
void display_update(void);
void display_shutdown(void);
void display_clear(void);
void display_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void display_ntext(uint8_t x, uint8_t y, const char *text, size_t size);
void display_text(uint8_t x, uint8_t y, const char *text);

#endif /* _PROG_DISPLAY_H */
