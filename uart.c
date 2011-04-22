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

#include <stdint.h>
#include <string.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include "gui.h"
#include "navi.h"
#include "ds1307.h"

int pos2ffnum(char *, char **);
int str2ffnum(char *, char **);

signed char gpst;

extern const char hexd[] PROGMEM;

/*
 * uart: char received
 */
ISR(USART_RX_vect)	/* vector 18 */
{
	static char tt[10], dd[10], bbb[10], *bp, cksum;
	static int nlongit, nlatit, nspeed, ncourse;
	int *var;
	char c, *buf = &bbb[1];

	if (!(UCSR0A & _BV(RXC0)))
		return;

	c = UDR0;
	if (gpst >= 0) {
		cksum ^= c;
		/* even state only read into the buffer */
		if (!(gpst & 1)) {
			if (c != ',' && c != '\r') {
				if (bp - buf >= 10)
					goto fail;
				*bp++ = c;
				return;
			} else {
				gpst++;
				*bp = '\0';
			}
		}
	}

	switch (gpst) {
	case -1:	/* need a dollar! */
		if (c == '$') {
			TCNT2 = 0;	/* reset UI clock */
			cksum = 0;
			/* start other data acquisition */
			if (state < 0)
				state++;
			goto next;
		}
		break;
	case 1:		/* check if it's our msg type */
/* $GPRMC,161229.487,A,3723.2475,N,12158.3416,W,0.13,309.62,120598,E,*10 */
/* $GPRMC,hhmmss.sss,A,ddmm.mmmm,N,dddmm.mmmm,W,n.nn,ddd.dd,ddmmyy,A,*cs */
		if (buf[0] == 'G' && buf[1] == 'P' &&
		    buf[2] == 'R' && buf[3] == 'M' && buf[4] == 'C')
			goto next;
		goto fail;
	case 3:		/* UTC time */
	PORTB ^= _BV(PORTB5);
		if (bp - buf < 10)
			goto fail;
		/* only copy now; if data is valid update times */
		memcpy(tt, buf, sizeof tt);
		goto next;
	case 5:		/* data valid */
		if (buf[0] == 'A')
			goto next;
		else
			goto fail;
	case 7:		/* latitude */
		var = &nlatit;
		buf = bbb;
	cooin:
		*var = pos2ffnum(buf, &bp);
		if (*bp != '\0')
			goto fail;
		else
			goto next;
	case 9:		/* N/S */
		if (buf[0] == 'N')
			nlatit = -nlatit;
		else if (buf[0] != 'S')
			goto fail;
		goto next;
	case 11:	/* longitude */
		var = &nlongit;
		goto cooin;
	case 13:	/* E/W */
		if (buf[0] == 'W')
			nlongit = -nlongit;
		else if (buf[0] != 'E')
			goto fail;
		goto next;
	case 15:	/* speed */
		var = &nspeed;
	numin:
		*var = str2ffnum(buf, &bp);
		if (*bp != '\0')
			goto fail;
		goto next;
	case 17:	/* course */
		var = &ncourse;
		goto numin;
	case 19:	/* date */
		if (bp - buf < 6)
			goto fail;
		memcpy(dd, buf, sizeof dd);
		goto next;
	case 23:	/* checksum */
		if (bp - buf < 3 || buf[0] != '*')
			goto fail;
		/* compensate for the buffer */
		c ^= buf[0];
		c ^= buf[1];
		c ^= buf[2];
		c ^= '\r';
#define	tolower(c)	((c) + 0x20)
		buf[4] = pgm_read_byte(&hexd[(int)c & 0xf]);
		buf[3] = pgm_read_byte(&hexd[(int)c >> 4]);
		if (buf[4] != buf[2] || buf[3] != buf[1])
			goto fail;
		else {
			/* skip buffering */
			gpst++;
			goto next;
		}
		break;
	case 25:
		if (c != '\n')
			goto fail;
		longit = nlongit;
		latit = nlatit;
		speed = nspeed;
		course = ncourse;
		/* update times (hhmmss.sss) */
		if (tt[3] != '0' + times.min ||
		    tt[2] != '0' + times.min10)
			changed |= FLDTIMES;
		/* update RTC every hour */
		if (tt[1] != '0' + times.hour ||
		    tt[0] != '0' + times.hh10)
			changed |= FLDRTC;
		times.sec = tt[8] - '0';
		times.sec10 = tt[7] - '0';
		times.min = tt[3] - '0';
		times.min10 = tt[2] - '0';
		times.hour = tt[1] - '0';
		times.hh10 = tt[0] - '0';
		times.h1224 = 1;
		/* ... and dates (ddmmyy) */
		times.date = dd[1] - '0';
		times.dd10 = dd[0] - '0';
		times.mon = dd[3] - '0';
		times.mm10 = dd[2] - '0';
		times.year = dd[5] - '0';
		times.yy10 = dd[4] - '0';
		PORTB &= ~_BV(PORTB5);
		/* FALLTHROUGH */
	default:
		if (0) {
	fail:
			/* DEBUG FAIL */
			PORTB &= ~_BV(PORTB5);
		}
		state++;
		gpst = -1;
		return;
	case 21:	/* magnetic variation(EW)/mode(ADEN) */
	next:
		bbb[0] = '0';
		bp = buf;
		gpst++;
	}
}

#if 0
/*
 * uart: data register empty
 */
ISR(USART_UDRE_vect)	/* vector 19 */
{
}

/*
 * uart: data xmit complete
 */
ISR(USART_TXC_vect)	/* vector 20 */
{
}
#endif

int
pos2ffnum(char *buf, char **bp)
{
	char *p, rif;
	int rv;

	for (rif = 0, rv = 0, p = buf; *p != '\0'; p++) {
		if (*p == '.') {
			rif = 4;
			continue;
		}
		if (*p < '0' || '9' < *p) {
			*bp = p;
			return 0;
		}
		rv *= p - buf == 3? 6 : 10;
		rv += *p - '0';
		if (rif)
			rif--;
	}

	while (rif-- > 0)
		rv *= 10;

	*bp = p;
	return rv;
}

int
str2ffnum(char *buf, char **bp)
{
	char *p, rif;
	int rv;

	for (rif = 0, rv = 0, p = buf; *p != '\0'; p++) {
		if (*p == '.') {
			rif = 3;
			continue;
		}
		if (*p < '0' || '9' < *p) {
			*bp = p;
			return 0;
		}
		rv *= 10;
		rv += *p - '0';
		if (rif)
			rif--;
	}

	while (rif-- > 0)
		rv *= 10;

	*bp = p;
	return rv;
}
