/*
 * sampling.c
 *
 *  Created on: Apr 6, 2020
 *      Author: johnm
 */

#include "sampling.h"

//extern const float gVoltageScale[] = { 0.1, 0.2, 0.5, 1.0, 2.0 };

/**
 * Enables ADC1 channel 3 sequence 0 for capture with interrupts enabled
 */
extern const float gVoltageScale[];
void init_adc_sampling() {
	fft_mode = false;
	//gADCBufferIndex = ADC_BUFFER_SIZE - 1;
	gADCErrors = 0;
	trigger = 2048;
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
	GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_0); // GPIO setup for analog input AIN3
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC1);
	// ADC clock
	uint32_t pll_frequency = SysCtlFrequencyGet(CRYSTAL_FREQUENCY);
	uint32_t pll_divisor = (pll_frequency - 1) / (16 * ADC_SAMPLING_RATE_2X) + 1; //round up
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
	ADCSequenceDMAEnable(ADC1_BASE, 0); // enable DMA for ADC1 sequence 0
	ADCIntEnableEx(ADC1_BASE, ADC_INT_DMA_SS0); // enable ADC1 sequence 0 DMA interrupt

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
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_1,
			roundf((float) gSystemClock / PWM_FREQUENCY));
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_2,
			roundf((float) gSystemClock / PWM_FREQUENCY * 0.4f));
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_3,
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
			getADCBufferIndex() - (LCD_HORIZONTAL_MAX / 2));
	uint16_t trigger_stop = trigger_index - ADC_BUFFER_SIZE / 2;
	uint16_t i = 0;
	while (i < trigger_stop) {
		if (rising_trigger
				&& (gADCBuffer[ADC_BUFFER_WRAP(trigger_index)] >= trigger
						&& gADCBuffer[ADC_BUFFER_WRAP(trigger_index - 1)]
								< trigger)) {
			break;
		} else if ((!rising_trigger)
				&& (gADCBuffer[ADC_BUFFER_WRAP(trigger_index)] <= trigger
						&& gADCBuffer[ADC_BUFFER_WRAP(trigger_index - 1)]
								> trigger)) {
			break;
		}
		i++;
		trigger_index--;
	}
	if (trigger_index == trigger_stop) {
		trigger_index = getADCBufferIndex() - (LCD_HORIZONTAL_MAX / 2);
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
		scaled_samples[i].x = i;
		scaled_samples[i].y = gADCBuffer[ADC_BUFFER_WRAP(half_index + i)];
	}
}
/**
 * Copy fft spectrum into its buffer
 */
