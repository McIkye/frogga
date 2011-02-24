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

/*

pins:

adc6 -				joy-x			295,38 uA
adc7 -				joy-y

pb0 -	d8/icp1			?-cs
pb1 -	d9/oc1a			zigbee-cs		176,25 uA
pb2 -	d10/oc1b/spi-ss		sd-cs			186,50 uA
pb3 -	d11/oc2a/spi-mosi	sd-di
pb4 -	d12/spi-miso		sd-do
pb5 -	d13/spi-sclk/D5		sd-sclk
						sdhc	75/0,25 mA
						music	60/5 mA
						zigbee	15 mA
						gps?????????? mA
pb6 -	d14/xtal			use int osc and pps correction 8mhz
pb7 -	d15/xtal			use int osc and pps correction 8mhz

pc0 -	a0			music-rst
pc1 -	a1			music-dreq
pc2 -	a2			music-xdcs
pc3 -	a3			music-xcs
pc4 -	a4			i2c_sda			199,25 uA
pc5 -	a5			i2c_scl
						lcd	3,25 mA 455 uA
						compass	800 uA
						gyro	6,5 mA 5 uA
						accel	47 uA
						heart
						volt/bat
						light
						alt
						temp
pc6 -				reset

pd0 -	d0			gps-dout		100,25 uA + 150/44 mA
pd1 -	d1			gps-din
pd2 -	d2/pcint18/int0		gps-pps-int/D3/mus-dock/power-butt
pd3 -	d3/pcint19/oc2b/int1	power-led		224,25 uA + 10 mA
pd4 -	d4/pcint20		rpm-int
pd5 -	d5/oc0b			cadence-int
pd6 -	d6/oc0a			lcd-backlit		61,13 uA + 240 mA
pd7 -	d7/pcint23		joy-butt

devices							 11,3 mA
cpu							  9,0 mA
load							210,0 mA   75 mA
lcd							240,0 mA  120 mA

total							470,3 mA (@5v) 215.3 mA


interrupts overhead/delay:

isp any (total 5ms?)
	N = 80(mmc) + 2*80(dac)
	N * 2						3840 cycles
uart 2ms (total 142 ms)
	71 chars/msg/sec				568 cycles
i2c any (total 40ms?)
	N = 10(g) + 3(c) + ~4*(256)(lcd) = ~1000
	N * (4 * io) /sec				32000 cycles
timer
	4/sec						32 cycles
adc any (total 200 + 13,5*7 = 1,2ms )
	8						64 cycles
*/

#include <stdint.h>
#include <string.h>

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#include <util/twi.h>

#include "gui.h"
#include "navi.h"
#include "i2c.h"
#include "bma180.h"
#include "ds1307.h"
#include "hmc5843.h"
#include "itg3200.h"
#include "pca9555.h"

/* fsm states */
signed char state;
/* post-states flags */
char floop, userin, reset, mode, dead;
short changed, pwrb;
/* ui status */
char adc1, butt, obutt, power, opower;
char rpm, orpm, hbeat, ohbeat, pecad, opecad;
/* directions */
short target, ntarget, compass;
/* system times (RTC and SRAM) */
struct ds1307_times times;
/* gps positioning */
int olongit, olatit, ospeed, ocourse;
int longit, latit, speed, course;
#define	NADC	16
short adc[NADC], oadc[NADC];

/* target */
int tlon, tlat;

void streset(void);

struct i2c_cmd i2c_acquire[] = {
	{ (ITG3200 << 1) | TW_READ, 0, 0x1b, NGYR, gyro },
	{ (BMA180  << 1) | TW_READ, 0, 0x02, NACCEL, accel },
	{ (HMC5843 << 1) | TW_READ, 0, 0x03, NCMP, compraw },
/*	{ (NE05 << 1)	 | TW_READ, 0, 0x00, 0, 0 }, */

	/* right now we rely on gps for timestamps */
/*	{ (DS1307  << 1) | TW_READ, 0, 0x00, 7, times }, */
	{ 0 }

/* just for ref */
/*	{ (MMA7660 << 1) | TW_READ, 0, 0x00, 4, accel }, */
/*	{ (HMC6352 << 1) | TW_READ, 0, 0x00, 2, (char *)&ncompass }, */
};

