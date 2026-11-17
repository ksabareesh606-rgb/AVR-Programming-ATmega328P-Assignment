/* Q15: BMP280 (I2C) -> 128x64 SSD1306 OLED (I2C), update every 2 s.
   Shared I2C bus: SDA = PC4, SCL = PC5 (external pull-ups, typically on the modules).
   BMP280 address 0x76 (SDO=GND) or 0x77 (SDO=VCC).  OLED address 0x3C.
   Shows temperature in degC and pressure in hPa.
   Compensation formulas are the integer versions from the BMP280 datasheet. */
#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define BMP_ADDR   0x76
#define OLED_ADDR  0x3C

/* ======================= I2C (TWI) ======================= */
static void twi_init(void)
{
    TWSR = 0;
    TWBR = ((F_CPU / 100000UL) - 16) / 2;         /* 100 kHz */
    TWCR = (1 << TWEN);
}

static void twi_wait(void) { while (!(TWCR & (1 << TWINT))); }

static void twi_start(uint8_t addr_rw)
{
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    twi_wait();
    TWDR = addr_rw;
    TWCR = (1 << TWINT) | (1 << TWEN);
    twi_wait();
}

static void twi_stop(void)
{
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
    while (TWCR & (1 << TWSTO));
}

static void twi_write(uint8_t b)
{
    TWDR = b;
    TWCR = (1 << TWINT) | (1 << TWEN);
    twi_wait();
}

static uint8_t twi_read(uint8_t ack)
{
    TWCR = (1 << TWINT) | (1 << TWEN) | (ack ? (1 << TWEA) : 0);
    twi_wait();
    return TWDR;
}

/* ======================= BMP280 ======================= */
static uint16_t dig_T1, dig_P1;
static int16_t  dig_T2, dig_T3, dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
static int32_t  t_fine;

static void bmp_write_reg(uint8_t reg, uint8_t val)
{
    twi_start((BMP_ADDR << 1) | 0);
    twi_write(reg);
    twi_write(val);
    twi_stop();
}

static void bmp_read_regs(uint8_t reg, uint8_t *buf, uint8_t len)
{
    twi_start((BMP_ADDR << 1) | 0);
    twi_write(reg);
    twi_start((BMP_ADDR << 1) | 1);               /* repeated start */
    for (uint8_t i = 0; i < len; i++)
        buf[i] = twi_read(i < len - 1);           /* NACK on last byte */
    twi_stop();
}

static void bmp_init(void)
{
    uint8_t c[24];

    bmp_write_reg(0xE0, 0xB6);                    /* soft reset */
    _delay_ms(10);

    bmp_read_regs(0x88, c, 24);                   /* calibration data */
    dig_T1 = (uint16_t)(c[1]  << 8 | c[0]);
    dig_T2 = (int16_t) (c[3]  << 8 | c[2]);
    dig_T3 = (int16_t) (c[5]  << 8 | c[4]);
    dig_P1 = (uint16_t)(c[7]  << 8 | c[6]);
    dig_P2 = (int16_t) (c[9]  << 8 | c[8]);
    dig_P3 = (int16_t) (c[11] << 8 | c[10]);
    dig_P4 = (int16_t) (c[13] << 8 | c[12]);
    dig_P5 = (int16_t) (c[15] << 8 | c[14]);
    dig_P6 = (int16_t) (c[17] << 8 | c[16]);
    dig_P7 = (int16_t) (c[19] << 8 | c[18]);
    dig_P8 = (int16_t) (c[21] << 8 | c[20]);
    dig_P9 = (int16_t) (c[23] << 8 | c[22]);

    bmp_write_reg(0xF5, 0x00);                    /* config: filter off */
    bmp_write_reg(0xF4, 0x27);                    /* temp x1, press x1, normal mode */
}

/* returns temperature in 0.01 degC (e.g. 2543 = 25.43 C), also updates t_fine */
static int32_t bmp_compensate_T(int32_t adc_T)
{
    int32_t v1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * (int32_t)dig_T2) >> 11;
    int32_t v2 = (((((adc_T >> 4) - (int32_t)dig_T1) *
                    ((adc_T >> 4) - (int32_t)dig_T1)) >> 12) * (int32_t)dig_T3) >> 14;
    t_fine = v1 + v2;
    return (t_fine * 5 + 128) >> 8;
}

/* returns pressure in Pa */
static uint32_t bmp_compensate_P(int32_t adc_P)
{
    int64_t var1, var2, p;
    var1 = (int64_t)t_fine - 128000;
    var2 = var1 * var1 * (int64_t)dig_P6;
    var2 = var2 + ((var1 * (int64_t)dig_P5) << 17);
    var2 = var2 + (((int64_t)dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)dig_P3) >> 8) + ((var1 * (int64_t)dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1) * (int64_t)dig_P1) >> 33;
    if (var1 == 0) return 0;
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)dig_P7) << 4);
    return (uint32_t)(p >> 8);
}

