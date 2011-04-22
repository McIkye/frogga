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

#include "navi.h"
#include "gui.h"

char gyro[NGYR];
char accel[NACCEL];
char compraw[NCMP];

int velocity;	/* m/s */
short roll, tilt;

/*
 * from comp.dsp by Jim Shima on 23/04/99. public domain
 * this supposedly has a max error of 0.07 rad
 * (for a 0.01 rad error use:
 *	a = (0.1963 * r * r - 0.9817) * r + pi / 4;
 *	and
 *	a = (0.1963 * r * r - 0.9817) * r + 3 * pi / 4;
 *
 * returns a fixed point *10000
 */
long
atan2(long ix, long iy)
{
	long a, x = ix << 4, y = iy << 4, ay;

	ay = iy < 0? -y : y;
	ay++;
	if (ix >= 0)
		a = 31416 / 4 - 31416 / 4 * (x - ay) / (x + ay);
	else
		a = 31416 / 4 * 3 - 31416 / 4 * (ay + x) / (ay - x);

	return iy < 0? -a : a;
}

/*
cos = 1/1,6468
sin = 0
for i=0 to iterations
  cos_tmp = cos
  sin_tmp = sin
  if (angle > 0)
    cos = cos - sin_tmp*2^(-i)
    sin = sin + cos_tmp*2^(-i)
    angle = angle - arctan(2^(-i))
  else
    cos = cos + sin_tmp*2^(-i)
    sin = sin - cos_tmp*2^(-i)
    angle = angle + arctan(2^(-i))
  endif
next i
*/

/*
 * from comp.dsp by rainer storn on 19/04/99. public domain
 * binary search the square root
 */
long
sqrt(long x)
{
	long y;
	int a;
	char i;

	y = 0xffff;
	a = 0x8000;
	for (i = 16; i--; a >>= 1) {
		long d = y * y;
		if (x > d)
			y += a;
		else if (x < d)
			y -= a;
	}

	return y;
}

/* convert 0.00025g step value into 0.001 ms2 */
short
gtoms2(short v)
{
	long a = v;

	/* originally a * 9.81 * 25 */
	/*   and compensate for <<2 on the argument */
	/* thus: a * 4 * 9810 / 4 */
	return a * 9810;
}

short
com_dir(void)
{
	long ncompass, x, y, z;
	long o_roll, a_roll, h_roll;
	long o_pitch, a_pitch, h_pitch;
	short na[3], ng[3];

	/* valid-check the compass reading */
	if ((0xf8 > compraw[0] && compraw[0] >= 0x08) ||
	    (0xf8 > compraw[2] && compraw[2] >= 0x08) ||
	    (0xf8 > compraw[4] && compraw[4] >= 0x08))
		return 0;

	o_pitch = 0;
	a_pitch = h_pitch = 1;
	o_roll = 0;
	a_roll = h_roll = 1;

	ng[0] = ((short)gyro[0] << 8) | gyro[1];
	ng[1] = ((short)gyro[2] << 8) | gyro[3];
	ng[2] = ((short)gyro[4] << 8) | gyro[5];

	/* sign extending: <<2 will be compensated on return */
	na[0] = gtoms2((((short)accel[1] << 8) | accel[0]) << 2);
	na[1] = gtoms2((((short)accel[3] << 8) | accel[2]) << 2);
	na[2] = gtoms2((((short)accel[5] << 8) | accel[4]) << 2);

#if 0
	/* dumb accel-only roll/pitch calculation */
	a_roll = -na[2];
	o_roll = na[1];
	h_roll = sqrt((long)na[1] * na[1] + (long)na[2] * na[2]);
	o_pitch = (long)na[0];
	a_pitch = sqrt(9810L * 9810L - o_pitch * o_pitch);
	h_pitch = 9810L;
#endif

/* here we can accumulate the remainders to add on the next call */
#define	sin_roll	o_roll / h_roll
#define	cos_roll	a_roll / h_roll
#define	sin_pitch	o_pitch / h_pitch
#define	cos_pitch	a_pitch / h_pitch

	x = ((short)compraw[0] << 8) | compraw[1];
	y = ((short)compraw[2] << 8) | compraw[3];
	z = ((short)compraw[4] << 8) | compraw[5];

	x = x * cos_pitch +
	    y * sin_roll * sin_pitch +
	    z * cos_roll * sin_pitch;
	y = y * cos_roll - z * sin_roll;

	ncompass = atan2(-y, x);
#if 0
ks_puthexb(56, 74, ncompass >> 16, 0);
ks_puthexb(56, 86, ncompass >> 8, 0);
ks_puthexb(56, 98, ncompass, 0);
#endif
	ncompass = 1800 + 1800 * ncompass / 31416;
	if ((ncompass - compass) / (3600 / 20)) {
		compass = ncompass;
		return FLDCOMPASS;
	}

	return 0;
}

short
gps_dir(void)
{
	ntarget = target;

	/* TODO convert to universal coords */
	ntarget = 180 + 180 * atan2(tlat - latit, tlon - longit) / 3142;
	if ((ntarget - target) / (360 / 24))
		return FLDDIRECTS;

	return 0;
}
