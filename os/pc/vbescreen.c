/*
 * VBE framebuffer
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"


#define	Image	IMAGE
#include <draw.h>
#include <memdraw.h>
#include <cursor.h>

#include "screen.h"

enum {
	Tabstop	= 4,
	Scroll	= 8,
	Wid	= 1920,
	Ht	= 1080,
	Depth	= 16,
};

Cursor  arrow = {
    { -1, -1 },
    { 0xFF, 0xFF, 0x80, 0x01, 0x80, 0x02, 0x80, 0x0C,
      0x80, 0x10, 0x80, 0x10, 0x80, 0x08, 0x80, 0x04,
      0x80, 0x02, 0x80, 0x01, 0x80, 0x02, 0x8C, 0x04,
      0x92, 0x08, 0x91, 0x10, 0xA0, 0xA0, 0xC0, 0x40,
    },
    { 0x00, 0x00, 0x7F, 0xFE, 0x7F, 0xFC, 0x7F, 0xF0,
      0x7F, 0xE0, 0x7F, 0xE0, 0x7F, 0xF0, 0x7F, 0xF8,
      0x7F, 0xFC, 0x7F, 0xFE, 0x7F, 0xFC, 0x73, 0xF8,
      0x61, 0xF0, 0x60, 0xE0, 0x40, 0x40, 0x00, 0x00,
    },
};

Memimage *gscreen;
Cursorinfo cursor;

static Memdata xgdata;

static Memimage xgscreen =
{
	{ 0, 0, Wid, Ht },	/* r */
	{ 0, 0, Wid, Ht },	/* clipr */
	Depth,			/* depth */
	3,			/* nchan */
	RGB15,			/* chan */
	nil,			/* cmap */
	&xgdata,		/* data */
	0,			/* zero */
	0,			/* width in words of a single scan line */
	0,			/* layer */
	0,			/* flags */
};

static Memimage *conscol;
static Memimage *back;
static Memsubfont *memdefont;

static Lock screenlock;

static Point	curpos;
static int	h, w;
static Rectangle window;

static void myscreenputs(char *s, int n);
static void screenputc(char *buf);
static void screenwin(void);

extern long _multibootheader[12];

char*
rgbmask2chan(char *buf, int depth, u32int rm, u32int gm, u32int bm)
{
	u32int m[4], dm;	/* r,g,b,x */
	char tmp[32];
	int c, n;

	dm = 1<<depth-1;
	dm |= dm-1;

	m[0] = rm & dm;
	m[1] = gm & dm;
	m[2] = bm & dm;
	m[3] = (~(m[0] | m[1] | m[2])) & dm;

	buf[0] = 0;
Next:
	for(c=0; c<4; c++){
		for(n = 0; m[c] & (1<<n); n++)
			;
		if(n){
			m[0] >>= n, m[1] >>= n, m[2] >>= n, m[3] >>= n;
			snprint(tmp, sizeof tmp, "%c%d%s", "rgbx"[c], n, buf);
			strcpy(buf, tmp);
			goto Next;
		}
	}
	return buf;
}

void
screeninit(void)
{
	long *mbh = _multibootheader;
	uchar *fb;
	ulong chan;

	if (mbh[11] == 0)
		return;

	char schan[32];
	rgbmask2chan(schan, mbh[11], mbh[12], mbh[13], mbh[14]);
	chan = strtochan(schan);

	xgscreen.r.max.x = mbh[9];
	xgscreen.r.max.y = mbh[10];
	xgscreen.depth = mbh[11];

	// Map framebuffer physical address with size = height * bytes per line.
	fb = vmap(mbh[16], mbh[10] * mbh[15]);
	xgscreen.clipr = xgscreen.r;

	memsetchan(&xgscreen, chan);
	conf.monitor	= 1;
	xgdata.bdata	= fb;
	xgdata.ref	= 1;
	gscreen		= &xgscreen;
	gscreen->width	= wordsperline(gscreen->r, gscreen->depth);

	memimageinit();
	memdefont = getmemdefont();
	screenwin();
	screenputs = myscreenputs;
}

static void
screenwin(void)
{
	char *greet;
	Memimage *orange;
	Point p, q;
	Rectangle r;

	back = memblack;
	conscol = memwhite;

	orange = allocmemimage(Rect(0, 0, 1, 1), RGB16);
	orange->flags |= Frepl;
	orange->clipr = gscreen->r;
	orange->data->bdata[0] = 0x40;		/* magic: colour? */
	orange->data->bdata[1] = 0xfd;		/* magic: colour? */

	w = memdefont->info[' '].width;
	h = memdefont->height;

	r = insetrect(gscreen->r, 4);

	memimagedraw(gscreen, r, memblack, ZP, memopaque, ZP, S);
	window = insetrect(r, 4);
	memimagedraw(gscreen, window, memblack, ZP, memopaque, ZP, S);

	memimagedraw(gscreen, Rect(window.min.x, window.min.y,
		window.max.x, window.min.y + h + 5 + 6), orange, ZP, nil, ZP, S);
	freememimage(orange);
	window = insetrect(window, 5);

	greet = "Console";
	p = addpt(window.min, Pt(10, 0));
	q = memsubfontwidth(memdefont, greet);
	memimagestring(gscreen, p, conscol, ZP, memdefont, greet);
	flushmemscreen(r);
	window.min.y += h + 6;
	curpos = window.min;
	window.max.y = window.min.y + ((window.max.y - window.min.y) / h) * h;
}

