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
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include "i2c.h"
#include "pca9555.h"

#define	CE	0x01
#define	CRW	0x02
#define	CRS	0x04
#define	CCS0	0x08
#define	CCS1	0x10
#define	CRES	0x20
#define	CSW4	0x80

#define	KSON(on)	(0x3e | (on))
#define	SETX(x)		(0xb8 | (x))
#define	SETY(y)		(0x40 | (y))
#define	SETZ(z)		(0xc0 | (z))

#define	ks_iwr(b,cw,v)	\
	((b)[0] = CRES | (cw), (b)[1] = (v), (b)[2] = (b)[0] | CE, \
	 (b)[3] = (v), (b)[4] = (b)[0], (b)[5] = (v))
#define	KS_IWR	6

void
ks_write(char cw, char v)
{
	char buf[6];

	ks_iwr(buf, cw, v);
	i2c_write(PCA9555, 3, buf, KS_IWR - 1);	/* do not need the d-pad */
}

#ifndef DEBUG
const PROGMEM
#include "splash.xbm"
#endif

void
ks_clear(void)
{
	register char x, i, cw;

	for (x = 8; x--; ) {
		cw = CCS0 | CCS1;
		ks_write(cw, SETX(x & 7));
		ks_write(cw, SETY(0));
		cw |= CRS;
		for (i = 64; i--; ks_write(cw, 0))
			;
	}
}

void
ks_lcd(char bright)
{
	char x;

	pca9555_init();
	x = CRES;
	i2c_write(PCA9555, 3, &x, 1);

	if (bright) {
		/* clear/splash screen */
		ks_write(CCS0 | CCS1, SETZ(0));
#ifndef DEBUG
		for (x = 8; x--; ) {
			register char i, cw;

			cw = CCS0 | CCS1;
			ks_write(cw, SETX(x & 7));
			ks_write(cw, SETY(0));
			cw = CRS | CCS0;
			for (i = 64; i--; )
				ks_write(cw, pgm_read_byte(
				    &splash_bits[(i + 64) * 8 + x]));
			cw = CRS | CCS1;
			for (i = 64; i--; )
				ks_write(cw,
				    pgm_read_byte(&splash_bits[i * 8 + x]));
		}
#else
		ks_clear();
#endif
	}

/* TODO set BLA pwm to brightness */

	if (bright) {
		OCR0A = bright;
		ks_write(CCS0 | CCS1, KSON(bright? 1 : 0));
	} else {
		OCR0A = 0;
		ks_write(CCS0 | CCS1, KSON(0));
	}
}

char
ks_setxy(char x, char y)
{
	char cw;

	if (y >= 64)
		cw = CCS1;
	else
		cw = CCS0;

	ks_write(cw, SETX(x >> 3));
	ks_write(cw, SETY(y & 0x3f));

	return cw;
}

/*
 * cannot optimise since output length is variable any
 */
void
ks_bitmap(char x, char y, const char *p, char i, char attr)
{
	char cw, ey;

	cw = CRS | ks_setxy(x, y);
	for (ey = y + i; y < ey; y++, p++) {
		if (y == 64)
			cw = CRS | ks_setxy(x, y);
		switch (attr) {
		case 0: ks_write(cw,  *p); break;
		case 1: ks_write(cw, ~*p); break;
		case 2: ks_write(cw,   0); break;
/* TODO x0r */
		}
	}
}

/*
 * this is normally only used for fonts and thus optimise!
 */
void
ks_pgbitmap(char x, char y, const char *p, char i, char attr)
{
	char buf[12 * KS_IWR], *bp;	/* no char is wider than 12 */
	char c, cw, ey, cnt;

	cw = CRS | ks_setxy(x, y);
	for (bp = buf, cnt = 0, ey = y + i; y < ey; y++, p++, cnt += KS_IWR) {
		if (y == 64) {
			i2c_write(PCA9555, 3, buf, cnt);
			cw = CRS | ks_setxy(x, y);
			cnt = 0;
			bp = buf;
		}
		if (attr == 2)
			c = 0;
		else {
			c = pgm_read_byte(p);
			if (attr)
				c = ~c;
		}
		ks_iwr(bp, cw,  c);
		bp += KS_IWR;
	}

	if (cnt)
		i2c_write(PCA9555, 3, buf, cnt);
}

#include "font.h"

/*
 * do not support unaligned chars
 */
void
ks_putchar(char x, char y, char c, char attr)
{
	const char *p = (const char *)pgm_read_word(&fidx[(int)c]);

	c = pgm_read_byte(p);
	ks_pgbitmap(x, y, p + 1, c, attr);
}

char
ks_length(const char *s)
{
	char l;

	for (l = 0; *s; s++)
		l += pgm_read_byte(pgm_read_word(&fidx[(int)*s]));

	return l;
}

void
ks_putstr(char x, char y, const char *s, char attr)
{
	char c;

	for (; *s && y < 128; s++, y += c) {
		const char *p = (const char *)pgm_read_word(&fidx[(int)*s]);
		
		c = pgm_read_byte(p);
		ks_pgbitmap(x, y, p + 1, c, attr);
	}
}

extern const char hexd[] PROGMEM;

void
ks_put0(char x, char y, char v, char attr)
{
	ks_putchar(x, y, pgm_read_byte(&hexd[(int)v]), attr);
}

#ifdef DEBUG
void
ks_puthexb(char x, char y, char b, char attr)
{

	ks_putchar(x, y, pgm_read_byte(&hexd[b >> 4]), attr);
	ks_putchar(x, y + 6, pgm_read_byte(&hexd[b & 0xf]), attr);
}
#endif
