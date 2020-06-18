/*
 * display.c
 *
 *  Created on: Apr 9, 2020
 *      Author: johnm
 */

#include "display.h"
const char * const gVoltageScaleStr[] = { "100 mV", "200 mV", "500 mV", " 1 V",
		" 2 V" };
const float gVoltageScale[] = { 0.1, 0.2, 0.5, 1.0, 2.0 };
/**
 * Initializes the display
 */
void display_init() {
	Crystalfontz128x128_Init();
	Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP);
	voltsPerDiv = 3;
	ts = 20;
	rising_trigger = 1;
}
/**
 * Main display function
 * Calls drawing functions for the waveform, grid, and the text on the screen
 */
void display_screen() {
	tContext sContext;
	GrContextInit(&sContext, &g_sCrystalfontz128x128);
	GrContextFontSet(&sContext, &g_sFontFixed6x8);

	tRectangle rectFullScreen = { 0, 0, GrContextDpyWidthGet(&sContext) - 1,
	GrContextDpyHeightGet(&sContext) - 1 };

	GrContextForegroundSet(&sContext, ClrBlack);
	GrRectFill(&sContext, &rectFullScreen);

	draw_grid(&sContext);
	update_waveform(gVoltageScale[voltsPerDiv], &sContext);
	draw_text(&sContext, ts, voltsPerDiv, cpu_load, trigger);
	GrFlush(&sContext);

}
/**
 * Renders the wave form via connect lines together
 */
void update_waveform(float fVoltsPerDiv, tContext *sContext) {
	GrContextForegroundSet(sContext, ClrYellow);

	uint16_t x = 0;
	uint16_t y = scale_samples(fVoltsPerDiv, local_sample_buffer[0]);

	GrPixelDraw(sContext, x, y);

	for (uint16_t i = 0; i < LCD_HORIZONTAL_MAX; i++) {
		GrLineDraw(sContext, x, y, i,
				scale_samples(fVoltsPerDiv, local_sample_buffer[i]));
		x = i;
		y = scale_samples(fVoltsPerDiv, local_sample_buffer[i]);
	}
}
/**
 * Draws the text on the screen for the various features such as CPU, error count, and trigger mode
 */
void draw_text(tContext *sContext, uint16_t ts, uint8_t voltsPerDiv,
		float cpu_usage, uint8_t trigger) {
	GrContextForegroundSet(sContext, ClrWhite);
	char buffer[64];
	snprintf(buffer, sizeof(buffer), "%u us", ts);

	GrStringDraw(sContext, buffer, -1, 5, 0, false);

	snprintf(buffer, sizeof(buffer), "%s", gVoltageScaleStr[voltsPerDiv]);
	GrStringDraw(sContext, buffer, -1, 50, 0, false);

	snprintf(buffer, sizeof(buffer), "CPU: %.1f%%", cpu_usage);
	GrStringDraw(sContext, buffer, -1, 0, 120, false);
	snprintf(buffer, sizeof(buffer), "ADC1 Err: %d", gADCErrors);
	GrStringDraw(sContext, buffer, -1, 0, 105, false);

	if (rising_trigger) {
		GrLineDrawH(sContext, 114, 121, 0);
		GrLineDrawV(sContext, 114, 0, 7);
		GrLineDrawH(sContext, 107, 114, 7);
		GrLineDraw(sContext, 114, 2, 111, 5);
		GrLineDraw(sContext, 114, 2, 117, 5);
	} else {
		GrLineDrawH(sContext, 107, 114, 0);
		GrLineDrawV(sContext, 114, 0, 7);
		GrLineDrawH(sContext, 114, 121, 7);
		GrLineDraw(sContext, 114, 5, 111, 2);
		GrLineDraw(sContext, 114, 5, 117, 2);
	}

}
/*
 * Draws the grid, with a special red cross for the center axis
 */
void draw_grid(tContext *sContext) {
	GrContextForegroundSet(sContext, ClrMidnightBlue);
	for (uint16_t i = 0; i < 7; i++) {
		GrLineDrawH(sContext, 0, 128, PIXELS_PER_DIV * i + 4);
		GrLineDrawV(sContext, PIXELS_PER_DIV * i + 4, 0, 128);
	}
	GrContextForegroundSet(sContext, ClrRed);
	GrLineDrawH(sContext, 0, 128, 64);
	GrLineDrawV(sContext, 64, 0, 128);
}