void
flushmemscreen(Rectangle)
{
	;
}

uchar*
attachscreen(Rectangle *r, ulong *chan, int* d, int *width, int *softscreen)
{
	*r = gscreen->r;
	*d = gscreen->depth;
	*chan = gscreen->chan;
	*width = gscreen->width;
	*softscreen = 0;

	return gscreen->data->bdata;
}

void
getcolor(ulong p, ulong *pr, ulong *pg, ulong *pb)
{
	USED(p, pr, pg, pb);
}

int
setcolor(ulong p, ulong r, ulong g, ulong b)
{
	USED(p, r, g, b);
	return 0;
}

void
blankscreen(int)
{
	;
}

static void
myscreenputs(char *s, int n)
{
	int i;
	Rune r;
	char buf[4];

	if(!islo()) {
		/* don't deadlock trying to print in interrupt */
		if(!canlock(&screenlock))
			return;
	}
	else
		lock(&screenlock);

	while(n > 0){
		i = chartorune(&r, s);
		if(i == 0){
			s++;
			--n;
			continue;
		}
		memmove(buf, s, i);
		buf[i] = 0;
		n -= i;
		s += i;
		screenputc(buf);
	}
	unlock(&screenlock);
}

static void
scroll(void)
{
	int o;
	Point p;
	Rectangle r;

	o = Scroll*h;
	r = Rpt(window.min, Pt(window.max.x, window.max.y-o));
	p = Pt(window.min.x, window.min.y+o);
	memimagedraw(gscreen, r, gscreen, p, nil, p, S);
	flushmemscreen(r);
	r = Rpt(Pt(window.min.x, window.max.y-o), window.max);
	memimagedraw(gscreen, r, back, ZP, nil, ZP, S);
	flushmemscreen(r);

	curpos.y -= o;
}

static void
screenputc(char *buf)
{
	int w;
	uint pos;
	Point p;
	Rectangle r;
	static int *xp;
	static int xbuf[256];

	if (xp < xbuf || xp >= &xbuf[sizeof(xbuf)])
		xp = xbuf;

	switch (buf[0]) {
	case '\n':
		if (curpos.y + h >= window.max.y)
			scroll();
		curpos.y += h;
		screenputc("\r");
		break;
	case '\r':
		xp = xbuf;
		curpos.x = window.min.x;
		break;
	case '\t':
		p = memsubfontwidth(memdefont, " ");
		w = p.x;
		if (curpos.x >= window.max.x - Tabstop * w)
			screenputc("\n");

		pos = (curpos.x - window.min.x) / w;
		pos = Tabstop - pos % Tabstop;
		*xp++ = curpos.x;
		r = Rect(curpos.x, curpos.y, curpos.x + pos * w, curpos.y + h);
		memimagedraw(gscreen, r, back, back->r.min, nil, back->r.min, S);
		flushmemscreen(r);
		curpos.x += pos * w;
		break;
	case '\b':
		if (xp <= xbuf)
			break;
		xp--;
		r = Rect(*xp, curpos.y, curpos.x, curpos.y + h);
		memimagedraw(gscreen, r, back, back->r.min, nil, back->r.min, S);
		flushmemscreen(r);
		curpos.x = *xp;
		break;
	case '\0':
		break;
	default:
		p = memsubfontwidth(memdefont, buf);
		w = p.x;

		if (curpos.x >= window.max.x - w)
			screenputc("\n");

		*xp++ = curpos.x;
		r = Rect(curpos.x, curpos.y, curpos.x + w, curpos.y + h);
		memimagedraw(gscreen, r, back, back->r.min, nil, back->r.min, S);
		memimagestring(gscreen, curpos, conscol, ZP, memdefont, buf);
		flushmemscreen(r);
		curpos.x += w;
		break;
	}
}

int
cursoron(int dolock)
{
	int retry;

	if (dolock)
		lock(&cursor);
	if (candrawqlock()) {
		retry = 0;
		swcursorhide();
		swcursordraw();
		drawqunlock();
	} else
		retry = 1;
	if (dolock)
		unlock(&cursor);
	return retry;
}

void
cursoroff(int dolock)
{
	if (dolock)
		lock(&cursor);
	swcursorhide();
	if (dolock)
		unlock(&cursor);
}

void
setcursor(Cursor* curs)
{
	cursoroff(0);
	swcursorload(curs);
	cursoron(0);
}

void
cursorenable(void)
{
	if (swcursor)
		swcursorinit();
	cursoron(1);
}

void
cursordisable(void)
{
	cursoroff(0);
}
