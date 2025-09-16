#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"

#define	Image	IMAGE
#include <draw.h>
#include <memdraw.h>
#include <cursor.h>
#include "screen.h"

static int mode;
static int needredraw;
static Point lastcur = {-1, -1};

/*
 * The values here are borrowed from svgalib.
 */
static void
basevgainit(VGAscr *scr)
{
	int cmap16[] = {0, 0, 0, 5, 0, 0, 7, 0, 0, 9, 0, 0, 11, 0, 0,
		5, 5, 5, 0, 7, 0, 7, 7, 7, 0, 9, 0, 0, 11, 0,
		10, 10, 10, 0, 0, 7, 0, 0, 9, 13, 13, 13, 14, 14, 14,
		15, 15, 15};
	Rectangle *r;
	ulong i;

	ilock(&scr->devlock);
	r = &(scr->gscreen->r);
	if(r->max.x - r->min.x == 320 && r->max.y - r->min.y == 200 &&
			scr->gscreen->depth == 8)
		mode = 0x13;
	else 	if(r->max.x - r->min.x == 640 && r->max.y - r->min.y == 480 &&
			scr->gscreen->depth == 4)
		mode = 0x12;
	else
		mode = 13;

	/* Put the sequencer in a reset mode */
	vgaxo(Seqx, 0x00, 0x01);

	/* Program the Misc Output and Sequencer regs */
	if(mode == 0x12)
		vgao(MiscW, 0xe3);
	else
		vgao(MiscW, 0x63);
	vgaxo(Seqx, 0x01, 0x01);
	vgaxo(Seqx, 0x02, 0x0f);
	if(mode == 0x12)
		vgaxo(Seqx, 0x04, 0x06);
	else
		vgaxo(Seqx, 0x04, 0x0e);

	/* Now do the CRTC regs */
	vgaxo(Crtx, 0x11, vgaxi(Crtx, 0x11) & 0x7f);
	vgaxo(Crtx, 0x00, 0x5f);
	vgaxo(Crtx, 0x01, 0x4f);
	vgaxo(Crtx, 0x02, 0x50);
	vgaxo(Crtx, 0x03, 0x82);
	vgaxo(Crtx, 0x04, 0x54);
	vgaxo(Crtx, 0x05, 0x80);
	if(mode == 0x12){
		vgaxo(Crtx, 0x06, 0x0b);
		vgaxo(Crtx, 0x07, 0x03e);
		vgaxo(Crtx, 0x08, 0x00);
		vgaxo(Crtx, 0x09, 0x40);
	}
	else{
		vgaxo(Crtx, 0x06, 0xbf);
		vgaxo(Crtx, 0x07, 0x1f);
		vgaxo(Crtx, 0x08, 0x00);
		vgaxo(Crtx, 0x09, 0x41);
	}
	vgaxo(Crtx, 0x0a, 0x00);
	vgaxo(Crtx, 0x0b, 0x00);
	vgaxo(Crtx, 0x0c, 0x00);
	vgaxo(Crtx, 0x0d, 0x00);
	if(mode == 0x12){
		vgaxo(Crtx, 0x10, 0xea);
		vgaxo(Crtx, 0x11, 0x8c);
		vgaxo(Crtx, 0x12, 0xdf);
		vgaxo(Crtx, 0x13, 0x28);
		vgaxo(Crtx, 0x14, 0x00);
		vgaxo(Crtx, 0x15, 0xe7);
		vgaxo(Crtx, 0x16, 0x04);
		vgaxo(Crtx, 0x17, 0xe3);
	}
	else{
		vgaxo(Crtx, 0x10, 0x9c);
		vgaxo(Crtx, 0x11, 0x8e);
		vgaxo(Crtx, 0x12, 0x8f);
		vgaxo(Crtx, 0x13, 0x28);
		vgaxo(Crtx, 0x14, 0x40);
		vgaxo(Crtx, 0x15, 0x96);
		vgaxo(Crtx, 0x16, 0xb9);
		vgaxo(Crtx, 0x17, 0xa3);
	}

	/* Next the graphics regs */
	vgaxo(Grx, 0x00, 0x00);
	vgaxo(Grx, 0x01, 0x00);
	vgaxo(Grx, 0x02, 0x00);
	vgaxo(Grx, 0x03, 0x00);
	vgaxo(Grx, 0x04, 0x00);
	if(mode == 0x12){
		vgaxo(Grx, 0x05, 0x00);
	}
	else{
		vgaxo(Grx, 0x05, 0x40);
	}
	vgaxo(Grx, 0x06, 0x05);
	vgaxo(Grx, 0x07, 0x0f);
	vgaxo(Grx, 0x08, 0xff);

	/* Finally the Attr regs */
	vgaxo(Attrx, 0x00, 0x00);
	vgaxo(Attrx, 0x01, 0x01);
	vgaxo(Attrx, 0x02, 0x02);
	vgaxo(Attrx, 0x03, 0x03);
	vgaxo(Attrx, 0x04, 0x04);
	vgaxo(Attrx, 0x05, 0x05);
	vgaxo(Attrx, 0x06, 0x06);
	vgaxo(Attrx, 0x07, 0x07);
	vgaxo(Attrx, 0x08, 0x08);
	vgaxo(Attrx, 0x09, 0x09);
	vgaxo(Attrx, 0x0a, 0x0a);
	vgaxo(Attrx, 0x0b, 0x0b);
	vgaxo(Attrx, 0x0c, 0x0c);
	vgaxo(Attrx, 0x0d, 0x0d);
	vgaxo(Attrx, 0x0e, 0x0e);
	vgaxo(Attrx, 0x0f, 0x0f);
	if(mode == 0x12)
		vgaxo(Attrx, 0x10, 0x01);
	else
		vgaxo(Attrx, 0x10, 0x41);
	vgaxo(Attrx, 0x11, 0x00);
	vgaxo(Attrx, 0x12, 0x0f);
	vgaxo(Attrx, 0x13, 0x00);
	vgaxo(Attrx, 0x14, 0x00);

	/* Turn off the reset */
	vgaxo(Seqx, 0x00, 0x03);

	/*
	 * In 640x480x4, there aren't enough colors to make
	 * the default colormap look "palettable" so we're
	 * going to fall back to a predefined colormap.
	 */
	if(mode == 0x12)
		for(i = 0; i < 16; ++i){
			setcolor(i,
				cmap16[3*i] * 0x10101010,
				cmap16[3*i+1] * 0x10101010,
				cmap16[3*i+2] * 0x10101010);
		}
	iunlock(&scr->devlock);
}

