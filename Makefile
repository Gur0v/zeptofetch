.POSIX:
CC		:= $(shell which gcc 2>/dev/null || which clang 2>/dev/null || echo gcc)
CFLAGS  = -march=native -O3 -flto -pipe
LDFLAGS = -Wl,-z,relro,-z,now,-z,noexecstack,--as-needed,--gc-sections,--hash-style=gnu

STD     = -std=c99 -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE
WARN    = -Wall -Wextra -Wpedantic -Werror=format=2 -Werror=implicit-fallthrough \
          -Werror=shift-overflow=2 -Werror=cast-function-type -Werror=stringop-overflow=4 \
          -Werror=vla -Werror=pointer-arith
SEC     = -D_FORTIFY_SOURCE=2 -fstack-protector-strong -fno-strict-overflow \
          -fno-strict-aliasing -fno-delete-null-pointer-checks
SIZE    = -ffunction-sections -fdata-sections

PREFIX  ?= /usr/local
BINDIR  = $(PREFIX)/bin
TARGET  = zeptofetch
SRC     = zeptofetch.c
DEPS    = config.h

export TZ=UTC

.PHONY: all debug clean install uninstall

all: $(TARGET)

$(TARGET): $(SRC) $(DEPS)
	$(CC) $(STD) $(CFLAGS) $(WARN) $(SEC) $(SIZE) $(LDFLAGS) -o $@ $<
	strip --strip-all --remove-section=.note --remove-section=.comment $@

debug: CFLAGS = -O0 -g3 -fsanitize=address,undefined -fno-omit-frame-pointer
debug: LDFLAGS =
debug: clean
	$(CC) $(STD) $(CFLAGS) $(WARN) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

install: $(TARGET)
	mkdir -p $(DESTDIR)$(BINDIR)
	install -m755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)
