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

/* Dallas Semiconductor RTC+SRAM */

struct ds1307_times {
	char sec : 4;	/* 00 */
	char sec10 : 3;
	char ch : 1;
	char min : 4;	/* 01 */
	char min10 : 3;
	char minpad : 1;
	char hour : 4;	/* 02 */
	char hh10 : 2;
	char h1224 : 1;
	char hurpad : 1;
	char wday : 3;	/* 03 */
	char wpad : 5;
	char date : 4;	/* 04 */
	char dd10 : 2;
	char dpad : 2;
	char mon : 4;	/* 05 */
	char mm10 : 1;
	char mpad : 3;
	char year : 4;	/* 06 */
	char yy10 : 4;
	char ctrl;	/* 07 */

	/* follows is 56 bytes of sram */
	char pad[56];
};

#ifdef DEBUG
#define	ds1307_debug(c)	ks_putchar(56, 6, c, 0)
#else
#define	ds1307_debug(c)	(void)(c)
#endif

#define	ds1307_init()	do {				\
	i2c_write1(DS1307, 0, 0);	/* start */	\
	i2c_write1(DS1307, 7, 0);	/* no out */	\
	if (i2c_read(DS1307, 0, (char *)&times, 8)) {	\
		ds1307_debug('d');			\
	}						\
} while (0)

#define	ds1307_update()	do {				\
	if (i2c_write(DS1307, 0, (char *)&times, 8)) {	\
		ds1307_debug('D');			\
	}						\
} while (0)

#define	ds1307_stop()	do {				\
	i2c_write1(DS1307, 0, 0x80);	/* stop */	\
} while (0)
