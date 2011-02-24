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

/* Honeywell digital compass */

#define	HMC5843_CONT	0	/* continuous measurement */
#define	HMC5843_ONE	1	/* single-measurement */
#define	HMC5843_IDLE	2	/* idle */
#define	HMC5843_SLEEP	3	/* sleep */

#define	hmc5843_init()	do {				\
	i2c_write1(HMC5843, 0x00, 0x10);		\
	i2c_write1(HMC5843, 0x01, 0x20);		\
	i2c_write1(HMC5843, 0x02, HMC5843_CONT);	\
	msdelay(100);	/* resynchronise */		\
} while (0)

#define	hmc5843_stop()	do {				\
	i2c_write1(HMC5843, 0x02, HMC5843_SLEEP);	\
} while (0)
