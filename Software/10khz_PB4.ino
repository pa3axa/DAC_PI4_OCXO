/*
  Simple test program to find the right calibration value
  for the RC-oscilator to improve accuracy of the delay
  timers used to send PI4.

  Found on

  http://becomingmaker.com/tuning-attiny-oscillator/
 
*/

ISR (TIMER0_COMPA_vect) {
   PORTB ^= 1 << PINB4;        // Invert pin PB4
}
 
void setup() {
    OSCCAL -= 3;                // User calibration
    pinMode(4,OUTPUT);          // Set PB4 to output
    TCNT0 = 0;                  // Count up from 0
    TCCR0A = 2 << WGM00;        // CTC mode
    if (CLKPR == 3)             // If clock set to 1MHz
        TCCR0B = (1<<CS00);     // Set prescaler to /1 (1uS at 1Mhz)
    else                        // Otherwise clock set to 8MHz
        TCCR0B = (2<<CS00);     // Set prescaler to /8 (1uS at 8Mhz)
    GTCCR |= 1 << PSR0;         // Reset prescaler
    OCR0A = 49;                 // 49 + 1 = 50 microseconds (10KHz)
    TIFR = 1 << OCF0A;          // Clear output compare interrupt flag
    TIMSK |= 1 << OCIE0A;       // Enable output compare interrupt
}
 
void loop() {}
