#include <mcs51/8051.h>
#include <stdint.h>

__sfr __at (0xC2) IAP_DATA;
__sfr __at (0xC3) IAP_ADDRH;
__sfr __at (0xC4) IAP_ADDRL;
__sfr __at (0xC5) IAP_CMD;
__sfr __at (0xC6) IAP_TRIG;
__sfr __at (0xC7) IAP_CONTR;

#define LED         P3_1
#define UART_TX     P3_1
#define SD_CS       P3_1
#define RAM_CS      P3_4
#define LCD_CS      P3_0
#define SPI_SCK     P3_5
#define SPI_MOSI    P3_2
#define SPI_MISO    P3_3

#define SD_ACMD41_HCS   0x40000000UL
#define SD_CS_LOW()     (SD_CS = 0)
#define SD_CS_HIGH()    (SD_CS = 1)

#define SD_OK               0
#define SD_ERROR            1
#define SD_TIMEOUT          2
#define RAM_CMD_READ        0x03
#define RAM_CMD_WRITE       0x02
#define RAM_CMD_RESET_EN    0x66
#define RAM_CMD_RESET       0x99

#define IAP_CMD_READ    0x01
#define IAP_CMD_WRITE   0x02
#define IAP_CMD_ERASE   0x03
#define IAP_ENABLE      0x83

static uint8_t sd_sdhc = 0;

static void iap_disable(void)
{
    IAP_CONTR = 0x00;
    IAP_CMD   = 0x00;
    IAP_TRIG  = 0x00;

    IAP_ADDRH = 0x80;
    IAP_ADDRL = 0x00;
}

static void iap_trigger(void)
{
    __asm
        nop
    __endasm;
}

uint8_t eeprom_read_byte(uint16_t addr)
{
    uint8_t value;
    __bit old_ea;

    old_ea = EA;
    EA = 0;

    IAP_ADDRH = (uint8_t)(addr >> 8);
    IAP_ADDRL = (uint8_t)addr;
    IAP_CONTR = IAP_ENABLE;
    IAP_CMD   = IAP_CMD_READ;

    IAP_TRIG = 0x5A;
    IAP_TRIG = 0xA5;
    iap_trigger();

    value = IAP_DATA;

    iap_disable();
    EA = old_ea;

    return value;
}

void eeprom_write_byte(uint16_t addr, uint8_t value)
{
    __bit old_ea;

    old_ea = EA;
    EA = 0;

    IAP_DATA = value;
    IAP_ADDRH = (uint8_t)(addr >> 8);
    IAP_ADDRL = (uint8_t)addr;
    IAP_CONTR = IAP_ENABLE;
    IAP_CMD   = IAP_CMD_WRITE;

    IAP_TRIG = 0x5A;
    IAP_TRIG = 0xA5;
    iap_trigger();

    iap_disable();
    EA = old_ea;
}

static void spi_delay(void)
{
    __asm
        nop
        nop
    __endasm;
}

static void ram_select(void)
{
    RAM_CS = 0;
}

static void ram_deselect(void)
{
    RAM_CS = 1;
}

static void spi_write_byte(unsigned char value)
{
    unsigned char i = 0;

    for (i = 0; i < 8; i++) {
        SPI_MOSI = (value & 0x80) ? 1 : 0;
        spi_delay();
        SPI_SCK = 1;
        spi_delay();
        SPI_SCK = 0;
        value <<= 1;
    }
}

static unsigned char spi_read_byte(void)
{
    unsigned char i = 0;
    unsigned char value = 0;

    SPI_MISO = 1;
    for (i = 0; i < 8; i++) {
        value <<= 1;

        SPI_SCK = 1;
        spi_delay();

        if (SPI_MISO) {
            value |= 1;
        }

        SPI_SCK = 0;
        spi_delay();
    }

    return value;
}

static void spi_write_address(unsigned long addr)
{
    spi_write_byte((unsigned char)(addr >> 16));
    spi_write_byte((unsigned char)(addr >> 8));
    spi_write_byte((unsigned char)addr);
}

static void ram_command(unsigned char cmd)
{
    ram_select();
    spi_write_byte(cmd);
    ram_deselect();
}

