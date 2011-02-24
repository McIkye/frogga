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

/* InvenSense 3-axis digital gyro */

struct itg3200 {
	char id;		/* 00: i2c address: 0x68 */
	char pad0[20];
	char div;		/* 15: sample-rate divisor */
	char dlpf_cfg : 3;	/* 16: */
	char fs_sel : 2;	/* */
	char int_cfg;		/* 17: interrupt config */
	char pad1[2];
	char istat;		/* 1a: interrupt status */
	char temp_h;		/* 1b: chip temperature */
	char temp_l;
	char xout_h;		/* 1d: x-axis reading */
	char xout_l;
	char yout_h;		/* 1f: y-axis reading */
	char yout_l;
	char zout_h;		/* 21: z-axis reading */
	char zout_l;
	char pad2[27];
	char pwr_mgm;		/* 3e: power management */
#define	ITG3200_RESET	0x80	/* reset to power-on defaults */
#define	ITG3200_SLEEP	0x40	/* get some sleep */
#define	ITG3200_STBY_X	0x20	/* chill the X-wheel */
#define	ITG3200_STBY_Y	0x10	/* chill the Y-wheel */
#define	ITG3200_STBY_Z	0x08	/* chill the Z-wheel */
#define	ITG3200_CLK_INT	0x00	/* use internal oscilator (bad) */
#define	ITG3200_CLK_X	0x01	/* use gyro-X */
#define	ITG3200_CLK_Y	0x02	/* use gyro-Y */
#define	ITG3200_CLK_Z	0x03	/* use gyro-Z */
#define	ITG3200_CLK_32	0x04	/* use ext 32,768 khz signal */
#define	ITG3200_CLK_19	0x05	/* use ext 19,2 mhz signal */
};

#define	itg3200_init()	do {					\
	i2c_write1(ITG3200, 0x3e, ITG3200_RESET);		\
	msdelay(5);						\
	i2c_write1(ITG3200, 0x3e, ITG3200_CLK_Z);		\
	msdelay(5);						\
	i2c_write1(ITG3200, 0x15, 99);	/* 10hz sampl rate */	\
	i2c_write1(ITG3200, 0x16, 0x1d);/* 10hz dlpf */		\
} while (0)

#define	itg3200_fetch()	do {					\
	if (i2c_read(ITG3200, 0x1b, (char *)&gyro, 8))		\
		ks_putchar(56, 12, 'g', 0);			\
} while (0)

#define	itg3200_stop()	do {					\
	i2c_write1(ITG3200, 0x3e, ITG3200_SLEEP |		\
	    ITG3200_STBY_X | ITG3200_STBY_Y | ITG3200_STBY_Z);	\
} while (0)