static void bmp_read(int32_t *temp_c100, uint32_t *press_pa)
{
    uint8_t d[6];
    bmp_read_regs(0xF7, d, 6);
    int32_t adc_P = ((int32_t)d[0] << 12) | ((int32_t)d[1] << 4) | (d[2] >> 4);
    int32_t adc_T = ((int32_t)d[3] << 12) | ((int32_t)d[4] << 4) | (d[5] >> 4);
    *temp_c100 = bmp_compensate_T(adc_T);         /* must be first (sets t_fine) */
    *press_pa  = bmp_compensate_P(adc_P);
}

/* ======================= SSD1306 OLED ======================= */
/* 5x7 font, only the characters this program needs */
static const char font_chars[] = " .-:0123456789CPTaehmprs";
static const uint8_t font5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, /* ' ' */
    {0x00,0x60,0x60,0x00,0x00}, /* '.' */
    {0x08,0x08,0x08,0x08,0x08}, /* '-' */
    {0x00,0x36,0x36,0x00,0x00}, /* ':' */
    {0x3E,0x51,0x49,0x45,0x3E}, /* 0 */
    {0x00,0x42,0x7F,0x40,0x00}, /* 1 */
    {0x42,0x61,0x51,0x49,0x46}, /* 2 */
    {0x21,0x41,0x45,0x4B,0x31}, /* 3 */
    {0x18,0x14,0x12,0x7F,0x10}, /* 4 */
    {0x27,0x45,0x45,0x45,0x39}, /* 5 */
    {0x3C,0x4A,0x49,0x49,0x30}, /* 6 */
    {0x01,0x71,0x09,0x05,0x03}, /* 7 */
    {0x36,0x49,0x49,0x49,0x36}, /* 8 */
    {0x06,0x49,0x49,0x29,0x1E}, /* 9 */
    {0x3E,0x41,0x41,0x41,0x22}, /* C */
    {0x7F,0x09,0x09,0x09,0x06}, /* P */
    {0x01,0x01,0x7F,0x01,0x01}, /* T */
    {0x20,0x54,0x54,0x54,0x78}, /* a */
    {0x38,0x54,0x54,0x54,0x18}, /* e */
    {0x7F,0x08,0x04,0x04,0x78}, /* h */
    {0x7C,0x04,0x18,0x04,0x78}, /* m */
    {0x7C,0x14,0x14,0x14,0x08}, /* p */
    {0x7C,0x08,0x04,0x04,0x08}, /* r */
    {0x48,0x54,0x54,0x54,0x20}, /* s */
};

static void oled_cmd(uint8_t c)
{
    twi_start((OLED_ADDR << 1) | 0);
    twi_write(0x00);                              /* control byte: command */
    twi_write(c);
    twi_stop();
}

static void oled_init(void)
{
    static const uint8_t seq[] = {
        0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40, 0x8D, 0x14,
        0x20, 0x00, 0xA1, 0xC8, 0xDA, 0x12, 0x81, 0xCF, 0xD9, 0xF1,
        0xDB, 0x40, 0xA4, 0xA6, 0xAF
    };
    _delay_ms(100);
    for (uint8_t i = 0; i < sizeof(seq); i++) oled_cmd(seq[i]);
}

static void oled_set_pos(uint8_t page, uint8_t col)
{
    oled_cmd(0xB0 | (page & 0x07));
    oled_cmd(col & 0x0F);
    oled_cmd(0x10 | (col >> 4));
}

static void oled_clear(void)
{
    for (uint8_t page = 0; page < 8; page++) {
        oled_set_pos(page, 0);
        twi_start((OLED_ADDR << 1) | 0);
        twi_write(0x40);                          /* control byte: data */
        for (uint8_t i = 0; i < 128; i++) twi_write(0x00);
        twi_stop();
    }
}

static void oled_puts(uint8_t page, uint8_t col, const char *s)
{
    oled_set_pos(page, col);
    twi_start((OLED_ADDR << 1) | 0);
    twi_write(0x40);
    while (*s) {
        const char *p = strchr(font_chars, *s++);
        uint8_t idx = p ? (uint8_t)(p - font_chars) : 0;
        for (uint8_t i = 0; i < 5; i++) twi_write(font5x7[idx][i]);
        twi_write(0x00);                          /* 1-column spacing */
    }
    twi_stop();
}

/* ======================= main ======================= */
int main(void)
{
    char line[24];
    int32_t temp;
    uint32_t press;

    twi_init();
    oled_init();
    bmp_init();

    while (1) {
        bmp_read(&temp, &press);

        oled_clear();                             /* clear screen, then redraw */

        /* temperature, e.g. "Temp: 25.43 C" */
        int32_t t = temp;
        char sign = ' ';
        if (t < 0) { sign = '-'; t = -t; }
        snprintf(line, sizeof(line), "Temp: %c%d.%02d C", sign, (int)(t / 100), (int)(t % 100));
        oled_puts(1, 0, line);

        /* pressure, e.g. "Press: 1013.2 hPa" */
        snprintf(line, sizeof(line), "Press: %u.%u hPa",
                 (unsigned)(press / 100), (unsigned)((press % 100) / 10));
        oled_puts(4, 0, line);

        for (uint16_t i = 0; i < 2000; i++) _delay_ms(1);   /* every 2 seconds */
    }
}
