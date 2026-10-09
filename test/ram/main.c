#include <mcs51/8051.h>

#define LED         P3_1
#define UART_TX     P3_1
#define SD_CS       P3_1
#define RAM_CS      P3_4
#define LCD_CS      P3_0
#define SPI_SCK     P3_5
#define SPI_MOSI    P3_2
#define SPI_MISO    P3_3

#define RAM_CMD_READ        0x03
#define RAM_CMD_WRITE       0x02
#define RAM_CMD_RESET_EN    0x66
#define RAM_CMD_RESET       0x99

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

static void uart_send_string(const char *buf)
{
    const char *p = buf;

    while (p && *p) {
        uart_send_byte(*p++);
    }
}

void main(void)
{
    unsigned char write_value = 0;
    unsigned char read_value = 0;
    unsigned char pass = 0;
    unsigned long i = 0;

    RAM_CS = 1;
    SPI_SCK = 0;
    SPI_MOSI = 0;
    SPI_MISO = 1;
    LED = 1;

    delay_ms(2);
    ram_reset();
    delay_ms(1);

    pass = 1;
    for (i = 0; i < 65536; i++) {
        write_value = (unsigned char)(0xA5 ^ i);
        ram_write_byte((unsigned long)i, write_value);
        read_value = ram_read_byte((unsigned long)i);
        if (read_value != write_value) {
            pass = 0;
            break;
        }
    }

    while (1) {
        if (pass) {
            uart_send_string("PASS !\r\n");
            LED = 0;
            delay_ms(100);
            LED = 1;
            delay_ms(900);
        } else {
            uart_send_string("ERROR !\r\n");
            for (i = 0; i < 3; i++) {
                LED = 0;
                delay_ms(150);
                LED = 1;
                delay_ms(150);
            }
            delay_ms(600);
        }
    }
}
