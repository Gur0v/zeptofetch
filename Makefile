CC = cc
CFLAGS = -march=native -O2 -s -fdelete-null-pointer-checks -fno-unwind-tables -fno-asynchronous-unwind-tables -fno-math-errno -pipe
TARGET = zeptofetch
SRC = zeptofetch.c

.PHONY: all clean install remove

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC)

clean:
	rm -f $(TARGET)

install: $(TARGET)
	install -Dm755 $(TARGET) /usr/local/bin/$(TARGET)

remove:
	rm -f /usr/local/bin/$(TARGET)
