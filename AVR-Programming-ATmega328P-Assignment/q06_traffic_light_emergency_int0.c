/* Q6: Traffic light with emergency button on INT0 (PD2).
   Green = PB0, Orange = PB1, Red = PB2.
   ISR only sets a flag. The main loop checks it at the END of each
   light's duration; if set, Green is given priority for a fixed time,
   the flag is cleared, and the normal cycle resumes. */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#define GREEN   PB0
#define ORANGE  PB1
#define RED     PB2

#define GREEN_TIME      10
#define ORANGE_TIME     3
#define RED_TIME        10
#define EMERGENCY_GREEN 15      /* priority green duration (s) */

volatile uint8_t emergency_flag = 0;

ISR(INT0_vect)
{
    emergency_flag = 1;         /* just set the flag, no state jump */
}

static void timer1_init(void)   /* 1 s tick, polled */
{
    TCCR1A = 0;
    TCCR1B = (1 << WGM12) | (1 << CS12);
    OCR1A  = 62499;
    TCNT1  = 0;
    TIFR1 |= (1 << OCF1A);
}

static void wait_seconds(uint8_t s)
{
    while (s--) {
        while (!(TIFR1 & (1 << OCF1A)));
        TIFR1 |= (1 << OCF1A);
    }
}

static void light(uint8_t pin)
{
    PORTB = (PORTB & ~((1 << GREEN) | (1 << ORANGE) | (1 << RED))) | (1 << pin);
}

static void int0_init(void)
{
    DDRD  &= ~(1 << PD2);
    PORTD |= (1 << PD2);                 /* pull-up, button to GND */
    EICRA  = (1 << ISC01);               /* falling edge */
    EIMSK  = (1 << INT0);
}

typedef enum { ST_GREEN, ST_ORANGE, ST_RED } state_t;

int main(void)
{
    state_t state = ST_GREEN;

    DDRB |= (1 << GREEN) | (1 << ORANGE) | (1 << RED);
    timer1_init();
    int0_init();
    sei();

    while (1) {
        switch (state) {
        case ST_GREEN:  light(GREEN);  wait_seconds(GREEN_TIME);  state = ST_ORANGE; break;
        case ST_ORANGE: light(ORANGE); wait_seconds(ORANGE_TIME); state = ST_RED;    break;
        case ST_RED:    light(RED);    wait_seconds(RED_TIME);    state = ST_GREEN;  break;
        }

        /* End of this light's minimum duration: check emergency flag */
        if (emergency_flag) {
            light(GREEN);
            wait_seconds(EMERGENCY_GREEN);
            emergency_flag = 0;          /* clear flag, resume normal cycle */
            state = ST_ORANGE;           /* after priority green, go Orange -> Red */
        }
    }
}
