CC  := $(shell command -v gcc 2>/dev/null || command -v clang 2>/dev/null)
ifeq ($(CC),)
$(error install gcc or clang)
endif

BASE  = -std=c99 -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE
HARD  = -D_FORTIFY_SOURCE=2 -fstack-protector-strong
SAFE  = -fno-strict-overflow -fno-strict-aliasing -fno-delete-null-pointer-checks
WARN  = -Wall -Wextra -Wpedantic -Werror=format=2 -Werror=implicit-fallthrough \
        -Werror=shift-overflow=2 -Werror=cast-function-type -Werror=stringop-overflow=4 \
        -Werror=vla -Werror=pointer-arith
LINK  = -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack

PREFIX  ?= /usr/local
DESTDIR ?=
TARGET   = zeptofetch
SRC      = zeptofetch.c
DEPS     = config.h
export TZ = UTC

.PHONY: all release debug clean install uninstall

all: CFLAGS  = $(BASE) $(HARD) $(SAFE) $(WARN) -march=native -O3 -flto -pipe
all: LDFLAGS = $(LINK) -Wl,--gc-sections -Wl,--hash-style=gnu
all: $(TARGET)

release: CFLAGS  = $(BASE) $(HARD) $(SAFE) $(WARN) -march=x86-64-v3 -mtune=generic -O3 -flto -pipe
release: LDFLAGS = -static $(LINK) -Wl,--gc-sections -Wl,--hash-style=gnu
release: clean
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)
	strip --strip-all --remove-section=.note --remove-section=.comment $(TARGET)

debug: CFLAGS  = $(BASE) -O0 -g -fno-omit-frame-pointer $(WARN)
debug: LDFLAGS = $(LINK)
debug: clean
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

$(TARGET): $(SRC) $(DEPS)
	$(CC) $(CFLAGS) -ffunction-sections -fdata-sections -o $@ $(SRC) $(LDFLAGS)
	strip --strip-all --remove-section=.note --remove-section=.comment $@

clean:
	rm -f $(TARGET)

install: $(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(TARGET)
