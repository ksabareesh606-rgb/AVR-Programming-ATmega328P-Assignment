/* Q13: UART (9600 8N1) controlled LED with PWM brightness.
   Commands (end with Enter / CR / LF):  ON | OFF | 0..100
   Timer choice: Timer0, Fast PWM, 8-bit, OC0A = PD6.
     - PD6 is a hardware PWM pin, ~976 Hz (prescaler 64) is flicker-free for an LED,
       8-bit resolution (256 steps) is more than enough for a 0..100 % scale,
       and Fast PWM gives the highest frequency for a given resolution.
   LED (with resistor) on PD6. */
#define F_CPU 16000000UL
#include <avr/io.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>

#define BAUD   9600
#define UBRR_V ((F_CPU / 16UL / BAUD) - 1)      /* 103 */
#define RX_LEN 16

/* ---------------- UART ---------------- */
static void uart_init(void)
{
    UBRR0H = (uint8_t)(UBRR_V >> 8);
    UBRR0L = (uint8_t)UBRR_V;
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);     /* 8 data bits, 1 stop, no parity */
}

static void uart_putc(char c)
{
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
}

static void uart_puts(const char *s)
{
    while (*s) uart_putc(*s++);
}

static char uart_getc(void)
{
    while (!(UCSR0A & (1 << RXC0)));
    return UDR0;
}

/* read one line, returns when CR/LF received (empty lines ignored) */
static void uart_readline(char *buf, uint8_t max)
{
    uint8_t i = 0;
    while (1) {
        char c = uart_getc();
        if (c == '\r' || c == '\n') {
            if (i > 0) break;
            continue;
        }
        if (i < max - 1) buf[i++] = c;
    }
    buf[i] = '\0';
}

/* ---------------- PWM ---------------- */
static void pwm_init(void)
{
    DDRD  |= (1 << PD6);
    TCCR0A = (1 << WGM01) | (1 << WGM00);       /* Fast PWM */
    TCCR0B = (1 << CS01) | (1 << CS00);         /* /64 */
    OCR0A  = 0;
}

/* percent 0..100 */
static void pwm_set_percent(uint8_t percent)
{
    if (percent == 0) {
        TCCR0A &= ~(1 << COM0A1);               /* disconnect: true 0 % */
        PORTD  &= ~(1 << PD6);
    } else {
        TCCR0A |= (1 << COM0A1);
        OCR0A = (uint8_t)(((uint16_t)percent * 255U) / 100U);
    }
}

/* ---------------- Command handling ---------------- */
static uint8_t is_number(const char *s)
{
    if (!*s) return 0;
    while (*s) if (!isdigit((unsigned char)*s++)) return 0;
    return 1;
}

static void process_command(char *cmd)
{
    static uint8_t brightness = 100;            /* last brightness, used by "ON" */
    char msg[32];

    for (char *p = cmd; *p; p++) *p = toupper((unsigned char)*p);

    if (strcmp(cmd, "ON") == 0) {
        pwm_set_percent(brightness ? brightness : 100);
        uart_puts("LED is now ON\r\n");
    } else if (strcmp(cmd, "OFF") == 0) {
        pwm_set_percent(0);
        uart_puts("LED is now OFF\r\n");
    } else if (is_number(cmd) && strlen(cmd) <= 3 && atoi(cmd) <= 100) {
        brightness = (uint8_t)atoi(cmd);
        pwm_set_percent(brightness);
        snprintf(msg, sizeof(msg), "Brightness set to %u%%\r\n", brightness);
        uart_puts(msg);
    } else {
        uart_puts("Invalid command. Use ON, OFF or 0-100\r\n");
    }
}

int main(void)
{
    char rx[RX_LEN];

    uart_init();
    pwm_init();
    uart_puts("Ready. Commands: ON, OFF, 0-100\r\n");

    while (1) {
        uart_readline(rx, RX_LEN);
        process_command(rx);
    }
}
