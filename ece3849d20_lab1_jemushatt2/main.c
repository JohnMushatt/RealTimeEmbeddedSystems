/**
 * main.c
 *
 * ECE 3849 Lab 0 Starter Project
 * Gene Bogdanov    10/18/2017
 *
 * This version is using the new hardware for B2017: the EK-TM4C1294XL LaunchPad with BOOSTXL-EDUMKII BoosterPack.
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include "driverlib/fpu.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "Crystalfontz128x128_ST7735.h"
#include "buttons.h"
#include "sampling.h"
#include "cpu_timer.h"
#include <stdio.h>
uint32_t gSystemClock; // [Hz] system clock frequency

int main(void) {
	IntMasterDisable();

	// Enable the Floating Point Unit, and permit ISRs to use it
	FPUEnable();
	FPULazyStackingEnable();

	// Initialize the system clock to 120 MHz
	gSystemClock = SysCtlClockFreqSet(
	SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480,
			120000000);

	//cpu load init
	cpu_load_init();
	//Display init
	display_init();
	//Button init
	ButtonInit();
	//PWM init
	config_pwm();
	//ADC init
	init_adc_sampling();
	IntMasterEnable();

	while (true) {
		//Get button input
		process_button();
		//Copy to local buffer for processing
		copy_to_local_buffer();
		//Update screen
		display_screen(voltsPerDiv, ts, cpu_load, trigger);
		/**
		 * Get cpu load data
		 */
		cpu_load_count = get_cpu_load_count();
		cpu_load = (1.0f - (float)(cpu_load_count)/cpu_unloaded_count) * 100.0f;
	}
}
