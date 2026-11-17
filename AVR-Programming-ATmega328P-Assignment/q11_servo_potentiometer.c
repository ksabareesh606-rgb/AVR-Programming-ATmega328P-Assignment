/* Q11: Servo position from potentiometer.
   Pot on ADC0 (PC0), servo signal on OC1A (PB1).
   Timer1: 16-bit Phase Correct PWM, mode 10 (TOP = ICR1).
   Prescaler 8 -> timer clock 2 MHz (0.5 us per tick).
   f_pwm = F_CPU / (2 * N * TOP) = 16e6 / (2*8*20000) = 50 Hz  -> ICR1 = 20000.
   In phase-correct mode the high time = 2 * OCR1A ticks = OCR1A microseconds,
   so OCR1A = pulse width in us (1000 us = 0 deg, 2000 us = 180 deg).
   (Many servos accept 544..2400 us; change the defines if yours does.) */
#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

#define SERVO_MIN_US 1000UL
#define SERVO_MAX_US 2000UL

static void adc_init(void)
{
    ADMUX  = (1 << REFS0);
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

static void timer1_servo_init(void)
{
    DDRB |= (1 << PB1);
    TCCR1A = (1 << COM1A1) | (1 << WGM11);                   /* non-inverting, WGM1[1:0]=10 */
    TCCR1B = (1 << WGM13) | (1 << CS11);                     /* WGM1[3:2]=10 -> mode 10, /8 */
    ICR1   = 20000;                                          /* 50 Hz */
    OCR1A  = (uint16_t)SERVO_MIN_US;
}

/* read 10-bit pot value and map to servo pulse width in microseconds */
static uint16_t read_pot_pulse_us(void)
{
    ADMUX = (ADMUX & 0xF0) | 0;                              /* channel 0 */
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    uint16_t adc = ADC;                                      /* 0..1023 */
    return (uint16_t)(SERVO_MIN_US + ((uint32_t)adc * (SERVO_MAX_US - SERVO_MIN_US)) / 1023UL);
}

int main(void)
{
    adc_init();
    timer1_servo_init();

    while (1) {
        OCR1A = read_pot_pulse_us();                         /* real-time update */
        _delay_ms(10);
    }
}
