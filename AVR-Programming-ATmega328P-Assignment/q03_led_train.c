/* Q3: "Train" animation on 8 LEDs on PORTB (PB0..PB7)
   Note: on an Arduino Uno PB6/PB7 are the crystal pins; use a bare
   ATmega328P (internal oscillator / simulator) for all 8 bits. */
#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

int main(void)
{
    DDRB = 0xFF;                     /* all PORTB pins output */

    while (1) {
        for (uint8_t i = 0; i < 8; i++) {
            PORTB = (1 << i);        /* only LED i ON */
            _delay_ms(200);
        }
        /* loop restarts with LED 0 immediately */
    }
}
