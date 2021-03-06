/*
 * time_display.c
 *
 *  Created on: Mar 25, 2020
 *      Author: johnm
 */

#include "time_display.h"
void display_time_standard(float time_in_seconds)
{

    uint32_t minutes = (uint32_t) time_in_seconds / 60;
    time_in_seconds -= minutes * 60;
    uint32_t seconds = (uint32_t) (time_in_seconds);
    time_in_seconds -= seconds;
    uint32_t ms = (uint32_t) (time_in_seconds * 100);

    snprintf(time_display_buffer, sizeof(time_display_buffer),
             "Time = %02u:%02u:%02u", minutes, seconds, ms); // convert time to string
    GrContextForegroundSet(&sContext, ClrYellow); // yellow text
    GrStringDraw(&sContext, time_display_buffer, -1, 0, 0, false);

}
void display_buttons_currently_pressed(uint32_t bitmap)
{
    button_display_buffer[8] = bitmap & 1 ? '1' : '0';
    button_display_buffer[7] = bitmap & 2 ? '1' : '0';
    button_display_buffer[6] = bitmap & 4 ? '1' : '0';
    button_display_buffer[5] = bitmap & 8 ? '1' : '0';
    button_display_buffer[4] = bitmap & 16 ? '1' : '0';
    button_display_buffer[3] = bitmap & 32 ? '1' : '0';
    button_display_buffer[2] = bitmap & 64 ? '1' : '0';
    button_display_buffer[1] = bitmap & 128 ? '1' : '0';
    button_display_buffer[0] = bitmap & 256 ? '1' : '0';

    GrContextForegroundSet(&sContext, ClrYellow); // yellow text
    GrStringDraw(&sContext, button_display_buffer, -1, 40, 40, true);
}
