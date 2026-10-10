all:
	sdcc -c main.c
	sdas8051 -plosgff startup.asm
	sdcc startup.rel main.rel -o main.ihx
	packihx main.ihx > main.hex

flash:
	timeout 10 stcgal -P stc15 -t 35000 main.hex

clean:
	rm -rf main.ihx main.lst main.mem main.rst main.lk main.map main.rel main.sym main.hex main.asm startup.lst  startup.rel  startup.rst  startup.sym
