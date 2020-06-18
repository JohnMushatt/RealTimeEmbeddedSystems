/*
 * sampling.h
 *
 *  Created on: Apr 6, 2020
 *      Author: johnm
 */

#ifndef SAMPLING_H_
#define SAMPLING_H_
#include "buttons.h"
#include <stdint.h>

#include <stdbool.h>
#include "sysctl_pll.h"
#include "driverlib/adc.h"
#include "inc/tm4c1294ncpdt.h"
#include "Crystalfontz128x128_ST7735.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include <math.h>
#include "driverlib/pwm.h"
#include "driverlib/pin_map.h"
#include "display.h"


#define PWM_FREQUENCY 20000 // PWM frequency = 20 kHz
#define ADC_BUFFER_SIZE 2048
#define ADC_BUFFER_WRAP(i) ((i) & (ADC_BUFFER_SIZE -1))
#define ADC_OFFSET 2048
#define ADC_BITS 12
#define VIN_RANGE 3.3f

volatile int32_t gADCBufferIndex;// = ADC_BUFFER_SIZE -1;
volatile uint16_t gADCBuffer[ADC_BUFFER_SIZE];
uint16_t local_sample_buffer[LCD_HORIZONTAL_MAX];
volatile uint32_t gADCErrors;// = 0;
extern uint32_t gSystemClock;
uint16_t trigger;

void ADC_ISR();
void init_adc_sampling();
uint16_t adc_trigger(void);
void copy_to_local_buffer();
uint16_t scale_samples(float fVoltsPerDiv, uint16_t sample);
void config_pwm();



#endif /* SAMPLING_H_ */
