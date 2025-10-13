TARGET = gbwm
CC ?= cc
HGVERSION = $(shell hg log -r . --template "{latesttag}-{latesttagdistance}-{node|short}" 2>/dev/null)
VERSION = $(if $(HGVERSION),$(HGVERSION),dev)
CFLAGS ?= -O3 -std=c99 -Wall -DVERSION=\"$(VERSION)\"
PREFIX ?= /usr/local

$(TARGET):
	$(CC) $(CFLAGS) gbwm.c -o $@ -lX11 -lXft -I/usr/include/freetype2/ -lXtst

config.h: default.config.h
	cp default.config.h config.h

.PHONY: install uninstall clean

install: $(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(TARGET)

clean:
	rm -f $(TARGET)
