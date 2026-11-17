/* Q4: 4-bit binary counter. LEDs PB0..PB3, switch PD2 (active-low) */
#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

static void update_leds(uint8_t count)
{
    PORTB = (PORTB & 0xF0) | (count & 0x0F);   /* only touch PB0..PB3 */
}

static uint8_t switch_pressed(void)
{
    return !(PIND & (1 << PD2));
}

int main(void)
{
    uint8_t counter = 0;

    DDRB |= 0x0F;
    DDRD &= ~(1 << PD2);
    PORTD |= (1 << PD2);             /* pull-up */

    update_leds(counter);

    while (1) {
        if (switch_pressed()) {
            _delay_ms(20);
            if (switch_pressed()) {
                counter++;
                if (counter > 15) counter = 0;   /* wrap 1111 -> 0000 */
                update_leds(counter);
                while (switch_pressed());
                _delay_ms(20);
            }
        }
    }
}
