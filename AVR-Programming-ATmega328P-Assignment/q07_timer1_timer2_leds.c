/* Q7: Two independent timing tasks using Compare Match interrupts.
   Timer1 (16-bit): toggle Status LED  PB1 every 2 s
   Timer2 (8-bit) : toggle Heartbeat LED PB2 every 500 ms

   Timer1: 16 MHz / 1024 = 15625 Hz, OCR1A = 31249 -> 2.000 s
   Timer2: 16 MHz / 64 = 250 kHz, OCR2A = 249 -> 1 ms tick,
           500 ticks (counted in the ISR) = 500 ms
           (an 8-bit timer cannot reach 500 ms by itself). */
#include <avr/io.h>
#include <avr/interrupt.h>

ISR(TIMER1_COMPA_vect)
{
    PORTB ^= (1 << PB1);                 /* Status LED */
}

ISR(TIMER2_COMPA_vect)
{
    static uint16_t ms = 0;
    if (++ms >= 500) {
        ms = 0;
        PORTB ^= (1 << PB2);             /* Heartbeat LED */
    }
}

static void timer1_init(void)
{
    TCCR1A = 0;
    TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10);   /* CTC, /1024 */
    OCR1A  = 31249;
    TIMSK1 = (1 << OCIE1A);
}

static void timer2_init(void)
{
    TCCR2A = (1 << WGM21);                               /* CTC */
    TCCR2B = (1 << CS22);                                /* /64 */
    OCR2A  = 249;
    TIMSK2 = (1 << OCIE2A);
}

int main(void)
{
    DDRB |= (1 << PB1) | (1 << PB2);
    timer1_init();
    timer2_init();
    sei();

    while (1) {
        /* nothing: everything runs in the ISRs */
    }
}