static void ram_reset(void)
{
    ram_deselect();
    SPI_SCK = 0;
    SPI_MOSI = 0;

    ram_command(RAM_CMD_RESET_EN);
    ram_command(RAM_CMD_RESET);
}

static void ram_write_byte(unsigned long addr, unsigned char value)
{
    ram_select();
    spi_write_byte(RAM_CMD_WRITE);
    spi_write_address(addr);
    spi_write_byte(value);
    ram_deselect();
}

static unsigned char ram_read_byte(unsigned long addr)
{
    unsigned char value = 0;

    ram_select();
    spi_write_byte(RAM_CMD_READ);
    spi_write_address(addr);
    value = spi_read_byte();
    ram_deselect();

    return value;
}

static void delay_ms(unsigned int ms)
{
    unsigned int i = 0;

    while (ms--) {
        for (i = 0; i < 1000; i++) {
            __asm
                nop
                nop
                nop
            __endasm;
        }
    }
}

static void uart_bit_delay(void)
{
    __asm
        push 5
        mov r5, #68
loop:   djnz r5, loop
        pop 5
    __endasm;
}

static void uart_send_byte(unsigned char ch)
{
    unsigned char i = 0;

    UART_TX = 0;
    uart_bit_delay();

    for (i = 0; i < 8; i++) {
        UART_TX = ch & 0x01;
        uart_bit_delay();
        ch >>= 1;
    }

    UART_TX = 1;
    uart_bit_delay();
}

static void uart_send_hex(unsigned char ch)
{
    unsigned char t = 0;

    t = (ch >> 4) & 0x0F;
    uart_send_byte(t < 10 ? t + '0' : t - 10 + 'A');

    t = ch & 0x0F;
    uart_send_byte(t < 10 ? t + '0' : t - 10 + 'A');
}

static void uart_send_string(const char *buf)
{
    const char *p = buf;

    while (p && *p) {
        uart_send_byte(*p++);
    }
}

static uint8_t spi_transfer(uint8_t tx)
{
    uint8_t i = 0;
    uint8_t rx = 0;

    for (i = 0; i < 8; i++) {
        SPI_SCK = 0;
        SPI_MOSI = (tx & 0x80) ? 1 : 0;
        tx <<= 1;

        SPI_SCK = 1;
        rx <<= 1;
        if (SPI_MISO) {
            rx |= 1;
        }
    }
    SPI_SCK = 0;

    return rx;
}

static void sd_deselect(void)
{
    SD_CS_HIGH();
    spi_transfer(0xff);
}

static uint8_t sd_select(void)
{
    uint16_t timeout = 0;

    SD_CS_LOW();
    for (timeout = 0; timeout < 50000; timeout++) {
        if (spi_transfer(0xff) == 0xff) {
            return SD_OK;
        }
    }
    sd_deselect();

    return SD_TIMEOUT;
}

static uint8_t sd_wait_ready(void)
{
    uint16_t timeout = 0;

    for (timeout = 0; timeout < 50000; timeout++) {
        if (spi_transfer(0xff) == 0xff) {
            return SD_OK;
        }
    }

    return SD_TIMEOUT;
}

static uint8_t sd_command(uint8_t cmd, uint32_t arg, uint8_t crc)
{
    uint8_t r = 0;
    uint8_t i = 0;

    spi_transfer(0x40 | cmd);
    spi_transfer((uint8_t)(arg >> 24));
    spi_transfer((uint8_t)(arg >> 16));
    spi_transfer((uint8_t)(arg >> 8));
    spi_transfer((uint8_t)arg);
    spi_transfer(crc);

    for (i = 0; i < 10; i++) {
        r = spi_transfer(0xff);
        if ((r & 0x80) == 0) {
            return r;
        }
    }

    return 0xff;
}