void copy_waveform() {
	uint16_t spectrum_beginning = ADC_BUFFER_WRAP(
			getADCBufferIndex() - SPECTRUM_LENGTH);
	for (uint16_t i = 0; i < SPECTRUM_LENGTH; i++) {
		//scaled_samples[i].x = i;
		fft_buffer[i] = gADCBuffer[ADC_BUFFER_WRAP(spectrum_beginning + i)];
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
 * Instead of calling the scaling function in the display section,
 * now we just scale all the current sampels at once
 */
void scale_local_samples(void) {

	for (uint16_t i = 0; i < LCD_HORIZONTAL_MAX; i++) {
		scaled_samples[i].x = i;
		scaled_samples[i].y = scale_samples(gVoltageScale[voltsPerDiv],
				scaled_samples[i].y);
	}
}
volatile bool gDMAPrimary = true;
/**
 * ADC1 ISR that takes the signal source (PWM) and retrieves it from the ADC1 FIFO queue via direct
 * register access
 * LAB 3 - Reworked now to use DMA instead of sequence to offload CPU load
 */
void ADC_ISR(void) {
	ADCIntClearEx(ADC1_BASE, ADC_INT_DMA_SS0);
	// Check the primary DMA channel for end of transfer, and restart if needed.
	if (uDMAChannelModeGet(
	UDMA_SEC_CHANNEL_ADC10 | UDMA_PRI_SELECT) == UDMA_MODE_STOP) {
		uDMAChannelTransferSet(UDMA_SEC_CHANNEL_ADC10 | UDMA_PRI_SELECT,
		UDMA_MODE_PINGPONG, (void*) &ADC1_SSFIFO0_R, (void*) &gADCBuffer[0],
		ADC_BUFFER_SIZE / 2); // restart the primary channel (same as setup)
		gDMAPrimary = false; // DMA is currently occurring in the alternate buffer
	}
	if (uDMAChannelModeGet(
	UDMA_SEC_CHANNEL_ADC10 | UDMA_ALT_SELECT) == UDMA_MODE_STOP) {
		uDMAChannelTransferSet(UDMA_SEC_CHANNEL_ADC10 | UDMA_ALT_SELECT,
		UDMA_MODE_PINGPONG, (void*) &ADC1_SSFIFO0_R,
				(void*) &gADCBuffer[ADC_BUFFER_SIZE / 2], ADC_BUFFER_SIZE / 2); // restart the alternate channel (same as setup)
		gDMAPrimary = true; // DMA is currently occurring in the primary buffer
	}
	// The DMA channel may be disabled if the CPU is paused by the debugger.
	if (!uDMAChannelIsEnabled(UDMA_SEC_CHANNEL_ADC10)) {
		uDMAChannelEnable(UDMA_SEC_CHANNEL_ADC10); // re-enable the DMA channel
	}

	/*
	 //Clear interrupt vector
	 ADC1_ISC_R = ADC_ISC_IN0;
	 if (ADC1_OSTAT_R & ADC_OSTAT_OV0) {
	 gADCErrors++;
	 ADC1_OSTAT_R = ADC_OSTAT_OV0;
	 }
	 gADCBuffer[gADCBufferIndex = ADC_BUFFER_WRAP(gADCBufferIndex + 1)] =
	 ADC1_SSFIFO0_R;
	 */
}
/*
 * DMA initialization function
 */
void init_adc_DMA() {
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
	uDMAEnable();
	uDMAControlBaseSet(gDMAControlTable);
	uDMAChannelAssign(UDMA_CH24_ADC1_0); // assign DMA channel 24 to ADC1 sequence 0
	uDMAChannelAttributeDisable(UDMA_SEC_CHANNEL_ADC10, UDMA_ATTR_ALL);
	// primary DMA channel = first half of the ADC buffer
	uDMAChannelControlSet(UDMA_SEC_CHANNEL_ADC10 | UDMA_PRI_SELECT,
	UDMA_SIZE_16 | UDMA_SRC_INC_NONE | UDMA_DST_INC_16 | UDMA_ARB_4);
	uDMAChannelTransferSet(UDMA_SEC_CHANNEL_ADC10 | UDMA_PRI_SELECT,
	UDMA_MODE_PINGPONG, (void*) &ADC1_SSFIFO0_R, (void*) &gADCBuffer[0],
	ADC_BUFFER_SIZE / 2);
	// alternate DMA channel = second half of the ADC buffer
	uDMAChannelControlSet(UDMA_SEC_CHANNEL_ADC10 | UDMA_ALT_SELECT,
	UDMA_SIZE_16 | UDMA_SRC_INC_NONE | UDMA_DST_INC_16 | UDMA_ARB_4);
	uDMAChannelTransferSet(UDMA_SEC_CHANNEL_ADC10 | UDMA_ALT_SELECT,
	UDMA_MODE_PINGPONG, (void*) &ADC1_SSFIFO0_R,
			(void*) &gADCBuffer[ADC_BUFFER_SIZE / 2], ADC_BUFFER_SIZE / 2);
	uDMAChannelEnable(UDMA_SEC_CHANNEL_ADC10);

	//ADCSequenceDMAEnable(ADC1_BASE, 0); // enable DMA for ADC1 sequence 0
	//ADCIntEnableEx(ADC1_BASE, ADC_INT_DMA_SS0); // enable ADC1 sequence 0 DMA interrupt

}
/*
 * Thread safe version for retrieving the correct buffer index
 */
int32_t getADCBufferIndex(void) {
	int32_t index;
	bool local_channel_val;

	IArg key = GateHwi_enter(gateHwi0);
	local_channel_val = gDMAPrimary;
	if (local_channel_val) {
		index = ADC_BUFFER_SIZE / 2 - 1
				- uDMAChannelSizeGet(UDMA_SEC_CHANNEL_ADC10 | UDMA_PRI_SELECT);
	} else {
		index = ADC_BUFFER_SIZE - 1
				- uDMAChannelSizeGet(UDMA_SEC_CHANNEL_ADC10 | UDMA_ALT_SELECT);
	}
	GateHwi_leave(gateHwi0, key);

	return index;
}
/*
 * INit function for frequency counter
 */
void frequency_count_init() {
	period = 0;
	prev_time = 0;
	acc_interval = 0;
	acc_period = 0;
	freq = 0;
	current_source_period=PWM_PERIOD;
	// configure GPIO PD0 as timer input T0CCP0 at BoosterPack Connector #1 pin 14
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	GPIOPinTypeTimer(GPIO_PORTD_BASE, GPIO_PIN_0);
	GPIOPinConfigure(GPIO_PD0_T0CCP0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	TimerDisable(TIMER0_BASE, TIMER_BOTH);
	TimerConfigure(TIMER0_BASE, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_CAP_TIME_UP);
	TimerControlEvent(TIMER0_BASE, TIMER_A, TIMER_EVENT_POS_EDGE);
	TimerLoadSet(TIMER0_BASE, TIMER_A, 0xffff); // use maximum load value
	TimerPrescaleSet(TIMER0_BASE, TIMER_A, 0xff); // use maximum prescale value
	TimerIntEnable(TIMER0_BASE, TIMER_CFG_A_CAP_TIME_UP);
	TimerEnable(TIMER0_BASE, TIMER_A);
}
/**
 * ISR for timer that measures frequency
 */
void Timer_ISR() {
	TimerIntClear(TIMER0_BASE, TIMER_CAPA_EVENT);
	uint32_t count = TimerValueGet(TIMER0_BASE, TIMER_A);
	period = (count - prev_time) & 0xfffff;
	prev_time = count;

	acc_interval += period;
	acc_period++;
	freq = (float)(gSystemClock)/((float)(acc_interval)/acc_period);
}
volatile uint32_t gPWMSample = 0;
volatile uint32_t gSamplingRateDivider = 20;
/*
 * ISR for pwm
 */
void PWM_ISR() {
	PWMGenIntClear(PWM0_BASE,PWM_GEN_2,PWM_INT_CNT_ZERO);
	int i = (gPWMSample++) / gSamplingRateDivider;
	PWM0_2_CMPB_R = 1 + gWaveform[i];
	if( i == gWaveformSize) {
		PWMIntDisable(PWM0_BASE, PWM_INT_GEN_2);
		gPWMSample = 0;
	}
}
/*
 * Set the pwm to use gen 2 and output 5. Source period is initially 258 cycles, but can be increased/decreased
 */
void audio_init() {
	//Enable G1
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
	GPIOPinTypePWM(GPIO_PORTG_BASE, GPIO_PIN_1);
	GPIOPinConfigure(GPIO_PG1_M0PWM5);
	//GPIOPinConfigure(GPIO_PF3_M0PWM3);
	GPIOPadConfigSet(GPIO_PORTG_BASE, GPIO_PIN_1, GPIO_STRENGTH_2MA,GPIO_PIN_TYPE_STD);
	// configure the PWM0 peripheral, gen 1, outputs 2 and 3
	SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
	PWMClockSet(PWM0_BASE, PWM_SYSCLK_DIV_1); // use system clock without division
	PWMGenConfigure(PWM0_BASE, PWM_GEN_2,
			PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC);
	PWMGenPeriodSet(PWM0_BASE, PWM_GEN_2, PWM_PERIOD);
	PWMPulseWidthSet(PWM0_BASE, PWM_OUT_5, PWM_PERIOD / 2);
	PWMOutputState(PWM0_BASE, PWM_OUT_5_BIT, true);
	PWMGenEnable(PWM0_BASE, PWM_GEN_2);
	PWMGenIntTrigEnable(PWM0_BASE, PWM_GEN_2, PWM_INT_CNT_ZERO);
}



