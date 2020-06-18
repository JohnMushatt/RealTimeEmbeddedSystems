/*
 * lab2_tasks.c
 *
 *  Created on: Apr 23, 2020
 *      Author: johnm
 */

#include "lab2_tasks.h"
/**
 * Processing task that either displays waveform or displays fft spectrum
 */
void processing_task(UArg arg1, UArg arg2) {
	/**
	 * FFT code
	 */
	static char kiss_fft_cfg_buffer[KISS_FFT_CFG_SIZE];
	size_t buffer_size = KISS_FFT_CFG_SIZE;
	kiss_fft_cfg cfg;
	static kiss_fft_cpx in[NFFT], out[NFFT]; // complex waveform and spectrum buffers
	cfg = kiss_fft_alloc(NFFT, 0, kiss_fft_cfg_buffer, &buffer_size); // init Kiss FFT

	while (1) {
		/*
		 * Get both the processing semaphore and the display update semaphore to properly update display
		 */
		Semaphore_pend(Processing_Sem, BIOS_WAIT_FOREVER);

		Semaphore_pend(Update_Display_Sem, BIOS_WAIT_FOREVER);
		/**
		 * Either display normal oscilloscope or spectrum
		 */
		if (fft_mode) {
			//TODO Implement fft computation
			for (uint16_t i = 0; i < NFFT; i++) { // generate an input waveform
				in[i].r = fft_buffer[i] - ADC_OFFSET; // real part of waveform
				in[i].i = 0; // imaginary part of waveform
			}
			kiss_fft(cfg, in, out); // compute FFT
			// convert first 128 bins of out[] to dB for display		} else {
			for (uint16_t i = 0; i < LCD_HORIZONTAL_MAX; i++) {
				float db = 156
						- 10
								* log10f(
										(out[i].i * out[i].i)
												+ (out[i].r * out[i].r));
				scaled_samples[i].y = db;
			}

		} else {
			scale_local_samples();

		}
		//Release locks/semaphores once critical section is over
		Semaphore_post(Update_Display_Sem);
		Semaphore_post(Display_Sem);
		Semaphore_post(Waveform_Sem);

	}
}
/**
 * Copies from adc buffer to either the fft_buffer for spectrum mode or the normal scaled buffer
 */
void waveform_task(UArg arg1, UArg arg2) {
	while (1) {
		//Wait for signal to compute new waveform
		Semaphore_pend(Waveform_Sem, BIOS_WAIT_FOREVER);
		Semaphore_pend(Update_Display_Sem, BIOS_WAIT_FOREVER);
		if (fft_mode) {
			copy_waveform();

		} else {
			copy_to_local_buffer();
		}
		Semaphore_post(Update_Display_Sem);

		Semaphore_post(Processing_Sem);
	}
}
/**
 * Displays the current state of the wave + settings (Lowest prio)
 */
void display_task(UArg arg1, UArg arg2) {
	while (1) {
		Semaphore_pend(Display_Sem, BIOS_WAIT_FOREVER);
		display_screen();
	}
}
/*
 * Clock signal to check buttons
 */
void clock_signal(UArg arg1, UArg arg2) {
	Semaphore_post(Button_Sem);
}
/*
 * Highest priority task as it needs to receive user input potentially very quickly
 * and thus needs to preempt even the waveform task
 */
void button_task(UArg arg1, UArg arg2) {
	IntMasterEnable();

	uint16_t button_map = 0;

	while (1) {
		Mailbox_pend(mailbox0, &button_map, BIOS_WAIT_FOREVER);

		process_button(button_map);

	}
}
/*
 * Check button status and cpu load
 */
void user_input_task(UArg arg1, UArg arg2) {
	IArg cpu_gate;
	while (1) {
		Semaphore_pend(Button_Sem, BIOS_WAIT_FOREVER);
		ButtonISR();
		cpu_load_count = get_cpu_load_count();


		cpu_gate = GateTask_enter(gateTask0);
		cpu_load = (1.0f - (float) cpu_load_count / cpu_unloaded_count)
						* 100.0f;
		GateTask_leave(gateTask0,cpu_gate);
	}
}
