TARGET = gbwm
CC ?= cc
CFLAGS ?= -O2 -Wall
PREFIX ?= /usr/local

$(TARGET):
	$(CC) $(CFLAGS) gbwm.c -o $@ -lX11 -lXft -I/usr/include/freetype2/

.PHONY: install uninstall clean

install: $(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(TARGET)

clean:
	rm -f $(TARGET)