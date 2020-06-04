#include <sys/attribs.h>  // __ISR macro
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "i2c_master_noint.h"
#include "ssd1306.h"
#include "font.h"
#include "ws2812b.h"
#include "imu.h"
#include "adc.h"

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

// void *memset(void *buf, int ch, size_t count);

void delay(float time);
int calcDelay(float time);
int intArrayAvg(int * intArray, int len);

void bar_x(int xValue);
void bar_y(int yValue);

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
    imu_setup();
    adc_setup();
    ctmu_setup();
    
    //setPin(0x00, 0x00);   // All A pins are outputs
    setPin(I2C_PIN_ADDR, 0x00, 0x00);
    //setPin(0x01, 0xFF);   // All B pins are inputs
    setPin(I2C_PIN_ADDR, 0x01, 0xFF);
               
    __builtin_enable_interrupts();
    
    char whatsDrawn[50];    // Initialize 50 character limit string
    int timeTrack = 0;      // Initialize tracker for heartbeat

    int numLEDs             = 4;
    float flash             = 0;
    int hueArray[4]         = {120, 120, 240, 240};
    int caliHue[4]          = {0, 0, 0, 0};
    wsColor caliWS2812[numLEDs];
    wsColor triColors[numLEDs];
    
    int avgLen = 8;
    int triClose;
    int arrayTriClose[avgLen];
    memset(arrayTriClose, 0, avgLen);
    int triFar;
    int arrayTriFar[avgLen];
    memset(arrayTriClose, 0, avgLen);
    int triCount        = 0;    // Tracks where in moving average array we are
    int caliClose       = 980;  // No touch calibration for Close
    int caliFar         = 980;  // No touch calibration for Far
    
    while (1) {
        _CP0_SET_COUNT(0);  // Set core timer to 0 at top of while(1) loop    
          
        ssd1306_clear();      

        sprintf(whatsDrawn, "Stark HW7");
        ssd1306_drawString(0, 0, whatsDrawn);
        sprintf(whatsDrawn, "ME 433");
        ssd1306_drawString(98, 0, whatsDrawn);
        
        arrayTriClose[triCount] = ctmu_read(4, 48000000/2/800000);
        arrayTriFar[triCount]   = ctmu_read(5, 48000000/2/800000);
        
        triClose    =   intArrayAvg(arrayTriClose, avgLen);
        triFar      =   intArrayAvg(arrayTriFar, avgLen);

        double brightClose = (caliClose-triClose)/((double) caliClose);
        double brightFar   = (caliFar-triFar)/((double) caliFar);
        
        double pos = ctmu_slider_position(triClose, triFar, caliClose, caliFar);
        
        // 1 [Hz] heartbeat, 0.5 [s] on 0.5 [s] off
        if (timeTrack > calcDelay(0.5)) {
            timeTrack = 0;
            LATAbits.LATA4 = !LATAbits.LATA4;
            
            if (flash == 0.5) {
                flash = 0;
            }
            else {
                flash = 0.5;
            }
            
        }

        // If button is pushed, turn on yellow LED and calibrate CTMU
        if (!(readPin(I2C_PIN_ADDR, 0x13) & 0b00000001)) {
            setPin(I2C_PIN_ADDR, 0x14, 0xFF);  // Turn on GPA7
            
            sprintf(whatsDrawn, "Position: Calibrating ...");
            ssd1306_drawString(0, 12, whatsDrawn);
            
            int kk;
            for (kk = 0; kk < numLEDs; kk++) {
                caliWS2812[kk] = HSBtoRGB(caliHue[kk], 1, flash);
            }
            ws2812b_setColor(caliWS2812, numLEDs);  
            
            caliClose    =   intArrayAvg(arrayTriClose, avgLen);
            caliFar      =   intArrayAvg(arrayTriFar, avgLen);
        }
        else {
            setPin(I2C_PIN_ADDR, 0x14, 0x00);  // Turn off GPA7
                        
            if (((brightClose) < 0.1) && ((brightFar) < 0.1)) { // A percentage error of 0.1 relative to the calibration is required!
                
                sprintf(whatsDrawn, "Position: Untouched.");
                ssd1306_drawString(0, 12, whatsDrawn);
                
                int nope;
                for (nope = 0; nope < numLEDs; nope++) {
                    triColors[nope] = HSBtoRGB(hueArray[nope], 1, 0);
                }
                ws2812b_setColor(triColors, numLEDs);
            }
            else {

                sprintf(whatsDrawn, "Position: %0.2f [cm]", (7.5 - 0.15*pos));
                ssd1306_drawString(0, 12, whatsDrawn);
                
                int yep;
                for (yep = 0; yep < numLEDs/2; yep++) {
                    triColors[yep] = HSBtoRGB(hueArray[yep], 1, brightClose/(10*(brightClose+brightFar)));                
                }
                for (yep = numLEDs/2; yep < numLEDs; yep++) {
                    triColors[yep] = HSBtoRGB(hueArray[yep], 1, brightFar/(10*(brightClose+brightFar)));   
                }
                ws2812b_setColor(triColors, numLEDs);
            }
        }
        
        if (triCount < avgLen) {
            triCount++;
        } else {
            triCount = 0;        
        }
        ssd1306_update();                           // Update screen
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

int intArrayAvg(int * intArray, int len) {
    int sum = 0;
    int ii;
    for (ii = 0; ii < len; ii++) {
        sum = sum + intArray[ii];
    }
    int avg = sum/len;
    return avg;     // Integer average - not floating point!
}

void bar_x(int xValue) {
    int ii;     // for loop
    int xPixel = (int) ((float) 64.0*(xValue / 16384.0) + 64.0);
    
    if (xPixel < 64) {
        for (ii = 64; ii >= xPixel; ii--) {
            ssd1306_drawPixel(ii, 16, 1);
        }  
    }
    else {
        for (ii = 64; ii <= xPixel; ii++) {
            ssd1306_drawPixel(ii, 16, 1);
        }
    }
}

void bar_y(int yValue) {
    int jj;     // for loop
    int yPixel = (int) ((float) 16.0*(yValue / 16384.0) + 16.0);
    
    if (yPixel < 16) {
        for (jj = 16; jj >= yPixel; jj--) {
            ssd1306_drawPixel(64, jj, 1);
        }  
    }
    else {
        for (jj = 16; jj <= yPixel; jj++) {
            ssd1306_drawPixel(64, jj, 1);
        }
    }
}