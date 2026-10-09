all:
	mcu8051ide --compile main.s

flash:
	timeout 10 stcgal -P stc15 -t 35000 main.hex

clean:
	rm -rf main.hex main.adf  main.bin  main.lst main.adf~  main.bin~  main.hex~  main.lst~
