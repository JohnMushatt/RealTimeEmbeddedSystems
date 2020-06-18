/*
 * cpu_timer.c
 *
 *  Created on: Apr 9, 2020
 *      Author: johnm
 */


#include "cpu_timer.h"
extern uint32_t gSystemClock;


/**
 * Init function for cpu timer
 */
void cpu_load_init() {
	cpu_load = 0.0f;
	cpu_load_count=0;
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER3);
	TimerDisable(TIMER3_BASE,TIMER_BOTH);
	TimerConfigure(TIMER3_BASE,TIMER_CFG_ONE_SHOT);
	TimerLoadSet(TIMER3_BASE,TIMER_A,MEASURE_INTERVAL-1);
	cpu_unloaded_count=get_cpu_load_count();

}
/**
 * Get the load for the cpu to determine utilization
 */
uint32_t get_cpu_load_count() {
	uint32_t i = 0;
	TimerIntClear(TIMER3_BASE, TIMER_TIMA_TIMEOUT);

	TimerEnable(TIMER3_BASE,TIMER_A);
	while(!(TimerIntStatus(TIMER3_BASE, false) & TIMER_TIMA_TIMEOUT)) {
		i++;

	}

	return i;
}
