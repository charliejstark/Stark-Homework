#include<xc.h>           // processor SFR definitions
#include<sys/attribs.h>  // __ISR macro
#include<stdio.h>

#include "spi.h"
#include "ws2812b.h"

// DEVCFG0
#pragma config DEBUG = OFF // disable debugging
#pragma config JTAGEN = OFF // disable jtag
#pragma config ICESEL = ICS_PGx1 // use PGED1 and PGEC1
#pragma config PWP = OFF // disable flash write protect
#pragma config BWP = OFF // disable boot write protect
#pragma config CP = OFF // disable code protect

// DEVCFG1
#pragma config FNOSC = FRCPLL // use internal oscillator with pll
#pragma config FSOSCEN = OFF // disable secondary oscillator
#pragma config IESO = OFF // disable switching clocks
#pragma config POSCMOD = OFF // internal RC
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

float if360(float H);



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
    // Pin 12 is A4, pin 11 is B4
    TRISBbits.TRISB4 = 1;
    TRISAbits.TRISA4 = 0;
    LATAbits.LATA4   = 0;
    
    
    
    // HW10    
    ws2812b_setup();
    
    int numLEDs = 4;
    
    wsColor c[numLEDs];

    float H0 = 0;
    float H1 = 90;
    float H2 = 180;
    float H3 = 270;
    
    
    c[0] = HSBtoRGB(H0,1,0.05);
    c[1] = HSBtoRGB(H1,1,0.05);
    c[2] = HSBtoRGB(H2,1,0.05);
    c[3] = HSBtoRGB(H3,1,0.05);
    
    __builtin_enable_interrupts(); // enable interrupts after initializing things
    
    _CP0_SET_COUNT(0);
    while (1) {    
        // 1 [HZ] heartbeat
        if (_CP0_GET_COUNT() > ((48000000 / 2 / 2))) {
            LATAbits.LATA4 = !LATAbits.LATA4;
            
            
            H0 = if360(H0);
            H1 = if360(H1);
            H2 = if360(H2);
            H3 = if360(H3);
            
            _CP0_SET_COUNT(0);
        }     
        
        c[0] = HSBtoRGB(H0,1,0.05);
        c[1] = HSBtoRGB(H1,1,0.05);
        c[2] = HSBtoRGB(H2,1,0.05);
        c[3] = HSBtoRGB(H3,1,0.05);
        
        
        ws2812b_setColor(c, numLEDs);
               
    }
}

float if360(float H) {

    if (H >= 360) {
        H = 0;
    }
    else {
        H = H + 10;
    }

    return H;
}
