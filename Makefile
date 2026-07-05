.POSIX:

CC      = gcc
CFLAGS  = -march=native -O3 -flto -pipe -fPIE
LDFLAGS = -pie -Wl,-z,relro,-z,now,-z,noexecstack,--gc-sections,--hash-style=gnu

PREFIX ?= /usr/local
BINDIR  = $(PREFIX)/bin
TARGET  = zeptofetch

STD  = -std=c99 -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE
WARN = -Wall -Wextra -Wpedantic -Werror=format=2 -Werror=implicit-fallthrough \
       -Werror=shift-overflow -Werror=vla -Werror=pointer-arith
SEC  = -D_FORTIFY_SOURCE=2 -fstack-protector-strong -fstack-clash-protection \
       -fcf-protection=full -fno-strict-overflow -fno-strict-aliasing

export TZ=UTC

.PHONY: all debug clean install uninstall

all: $(TARGET)

$(TARGET): zeptofetch.c config.h
	$(CC) $(STD) $(CFLAGS) $(WARN) $(SEC) -ffunction-sections -fdata-sections $(LDFLAGS) -o $@ zeptofetch.c
	strip --strip-all --remove-section=.note --remove-section=.comment $@

debug: CFLAGS = -O0 -g3 -fsanitize=address,undefined -fno-omit-frame-pointer
debug: LDFLAGS =
debug: clean
	$(CC) $(STD) $(CFLAGS) $(WARN) -o $(TARGET) zeptofetch.c

clean:
	rm -f $(TARGET)

install: $(TARGET)
	mkdir -p $(DESTDIR)$(BINDIR)
	install -m755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)
