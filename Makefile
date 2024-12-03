CC = cc
CFLAGS = -march=native -O3 -pipe
LDFLAGS = -flto
TARGET = zeptofetch
SRC = main.c system_info.c ui.c

.PHONY: all clean install remove

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC)

clean:
	rm -f $(TARGET)

install: $(TARGET)
	install -Dm755 $(TARGET) /usr/local/bin/$(TARGET)

remove:
	rm -f /usr/local/bin/$(TARGET)
