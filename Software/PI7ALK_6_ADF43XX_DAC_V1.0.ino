/* 
 ADF4350/51 PLL Frequency Init and beacon message.

The Beacon 10Mhz ref frequency setting is controlled by a DAC.
PI4 en CW ident messages are generated by the same DAC modulating
the OCXO control voltage.

CW beacon software for PLL ADF4351 and ATTiny45 
can be compiled with arduino IDE.

Inspired by:
 
 V 0.1 Ondra OK1CDJ 9/2018 ondra@ok1cdj.com
 
 Parts of this code is based on routines written by PA3FYM

 Morse Based on Simple Arduino Morse Beacon  
 Written by Mark VandeWettering K6HX

 PI4 sending from code from BO OZ2M, https://www.rudius.net/oz2m/ 
 PI4 Data created by https://rudius.net/oz2m/ngnb/pi4encoding.php
 
 PI4, 146 symbols take 24.333382s
 ~ symbol time = 166.664ms
 ~ 6Hz symbol_rate
 

 Connections for PCB Master OCXO 
----------------------------------------------------------------
 Please check the ADF4351 datasheet or use ADF4351 software
 to get PLL register values. ADF reference OCXO is 10 Mhz.

 
 Attiny85 PIN layout for ADF4530/31 programming
 ----------------------------------------------------------------

 PB4 ADF4531 clock (CLK)   
 PB3 ADF4531 data (DATA)
 PB2 ADF4531 latch enable (LE)
 
 Attiny85 Clock is internal clock is set to 8 Mhz
 

 I2C DAC values for PI4 @  5760.935 Mhz
------------------------------------------------------
  PI4 Tone | Caculated Frequency    | DAC Value   
----------------------------------------------------------------
CW         : 5760935000,0000 Hz		:
CW FSK     : 5760934600,0000 Hz		:	
PI0        : 5760934882,8125 Hz		:
PI1        : 5760935117,1875 Hz		:
PI2        : 5760935351,5625 Hz		:
PI3        : 5760935585,9375 Hz		:

Calculated /2 Frequencies PI7ALK 6cm beacon

CW         : 2880467500,00000 Hz	: 0x838B
CW FSK	   : 2880467300,00000 Hz	: 0x7E86
PI0		   : 2880467441,40625 Hz	: 0x820A
PI1		   : 2880467558,59375 Hz	: 0x84F3
PI2		   : 2880467675,78125 Hz	: 0x87DC
PI3		   : 2880467792,96875 Hz	: 0x8ACF


 Setup Programs are provided to calibrate the
 internal Attiny RC-oscillator and OCXO for
 right frequency's for CW and PI4

 During setup() the RC-oscillator is adjusted
 to improve symbol accuracy.


 The control range in the prototype around 10Mhz
 is about 0,1877 Hz / digit. This will vary
 between OCXO.

 Version
 ----------------------------------------------------------------
 V1.0 Initial release for Aruino Nano and ATTiny85.
      ATTiny needs TinyWireM library for I2C. 


 Arduino Programming Library for ATTiny
 ----------------------------------------------------------------
 ATTINY CORE


 Test for CPU
 ----------------------------------------------------------------
*/

# if defined (__AVR_ATtiny85__)
 
  #include <TinyWireM.h>   // I2C control library for ATTiny

#else 

  #include <Wire.h>        // I2C control library

#endif


// Put your DAC I2C address here
const int MAX5217 = 0x1C;


//Setup your DAC values
//----------------------------------------
const uint16_t cwm = 0x838B;
const uint16_t cws = 0x7E86;
const uint16_t pi0 = 0x820A;
const uint16_t pi1 = 0x84F3;
const uint16_t pi2 = 0x87DC;  
const uint16_t pi3 = 0x8ACF;  



struct t_mtab {
  char c, pat;
} ;

// Bits represent morse code from LSB  1 dah, 0 dit, last bit is always 1 so "P" = 22 = (1)0110 

