/*
 * buttons.c
 *
 *  Created on: Aug 12, 2012, modified 9/8/2017
 *      Author: Gene Bogdanov
 *
 * ECE 3849 Lab button handling
 */
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "driverlib/adc.h"
#include "sysctl_pll.h"
#include "buttons.h"
#include "display.h"
#include "time_display.h"
// public globals
volatile uint32_t gButtons = 0; // debounced button state, one per bit in the lowest bits
// button is pressed if its bit is 1, not pressed if 0
uint32_t gJoystick[2] = { 0 };    // joystick coordinates
uint32_t gADCSamplingRate;      // [Hz] actual ADC sampling rate

// imported globals
extern uint32_t gSystemClock;   // [Hz] system clock frequency
extern volatile uint32_t gTime; // time in hundredths of a second

// initialize all button and joystick handling hardware
void ButtonInit(void) {
	// initialize a general purpose timer for periodic interrupts
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	TimerDisable(TIMER0_BASE, TIMER_BOTH);
	TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
	TimerLoadSet(TIMER0_BASE, TIMER_A,
			(float) gSystemClock / BUTTON_SCAN_RATE - 0.5f);
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	TimerEnable(TIMER0_BASE, TIMER_BOTH);

	// initialize interrupt controller to respond to timer interrupts
	IntPrioritySet(INT_TIMER0A, 32);
	IntEnable(INT_TIMER0A);

	// GPIO PJ0 and PJ1 = EK-TM4C1294XL buttons 1 and 2
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);
	//For boosterpack button 2
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);
	//For boosterpack button 1
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOH);
	//For boosterpack joystick select
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);

	GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	//For boosterpack button 2
	GPIOPinTypeGPIOInput(GPIO_PORTK_BASE, GPIO_PIN_6);
	//For boosterpack button 1
	GPIOPinTypeGPIOInput(GPIO_PORTH_BASE, GPIO_PIN_1);
	//For boosterpack joystick select
	GPIOPinTypeGPIOInput(GPIO_PORTD_BASE, GPIO_PIN_4);

	GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_0 | GPIO_PIN_1,
			GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
	//For boosterpack button 2
	GPIOPadConfigSet(GPIO_PORTK_BASE, GPIO_PIN_6, GPIO_STRENGTH_2MA,
			GPIO_PIN_TYPE_STD_WPU);
	//For boosterpack button 1
	GPIOPadConfigSet(GPIO_PORTH_BASE, GPIO_PIN_1, GPIO_STRENGTH_2MA,
			GPIO_PIN_TYPE_STD_WPU);
	//For boosterpack joystick select
	GPIOPadConfigSet(GPIO_PORTD_BASE, GPIO_PIN_4, GPIO_STRENGTH_2MA,
			GPIO_PIN_TYPE_STD_WPU);

	// analog input AIN13, at GPIO PD2 = BoosterPack Joystick HOR(X)
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_2);
	// analog input AIN17, at GPIO PK1 = BoosterPack Joystick VER(Y)
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);
	GPIOPinTypeADC(GPIO_PORTK_BASE, GPIO_PIN_1);

	// initialize ADC0 peripheral
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
	uint32_t pll_frequency = SysCtlFrequencyGet(CRYSTAL_FREQUENCY);
	uint32_t pll_divisor = (pll_frequency - 1) / (16 * ADC_SAMPLING_RATE) + 1; // round divisor up
	gADCSamplingRate = pll_frequency / (16 * pll_divisor); // actual sampling rate may differ from ADC_SAMPLING_RATE
	ADCClockConfigSet(ADC0_BASE, ADC_CLOCK_SRC_PLL | ADC_CLOCK_RATE_FULL,
			pll_divisor); // only ADC0 has PLL clock divisor control

	// initialize ADC sampling sequence
	ADCSequenceDisable(ADC0_BASE, 0);
	ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PROCESSOR, 0);
	ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_CH13);  // Joystick HOR(X)
	ADCSequenceStepConfigure(ADC0_BASE, 0, 1,
			ADC_CTL_CH17 | ADC_CTL_IE | ADC_CTL_END);  // Joystick VER(Y)
	ADCSequenceEnable(ADC0_BASE, 0);

}

// update the debounced button state gButtons
void ButtonDebounce(uint32_t buttons) {
	int32_t i, mask;
	static int32_t state[BUTTON_COUNT]; // button state: 0 = released
										// BUTTON_PRESSED_STATE = pressed
										// in between = previous state
	for (i = 0; i < BUTTON_COUNT; i++) {
		mask = 1 << i;
		if (buttons & mask) {
			state[i] += BUTTON_STATE_INCREMENT;
			if (state[i] >= BUTTON_PRESSED_STATE) {
				state[i] = BUTTON_PRESSED_STATE;
				gButtons |= mask; // update debounced button state
			}
		} else {
			state[i] -= BUTTON_STATE_DECREMENT;
			if (state[i] <= 0) {
				state[i] = 0;
				gButtons &= ~mask;
			}
		}
	}
}