uint8_t sd_init(void)
{
    uint8_t i = 0;
    uint8_t r = 0;
    uint8_t ocr[4] = { 0 };
    uint16_t timeout = 0;

    SD_CS_HIGH();
    SPI_SCK = 0;
    SPI_MOSI = 1;

    for (i = 0; i < 10; i++) {
        spi_transfer(0xff);
    }

    SD_CS_LOW();
    r = sd_command(0, 0, 0x95);
    sd_deselect();

    if (r != 1) {
        return SD_ERROR;
    }

    if (sd_select() != SD_OK) {
        return SD_ERROR;
    }
    r = sd_command(8, 0x000001AAUL, 0x87);

    if (r == 0x01) {
        for (i = 0; i < 4; i++) {
            ocr[i] = spi_transfer(0xff);
        }
        sd_deselect();

        if (ocr[2] != 0x01 || ocr[3] != 0xaa) {
            return SD_ERROR;
        }

        for (timeout = 0; timeout < 60000; timeout++) {
            if (sd_select() != SD_OK) {
                return SD_ERROR;
            }
            sd_command(55, 0, 0x01);
            sd_deselect();

            if (sd_select() != SD_OK) {
                return SD_ERROR;
            }

            r = sd_command(41, 0x40000000UL, 0x01);
            sd_deselect();

            if (r == 0) {
                break;
            }
        }

        if (r != 0) {
            return SD_TIMEOUT;
        }

        if (sd_select() != SD_OK) {
            return SD_ERROR;
        }
        r = sd_command(58, 0, 0x01);

        if (r != 0) {
            sd_deselect();
            return SD_ERROR;
        }

        for (i = 0; i < 4; i++) {
            ocr[i] = spi_transfer(0xff);
        }
        sd_deselect();
        sd_sdhc = (ocr[0] & 0x40) ? 1 : 0;

        return SD_OK;
    }
    sd_deselect();

    return SD_ERROR;
}

uint8_t sd_read_sector(uint32_t sector_number)
{
    uint16_t i = 0;
    uint8_t r = 0;
    uint16_t timeout = 0;
    uint32_t address = 0;

    address = sd_sdhc ? sector_number : sector_number * 512UL;

    if (sd_select() != SD_OK) {
        return SD_TIMEOUT;
    }
    r = sd_command(17, address, 0x01);

    if (r != 0x00) {
        sd_deselect();
        return SD_ERROR;
    }

    for (timeout = 0; timeout < 60000; timeout++) {
        r = spi_transfer(0xff);
        if (r == 0xfe) {
            break;
        }
        if (r != 0xff) {
            sd_deselect();
            return SD_ERROR;
        }
    }

    if (timeout == 60000) {
        sd_deselect();
        return SD_TIMEOUT;
    }

    for (i = 0; i < 512; i++) {
         eeprom_write_byte(i, spi_transfer(0xff));
    }

    spi_transfer(0xff);
    spi_transfer(0xff);
    sd_deselect();

    return SD_OK;
}

static void panic(const char *buf)
{
    while (1) {
        LED = 0;
        delay_ms(150);
        LED = 1;
        delay_ms(150);
        uart_send_string(buf);
        delay_ms(1000);
    }
}

void main(void)
{
    unsigned int i = 0;
    unsigned int j = 0;
    unsigned int pos = 0;

    LED = 1;
    SD_CS = 1;
    RAM_CS = 1;
    LCD_CS = 1;
    SPI_SCK = 0;
    SPI_MOSI = 0;
    SPI_MISO = 1;

    delay_ms(2);
    ram_reset();
    delay_ms(1);

    if (sd_init() != SD_OK) {
        panic("failed to initialize sdcard !\r\n");
    }

    if (sd_read_sector(0) != SD_OK) {
        panic("failed to read sector !\r\n");
    }

    uart_send_byte('\r');
    uart_send_byte('\n');
    uart_send_byte('\r');
    uart_send_byte('\n');
    for (i = 0; i < 32; ++i) {
        for (j = 0; j < 16; ++j) {
            uart_send_hex(eeprom_read_byte(pos++));
            uart_send_byte(' ');
        }
        uart_send_byte('\r');
        uart_send_byte('\n');
    }
    uart_send_byte('\r');
    uart_send_byte('\n');
    uart_send_byte('\r');
    uart_send_byte('\n');

    while (1) {
        LED = 0;
        delay_ms(3000);
        LED = 1;
        delay_ms(3000);
    }
}