void
sei_sleep()
{
	/* disable brown-out-detection */
	MCUCR = _BV(BODS) | _BV(BODSE);
	MCUCR = _BV(BODS);
	sei();
	sleep_cpu();
	cli();
	SMCR = 0;
}

void
suspend()
{
	short cnt;

	dead = 1;
	PORTD = 0;
	PORTB = 0;

	wdt_disable();

	/* wait fo button off */
	msdelay(1000);
	while (PIND & _BV(PD2))
		;

	/* disable digital input buffers */
	DIDR0 = _BV(ADC5D) | _BV(ADC4D) | _BV(ADC3D) | _BV(ADC2D) |
	    _BV(ADC1D) | _BV(ADC0D);
	DIDR1 = _BV(AIN1D) | _BV(AIN0D);

	/* stop the music */
/*	vs1053_stop(); */

	/* kill i2c devices */
	bma180_stop();
	hmc5843_stop();
	itg3200_stop();
	pca9555_stop();

	/* turn off the LCD controller */
	ks_lcd(0);

	/* disable devices */
	PRR |= ~_BV(PRTIM2);

	/* slower tmr2 */
	TCCR2B = _BV(CS22) | _BV(CS21) | _BV(CS20);	/* / 1024 */

	for (cnt = 0; cnt <= 300; ) {
		if (PIND & _BV(PD2))
			cnt++;
		else
			cnt = 0;
		/* power-save mode */
		SMCR = _BV(SE) | _BV(SM0) | _BV(SM1);
		sei_sleep();
	}

	/* reset */
	PRR = 0;
	wdt_enable(WDTO_120MS);
	for(;;);
}