// sample joystick and convert to button presses
void ButtonReadJoystick(void) {
	ADCProcessorTrigger(ADC0_BASE, 0); // trigger the ADC sample sequence for Joystick X and Y
	while (!ADCIntStatus(ADC0_BASE, 0, false))
		;  // wait until the sample sequence has completed
	ADCSequenceDataGet(ADC0_BASE, 0, gJoystick);  // retrieve joystick data
	ADCIntClear(ADC0_BASE, 0);              // clear ADC sequence interrupt flag

	// process joystick movements as button presses using hysteresis
	if (gJoystick[0] > JOYSTICK_UPPER_PRESS_THRESHOLD)
		gButtons |= 1 << 5; // joystick right in position 5
	if (gJoystick[0] < JOYSTICK_UPPER_RELEASE_THRESHOLD)
		gButtons &= ~(1 << 5);

	if (gJoystick[0] < JOYSTICK_LOWER_PRESS_THRESHOLD)
		gButtons |= 1 << 6; // joystick left in position 6
	if (gJoystick[0] > JOYSTICK_LOWER_RELEASE_THRESHOLD)
		gButtons &= ~(1 << 6);

	if (gJoystick[1] > JOYSTICK_UPPER_PRESS_THRESHOLD)
		gButtons |= 1 << 7; // joystick up in position 7
	if (gJoystick[1] < JOYSTICK_UPPER_RELEASE_THRESHOLD)
		gButtons &= ~(1 << 7);

	if (gJoystick[1] < JOYSTICK_LOWER_PRESS_THRESHOLD)
		gButtons |= 1 << 8; // joystick down in position 8
	if (gJoystick[1] > JOYSTICK_LOWER_RELEASE_THRESHOLD)
		gButtons &= ~(1 << 8);
}

// autorepeat button presses if a button is held long enough
uint32_t ButtonAutoRepeat(void) {
	static int count[BUTTON_AND_JOYSTICK_COUNT] = { 0 }; // autorepeat counts
	int i;
	uint32_t mask;
	uint32_t presses = 0;
	for (i = 0; i < BUTTON_AND_JOYSTICK_COUNT; i++) {
		mask = 1 << i;
		if (gButtons & mask)
			count[i]++;     // increment count if button is held
		else
			count[i] = 0;   // reset count if button is let go
		if (count[i] >= BUTTON_AUTOREPEAT_INITIAL
				&& (count[i] - BUTTON_AUTOREPEAT_INITIAL)
						% BUTTON_AUTOREPEAT_NEXT == 0)
			presses |= mask;    // register a button press due to auto-repeat
	}
	return presses;
}
uint32_t button_queue[BUTTON_QUEUE_LENGTH];
volatile uint32_t button_queue_head = 0;
volatile uint32_t button_queue_tail = 0;
/**
 * Concatenates the given button event to the end of the current queue
 */
int32_t enqueue(uint32_t button_map) {

	int32_t new_end = BUTTON_QUEUE_WRAPPER(button_queue_tail+1);
	if (button_queue_head != new_end) {
		button_queue[button_queue_tail] = button_map;
		button_queue_tail = new_end;
		return button_queue_tail;
	}
	return -1;
}
/**
 * Removes the head of the queue and returns the button event to the calling process
 */
int32_t dequeue() {
	int32_t button = 0;
	if (button_queue_head != button_queue_tail) {
		button = button_queue[button_queue_head];
		IntMasterDisable();
		button_queue_head = BUTTON_QUEUE_WRAPPER(button_queue_head + 1);
		IntMasterEnable();

	}
	return button;
}
/**
 * Removes the current head of the queue if it exists, then processes the acquired button even
 * to either increase/decrease voltage division, or change trigger mode
 */
void process_button() {
	int32_t button_event = dequeue();
	if(button_event) {

		if(button_event & (1<<4)) {
			rising_trigger = !rising_trigger;
		}
		if(button_event & (1<<7)) {
			if(voltsPerDiv==4) {
				voltsPerDiv=0;
			}
			else {
				voltsPerDiv++;
			}
		}
		if(button_event & (1<<8)) {
			if(voltsPerDiv==0) {
							voltsPerDiv=4;
						}
						else {
							voltsPerDiv--;
						}
		}
	}
}
// ISR for scanning and debouncing buttons
void ButtonISR(void) {
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT); // clear interrupt flag

	// read hardware button state
	uint32_t gpio_buttons = ~GPIOPinRead(GPIO_PORTJ_BASE, 0xff)
			& (GPIO_PIN_1 | GPIO_PIN_0); // EK-TM4C1294XL buttons in positions 0 and 1
	//For boosterpack button 2
	uint32_t pk_6_reading = ~GPIOPinRead(GPIO_PORTK_BASE, 0xff) & (GPIO_PIN_6);
	uint32_t ph_1_reading = ~GPIOPinRead(GPIO_PORTH_BASE, 0xff) & (GPIO_PIN_1);
	uint32_t pd_1_reading = ~GPIOPinRead(GPIO_PORTD_BASE, 0xff) & (GPIO_PIN_4);
	//If the boosterpack button two is pressed, insert it into the bitmap else just OR it with 0x0
	gpio_buttons |= (pk_6_reading >> 6) ? GPIO_PIN_3 : 0x0;
	gpio_buttons |= (ph_1_reading >> 1) ? GPIO_PIN_2 : 0x0;
	gpio_buttons |= (pd_1_reading >> 4) ? GPIO_PIN_4 : 0x0;
	uint32_t old_buttons = gButtons;    // save previous button state
	ButtonDebounce(gpio_buttons); // Run the button debouncer. The result is in gButtons.
	ButtonReadJoystick(); // Convert joystick state to button presses. The result is in gButtons.
	uint32_t presses = ~old_buttons & gButtons; // detect button presses (transitions from not pressed to pressed)
	presses |= ButtonAutoRepeat(); // autorepeat presses if a button is held long enough
	if (presses != old_buttons) {
		enqueue(presses);
	}
}
