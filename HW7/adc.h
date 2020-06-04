#ifndef ADC_H__
#define ADC_H__

#include <xc.h>

void adc_setup();
unsigned int adc_sample_convert(int pin);

void ctmu_setup();
int ctmu_read(int pin, int delay);

// My functions
double ctmu_slider_position(int triClose, int triFar, int caliClose, int caliFar);

#endif