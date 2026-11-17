/* Q5: One-way traffic light WITHOUT _delay_ms().
   Green = PB0, Orange = PB1, Red = PB2.
   Timer1 in CTC mode generates a 1-second tick:
   16 MHz / 256 = 62500 Hz -> OCR1A = 62499 -> 1 s.
   The compare-match flag (OCF1A) is polled. */
#include <avr/io.h>
#include <stdint.h>

#define GREEN   PB0
#define ORANGE  PB1
#define RED     PB2

static void timer1_init(void)
{
    TCCR1A = 0;
    TCCR1B = (1 << WGM12) | (1 << CS12);     /* CTC, prescaler 256 */
    OCR1A  = 62499;                          /* 1 second */
    TCNT1  = 0;
    TIFR1 |= (1 << OCF1A);                   /* clear flag */
}

static void wait_seconds(uint8_t s)
{
    while (s--) {
        while (!(TIFR1 & (1 << OCF1A)));     /* wait for compare match */
        TIFR1 |= (1 << OCF1A);               /* clear by writing 1 */
    }
}

static void light(uint8_t pin)
{
    PORTB = (PORTB & ~((1 << GREEN) | (1 << ORANGE) | (1 << RED))) | (1 << pin);
}

int main(void)
{
    DDRB |= (1 << GREEN) | (1 << ORANGE) | (1 << RED);
    timer1_init();

    while (1) {
        light(GREEN);  wait_seconds(10);
        light(ORANGE); wait_seconds(3);
        light(RED);    wait_seconds(10);
    }
}
