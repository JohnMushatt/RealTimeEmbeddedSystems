/*
 * ECE 3849 Lab2 starter project
 *
 * Gene Bogdanov    9/13/2017
 */
/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <xdc/cfg/global.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

#include <stdint.h>
#include <stdbool.h>
#include "driverlib/interrupt.h"
#include "lab2_tasks.h"
#include "kernel_include.h"
#include "ethernet.h"
#include "sampling.h"
uint32_t gSystemClock = 120000000; // [Hz] system clock frequency

/*
 *  ======== main ========
 */
int main(void)
{
    IntMasterDisable();

    //enet_init();
    /*
    // hardware initialization goes here
    cpu_load_init();
 	display_init();
	ButtonInit();
	init_adc_sampling();
    init_adc_DMA();

    config_pwm();
    audio_init();

    frequency_count_init();
	*/
    /* Start BIOS */
    BIOS_start();

    return (0);
}