int
main(void)
{
	char i, c = 0;

	cli();
	MCUSR = 0;	/* clear the reset bits */

	/* poweroff unused devices */
	PRR = _BV(PRTIM1) | _BV(PRTWI) | _BV(PRSPI) | _BV(PRADC);

	gui_init();
	msdelay(200);	/* allow i2c devices to startup */

	/* program uart */
#define	BRR	(F_CPU / 16 / 9600 - 1)
	PRR &= ~_BV(PRUSART0);
	UBRR0H = BRR >> 8;
	UBRR0L = BRR;				/* 9600 */
	UCSR0C = _BV(USBS0) | _BV(UCSZ00);	/* 8n1 */
	UCSR0B |= _BV(RXEN0) | _BV(RXCIE0);

	/* config gps */
/* TODO
   $PSRF102
   $PSRF103
 */
	ds1307_init();	/* fetch time/pos from RTC */
/* TODO preload gps pos/time
   $PSRF104,lat,lon,alt,clkdrift,timeofweek,week,nsat,flags,*ck
 */

	/* start the gyros rolling */
	itg3200_init();
	i2c_read(ITG3200, 0x1b, gyro, NGYR);

	/* setup the accelerometer */
	bma180_init();

	/* init the compass */
	hmc5843_init();
	i2c_read(HMC5843, 0x03, compraw, NCMP);

	/* setup vs1053 jukebox */
/*	vs1053_init(); */

#if 0
/* XXX move upwards to cli */
	/* start the watchdog */
	wdt_enable(WDTO_500MS);
#endif
ks_puthexb(48, 0, UDR0, 0);

	/* setup power/rpm/cadence/joystick interrupts */
	PORTD = 0;
	PCICR = _BV(PCIE2);
	PCIFR = _BV(PCIF2);
	PCMSK2 = _BV(PCINT20) | _BV(PCINT21) | _BV(PCINT23);
	DDRD &= ~(_BV(DDD2) | _BV(DDD4) | _BV(DDD5) | _BV(DDD7));
	DDRD |= _BV(DDD3) | _BV(DDD6);

	/* set tmr0 for pwm on lcd backlight */
	PRR &= ~_BV(PRTIM0);
	OCR0A = 1;				/* fastPWM mode */
	TCCR0A = _BV(COM0A1) | _BV(WGM01) | _BV(WGM00);
	TCCR0B = _BV(CS22) | _BV(CS21);		/* / 256 */
	TIMSK0 = 0;				/* no interrupts */

	/* set tmr2 to interrupt each 8ms */
	PRR &= ~_BV(PRTIM2);
	OCR2B = 0x90;				 /* fastPWM mode */
	TCCR2A = _BV(COM2B1) | _BV(WGM21) | _BV(WGM20);
	TCCR2B = _BV(CS22) | _BV(CS21);		/* / 256 */
	TIFR2 = _BV(TOV2);
	TIMSK2 = _BV(TOIE2);

	streset();
	for (changed = FLDCLRSCR; !reset; c++, sei_sleep()) {
/* ks_puthexb(24 + (state & ~7), (state & 7) * 12, state, c & 1); */

		if ((PIND & _BV(PD2))) {
			if (pwrb++ > 2000)
				suspend();	/* this will reset on exit */
		} else
			pwrb = 0;

		switch (state) {
		case -1: /* idle */
			SMCR = _BV(SE);
			continue;

		/* GPS or timer has started */
		case 0:
			i2c_cmd(i2c_acquire);
			/* all low analog lines are used by the vs1053 iface */
			state = 12;
			/* FALLTHROUGH */
		case 12: /* joy-X */
			PRR &= ~_BV(PRADC);	/* enable adc */
			ADCSRA = _BV(ADEN);
			ADCSRB = 0;
			/* FALLTHROUGH */
		case 14: /* joy-Y */
		case 16: /* core temp */
		case 18: /* mind the gap */
			if (state == 18)
		case 20: case 22: case 24: case 26:
				state = 28;
		case 28:/* 1.1v */
		case 30:/* 0.0v */
			/* int 1.1V reference, left-adjust result */
			ADMUX = _BV(REFS1) | _BV(REFS0) | _BV(ADLAR) |
			    (state >> 1);
			ADCSRA = _BV(ADEN) | _BV(ADSC) | _BV(ADIF) |
			    _BV(ADPS2) |_BV(ADPS1) | _BV(ADPS0) | /* 125 khz */
			    _BV(ADIE);
			state++;

		case 13: case 15: case 17: case 19:
		case 21: case 23: case 25: case 27: case 29:
			/* gotta be sure this is way less than 100us */
			SMCR = _BV(SE); /* | _BV(SM0);	* noise reduction */
			continue;
		case 31:
			ADCSRA = 0;
			PRR |= _BV(PRADC);
			state++;

		case 32: /* NADC * 2 */
/* TODO poll buttons/joy */
			if (adc1 && butt != obutt)
				userin++;
			state++;
			/* do not wait for uart */
			if (floop)
				state += 1;
			SMCR = _BV(SE);
			continue;

		case 33:	/* waiting for uart and i2c */
		case 34:
			SMCR = _BV(SE);
			continue;

		default: /* DONE */
			/* turn off i2c */
			TWCR = 0;
			PRR |= _BV(PRTWI);
			break;
		}
		if (opecad != pecad || orpm != rpm)
			changed |= FLDHEART;
		opecad = pecad;
		orpm = rpm;

		if (userin) {
/* TODO set-update LCD */
/* TODO update relays */
		}

		obutt = butt;
		if (changed & FLDRTC) {
			ds1307_update();
			changed ^= FLDRTC;
		}
		sei();

		changed |= com_dir();
		changed |= gps_dir();

#ifdef DEBUG
if (changed && (hbeat & 2)) {
ks_puthexb(56, 0, changed >> 8, c & 1);
ks_puthexb(56, 12, changed, c & 1);
    ks_puthexb(32, 0, i2c_acquire[0].stat, c & 1);
	ks_puthexb(32, 12, gyro[0], 0);
	ks_puthexb(32, 24, gyro[1], 0);
	ks_puthexb(32, 36, gyro[2], 0);
	ks_puthexb(32, 48, gyro[3], 0);
	ks_puthexb(32, 60, gyro[4], 0);
	ks_puthexb(32, 72, gyro[5], 0);
    ks_puthexb(40, 0, i2c_acquire[1].stat, c & 1);
	ks_puthexb(40, 12, accel[1], 0);
	ks_puthexb(40, 24, accel[0], 0);
	ks_puthexb(40, 36, accel[3], 0);
	ks_puthexb(40, 48, accel[2], 0);
	ks_puthexb(40, 60, accel[5], 0);
	ks_puthexb(40, 72, accel[4], 0);
#if 0
    ks_puthexb(48, 0, i2c_acquire[2].stat, c & 1);
	ks_puthexb(48, 12, compraw[0], 0);
	ks_puthexb(48, 24, compraw[1], 0);
	ks_puthexb(48, 36, compraw[2], 0);
	ks_puthexb(48, 48, compraw[3], 0);
	ks_puthexb(48, 60, compraw[4], 0);
	ks_puthexb(48, 72, compraw[5], 0);
#endif
}
changed |= FLDCOMPASS;
#endif
		gui_update(changed);
		changed = 0;
		target = ntarget;

/* TODO sometimes update RTC from GPS with timestamp and location */
		wdt_reset();

		cli();
		PRR |= _BV(PRTWI);
		adc1 = 1;
		streset();
		for (i = NADC; i--; oadc[(int)i] = adc[(int)i])
			;
		SMCR = _BV(SE); /* | _BV(SM0) | _BV(SM1);	* power-save */
	}

	/* panic! */
	for (;;);	/* watchdog will reset */
	return 0;
}

