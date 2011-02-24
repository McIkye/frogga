/*
 * Copyright (c) 2011 Michael Shalayeff
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <avr/io.h>
#include <avr/pgmspace.h>

#include "ds1307.h"
#include "navi.h"
#include "gui.h"

char itoa(char *, char);

const char hexd[] PROGMEM = "0123456789abcdef";

void
gui_init()
{
	ks_lcd(100);
#ifdef DEBUG1
PORTB &= ~_BV(PORTB5);
msdelay(200);
PORTB |= _BV(PORTB5);
msdelay(500);
PORTB ^= _BV(PORTB5);
msdelay(200);
PORTB |= _BV(PORTB5);
msdelay(200);
PORTB ^= _BV(PORTB5);
msdelay(200);
	ks_putchar(8, 6, '0', 0);
#endif
}

const char darrs[8][8] PROGMEM = {
	{ 0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01 },
	{ 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff },
	{ 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff },
	{ 0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80 },
	{ 0x18, 0x3c, 0x3c, 0x7e, 0x7e, 0xff, 0x18, 0x18 },
	{ 0x20, 0x30, 0x3c, 0xff, 0xff, 0x3c, 0x30, 0x20 },
};

void
gui_direct(char row, char col)
{

	/* clean the old target */
	ks_pgbitmap(row - 16, col + 00, darrs[5], 8, 0);

	/* print the new one */
	ks_pgbitmap(row - 16, col + 00, darrs[0], 8, 0);
	ks_pgbitmap(row - 16, col + 16, darrs[1], 8, 0);

	ks_pgbitmap(row - 00, col + 16, darrs[2], 8, 0);
	ks_pgbitmap(row - 00, col + 00, darrs[3], 8, 0);
}

const char compach[] PROGMEM =
    "\x95\x95\x95\x95"
    "N\x95\x95\x95\x95"
    "E\x95\x95\x95\x95"
    "S\x95\x95\x95\x95"
    "W\x95\x95\x95\x95"
    "N\x95\x95\x95\x95"
    "E\x95\x95\x95\x95";
void
gui_compass(char row, char col, char cmp)
{
	char c[2], ec;

#ifdef DEBUG
ks_puthexb(row, col - 12, cmp, 0);
#endif
	c[1] = 0;
	for (ec = col + 30; col < ec; cmp++) {
		c[0] = pgm_read_byte(&compach[(int)cmp]);
		ks_putchar(row, col, c[0], 0);
		col += ks_length(c);
	}
}

void
gui_hbeat(char row, char col)
{
	register char i;
	char hbmap[27];

	ks_putchar(row, col, 0177, hbeat & 2);
	col += 10;

#define	HBMASK0	0x2a
#define	HBMASK1	0x14
#define	HBEMPTY	0x41
	hbmap[26] = 0x7f;
	for (i = 1; i < pecad; i++)
		hbmap[26 - i] = HBEMPTY | (i & 1? HBMASK1 : HBMASK0);
	for (; i < 25; i++)
		hbmap[26 - i] = HBEMPTY;
	hbmap[1] = 0x7f;
	hbmap[0] = 0x1c;
	ks_bitmap(row, col, hbmap, sizeof hbmap, 0);
	row += 8;

	if (pecad) {
		char buf[8], *p = buf;

		p += itoa(buf, hbeat);
		*p++ = '/';
		itoa(p, pecad);
		ks_putstr(row, col, buf, 0);
	}
}

void
gui_times(char row, char col)
{
	char i, l, *p, buf[sizeof("     13'13'13  13:13     ")];
#define	TSLENGTH	(5*12 + 5*3)

	p = &buf[5];
	*p++ = '0' + times.dd10;
	*p++ = '0' + times.date;
	*p++ = '\'';
	if (times.mm10)
		*p++ = '1';
	*p++ = '0' + times.mon;
	*p++ = '\'';
	*p++ = '0' + times.yy10;
	*p++ = '0' + times.year;
	*p++ = ' ';
	*p++ = ' ';

	/* hours are always kept in 24h format */
	*p++ = '0' + times.hh10;
	*p++ = '0' + times.hour;
	*p++ = ':';
	*p++ = '0' + times.min10;
	*p++ = '0' + times.min;
	*p = '\0';

	/* try to center the timestamp by pre- and post-fixing the string */
	l = ks_length(buf + 5);
	for (i = 5; l < TSLENGTH; l += 6)
		buf[(int)--i] = *p++ = ' ';
	*p = '\0';

	ks_putstr(row, col, buf + i, 0);
}

void
gui_power(char row, char col, char pw)
{
	register char i;
	char pwbmap[12];

#define	PWMASK0	0x14
#define	PWMASK1	0x2a
#define	PWEMPTY	0x41
	pwbmap[0] = 0x7f;
	for (i = 1; i < pw; i++)
		pwbmap[(int)i] = PWEMPTY | (i & 1? PWMASK1 : PWMASK0);
	for (; i < 10; i++)
		pwbmap[(int)i] = PWEMPTY;
	pwbmap[10] = 0x7f;
	pwbmap[11] = 0x1c;
	ks_bitmap(row, col, pwbmap, sizeof pwbmap, 0);
}

void
gui_speed(char row, char col)
{
	short v = velocity;

	ks_put0(row, col, v / 100, 0);
	v = (v / 10) % 100;
	ks_put0(row, col, velocity / 100, 0);
}

void
gui_dist(char row, char col)
{

}

void
gui_ticker(char row, char col)
{

	ks_putstr(row, col + 24,
	    "\x95\x95\x95\x95\x95  frogga teh travla  \x95\x95\x95\x95\x95", 0);
}

void
gui_update(short f)
{
/*
	if (f == FLDCLRSCR)
		ks_clear();
*/

#define	HBROW	0
#define	HBCOL	0
		if ((f & FLDHEART) && hbeat)
			gui_hbeat(HBROW, HBCOL);

#define	TSROW	0
#define	TSCOL	39
		if (f & FLDTIMES)
			gui_times(TSROW, TSCOL);

#define	PWROW	0
#define	PWCOL	116
		if (f & FLDPOWER)
			gui_power(PWROW, PWCOL, power);

#define	TKROW	56
#define	TKCOL	0
		if (f & FLDID3)
			gui_ticker(TKROW, TKCOL);

	switch (mode) {
	case MODROLL:
#define	SPROW	32
#define	SPCOL	115
		if (f & FLDSPEED)
			gui_speed(SPROW, SPCOL);

#define	DSROW	48
#define	DSCOL	98
		if (f & FLDDISTANCE)
			gui_dist(DSROW, DSCOL);

#define	DDROW	32
#define	DDCOL	98
		if (f & FLDDIRECTS)
			gui_direct(DDROW, DDCOL);

#define	CPROW	48
#define	CPCOL	98
		if (f & FLDCOMPASS)
			gui_compass(CPROW, CPCOL, compass / 180);
		break;
	case MODNARF:
		break;
	case MODWPT:
		break;
	case MODMUS:
		break;
	}
}

char
itoa(char *buf, char c)
{
	char *p = buf;
	char v;

	if ((v = c / 100)) {
		c %= 100;
		*p++ = '0' + v;
	}

	if ((v = c / 10)) {
		c %= 10;
		*p++ = '0' + v;
	}

	if (c)
		*p++ = '0' + c;

	*p = '\0';
	return p - buf;
}