struct t_mtab morsetab[] = {
  {'.', 106},
  {',', 115},
  {'?', 76},
  {'/', 41},
  {'A', 6},
  {'B', 17},
  {'C', 21},
  {'D', 9},
  {'E', 2},
  {'F', 20},
  {'G', 11},
  {'H', 16},
  {'I', 4},
  {'J', 30},
  {'K', 13},
  {'L', 18},
  {'M', 7},
  {'N', 5},
  {'O', 15},
  {'P', 22},
  {'Q', 27},
  {'R', 10},
  {'S', 8},
  {'T', 3},
  {'U', 12},
  {'V', 24},
  {'W', 14},
  {'X', 25},
  {'Y', 29},
  {'Z', 19},
  {'1', 62},
  {'2', 60},
  {'3', 56},
  {'4', 48},
  {'5', 32},
  {'6', 33},
  {'7', 35},
  {'8', 39},
  {'9', 47},
  {'0', 63}
} ;

#define N_MORSE  (sizeof(morsetab)/sizeof(morsetab[0]))

int dotlen;
int dashlen;

/* Variable for millis timers */
const uint32_t timeout_time = 30000;
const uint32_t car_time = 59500;

bool cw_only = false;

/* 1 PPM Pin definition */
const int8_t GPI_pin = 4;    // Define GPI Input Pin PB2

char *txstr;

/*
 Data created by https://rudius.net/oz2m/ngnb/pi4encoding.php
 146 symbols take 24.333382s , symbol time = 166.664ms ~ 6Hz symbol_rate
 
*/

// PI7ALK, 5760,935 Mhz
//
int8_t fsymbols[] = {2,0,3,2,0,1,3,3,1,2,3,2,3,2,1,2,0,1,2,2,0,1,2,2,2,3,3,2,2,1,3,1,3,0,0,1,
                     1,1,1,3,0,0,1,1,2,1,3,3,1,0,1,2,3,3,0,1,3,0,1,2,0,2,0,0,1,3,3,3,1,0,3,0,
                     1,2,0,2,0,2,3,3,3,3,3,2,1,0,0,3,0,2,3,2,3,2,0,0,2,3,0,0,1,3,0,2,2,0,2,3,
                     1,0,2,2,0,1,3,2,2,1,1,3,2,3,3,1,0,1,3,0,3,2,3,2,3,2,2,0,2,1,1,3,0,2,2,2,1,1};


/* Init PI4 symbol time in ms
   A delay of 166; this gives a measured latch time of 167,6 is ~ ms 1.3ms */
const int dsymbol = 166;
 
/*Int PWM values L0 is half range, expected range 27, 127, 227 Hz
 Values need to be adjusted to the hardware in the sendpi4().*/

/* ADF PLL registers */
long int r0, r1, r2, r3, r4, r5;


// Write data to ADF code developed by PA0FYM, PA3AXA Hardware
//----------------------------------------------------------------------------------------

void write2PLL(uint32_t PLLword) {          // clocks 32 bits word  directly to the ADF4351
                                            // msb (b31) first, lsb (b0) last

  noInterrupts();                           // disable interrupts to keep accurate timing. 
  
  for (byte i=32; i>0; i--) {               // PLL word 32 bits
     
    (PLLword & 0x80000000? PORTB |= 0b00001000 : PORTB &= 0b11110111);   // data on PB3
                                                                               
    PORTB |= 0b00010000;                   // clock in bit on rising edge of CLK (PB4 = 1)
    PORTB &= 0b11101111;                   // CLK (PB4 = 0)      
    PLLword <<= 1;                         // rotate left for next bit
    }
    PORTB |= 0b00000100;                   // latch in PLL word on rising edge of LE (PB2 = 1)
    PORTB &= 0b11111011;                   // LE (PB2 = 0)      

  interrupts();                           // enable interrupts 

} 


// FSK CW Routines
//----------------------------------------------------------------------------------------

void dit(){

  // FSK CW - 400 Hz
  
  dacvolt(cwm);          // PWM CW
  delay(dotlen);
  
  dacvolt(cws);             // PWM CW_FSK -400
  delay(dotlen);  
  }


