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

#define	DBGADDR	PCA9555+1

#include <avr/io.h>
#include <avr/interrupt.h>

#include <util/twi.h>

#include "i2c.h"
#include "gui.h"
#include "errno.h"

#define	I2CTMO	200	/* 200us in 1us */
#define	I2CDLY	1

char i2c_wait(void);
char i2c_wraddr(char, char);

struct i2c_cmd *i2c_run;
signed char i2cst;

#ifdef DEBUG
#define	I2CDEBUG	do {	\
	PORTB |= _BV(PORTB5);	\
	usdelay(10000);		\
	PORTB ^= _BV(PORTB5);	\
} while (0)
#else
#define	I2CDEBUG	/* */
#endif

void
i2c_start(char ie)
{
	/* enable clock */
	PRR &= ~_BV(PRTWI);

	/* setup i2c, 400kHz */
	TWSR = 0;	/* prescaler: 0/4/16/64 */
	TWBR = (F_CPU / 400000UL - 16) / 2;
	TWCR = _BV(TWSTA) | _BV(TWINT) | _BV(TWEN) | ie;
}

char
i2c_wait(void)
{
	register char i;

	for (i = I2CTMO; i-- && !(TWCR & _BV(TWINT)); usdelay(I2CDLY))
/* if (addr == DBGADDR) {PORTB ^= _BV(PORTB5); msdelay(100);} */
		;

	return TWCR;
}

char
i2c_wraddr(char addr, char reg)
{
	register char sr;

	i2c_start(0);

	if (!(i2c_wait() & _BV(TWINT)))
		goto enxio;
	sr = TWSR & TW_STATUS_MASK;
	if (sr != TW_START && sr != TW_REP_START)
		goto eio;

	TWDR = addr;
	TWCR = _BV(TWINT) | _BV(TWEN);
	if (!(i2c_wait() & _BV(TWINT)))
		goto enxio;
	sr = TWSR & TW_STATUS_MASK;
	/* for a read we do not send reg number */
	if (addr & TW_READ) {
		if (sr == TW_MR_SLA_ACK)
			return 0;
		goto eio;
	}

	if (sr != TW_MT_SLA_ACK)
		goto eio;

	TWDR = reg;
	TWCR = _BV(TWINT) | _BV(TWEN);
	if (!(i2c_wait() & _BV(TWINT)))
		goto eagain;
	sr = TWSR & TW_STATUS_MASK;
	if (sr != TW_MT_DATA_ACK)
		goto eio;

	return 0;
  enxio:
	TWCR = _BV(TWSTO) | _BV(TWINT);
#ifdef DEBUG1
PORTB &= ~_BV(PORTB5);
msdelay(1000);
PORTB |= _BV(PORTB5);
msdelay(500);
PORTB ^= _BV(PORTB5);
msdelay(500);
PORTB |= _BV(PORTB5);
msdelay(500);
PORTB ^= _BV(PORTB5);
msdelay(500);
#endif
	return ENXIO;
 eagain:
	TWCR = _BV(TWSTO) | _BV(TWINT);
#ifdef DEBUG1
PORTB &= ~_BV(PORTB5);
msdelay(1000);
PORTB |= _BV(PORTB5);
msdelay(500);
PORTB ^= _BV(PORTB5);
msdelay(500);
PORTB |= _BV(PORTB5);
msdelay(500);
PORTB ^= _BV(PORTB5);
msdelay(500);
PORTB |= _BV(PORTB5);
msdelay(500);
PORTB ^= _BV(PORTB5);
msdelay(500);
#endif
	return EAGAIN;
    eio:
	TWCR = _BV(TWSTO) | _BV(TWINT);
#ifdef DEBUG1
PORTB &= ~_BV(PORTB5);
msdelay(1000);
PORTB |= _BV(PORTB5);
msdelay(500);
PORTB ^= _BV(PORTB5);
msdelay(500);
PORTB |= _BV(PORTB5);
msdelay(500);
PORTB ^= _BV(PORTB5);
msdelay(500);
PORTB |= _BV(PORTB5);
msdelay(500);
PORTB ^= _BV(PORTB5);
msdelay(500);
PORTB |= _BV(PORTB5);
msdelay(500);
PORTB ^= _BV(PORTB5);
msdelay(500);
#endif
	return EIO;
}

char
i2c_read(char addr, char reg, char *ptr, char n)
{
	register char sr;

#ifdef DEBUG
	if (addr != PCA9555)
		ks_putchar(8, 0, '_', 0);
#endif
	if (i2c_wraddr((addr << 1) | TW_WRITE, reg)) {
#ifdef DEBUG
		if (addr != PCA9555)
			ks_putchar(8, 8, 'r', 0);
#endif
		return -1;
	}

if (addr == HMC6352) msdelay(10);
	if (i2c_wraddr((addr << 1) | TW_READ, reg)) {
#ifdef DEBUG
		if (addr != PCA9555)
			ks_putchar(8, 8, 'R', 0);
#endif
		return -1;
	}

	for (sr = TW_MR_DATA_NACK; n; n--) {
		TWCR = _BV(TWINT) | _BV(TWEN) | (n > 1? _BV(TWEA) : 0);
		if (!(i2c_wait() & _BV(TWINT)))
			break;
		sr = TWSR & TW_STATUS_MASK;
		if (n == 1 && sr == TW_MR_DATA_NACK)
			;
		else if (sr != TW_MR_DATA_ACK)
			break;
		*ptr++ = TWDR;
		/* NACK at the end of receive will also fit here */
	}
	TWCR = _BV(TWSTO) | _BV(TWINT) | _BV(TWEN);

#ifdef DEBUG
	if (addr != PCA9555)
		ks_putchar(8, 0, '0' + n, 0);
#endif
	return n;
}

