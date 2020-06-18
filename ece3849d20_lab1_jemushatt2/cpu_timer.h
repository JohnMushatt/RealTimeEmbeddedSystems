/*
 * cpu_timer.h
 *
 *  Created on: Apr 9, 2020
 *      Author: johnm
 */

#ifndef CPU_TIMER_H_
#define CPU_TIMER_H_
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "sysctl_pll.h"


#define MEASURE_INTERVAL 10000 //10 ms


float cpu_load;// = 0.0f;
uint32_t cpu_load_count;// = 0;
uint32_t cpu_unloaded_count;// = 0;


void cpu_load_init();
uint32_t get_cpu_load_count();

#endif /* CPU_TIMER_H_ */