void
streset(void)
{
	state = gpst = i2cst = spist = -1;
	floop = changed = userin = reset = 0;
}

void
usdelay(unsigned short us)
{
	while(us--) {
#if F_CPU == 20000000
		asm volatile("nop\n\tnop\n\tnop\n\tnop");
		asm volatile("nop\n\tnop\n\tnop\n\tnop");
		asm volatile("nop\n\tnop\n\tnop\n\tnop");
		asm volatile("nop\n\tnop\n\tnop\n\tnop");
		asm volatile("nop\n\tnop");
#elif F_CPU == 16000000
		asm volatile("nop\n\tnop\n\tnop\n\tnop");
		asm volatile("nop\n\tnop\n\tnop\n\tnop");
		asm volatile("nop\n\tnop\n\tnop\n\tnop");
		asm volatile("nop\n\tnop");
#elif F_CPU == 12000000
		asm volatile("nop\n\tnop\n\tnop\n\tnop");
		asm volatile("nop\n\tnop\n\tnop\n\tnop");
		asm volatile("nop\n\tnop");
#elif F_CPU == 8000000
		asm volatile("nop\n\tnop\n\tnop\n\tnop");
		asm volatile("nop\n\tnop");
#else
#error "fix usdelay()"
#endif
	}
}

void
msdelay(unsigned short ms)
{
	while(ms--)
		usdelay(998);	/* compensate for a function call */
}

const char blsin[32] PROGMEM = {
	/* generated from (plus some padding on the front and back):
	 * awk 'BEGIN { for (a = 270; a < 270+180; a += 6) \
	 *	printf("%d, ", 128 + sin(a * 3.1415926 / 180) * 127)}'
	 */
1,
1, 1, 2, 4, 6, 9, 13, 17, 21, 26, 32, 38, 44, 50, 57, 63, 70, 77, 83, 89, 95, 101, 106, 110, 114, 118, 121, 123, 125, 126,
127
};

/*
 * tmr2: ui clock tick
 */
ISR(TIMER2_OVF_vect)	/* vector 9 */
{
	static char cnt;	/* count down from ~8ms */
	static char sec;	/* count down from 100ms */
	char c;

	if (dead) {
		c = cnt++ >> 2;
		if (c & 32)
			c = 63 - c;
		OCR2B = pgm_read_byte(&blsin[(int)c]);
		return;
	}

	if (cnt++ % 12)
		return;
#ifdef DEBUG
	if (state != -1) {
/* PORTB ^= _BV(PORTB5); */
		return;
	}
#endif
/* PORTD ^= _BV(PORTD3); */
	if (!(++sec % 100))
		pecad = rpm = 0;

/* PORTB ^= _BV(PORTB5); */
	state = 0;
	floop = 1;
	SMCR = _BV(SE);

if (!(sec % 4)) {
hbeat++;
changed |= FLDHEART;
}
}

/*
 * power-butt/rpm/cadence/joybutt pulses
 */
ISR(PCINT2_vect)	/* vector 5 */
{
	if (PIND & _BV(PD4))
		pecad++;
	if (PIND & _BV(PD5))
		rpm++;
	if (PIND & _BV(PD7))
		butt++;
}

/*
 * acquire adcN measure
 */
ISR(ADC_vect)	/* vector 21 */
{
	char st;

	if (state < 0 || state >= NADC * 2) {
		PRR |= _BV(PRADC);
		return;
	}

/* PORTB ^= _BV(PORTB5); */
	st = state / 2;
	adc[(int)st] = ((short)ADCH << 8) | ADCL;
	if (adc1 && st < 2 && adc[(int)st] != oadc[(int)st])
		userin++;
	state++;
}
