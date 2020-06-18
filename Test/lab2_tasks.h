/*
 * lab2_tasks.h
 *
 *  Created on: Apr 23, 2020
 *      Author: johnm
 */

#ifndef LAB2_TASKS_H_
#define LAB2_TASKS_H_


#include "sampling.h"
#include "display.h"
#include "kernel_include.h"
Void processing_task(UArg arg1, UArg arg2);
void waveform_task(UArg arg1, UArg arg2);
void display_task(UArg arg1, UArg arg2);
void button_task(UArg arg1, UArg arg2);
void clock_signal(UArg arg1, UArg arg2);
void user_input_task(UArg arg1, UArg arg2);
#endif /* LAB2_TASKS_H_ */