void dash(){

  // FSK CW - 400 Hz
  
  dacvolt(cwm);          // PWM CW
  delay(dashlen);
  
  dacvolt(cws);          // PWM CW_FSK -400
  delay(dotlen);
  }


// Look up a character in the tokens array and send it
void send(char c)
{
  int i ;
  if (c == ' ') {

    delay(7 * dotlen) ;
    return ;
  }
  for (i = 0; i < N_MORSE; i++) {
    if (morsetab[i].c == c) {
      unsigned char p = morsetab[i].pat ;

      while (p != 1) {
        if (p & 1)
          dash() ;
        else
          dit() ;
        p = p / 2 ;
      }
      delay(2 * dotlen) ;
      return ;
    }
  }
 //if we drop off the end, then we send a space

}

void sendmsg(char *txstr)
{
  while (*txstr)
    send(*txstr++) ;
}


 void dacvolt(uint16_t dac_16 )
 {
  byte dac_low = byte(dac_16 & 0x00FF);
  byte dac_high = byte((dac_16 & 0xFF00) >> 8);

#if defined (__AVR_ATtiny85__) 

  TinyWireM.beginTransmission(MAX5217);
  delayMicroseconds(10);
  TinyWireM.send(0x01);
  delayMicroseconds(10);
  TinyWireM.send(dac_high);
  delayMicroseconds(10);
  TinyWireM.send(dac_low);
  delayMicroseconds(10);
  TinyWireM.endTransmission();

 #else

  Wire.beginTransmission(MAX5217);
  delayMicroseconds(10);
  Wire.write(0x01);
  delayMicroseconds(10);
  Wire.write(dac_high);
  delayMicroseconds(10);
  Wire.write(dac_low);
  delayMicroseconds(10);
  Wire.endTransmission();

#endif
  
} 



/* Send PI4
------------------------------------------------------------
 We have to deal with symbols 0 to 3 and with 4 frequency's
 For this config need to generate PI4 at half the value due
 to the extra doubler after the PLL. 

 PI0 Case 0 : 5760934882,8125 Hz		:
 PI1 Case 1 : 5760935117,1875 Hz		:
 PI2 Case 2 : 5760935351,5625 Hz		:
 PI3 Case 3 : 5760935585,9375 Hz		:
 
PI0	Case 0	: 2880467441,40625 Hz	    : 0x820A
PI1	Case 1  : 2880467558,59375 Hz   	: 0x84F3
PI2	Case 2  : 2880467675,78125 Hz   	: 0x87DC
PI3	Case 3  : 2880467792,96875 Hz   	: 0x8ACF
 
*/

void sendpi4(){
 
  for (int tx = 0 ; tx < 147 ; tx++){

    switch (fsymbols[tx]){
       
      case 0:                     // 2880467441,40625 Hz

      delay(dsymbol);
      dacvolt(pi0);
      break;
      
      case 1:                     // 2880467558,59375 Hz

      delay(dsymbol);
  	  dacvolt(pi1);
      break;
     
      case 2:                     // 2880467675,78125 Hz
      
      delay(dsymbol);
      dacvolt(pi2);   
      break;
      
      case 3:                     // 2880467792,96875 Hz

      delay(dsymbol);
      dacvolt(pi3);   
      break;  
    }       

  }

}


                                   
// Setup Hardware Defaults
//------------------------------------------------------------

