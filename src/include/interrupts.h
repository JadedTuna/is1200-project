#ifndef _INTERRUPTS_H
#define _INTERRUPTS_H

static inline void enable_interrupts(void) { __asm__("ei"); }
static inline void disable_interrupts(void) {
    __asm__("di");
    __asm__("ehb");
}

void setup_interrupts(void);
void enable_interrupts(void);
void disable_interrupts(void);
void default_isr(int);

#endif /* _INTERRUPTS_H */
