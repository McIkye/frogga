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

/* these are there on any mode */
#define	FLDHEART	0x0001
#define	FLDTIMES	0x0002
#define	FLDPOWER	0x0004
#define	FLDID3		0x0008

/* mode 0&1 only */
#define	FLDDIRECTS	0x0010
#define	FLDSPEED	0x0020
#define	FLDDISTANCE	0x0040
#define	FLDCOMPASS	0x0080

/* modes 2&3 only have 4 lines to scroll thru */
#define	FLDHEAD		0x0100
#define	FLDLST0		0x0200
#define	FLDLST1		0x0400
#define	FLDLST2		0x0800
#define	FLDLST3		0x1000
#define	FLDLST4		0x2000

/* these are not necessarily on the screen */
#define	FLDUNNO		0x4000
#define	FLDRTC		0x8000
#define	FLDCLRSCR	0xffff

/* modes of the gui */
#define	MODROLL	0	/* [bh]iking mode */
#define	MODNARF	1	/* navigating mode */
#define	MODWPT	2	/* select waypoints */
#define	MODMUS	3	/* select music track */

extern struct ds1307_times times;
extern char rpm, hbeat, pecad, power, mode;
extern signed char state, gpst, i2cst, spist, jbst;
extern short changed;

void ks_lcd(char);
void ks_clear(void);
void ks_putchar(char, char, char, char);
void ks_put0(char, char, char, char);
void ks_putstr(char, char, const char *, char);
char ks_length(const char *);
void ks_puthexb(char, char, char, char);
void ks_bitmap(char, char, const char *, char, char);
void ks_pgbitmap(char, char, const char *, char, char);

void gui_init(void);
void gui_update(short);

void usdelay(unsigned short);
void msdelay(unsigned short);
