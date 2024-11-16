zeptofetch: zeptofetch.c
	gcc -Os -s -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-stack-protector -Wl,--gc-sections -o zeptofetch zeptofetch.c
clean:
	rm -f zeptofetch
