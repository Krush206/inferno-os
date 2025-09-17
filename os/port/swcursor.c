#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#define	Image	IMAGE
#include	<draw.h>
#include	<memdraw.h>
#include	<cursor.h>
#include	"screen.h"

extern Memimage* gscreen;

/*
 * Software cursor.
 */
static int	swvisible;	/* is the cursor visible? */
static int	swenabled;	/* is the cursor supposed to be on the screen? */
static Memimage *swback;	/* screen under cursor */
static Memimage *swimg;		/* cursor image */
static Memimage *swmask;	/* cursor mask */
static Memimage *swimg1;
static Memimage *swmask1;

static Point	swoffset;
static Rectangle swrect;	/* screen rectangle in swback */
static Point	swpt;		/* desired cursor location */
static Point	swvispt;	/* actual cursor location */
static int	swvers;		/* incremented each time cursor image changes */
static int	swvisvers;	/* the version on the screen */

/*
 * called with drawlock locked for us, most of the time.
 * kernel prints at inopportune times might mean we don't
 * hold the lock, but memimagedraw is now reentrant so
 * that should be okay: worst case we get cursor droppings.
 */
void
swcursorhide(void)
{
	if(swvisible == 0)
		return;
	if(swback == nil)
		return;
	swvisible = 0;
	memimagedraw(gscreen, swrect, swback, ZP, memopaque, ZP, S);
	flushmemscreen(swrect);
}

void
swcursordraw(void)
{
	int dounlock;

	if(swvisible)
		return;
	if(swenabled == 0)
		return;
	if(swback == nil || swimg1 == nil || swmask1 == nil)
		return;
	dounlock = candrawqlock();
	swvispt = swpt;
	swvisvers = swvers;
	swrect = rectaddpt(Rect(0,0,16,16), swvispt);
	memimagedraw(swback, swback->r, gscreen, swpt, memopaque, ZP, S);
	memimagedraw(gscreen, swrect, swimg1, ZP, swmask1, ZP, SoverD);
	flushmemscreen(swrect);
	swvisible = 1;
	if(dounlock)
		drawqunlock();
}

static int
swmove(Point p)
{
	if(p.x < 0)
		p.x = 0;
	else if(p.x >= gscreen->r.max.x - 1)
		p.x = gscreen->r.max.x - 2;
	if(p.y < 0)
		p.y = 0;
	else if(p.y >= gscreen->r.max.y - 1)
		p.y = gscreen->r.max.y - 2;
	mousetrack(0, p.x, p.y, 0);
	swpt = addpt(p, swoffset);
	return 0;
}

static void
swcursorclock(void)
{
	int x;

	if(!swenabled)
		return;
	swmove(mousexy());
	if(swvisible && eqpt(swpt, swvispt) && swvers==swvisvers)
		return;

	x = splhi();
	if(swenabled)
	if(!swvisible || !eqpt(swpt, swvispt) || swvers!=swvisvers)
	if(candrawqlock()){
		swcursorhide();
		swcursordraw();
		drawqunlock();
	}
	splx(x);
}

static void
Cursortocursor(Cursor *c)
{
	memcpy(&cursor.Cursor, c, sizeof(Cursor));
	setcursor(c);
}

void
swcursorinit(void)
{
	static int init;

	if(!init){
		init = 1;
		addclock0link(swcursorclock, 10);
		swenabled = 1;
	}
	if(swback){
		freememimage(swback);
		freememimage(swmask);
		freememimage(swmask1);
		freememimage(swimg);
		freememimage(swimg1);
	}

	swback  = allocmemimage(Rect(0,0,32,32), gscreen->chan);
	swmask  = allocmemimage(Rect(0,0,16,16), GREY8);
	swmask1 = allocmemimage(Rect(0,0,16,16), GREY1);
	swimg   = allocmemimage(Rect(0,0,16,16), GREY8);
	swimg1  = allocmemimage(Rect(0,0,16,16), GREY1);
	if(swback==nil || swmask==nil || swmask1==nil || swimg==nil || swimg1 == nil){
		print("software cursor: allocmemimage fails\n");
		return;
	}

	memfillcolor(swmask, DOpaque);
	memfillcolor(swmask1, DOpaque);
	memfillcolor(swimg, DBlack);
	memfillcolor(swimg1, DBlack);
	Cursortocursor(&arrow);
}

void
swcursorload(Cursor *curs)
{
	uchar *ip, *mp;
	int i, j, set, clr;

	if(!swimg || !swmask || !swimg1 || !swmask1)
		return;
	/*
	 * Build cursor image and mask.
	 * Image is just the usual cursor image
	 * but mask is a transparent alpha mask.
	 *
	 * The 16x16x8 memimages do not have
	 * padding at the end of their scan lines.
	 */
	ip = byteaddr(swimg, ZP);
	mp = byteaddr(swmask, ZP);
	for(i=0; i<32; i++){
		set = curs->set[i];
		clr = curs->clr[i];
		for(j=0x80; j; j>>=1){
			*ip++ = set&j ? 0x00 : 0xFF;
			*mp++ = (clr|set)&j ? 0xFF : 0x00;
		}
	}
	swoffset = curs->offset;
	swvers++;
	memimagedraw(swimg1,  swimg1->r,  swimg,  ZP, memopaque, ZP, S);
	memimagedraw(swmask1, swmask1->r, swmask, ZP, memopaque, ZP, S);
}

void
swcursoravoid(Rectangle r)
{
	if(swvisible && rectXrect(r, swrect))
		swcursorhide();
}
