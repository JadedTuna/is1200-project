/* mipslabwork.c

   This file written 2015 by F Lundevall
   Updated 2017-04-21 by F Lundevall

   This file should be changed by YOU! So you must
   add comment(s) here with your name(s) and date(s):

   This file modified 2017-04-31 by Ture Teknolog

   For copyright and licensing, see file COPYING */

#include "mipslab.h" /* Declatations for these labs */

#include <pic32mx.h> /* Declarations of system-specific addresses etc */
#include <stdint.h>  /* Declarations of uint_32 and the like */

int mytime = 0x5957;
int timeoutcount = 0;

char textstring[] = "text, more text, and even more text!";

/* Interrupt Service Routine */
void user_isr(void) { return; }

/* Lab-specific initialization goes here */
void labinit(void) {
    // volatile int * trise = (int *)0xbf886100;
    // *trise &= ~0xFF;
    TRISECLR = 0xFF;  // leds as output
    TRISDSET = 0xFE0; // 1111_111x_xxxx, buttons and switches as input
    TMR2 = 0;
    PR2 = 50 * 625; // 2 * 5^6 = 100ms / (256 * 1/80 µs)
    T2CON = 0xA070;
    return;
}

/* This function is called repetitively from the main program */
void labwork(void) {
    // volatile int * porte = (int *)0xbf886110;
    static int count = 0;
    int btns;
    int sws;
    btns = getbtns();
    sws = getsw();
    if (IFS(0) & 1 << 8) {
        IFSCLR(0) = 1 << 8;
        timeoutcount++;
    } else {
        return;
    }
    if (timeoutcount % 10 == 0) {
        timeoutcount = 0;
        for (int i = 0; i < 3; i++) {
            if (btns & (1 << i)) {
                int digit_select = 0xF << 4 * (i + 1);
                mytime = (mytime & ~digit_select) | (sws << 4 * (i + 1));
            }
        }
        time2string(textstring, mytime);
        display_string(3, textstring);
        display_update();
        // *porte = (*porte & ~0xFF) | (count & 0xFF);
        PORTECLR = 0xFF;
        PORTESET = count & 0xFF;
        tick(&mytime);
        count++;
        display_image(96, icon);
    }
}
