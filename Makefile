CFLAGS = -std=c99 -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE -march=native -mtune=native -O3 -flto=auto -s \
         -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables \
         -fomit-frame-pointer -fno-math-errno -fmerge-all-constants \
         -ffunction-sections -fdata-sections -pipe -Wall -Wextra
LDFLAGS = -Wl,--gc-sections -Wl,-z,norelro -Wl,--hash-style=gnu
PREFIX = /usr
TARGET = zeptofetch
SRC = zeptofetch.c

.PHONY: all clean install remove strip debug

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC)

clean:
	rm -f $(TARGET)

install: $(TARGET)
	install -Dm755 $(TARGET) $(PREFIX)/bin/$(TARGET)

remove:
	rm -f $(PREFIX)/bin/$(TARGET)

strip: $(TARGET)
	strip --strip-all --remove-section=.note --remove-section=.comment $(TARGET)

debug: CFLAGS = -std=c99 -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE -O0 -g -Wall -Wextra -Wpedantic
debug: $(TARGET)
