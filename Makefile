
AVRMCU=m328p
MCU=atmega328p
FREQ=16000000

PROG=	frogga
SRCS=	frogga.c gui.c navi.c
#	drivers
SRCS+=	ks0108.c i2c.c spi.c uart.c	# zigbee vs1053 sdcard
#	keep PROGMEM data as low as possible
SRCS+=	dosfs.c

CPPFLAGS+= -D__AVR_ATmega328P__ -I${.CURDIR} -I. -DDEBUG
CFLAGS += -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
LDFLAGS	= -Wl,-Map=$(PROG).map,--cref
#LDADD += -Wl,-u,vfscanf -lscanf_min
CLEANFILES+=font.h ${PROG}.map

${.CURDIR}/ks0108.c: font.h

font.h: font.src
	${SHELL} ${.CURDIR}/font.src > font.h

.include "../bsd.avr.mk"
