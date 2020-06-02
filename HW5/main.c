#include <sys/attribs.h>  // __ISR macro
#include <stdio.h>
#include "i2c_master_noint.h"
#include "ssd1306.h"
#include "font.h"
#include "ws2812b.h"

// DEVCFG0
#pragma config DEBUG = OFF // disable debugging
#pragma config JTAGEN = OFF // disable jtag
#pragma config ICESEL = ICS_PGx1 // use PGED1 and PGEC1
#pragma config PWP = OFF // disable flash write protect
#pragma config BWP = OFF // disable boot write protect
#pragma config CP = OFF // disable code protect

// DEVCFG1
#pragma config FNOSC = PRIPLL // use primary oscillator with pll
#pragma config FSOSCEN = OFF // disable secondary oscillator
#pragma config IESO = OFF // disable switching clocks
#pragma config POSCMOD = HS // high speed crystal mode
#pragma config OSCIOFNC = OFF // disable clock output
#pragma config FPBDIV = DIV_1 // divide sysclk freq by 1 for peripheral bus clock
#pragma config FCKSM = CSDCMD // disable clock switch and FSCM
#pragma config WDTPS = PS1048576 // use largest wdt
#pragma config WINDIS = OFF // use non-window mode wdt
#pragma config FWDTEN = OFF // wdt disabled
#pragma config FWDTWINSZ = WINSZ_25 // wdt window at 25%

// DEVCFG2 - get the sysclk clock to 48MHz from the 8MHz crystal
#pragma config FPLLIDIV = DIV_2 // divide input clock to be in range 4-5MHz
#pragma config FPLLMUL = MUL_24 // multiply clock after FPLLIDIV
#pragma config FPLLODIV = DIV_2 // divide clock after FPLLMUL to get 48MHz

// DEVCFG3
#pragma config USERID = 0 // some 16bit userid, doesn't matter what
#pragma config PMDL1WAY = OFF // allow multiple reconfigurations
#pragma config IOL1WAY = OFF // allow multiple reconfigurations

void delay(float time);
int calcDelay(float time);

int main() {

    __builtin_disable_interrupts(); // disable interrupts while initializing things

    // set the CP0 CONFIG register to indicate that kseg0 is cacheable (0x3)
    __builtin_mtc0(_CP0_CONFIG, _CP0_CONFIG_SELECT, 0xa4210583);

    // 0 data RAM access wait states
    BMXCONbits.BMXWSDRM = 0x0;

    // enable multi vector interrupts
    INTCONbits.MVEC = 0x1;
    
    // disable JTAG to get pins back
    DDPCONbits.JTAGEN = 0;

    // do your TRIS and LAT commands here
    // A4 is pin 12, B4 is pin 11
    TRISAbits.TRISA4 = 0;
    TRISBbits.TRISB4 = 1;
    LATAbits.LATA4 = 0;     // Initially off
    
    // Setup i2c, ssd1306, and ws2812b
    i2c_master_setup();
    ssd1306_setup();
    ws2812b_setup();
    
    setPin(0x00, 0x00);   // All A pins are outputs
    setPin(0x01, 0xFF);   // All B pins are inputs
               
    __builtin_enable_interrupts();
    
    char whatsDrawn[50];    // Initialize 50 character limit string
    int timeTrack = 0;      // Initialize tracker for heartbeat

    int numLEDs             = 4;
    int hueSpeed            = 1;
    float satSpeed          = hueSpeed / 60.0;
    float brightSpeed       = -0.10 * hueSpeed / 60.0;
       
    int hueArray[4]         = {0, 120, 180, 240};
    float satArray[4]       = {0.00, 0.25, 0.50, 0.75};
    float brightArray[4]    = {0.10, 0.10, 0.10, 0.10};
    wsColor rainbowArray[numLEDs];
   
    while (1) {
        _CP0_SET_COUNT(0);  // Set core timer to 0 at top of while(1) loop
       
        sprintf(whatsDrawn, "Charles Stark");
        ssd1306_drawString(0, 0, whatsDrawn);
        sprintf(whatsDrawn, "Homework 5");
        ssd1306_drawString(0, 8, whatsDrawn);
        sprintf(whatsDrawn, "June 1st, 2020");
        ssd1306_drawString(0, 16, whatsDrawn);
        sprintf(whatsDrawn, "Taste the Rainbow!");
        ssd1306_drawString(0, 24, whatsDrawn);
        
        ssd1306_update();                           // Update screen      
        
        int kk;     // for loop
        for (kk = 0; kk < numLEDs; kk++) {
            rainbowArray[kk] = HSBtoRGB(hueArray[kk/* numLEDs-(kk+1) */], /* satArray[kk] */ 1, brightArray[kk]);
            hueArray[kk] = hueArray[kk] + hueSpeed;
            satArray[kk] = satArray[kk] + satSpeed;
            brightArray[kk] = brightArray[kk] + brightSpeed;
                    
            if (hueArray[kk] > 360) {
                hueArray[kk] = 0;
            }
            if (satArray[kk] > 1) {
                satArray[kk] = 0;
            }
            if (brightArray[kk] < 0) {
                brightArray[kk] = 0.10;
            }
        }
        
        ws2812b_setColor(rainbowArray, numLEDs);      

        // 1 [Hz] heartbeat, 0.5 [s] on 0.5 [s] off
        if (timeTrack > calcDelay(0.5)) {
            timeTrack = 0;
            LATAbits.LATA4 = !LATAbits.LATA4;
        }

        // If button is pushed, turn on yellow LED
        if (!(readPin(0x13) & 0b00000001)) {
            setPin(0x14, 0xFF);  // Turn on GPA7
        }
        else {
            setPin(0x14, 0x00);  // Turn off GPA7
        }
        timeTrack = timeTrack + _CP0_GET_COUNT();   // Increment timeTrack for heartbeat LED
    }
}

void delay(float time) {    // Time is the value in seconds
    int delay = 24000000*time;  // Multiply time by 24 MHz
    while (_CP0_GET_COUNT() < delay) {
        ;   // Wait
    }
    _CP0_SET_COUNT(0);    
}

int calcDelay(float time) {
    int delay = 24000000*time;  // Returns needed delay given time in seconds
    return delay;
}