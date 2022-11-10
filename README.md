# DAC based 10Mhz OCXO Reference\PI4 and CW-FSK modulator

This is a 10Mhz reference oscillator with PI4 and\
CW-FSK modulation for use in Microwave Beacons.\
It is selfcontained once programmed for the right\
frequency and Callsign in both PI4 and CW-FSK.

The board is designed around the CTI-OSC5A2B02\
10Mhz OCXO which is still avialable for reasonable\
prices on AliExpress.

For PI4 operation a 1 PPM signal is needed to sync\
the transmission of PI4 to the start of the minute\

My other project Easy-1PPM can be used to provide\
this 1 PPM pulse

# Software

The software is preset with the values of PI7ALK\
but this easy to change and self explaining.

To optimize we need to check and calibrate the\
ATTiny85 internal RC-oscillator this can performed\
with 10khz_PB4.ino program and a frequency counter.

With the program PI4_Frequency__Setup_MAX5217.ino\
interactive frequency adjustment can be preformed,\
the found values for your OCXO need to be hardcode\
in PI7ALK_X_ADF43XX_DAC_V1.0.

The same is true for the callsign in both CW-FSK as PI4.

PA3AXA | 09/2022  | V1.0
 