void setup () {

delay(2000);                     // Wait for ADF5341 to powerup

  OSCCAL -= 3;                   // User calibration ATTiny clock
  								 // for correct PI4 symbol timing

 
/* Pre Init to program the ADF4350/51 */

  // DDRB  = 0xff; // PB are all outputs 
  // PORTB = 0x00; // make PB low

/*

 Calculated Frequencies PI7ALK 6cm beacon

    CW         : 5760935000,0000 Hz
    CW FSK     : 5760934600,0000 Hz
    PI4 Tone 0 : 5760934882,8125 Hz
    PI4 Tone 1 : 5760935117,1875 Hz
    PI4 Tone 2 : 5760935351,5625 Hz
    PI4 Tone 3 : 5760935585,9375 Hz

 Calculated /2 Frequencies PI7ALK 6cm beacon

    CW         : 2880467500,00000 Hz
    CW FSK     : 2880467300,00000 Hz
    PI4 Tone 0 : 2880467441,40625 Hz
    PI4 Tone 1 : 2880467558,59375 Hz
    PI4 Tone 2 : 2880467675,78125 Hz
    PI4 Tone 3 : 2880467792,96875 Hz



 Register Value

 PI7ALK 6CM 5760.935/2 = 2880.4675
----------------------------------------------------
 0.5khz Channel Spacing for CW PI4, Ref Doubler
 Spurs every 20Mhz for 80 Mhz + and - carrier
*/

    r0 = 0x009005D8;
    r1 = 0x0800FD01;
    r2 = 0x62008E42;  // Low spur mode
    r3 = 0x000004B3; 
    r4 = 0x00850034;  // +2 dbm RF OUTPUT
    r5 = 0x00580005;

/* write from r5 to r0 to init ADF4350/ADF4351 */
 
     write2PLL(r5); 
     write2PLL(r4); 
     write2PLL(r3);
     write2PLL(r2);
     write2PLL(r1);
     write2PLL(r0);  


/* set speed and text for CW in WPM */
     
     int wpm = 12;

     dotlen = (1200 / wpm);
     dashlen = (3 * (1200 / wpm));
     txstr = "PI7ALK JO22IP";


 /* Setup PIN D02  for GPI_IN 1PPM */
  
 pinMode(GPI_pin, INPUT_PULLUP );     // GPI_pin to control PPM input

// Setup DAC

#if defined (__AVR_ATtiny85__) 

  Serial.setTimeout(1000);

  TinyWireM.begin();
  TinyWireM.beginTransmission(0b00011100);
  delayMicroseconds(10);
  TinyWireM.send(0b00001000);
  TinyWireM.send(0x00);
  delayMicroseconds(10);
  TinyWireM.endTransmission();

#else

  Serial.setTimeout(1000);

  Wire.begin();
  Wire.beginTransmission(0b00011100);
  delayMicroseconds(10);
  Wire.write(0b00001000);
  Wire.write(0x00);
  delayMicroseconds(10);
  Wire.endTransmission();

#endif 

         
} // End Setup



/* main loop total sequence should last 60 sec when transmitting PI4 */
//----------------------------------------------------------------------------------------

void loop() {



      // Start Time looptime
      uint32_t loop_time = millis();

      // T = 0, PI4 message
      if ( cw_only == false ){
      sendpi4();
       }

      // dacvolt(cwm);             // CW
      // T = 24.333, CW_FSK
      dacvolt(cws);             // CW_FSK -400
      delay(667);

      // T = 25,  CW_FSK Message
      sendmsg(txstr);

      // T ~ 40,  CW_FSK
      dacvolt(cws);             // CW_FSK -400
      delay(500);
                      
      // T = YY, CW Carrier
      dacvolt(cwm);             // CW
         
      do {
        /* Clock is to fast, with Attiny85 with no crystal so never right 
           Needs GPS PPM ( pulse / minute ) to sync TX
           Clock error compesation 59500 - 350.

           For external PPM make this a little shorter so we have some
           waiting time for the puls to arrive.
           
           The car_time and loop_time variables needs to be
           uint32_t, the same as millis() then this
           will never fail.

           Early resync option when PPM pulse returns
      
         */    
            delay(10);      // Reduce loop speed
         
           } while ( (millis() - loop_time) <= car_time );
          

      // T=59,5
      dacvolt(cws);             // PWM CW_FSK -400

      loop_time = millis();

      do {
          /*
         Time out Wait for External 1 PPM Pulse 
         after 30 sec no PPM we switch to CW beacon only
         until PPM returns
        */
           if ( (millis() - loop_time) >= timeout_time ){
            sendmsg(txstr);
            loop_time = millis();
           }
          
        delay(10);      // Reduce loop speed

        } while (digitalRead(GPI_pin) == HIGH );

        cw_only = false;
        
} // Mainloop end
      
