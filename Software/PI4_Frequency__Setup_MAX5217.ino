 /*

 Simple Program to find the right DAC values to obtain
 the wanted frequency spacing For CW-FSK and PI4

 I used this program with an Arduino Nano directly
 connected to the DAC board I2C bus on JP2. 
 
  PI4 Tone | Caculated Frequency    | DAC Value   
----------------------------------------------------------------
 PI4 tone0 : 3.400.924.882,8125 Hz ;  0x7BB2 
 PI4 tone1 : 3.400.925.117,1875 Hz ;  0x8092  
 PI4 tone2 : 3.400.925.351,5625 Hz ;  0x8593  
 PI4 tone3 : 3.400.925.585,9375 Hz ;  0x8A92 

 CW-MARK   : 3.400.925.000,0000 Hz ;  0x7E22
 CW-Space  : 3.400.924.600,0000 Hz ;  0x75D0

73's Rens PA3AXA

 */

#include <Wire.h>

// Put your own I2C address here
const int MAX5217 = 0x1C;

// Variable
// 16bit DAC value

uint16_t value_16bit = 0x7E22; // 10Mhz Calibrated
uint16_t i = 0;

void setup() {

  

  Serial.begin(9600);
  Serial.setTimeout(1000);

  Wire.begin();
  Wire.beginTransmission(0b00011100);
  delayMicroseconds(10);
  Wire.write(0b00001000);
  Wire.write(0x00);
  delayMicroseconds(10);
  Wire.endTransmission();

 dacvolt();

}


void dacvolt(){

  byte dac_low = byte(value_16bit & 0x00FF);
  byte dac_high = byte((value_16bit & 0xFF00) >> 8);

  Wire.beginTransmission(MAX5217);
  delayMicroseconds(10);
  Wire.write(0x01);
  delayMicroseconds(10);
  Wire.write(dac_high);
  delayMicroseconds(10);
  Wire.write(dac_low);
  delayMicroseconds(10);
  Wire.endTransmission();
  
}  
  
 
void loop() {

  
  while (Serial.available() > 0){
   i = Serial.parseInt();
    if ( i != 0 ) {
    Serial.print ("i = ");
    Serial.println(i);
    value_16bit = i;
    Serial.println(value_16bit, HEX);
    dacvolt();
    }
 
  }

  delay(1000);
}