static void
basevgaflush(VGAscr *scr, Rectangle r)
{
	int i, j, k, n, rs, incs;
	uchar t, t0, t1, t2, t3;
	Memimage *s;
	uchar *p, *q;
	static uchar *plane0 = nil, *plane1 = nil, *plane2 = nil, *plane3 = nil;

	s = scr->gscreen;
	if(s == nil)
		return;
	if(rectclip(&r, s->r) == 0)
		return;
	incs = s->width * BY2WD;
	if(mode == 0x12){
		/*
		 * Mode 12 uses a screwy memory layout.  The four bits
		 * are divided into four memory planes which are accessed
		 * in parallel through a complex set of masks...
		 */
		if(plane0 == nil){
			i = (s->r.max.y * s->r.max.x + 7) / 8;
			plane0 = malloc(i);
			plane1 = malloc(i);
			plane2 = malloc(i);
			plane3 = malloc(i);
		}

		ilock(&scr->devlock);

		/* Make life easer and deal with full bytes */
		r.min.x &= ~0x07;
		r.max.x = (r.max.x + 7) & ~0x07;
		q = scr->gscreendata->bdata
			+ r.min.y * s->width * BY2WD
			+ (r.min.x * s->depth) / 8;
		rs = r.min.x / 8;
		n= (r.max.x - r.min.x) / 8;

		/* Extract the 4 planes */
		for(j = r.min.y; j < r.max.y; ++j){
			p = q;
			for(i = rs; i < rs + n; ++i){
				t0 = t1 = t2 = t3 = 0;
				for(k = 0; k < 8; ++k){
					t = *p;
					if(k & 01)
						++p;
					else
						t >>= 4;
					t0 = (t0 << 1) | t & 01;
					t >>= 1;
					t1 = (t1 << 1) | t & 01;
					t >>= 1;
					t2 = (t2 << 1) | t & 01;
					t >>= 1;
					t3 = (t3 << 1) | t & 01;
				}
				k = j * 80 + i;
				plane0[k] = t0;
				plane1[k] = t1;
				plane2[k] = t2;
				plane3[k] = t3;
			}
			q += incs;
		}
		/* Copy the planes to video memory */
		for(j = r.min.y; j < r.max.y; j++){
			k = j * 80 + rs;
			p = (uchar *)KADDR(VGAMEM()) + k;
			vgaxo(Seqx, 0x02, 0x01);
			memmove(p, plane0 + k, n);
			vgaxo(Seqx, 0x02, 0x02);
			memmove(p, plane1 + k, n);
			vgaxo(Seqx, 0x02, 0x04);
			memmove(p, plane2 + k, n);
			vgaxo(Seqx, 0x02, 0x08);
			memmove(p, plane3 + k, n);
		}
		iunlock(&scr->devlock);
	}
	else{
		q = scr->gscreendata->bdata
			+ r.min.y * s->width * BY2WD
			+ (r.min.x * s->depth) / 8;
		n = ((r.max.x - r.min.x) * s->depth) / 8;
		ilock(&scr->devlock);
		for(i = r.min.y; i < r.max.y; ++i){
			p = (uchar *)KADDR(VGAMEM()) + 320 * i + r.min.x;
			memmove(p, q, n);
			q += incs;
		}
		iunlock(&scr->devlock);
	}
	if(lastcur.x > r.min.x - CURSWID && lastcur.x < r.max.x
			&& lastcur.y > r.min.y - CURSHGT && lastcur.y < r.max.y)
		needredraw = 1;
}

VGAdev vgabasedev = {
	.name = "basevga",
	.drawinit = basevgainit,
	.flush = basevgaflush,
};
