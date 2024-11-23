zeptofetch:
	gcc -march=native -Os -s -ffunction-sections -fdata-sections -fomit-frame-pointer -fno-exceptions -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-stack-protector -fno-PIE -no-pie -fno-stack-clash-protection -flto -Wl,--gc-sections,--strip-all -pipe -o $@ $@.c
clean:
	rm -f zeptofetch
install: zeptofetch
	install -Dm755 zeptofetch /usr/local/bin/zeptofetch
remove: 
	rm -f /usr/local/bin/zeptofetch
