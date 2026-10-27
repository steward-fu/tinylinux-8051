APP	= uARM
CC	= gcc
LD	= gcc

BUILD ?= debug

ifeq ($(BUILD), avr)

	CC_FLAGS        = -O2 -mmcu=atmega1284p -I/usr/lib/avr/include -DEMBEDDED -D_SIM -ffunction-sections -DAVR_ASM
	LD_FLAGS        = -O2 -mmcu=atmega1284p -Wl,--gc-sections
	CC              = avr-gcc
	LD              = avr-gcc
	EXTRA           = avr-size -Ax $(APP) && avr-objcopy -j .text -j .data -O ihex $(APP) $(APP).hex
	EXTRA_OBJS      = sd.o main_avr.o avr_asm.o
endif

ifeq ($(BUILD), debug)
	CC_FLAGS	= -O0 -g -ggdb -ggdb3 -D_FILE_OFFSET_BITS=64 -D__USE_LARGEFILE64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -DLCD_SUPPORT
	LD_FLAGS	= -O0 -g -ggdb -ggdb3 -lSDL
	EXTRA_OBJS	= main_pc.o
endif

ifeq ($(BUILD), profile)
	CC_FLAGS	= -O3 -g -pg -fno-omit-frame-pointer -march=core2 -mpreferred-stack-boundary=4  -D_FILE_OFFSET_BITS=64 -D__USE_LARGEFILE64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE
	LD_FLAGS	= -O3 -g -pg -lSDL
	EXTRA_OBJS	= main_pc.o
endif

ifeq ($(BUILD), opt)
	CC_FLAGS	= -O3 -fomit-frame-pointer -march=core2 -mpreferred-stack-boundary=4 -momit-leaf-frame-pointer -D_FILE_OFFSET_BITS=64 -D__USE_LARGEFILE64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -flto
	LD_FLAGS	= $(CC_FLAGS) -flto -O3 -lSDL 
	EXTRA_OBJS	= main_pc.o
endif

ifeq ($(BUILD), opt64)
	CC_FLAGS	= -m64 -O3 -fomit-frame-pointer -march=core2 -momit-leaf-frame-pointer -D_FILE_OFFSET_BITS=64 -D__USE_LARGEFILE64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE
	LD_FLAGS	= -O3 -lSDL
	EXTRA_OBJS	= main_pc.o
endif

LDFLAGS = $(LD_FLAGS) -Wall -Wextra
CCFLAGS = $(CC_FLAGS) -Wall -Wextra

OBJS	= $(EXTRA_OBJS) rt.o math64.o cpu.o mmu.o cp15.o mem.o ram.o callout_ram.o soc.o pxa255_ic.o icache.o pxa255_timr.o pxa255_rtc.o pxa255_uart.o pxa255_pwrclk.o pxa255_gpio.o pxa255_dma.o pxa255_dsp.o pxa255_lcd.o

$(APP): $(OBJS)
	$(LD) -o $(APP) $(OBJS) $(LDFLAGS)
	$(EXTRA)

AVR:   $(APP)
	sudo avrdude -V -p ATmega1284p -c avrisp2 -P usb -U flash:w:$(APP).hex:i

math64.o: math64.c math64.h types.h
	$(CC) $(CCFLAGS) -o math64.o -c math64.c

cpu.o: cpu.c cpu.h types.h math64.h icache.h
	$(CC) $(CCFLAGS) -o cpu.o -c cpu.c

icache.o: icache.c icache.h types.h cpu.h
	$(CC) $(CCFLAGS) -o icache.o -c icache.c

dcache.o: dcache.c dcache.h types.h cpu.h
	$(CC) $(CCFLAGS) -o dcache.o -c dcache.c

mmu.o: mmu.c mmu.h types.h
	$(CC) $(CCFLAGS) -o mmu.o -c mmu.c

cp15.o: cp15.c cp15.h cpu.h types.h
	$(CC) $(CCFLAGS) -o cp15.o -c cp15.c

mem.o: mem.c mem.h types.h
	$(CC) $(CCFLAGS) -o mem.o -c mem.c

avr_asm.o: avr_asm.S
	$(CC) $(CCFLAGS) -o avr_asm.o -c avr_asm.S

ram.o: ram.c ram.h mem.h types.h
	$(CC) $(CCFLAGS) -o ram.o -c ram.c

callout_ram.o: callout_ram.c callout_ram.h mem.h types.h
	$(CC) $(CCFLAGS) -o callout_ram.o -c callout_ram.c

sd.o: sd.c sd.h types.h
	$(CC) $(CCFLAGS) -o sd.o -c sd.c

soc.o: soc.c soc.h ram.h mem.h cpu.h mmu.h pxa255_ic.h math64.h icache.h
	$(CC) $(CCFLAGS) -o soc.o -c soc.c

pxa255_ic.o: pxa255_ic.c pxa255_ic.h mem.h cpu.h
	$(CC) $(CCFLAGS) -o pxa255_ic.o -c pxa255_ic.c

pxa255_timr.o: pxa255_timr.c pxa255_timr.h pxa255_ic.h mem.h cpu.h
	$(CC) $(CCFLAGS) -o pxa255_timr.o -c pxa255_timr.c
	
pxa255_rtc.o: pxa255_rtc.c pxa255_rtc.h pxa255_ic.h mem.h cpu.h
	$(CC) $(CCFLAGS) -o pxa255_rtc.o -c pxa255_rtc.c

pxa255_lcd.o: pxa255_lcd.c pxa255_lcd.h pxa255_ic.h mem.h cpu.h
	$(CC) $(CCFLAGS) -o pxa255_lcd.o -c pxa255_lcd.c

pxa255_uart.o: pxa255_uart.c pxa255_uart.h pxa255_ic.h mem.h cpu.h
	$(CC) $(CCFLAGS) -o pxa255_uart.o -c pxa255_uart.c

pxa255_pwrclk.o: pxa255_pwrclk.c pxa255_pwrclk.h cpu.h mem.h
	$(CC) $(CCFLAGS) -o pxa255_pwrclk.o -c pxa255_pwrclk.c

pxa255_gpio.o: pxa255_gpio.c pxa255_gpio.h cpu.h mem.h
	$(CC) $(CCFLAGS) -o pxa255_gpio.o -c pxa255_gpio.c

pxa255_dma.o: pxa255_dma.c pxa255_dma.h cpu.h mem.h
	$(CC) $(CCFLAGS) -o pxa255_dma.o -c pxa255_dma.c

pxa255_dsp.o: pxa255_dsp.c pxa255_dsp.h cpu.h math64.h
	$(CC) $(CCFLAGS) -o pxa255_dsp.o -c pxa255_dsp.c

main_pc.o: soc.h main_pc.c types.h
	$(CC) $(CCFLAGS) -o main_pc.o -c main_pc.c

main_avr.o: soc.h main_avr.c types.h
	$(CC) $(CCFLAGS) -o main_avr.o -c main_avr.c

rt.o: rt.c types.h
	$(CC) $(CCFLAGS) -o rt.o -c rt.c

clean:
	rm -f $(APP) *.o


