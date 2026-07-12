.POSIX:

CC      = cc
STRIP   = strip
CFLAGS  = -O3 -flto -pipe -fPIE $(NATIVE)
LDFLAGS = -pie -Wl,-z,relro,-z,now,-z,noexecstack,--gc-sections,--hash-style=gnu

PREFIX = /usr/local
BINDIR  = $(PREFIX)/bin
TARGET  = zeptofetch
DEBUG_TARGET = zeptofetch-debug

STD  = -std=c99 -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE
WARN = -Wall -Wextra -Wpedantic -Werror=format=2 -Werror=implicit-fallthrough \
       -Werror=shift-overflow -Werror=vla -Werror=pointer-arith
SEC  = -D_FORTIFY_SOURCE=3 -fstack-protector-strong -fstack-clash-protection \
       -fcf-protection=full -fno-strict-overflow -fno-strict-aliasing

export TZ=UTC

.PHONY: all debug check clean install uninstall

all: $(TARGET)

$(TARGET): zeptofetch.c Makefile
	$(CC) $(STD) $(CFLAGS) $(WARN) $(SEC) -ffunction-sections -fdata-sections $(LDFLAGS) -o $@ zeptofetch.c
	$(STRIP) --strip-all --remove-section=.note --remove-section=.comment $@

debug: $(DEBUG_TARGET)

$(DEBUG_TARGET): zeptofetch.c Makefile
	$(CC) $(STD) -O0 -g3 -fsanitize=address,undefined -fno-omit-frame-pointer $(WARN) -o $@ zeptofetch.c

check: $(TARGET)
	./$(TARGET) --version >/dev/null
	! ./$(TARGET) --invalid >/dev/null 2>&1
	./$(TARGET) >/dev/null

clean:
	rm -f $(TARGET) $(DEBUG_TARGET)

install: $(TARGET)
	mkdir -p $(DESTDIR)$(BINDIR)
	install -m755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)
