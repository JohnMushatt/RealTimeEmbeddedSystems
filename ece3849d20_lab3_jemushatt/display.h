/*
 * display.h
 *
 *  Created on: Apr 9, 2020
 *      Author: johnm
 */

#ifndef DISPLAY_H_
#define DISPLAY_H_
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "sampling.h"
#include "cpu_timer.h"
#include "Crystalfontz128x128_ST7735.h"
#define PIXELS_PER_DIV 20

uint8_t voltsPerDiv;
bool rising_trigger;
uint8_t ts;

void display_init();
void display_screen();
void update_waveform(float fVoltsPerDiv,tContext *sContext);
void draw_grid(tContext *sContext);
void draw_text(tContext *sContext, uint16_t ts, uint8_t voltsPerDiv, float cpu_usage, uint8_t trigger);
#endif /* DISPLAY_H_ */
