/*
 * sampling.c
 *
 *  Created on: Apr 6, 2020
 *      Author: johnm
 */

#include "sampling.h"
/**
 * Enables ADC1 channel 3 sequence 0 for capture with interrupts enabled
 */
void init_adc_sampling() {
	gADCBufferIndex = ADC_BUFFER_SIZE - 1;
	gADCErrors = 0;
	trigger = 2048;
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
	GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_0); // GPIO setup for analog input AIN3
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0); // initialize ADC peripherals
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC1);
	// ADC clock
	uint32_t pll_frequency = SysCtlFrequencyGet(CRYSTAL_FREQUENCY);
	uint32_t pll_divisor = (pll_frequency - 1) / (16 * ADC_SAMPLING_RATE) + 1; //round up
	ADCClockConfigSet(ADC0_BASE, ADC_CLOCK_SRC_PLL | ADC_CLOCK_RATE_FULL,
			pll_divisor);
	ADCClockConfigSet(ADC1_BASE, ADC_CLOCK_SRC_PLL | ADC_CLOCK_RATE_FULL,
			pll_divisor);
	ADCSequenceDisable(ADC1_BASE, 0); // choose ADC1 sequence 0; disable before configuring
	ADCSequenceConfigure(ADC1_BASE, 0, ADC_TRIGGER_ALWAYS, 0); // specify the "Always" trigger
	ADCSequenceStepConfigure(ADC1_BASE, 0, 0,
	ADC_CTL_IE | ADC_CTL_END | ADC_CTL_CH3); // in the 0th step, sample channel 3 (AIN3)
	// enable interrupt, and make it the end of sequence
	ADCSequenceEnable(ADC1_BASE, 0); // enable the sequence. it is now sampling
	ADCIntEnable(ADC1_BASE, 0); // enable sequence 0 interrupt in the ADC1 peripheral
	IntPrioritySet(INT_ADC1SS0, 0); // set ADC1 sequence 0 interrupt priority
	IntEnable(INT_ADC1SS0); // enable ADC1 sequence 0 interrupt in int. controller
}
/**
 * Enables the pulse width modulation generator for signal input
 */
void config_pwm() {
	// configure M0PWM2, at GPIO PF2, BoosterPack 1 header C1 pin 2
	// configure M0PWM3, at GPIO PF3, BoosterPack 1 header C1 pin 3
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_3);
	GPIOPinConfigure(GPIO_PF2_M0PWM2);
	GPIOPinConfigure(GPIO_PF3_M0PWM3);
	GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_3,
	GPIO_STRENGTH_2MA,
	GPIO_PIN_TYPE_STD);
	// configure the PWM0 peripheral, gen 1, outputs 2 and 3
	SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
	PWMClockSet(PWM0_BASE, PWM_SYSCLK_DIV_1); // use system clock without division
	PWMGenConfigure(PWM0_BASE, PWM_GEN_1,
	PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2,PWM_FREQUENCY);
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2,
			roundf((float) gSystemClock / PWM_FREQUENCY * 0.4f));

	PWMOutputState(PWM0_BASE, PWM_OUT_2_BIT | PWM_OUT_3_BIT, true);
	PWMGenEnable(PWM0_BASE, PWM_GEN_1);
}
/**
 * Gets the trigger for the current frame given a local buffer, the current trigger mode
 * Attempts to find the trigger before the next interrupt, if it does not find it, it just returns
 * the starting search idnex
 */
uint16_t adc_trigger(void) {


	uint16_t trigger_index = ADC_BUFFER_WRAP(
			gADCBufferIndex - (LCD_HORIZONTAL_MAX / 2));
	uint16_t trigger_stop = trigger_index - ADC_BUFFER_SIZE / 2;
	uint16_t i = 0;
	while (i < trigger_stop) {
		if (rising_trigger
				&& (gADCBuffer[ADC_BUFFER_WRAP(trigger_index)] >= trigger
						&& gADCBuffer[ADC_BUFFER_WRAP(trigger_index - 1)]
								< trigger)) {
			break;
		} else if ( (!rising_trigger) && (gADCBuffer[ADC_BUFFER_WRAP(trigger_index)] <= trigger
				&& gADCBuffer[ADC_BUFFER_WRAP(trigger_index - 1)] > trigger)) {
			break;
		}
		i++;
		trigger_index--;
	}
	if (trigger_index == trigger_stop) {
		trigger_index = gADCBufferIndex - (LCD_HORIZONTAL_MAX / 2);
	}
	return trigger_index;

}
/**
 * Copies data from ADC buffer so that the rest of the system can operater without getting
 * interrupted from the ADC interrupts
 */
void copy_to_local_buffer() {
	uint16_t half_index = ADC_BUFFER_WRAP(adc_trigger() - LCD_HORIZONTAL_MAX/2);

	for (int16_t i = 0; i < LCD_HORIZONTAL_MAX; i++) {
		local_sample_buffer[i] = gADCBuffer[ADC_BUFFER_WRAP((half_index + i))];
	}
}
/**
 * Scales the given input based on the current voltage division
 */
uint16_t scale_samples(float fVoltsPerDiv, uint16_t sample) {
	float fScale = (VIN_RANGE * PIXELS_PER_DIV)
			/ ((1 << ADC_BITS) * fVoltsPerDiv);
	int16_t y_pos = LCD_VERTICAL_MAX / 2
			- (int16_t) roundf(fScale * ((int16_t) sample - ADC_OFFSET));
	return y_pos;
}
/**
 * ADC1 ISR that takes the signal source (PWM) and retrieves it from the ADC1 FIFO queue via direct
 * register access
 */
void ADC_ISR(void) {
	//Clear interrupt vector
	ADC1_ISC_R = ADC_ISC_IN0;
	if (ADC1_OSTAT_R & ADC_OSTAT_OV0) {
		gADCErrors++;
		ADC1_ACTSS_R = ADC_OSTAT_OV0;
	}
	gADCBuffer[gADCBufferIndex = ADC_BUFFER_WRAP(gADCBufferIndex + 1)] =
	ADC1_SSFIFO0_R;
}