char
i2c_write1(char addr, char reg, char v)
{
	return i2c_write(addr, reg, &v, 1);
}

char
i2c_write(char addr, char reg, const char *ptr, char n)
{
	register char sr;

	if (i2c_wraddr((addr << 1) | TW_WRITE, reg)) {
#ifdef DEBUG
		if (addr != PCA9555)
			ks_putchar(0, 8, 'w', 0);
#endif
		return -1;
	}

	for (sr = TW_MT_DATA_NACK; n; n--) {
		TWDR = *ptr++;
		TWCR = _BV(TWINT) | _BV(TWEN);
		if (!(i2c_wait() & _BV(TWINT)))
			break;
		sr = TWSR & TW_STATUS_MASK;
		if (sr != TW_MT_DATA_ACK)
			break;
	}
	TWCR = _BV(TWSTO) | _BV(TWINT);
#ifdef DEBUG
	if (addr != PCA9555)
		ks_putchar(8, 0, '0' + n, 0);
#endif
	return n;
}

#ifdef DEBUG
void
i2c_scan(void)
{
	char a, i;

	for (i = 0, a = 1; a < 0x7f; a++) {
		if (a != PCA9555)
			ks_puthexb(8, 16, a, 0);
		if (!i2c_wraddr((a << 1) | TW_READ, 0)) {
			if (a != PCA9555) {
				ks_puthexb(16, i, a, 0);
				i += 12;
			}
		}
	}
}
#endif

char
i2c_cmd(struct i2c_cmd *cmd)
{
	register char rv;

	if (!i2c_run) {
		i2c_run = cmd;
		i2c_start(_BV(TWIE));
		i2cst = 0;
		rv = 0;
	} else
		rv = 1;
	return rv;
}

/*
 * i2c: state machine
 */
ISR(TWI_vect)	/* vector 24 */
{
	extern signed char state;
	static char *ptr, n;
	register char sr, cr;

	if (i2cst < 0 || !i2c_run) {
		TWCR = _BV(TWINT);
		return;
	}
/* PORTB ^= _BV(PORTB5); */

	cr = _BV(TWINT) | _BV(TWEN) | _BV(TWIE);
	sr = TWSR & TW_STATUS_MASK;
	switch (i2cst) {
	case 0:	/* start acked; send sla+w */
		if (sr != TW_START && sr != TW_REP_START) {
			/* I2CDEBUG; */
			goto fail;
		} else {
			if (i2c_run->reg) {
				TWDR = i2c_run->addr & ~TW_READ;
				i2cst++;
			} else {
				i2cst = 6;
				TWDR = i2c_run->addr;
			}
		}
		ptr = i2c_run->ptr;
		n = i2c_run->num;
		break;
	case 1:	/* sla+w is acked; send register number */
		if (sr != TW_MT_SLA_ACK) {
			/* I2CDEBUG; */
			goto fail;
		} else {
			TWDR = i2c_run->reg;
			i2cst = i2c_run->addr & TW_READ? 4 : 2;
		}
		break;
	case 2:	/* reg is acked; procede to write data */
		if (sr != TW_MT_DATA_ACK)
			goto fail;
		if (n <= 1)
			i2cst++;
		TWDR = *ptr++;
		n--;
		break;
	case 3:
		i2c_run->stat = sr != TW_MT_DATA_ACK? sr : 0;
	next:	i2c_run++;
		/* nomore to poke */
		if (!i2c_run->addr) {
			state++;
			i2cst = -1;
			cr = _BV(TWSTO) | _BV(TWINT);
			i2c_run = (void *)0;
		} else {
			i2cst = 0;
			cr = _BV(TWSTA) | _BV(TWINT) | _BV(TWEN) | _BV(TWIE);
		}
		break;

	case 4:	/* reg is set; start read transaction */
		if (sr != TW_MT_DATA_ACK) {
			/* I2CDEBUG; */
			goto fail;
		}
		cr = _BV(TWSTA) | _BV(TWINT) | _BV(TWEN) | _BV(TWIE);
		i2cst++;
		break;
	case 5: /* start acked; send sla+w */
		if (sr != TW_START && sr != TW_REP_START) {
			/* I2CDEBUG; */
			goto fail;
		} else {
			TWDR = i2c_run->addr;
			i2cst++;
		}
		break;
	case 6:	/* sla+w is acked; start data receive */
		if (sr != TW_MR_SLA_ACK) {
			/* I2CDEBUG; */
			goto fail;
		} else {
			i2cst++;
			if (n > 1)
				cr |= _BV(TWEA);
		}
		break;
	case 7:	/* new data received */
		if (sr == TW_MR_DATA_NACK && n == 1) {
			i2c_run->stat = 0;
			*ptr = TWDR;
			goto next;
		}
		if (sr != TW_MR_DATA_ACK) {
			/* I2CDEBUG; */
			goto fail;
		}
		*ptr++ = TWDR;
		n--;
		if (n > 1)
			cr |= _BV(TWEA);
		break;

	fail:
	default:
/* PORTB &= ~_BV(PORTB5); */
		i2c_run->stat = sr;
		TWCR = _BV(TWSTO) | _BV(TWINT) | _BV(TWEN);
		goto next;
	}
	TWCR = cr;
}
