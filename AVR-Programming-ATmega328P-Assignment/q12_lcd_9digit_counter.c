/* Q12: 9-digit counter on 16x2 LCD (HD44780, 4-bit mode)
   LCD: RS = PB0, EN = PB1, D4..D7 = PB2..PB5 (RW tied to GND)
   Button on PD2 (active-low, pull-up).
   Short press -> +1.  Hold >= 2 s -> reset to 000000000. */
#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdint.h>

#define LCD_RS PB0
#define LCD_EN PB1
#define BUTTON PD2

#define LONG_PRESS_MS 2000
#define MAX_COUNT     999999999UL

typedef enum { EV_NONE, EV_SHORT, EV_LONG } btn_event_t;

/* ---------------- LCD driver ---------------- */
static void lcd_nibble(uint8_t n)
{
    PORTB = (PORTB & ~0x3C) | ((n & 0x0F) << 2);
    PORTB |= (1 << LCD_EN);
    _delay_us(1);
    PORTB &= ~(1 << LCD_EN);
    _delay_us(50);
}

static void lcd_cmd(uint8_t c)
{
    PORTB &= ~(1 << LCD_RS);
    lcd_nibble(c >> 4);
    lcd_nibble(c);
    _delay_ms(2);
}

static void lcd_data(uint8_t d)
{
    PORTB |= (1 << LCD_RS);
    lcd_nibble(d >> 4);
    lcd_nibble(d);
}

static void lcd_init(void)
{
    DDRB |= 0x3F;
    _delay_ms(50);
    PORTB &= ~(1 << LCD_RS);
    lcd_nibble(0x03); _delay_ms(5);
    lcd_nibble(0x03); _delay_us(150);
    lcd_nibble(0x03);
    lcd_nibble(0x02);                    /* 4-bit mode */
    lcd_cmd(0x28);                       /* 2 lines, 5x8 */
    lcd_cmd(0x0C);                       /* display on, cursor off */
    lcd_cmd(0x06);                       /* entry mode: increment */
    lcd_cmd(0x01);                       /* clear */
}

static void lcd_set_cursor(uint8_t row, uint8_t col)
{
    lcd_cmd(0x80 | ((row ? 0x40 : 0x00) + col));
}

static void lcd_print(const char *s)
{
    while (*s) lcd_data(*s++);
}

/* ---------------- Required functions ---------------- */
static uint8_t button_down(void)
{
    return !(PIND & (1 << BUTTON));
}

/* Measures how long the button is held and classifies it */
static btn_event_t check_button(void)
{
    uint16_t held = 0;

    if (!button_down()) return EV_NONE;
    _delay_ms(20);                                   /* debounce */
    if (!button_down()) return EV_NONE;

    while (button_down()) {
        _delay_ms(10);
        held += 10;
        if (held >= LONG_PRESS_MS) {                 /* long press detected */
            while (button_down());                   /* wait for release so it
                                                        doesn't count as a short press */
            _delay_ms(20);
            return EV_LONG;
        }
    }
    _delay_ms(20);
    return EV_SHORT;
}

static void update_display(uint32_t count)
{
    char buf[17];
    snprintf(buf, sizeof(buf), "%09lu", count);
    lcd_set_cursor(1, 0);
    lcd_print(buf);
}

int main(void)
{
    uint32_t count = 0;

    DDRD  &= ~(1 << BUTTON);
    PORTD |= (1 << BUTTON);

    lcd_init();
    lcd_set_cursor(0, 0);
    lcd_print("Counter:");
    update_display(count);

    while (1) {
        switch (check_button()) {
        case EV_SHORT:
            if (count < MAX_COUNT) count++; else count = 0;
            update_display(count);
            break;
        case EV_LONG:
            count = 0;
            update_display(count);
            break;
        default:
            break;
        }
    }
}
