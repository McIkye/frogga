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

/* NXP/Philips i2c->16pins port */

/* setup the pca9555 pins for lcd interface */
#define	pca9555_init()	do {			\
	/* reset pins to zeroes */		\
	i2c_write1(PCA9555, 2, 0);		\
	i2c_write1(PCA9555, 3, 0);		\
						\
	/* reset inversion registers */		\
	i2c_write1(PCA9555, 4, 0);		\
	i2c_write1(PCA9555, 5, 0);		\
						\
	/* set all pins to output */		\
	i2c_write1(PCA9555, 6, 0);		\
	i2c_write1(PCA9555, 7, 0);		\
} while (0)

#define	pca9555_stop()	do {			\
	/* set all pins to input */		\
	i2c_write1(PCA9555, 6, 0xff);		\
	i2c_write1(PCA9555, 7, 0xff);		\
} while (0)
