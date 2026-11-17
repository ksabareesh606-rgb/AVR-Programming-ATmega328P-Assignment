/* Q10: Automatic cooling system.
   LM35 on ADC0 (PC0), DC motor driver input on OC0A (PD6, Timer0 Fast PWM),
   ON/OFF push-button on PD2 (active-low, pull-up).
   LM35: 10 mV per degC.  Vref = AVcc = 5 V, 10-bit ADC:
        mV = adc * 5000 / 1024  ==> tenths of degC (e.g. 253 = 25.3 C) */
#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

#define BUTTON PD2

/* temperature thresholds in tenths of degC */
#define T_LOW   300     /* < 30.0 C  -> low speed    */
#define T_HIGH  400     /* >= 40.0 C -> full speed   */

static void adc_init(void)
{
    ADMUX  = (1 << REFS0);                                   /* AVcc ref, channel 0 */
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);  /* /128 */
}

static uint16_t adc_read(uint8_t ch)
{
    ADMUX = (ADMUX & 0xF0) | (ch & 0x0F);
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    return ADC;
}

/* --- sensor reading: returns temperature in tenths of degC (8-sample average) --- */
static int16_t read_temperature(void)
{
    uint32_t sum = 0;
    for (uint8_t i = 0; i < 8; i++) sum += adc_read(0);
    uint16_t adc = sum / 8;
    return (int16_t)((uint32_t)adc * 5000UL / 1024UL);
}

/* --- speed calculation: temperature -> PWM duty (0..255) --- */
static uint8_t calculate_speed(int16_t temp10)
{
    if (temp10 < T_LOW)  return 102;     /* ~40 % */
    if (temp10 < T_HIGH) return 178;     /* ~70 % */
    return 255;                          /* 100 % */
}

/* --- motor control --- */
static void motor_init(void)
{
    DDRD  |= (1 << PD6);
    TCCR0A = (1 << WGM01) | (1 << WGM00);                    /* Fast PWM */
    TCCR0B = (1 << CS01) | (1 << CS00);                      /* /64 */
    TCCR0A &= ~(1 << COM0A1);
    PORTD &= ~(1 << PD6);
}

static void motor_set(uint8_t duty)
{
    if (duty == 0) {
        TCCR0A &= ~(1 << COM0A1);        /* disconnect PWM, pin low = stop */
        PORTD  &= ~(1 << PD6);
    } else {
        TCCR0A |= (1 << COM0A1);
        OCR0A = duty;
    }
}

static uint8_t button_pressed(void)
{
    return !(PIND & (1 << BUTTON));
}

int main(void)
{
    uint8_t system_on = 0;

    DDRD  &= ~(1 << BUTTON);
    PORTD |= (1 << BUTTON);
    adc_init();
    motor_init();

    while (1) {
        if (button_pressed()) {
            _delay_ms(30);
            if (button_pressed()) {
                system_on ^= 1;
                if (!system_on) motor_set(0);
                while (button_pressed());
                _delay_ms(30);
            }
        }

        if (system_on) {
            int16_t t = read_temperature();
            motor_set(calculate_speed(t));
        }
        _delay_ms(50);
    }
}
