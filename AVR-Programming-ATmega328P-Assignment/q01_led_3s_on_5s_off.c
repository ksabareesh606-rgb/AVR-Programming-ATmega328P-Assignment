/* Q1: LED on PD5 -> ON 3 s, OFF 5 s, repeat forever */
#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

/* _delay_ms() needs a compile-time constant and has a limited maximum,
   so build long delays out of 1 ms pieces. */
static void delay_ms(uint16_t ms)
{
    while (ms--) _delay_ms(1);
}

int main(void)
{
    DDRD |= (1 << PD5);              /* PD5 as output */

    while (1) {
        PORTD |= (1 << PD5);         /* LED ON  (immediately at start/reset) */
        delay_ms(3000);
        PORTD &= ~(1 << PD5);        /* LED OFF */
        delay_ms(5000);
    }
}
