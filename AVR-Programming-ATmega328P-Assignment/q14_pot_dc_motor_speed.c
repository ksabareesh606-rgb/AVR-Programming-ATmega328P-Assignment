/* Q14: DC motor speed from potentiometer.
   Pot on ADC0 (PC0), motor driver input on OC0A (PD6), Timer0 Fast PWM.
   10-bit ADC (0..1023) is scaled to 8-bit (0..255) with a >>2 shift
   -> linear response: 0 V = stop, 5 V = full speed. */
#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

static void adc_init(void)
{
    ADMUX  = (1 << REFS0);
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

static void pwm_init(void)
{
    DDRD  |= (1 << PD6);
    TCCR0A = (1 << COM0A1) | (1 << WGM01) | (1 << WGM00);
    TCCR0B = (1 << CS01) | (1 << CS00);          /* /64 */
    OCR0A  = 0;
}

/* dedicated function: read 10-bit ADC and scale to 8-bit */
static uint8_t read_pot_8bit(void)
{
    ADMUX = (ADMUX & 0xF0);                      /* channel 0 */
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    return (uint8_t)(ADC >> 2);                  /* 0..1023 -> 0..255 */
}

static void set_motor_speed(uint8_t value)
{
    if (value == 0) {                            /* OCR0A = 0 still gives a narrow pulse */
        TCCR0A &= ~(1 << COM0A1);
        PORTD  &= ~(1 << PD6);                   /* motor fully stopped */
    } else {
        TCCR0A |= (1 << COM0A1);
        OCR0A = value;
    }
}

int main(void)
{
    adc_init();
    pwm_init();

    while (1) {
        set_motor_speed(read_pot_8bit());
        _delay_ms(10);
    }
}
