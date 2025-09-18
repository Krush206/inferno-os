#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"
#include "io.h"

/*
 *  mouse types
 */
enum
{
	Mouseother,
	MousePS2,
	Mouseintelli,
};

static int accelerated;
static int mousetype;

/*
 *  ps/2 mouse message is three bytes
 *
 *	byte 0 -	0 0 SDY SDX 1 M R L
 *	byte 1 -	DX
 *	byte 2 -	DY
 *
 *  shift & left button is the same as middle button
 */
static void
ps2mouseputc(int c, int shift)
{
	static ulong lasttick;
	static short msg[3];
	static int nb;
	static uchar b[] = {0, 1, 4, 5, 2, 3, 6, 7, 0, 1, 2, 5, 2, 3, 6, 7 };
	int buttons, dx, dy;
	ulong m;

	/*
	 * Resynchronize in stream with timing.
	 */
	m = MACHP(0)->ticks;
	if(TK2SEC(m - lasttick) > 2)
		nb = 0;
	lasttick = m;

	/* 
	 *  check byte 0 for consistency
	 */
	if(nb==0 && (c&0xc8)!=0x08)
		return;

	msg[nb] = c;
	if(++nb == 3){
		nb = 0;
		if(msg[0] & 0x10)
			msg[1] |= 0xFF00;
		if(msg[0] & 0x20)
			msg[2] |= 0xFF00;

		buttons = b[(msg[0]&7) | (shift ? 8 : 0)];
		dx = msg[1];
		dy = -msg[2];
		mousetrack(buttons, dx, dy, TK2MS(MACHP(0)->ticks));
	}
	return;
}

/*
 *  set up a ps2 mouse
 */
static void
ps2mouse(void)
{
        if(mousetype == MousePS2)
		return;

        i8042auxenable(ps2mouseputc);
        /* make mouse streaming, enabled */
        i8042auxcmd(0xEA);
        i8042auxcmd(0xF4);

        mousetype = MousePS2;
}

static void
setintellimouse(void)
{
        i8042auxcmd(0xF3);      /* set sample */
        i8042auxcmd(0xC8);
        i8042auxcmd(0xF3);      /* set sample */
        i8042auxcmd(0x64);
        i8042auxcmd(0xF3);      /* set sample */
        i8042auxcmd(0x50);

	mousetype = Mouseintelli;
}

void
pointerstatus(char *buf, int)
{
	char *s;

	s = buf;
	switch (mousetype) {
	case MousePS2:
		s += sprint(s, "ps2\n");
		break;
	case Mouseintelli:
		s += sprint(s, "intelli\n");
		break;
	default:
	case Mouseother:
		s += sprint(s, "unknown\n");
		break;
	}
	if (accelerated)
		s += sprint(s, "accelerated\n");
}

void
pointerctl(char *buf)
{
	int nf, x;
	char *field[10];
	nf = getfields(buf, field, 10, 1, " \t\n");
	if (nf < 1)
		return;
	if(strcmp(field[0], "ps2") == 0){
		ps2mouse();
	} else if (strcmp(field[0], "ps2intellimouse") == 0) {
		ps2mouse();
		setintellimouse();
	} else if(strcmp(field[0], "accelerated") == 0){
		switch(mousetype){
		case MousePS2:
			x = splhi();
			i8042auxcmd(0xE7);
			splx(x);
			accelerated = 1;
			break;
		}
	} else if(strcmp(field[0], "linear") == 0){
		switch(mousetype){
		case MousePS2:
			x = splhi();
			i8042auxcmd(0xE6);
			splx(x);
			accelerated = 0;
			break;
		}
	} else if(strcmp(field[0], "res") == 0){
		int n,m;
		switch(nf){
		default:
			n = 0x02;
			m = 0x23;
			break;
		case 2:
			n = atoi(field[1])&0x3;
			m = 0x7;
			break;
		case 3:
			n = atoi(field[1])&0x3;
			m = atoi(field[2])&0x7;
			break;
		}

		switch(mousetype){
		case MousePS2:
			x = splhi();
			i8042auxcmd(0xE8);
			i8042auxcmd(n);
			i8042auxcmd(0x5A);
			i8042auxcmd(0x30|m);
			i8042auxcmd(0x5A);
			i8042auxcmd(0x20|(m>>1));
			splx(x);
			break;
		}
	}
}
