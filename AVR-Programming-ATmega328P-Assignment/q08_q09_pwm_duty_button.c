/* Q8 + Q9: Timer0 Fast PWM on OC0A (PD6).
   Button on PD3 (active-low, pull-up) cycles duty: 25% -> 50% -> 75% -> 100% -> 25% ...
   Software debounce included.
   (The "100% OFF" wording in the question is read as 100% = fully ON,
    since the sequence is 25/50/75/100 %.) */
#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

#define BUTTON PD3

static const uint8_t duty_table[4] = { 64, 128, 191, 255 };  /* 25, 50, 75, 100 % of 255 */

static void pwm_init(void)
{
    DDRD |= (1 << PD6);                                      /* OC0A output */
    TCCR0A = (1 << COM0A1) | (1 << WGM01) | (1 << WGM00);    /* Fast PWM, non-inverting */
    TCCR0B = (1 << CS01) | (1 << CS00);                      /* prescaler 64 -> ~976 Hz */
    OCR0A  = duty_table[0];                                  /* start at 25 % */
}

/* dedicated function that updates the timer register */
static void set_duty_level(uint8_t level)
{
    OCR0A = duty_table[level];
}

static uint8_t button_pressed(void)
{
    return !(PIND & (1 << BUTTON));
}

int main(void)
{
    uint8_t level = 0;

    DDRD  &= ~(1 << BUTTON);
    PORTD |= (1 << BUTTON);
    pwm_init();

    while (1) {
        if (button_pressed()) {
            _delay_ms(30);                       /* debounce */
            if (button_pressed()) {
                level = (level + 1) & 0x03;      /* 0..3 then back to 0 */
                set_duty_level(level);
                while (button_pressed());        /* wait for release */
                _delay_ms(30);
            }
        }
    }
}
