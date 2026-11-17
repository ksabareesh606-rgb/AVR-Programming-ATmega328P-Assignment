/* Q2: Push button toggles between Red LED and Green LED
   Red = PB0, Green = PB1, Switch = PD2 (active-low, internal pull-up) */
#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

#define RED     PB0
#define GREEN   PB1
#define SWITCH  PD2

static uint8_t switch_pressed(void)
{
    return !(PIND & (1 << SWITCH));
}

int main(void)
{
    DDRB  |= (1 << RED) | (1 << GREEN);
    DDRD  &= ~(1 << SWITCH);
    PORTD |= (1 << SWITCH);          /* pull-up */

    PORTB |= (1 << RED);             /* start with Red ON */
    PORTB &= ~(1 << GREEN);

    while (1) {
        if (switch_pressed()) {
            _delay_ms(20);           /* debounce */
            if (switch_pressed()) {
                PORTB ^= (1 << RED) | (1 << GREEN);   /* swap both LEDs */
                while (switch_pressed());             /* wait for release -> one press = one change */
                _delay_ms(20);
            }
        }
    }
}
