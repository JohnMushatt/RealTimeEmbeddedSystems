This program creates a digital oscilloscope that renders the waveform of a 20 KHz 40% duty cycle signal generated 
from the onboard PWM generator.
The display shows the time scale (20 us), the voltage division (default 1 V, range of .1V-2V)
It also shows the current trigger mode (Rising or falling)
Additionally, at the bottom, there is a counter for the number of errors generated by the ADC1 Overflow Flag
as well as the current load the CPU is handling.