#include "mipslab.h"

#include <pic32mx.h>
#include <stdint.h>

int getsw(void) { return (PORTD & 0xF00) >> 8; }

int getbtns(void) { return (PORTD & 0x0E0) >> 5; }
