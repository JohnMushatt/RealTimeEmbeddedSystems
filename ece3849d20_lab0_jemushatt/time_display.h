/*
 * time_display.h
 *
 *  Created on: Mar 25, 2020
 *      Author: johnm
 */

#ifndef TIME_DISPLAY_H_
#define TIME_DISPLAY_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "driverlib/fpu.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "Crystalfontz128x128_ST7735.h"
#include <stdlib.h>
#endif /* TIME_DISPLAY_H_ */
char time_display_buffer[50];
char button_display_buffer[10];
tContext sContext;
void display_time_standard(float time_in_seconds);
void display_buttons_currently_pressed(uint32_t bitmap);
