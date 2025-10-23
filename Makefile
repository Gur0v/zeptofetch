CC = gcc
CFLAGS = -std=c99 -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE \
         -O3 -flto=auto -pipe -Wall -Wextra -Wpedantic
LDFLAGS = -Wl,--gc-sections -Wl,-z,relro -Wl,-z,now -Wl,--hash-style=gnu
PREFIX = /usr/local
DESTDIR =
TARGET = zeptofetch
SRC = zeptofetch.c
DEPS = config.h

.PHONY: all clean install uninstall release debug

all: $(TARGET)

$(TARGET): $(SRC) $(DEPS)
	$(CC) $(CFLAGS) -ffunction-sections -fdata-sections -o $@ $(SRC) $(LDFLAGS)
	strip --strip-all --remove-section=.note --remove-section=.comment $@

release: CFLAGS = -std=c99 -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE \
                  -march=x86-64-v3 -mtune=generic -O3 -flto=auto -pipe \
                  -Wall -Wextra -ffunction-sections -fdata-sections
release: LDFLAGS = -static -Wl,--gc-sections -Wl,--hash-style=gnu
release: clean
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)
	strip --strip-all --remove-section=.note --remove-section=.comment $(TARGET)

clean:
	rm -f $(TARGET)

install: $(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(TARGET)

debug: CFLAGS = -std=c99 -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE \
                -O0 -g -Wall -Wextra -Wpedantic -fsanitize=address,undefined
debug: LDFLAGS = -fsanitize=address,undefined
debug: clean
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)
