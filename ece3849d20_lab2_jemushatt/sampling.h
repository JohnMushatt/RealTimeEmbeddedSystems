/*
 * sampling.h
 *
 *  Created on: Apr 6, 2020
 *      Author: johnm
 */

#ifndef SAMPLING_H_
#define SAMPLING_H_
// Standard C libraries

#include <stdint.h>
#include <stdio.h>
#include <math.h>

// Driver libraries from TivaWare
#include "Crystalfontz128x128_ST7735.h"
#include "inc/hw_memmap.h"
//#include "inc/hw_ints.h"
#include "inc/tm4c1294ncpdt.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "driverlib/adc.h"
#include "sysctl_pll.h"
#include "driverlib/pwm.h"
#include "driverlib/pin_map.h"
#include "display.h"
#include "kiss_fft.h"
#include "_kiss_fft_guts.h"
#include "buttons.h"
#include <stdbool.h>

#define PWM_FREQUENCY 20000 // PWM frequency = 20 kHz
#define ADC_BUFFER_SIZE 2048
#define ADC_BUFFER_WRAP(i) ((i) & (ADC_BUFFER_SIZE -1))
#define ADC_OFFSET 2048
#define ADC_BITS 12
#define VIN_RANGE 3.3f
/**
 * FFT Section
 */
#define SPECTRUM_LENGTH 1024
#define PI 3.14159265358979f
#define NFFT 1024 // FFT length
#define KISS_FFT_CFG_SIZE (sizeof(struct kiss_fft_state)+sizeof(kiss_fft_cpx)*(NFFT-1))

int16_t fft_buffer[NFFT];

volatile int32_t gADCBufferIndex;// = ADC_BUFFER_SIZE -1;
volatile uint16_t gADCBuffer[ADC_BUFFER_SIZE];
uint16_t local_sample_buffer[LCD_HORIZONTAL_MAX];
volatile uint32_t gADCErrors;// = 0;
uint8_t fft_mode;
extern uint32_t gSystemClock;
uint16_t trigger;

typedef struct _scaled_sample {
	uint16_t x;
	uint16_t y;
} Scaled_Sample;

Scaled_Sample scaled_samples[LCD_HORIZONTAL_MAX];
void ADC_ISR();
void init_adc_sampling();
uint16_t adc_trigger(void);
void copy_to_local_buffer();
uint16_t scale_samples(float fVoltsPerDiv, uint16_t sample);
void scale_local_samples(void);
void config_pwm();

void copy_waveform();
void compute_fft();


#endif /* SAMPLING_H_ */
