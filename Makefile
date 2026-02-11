# gbwm - grid-based window manager with multi-monitor support
# See LICENSE file for copyright and license details.

include config.mk

SRC = gbwm.c
OBJ = ${SRC:.c=.o}

all: options gbwm

options:
	@echo gbwm build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	${CC} -c ${CFLAGS} $<

${OBJ}: config.h config.mk

config.h:
	cp config.def.h $@

gbwm: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	rm -f gbwm ${OBJ}

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f gbwm ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/gbwm

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/gbwm

.PHONY: all options clean install uninstall