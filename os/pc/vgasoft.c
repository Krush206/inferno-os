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

static Timer *swtimer;
static int swenabled;

static void
ptrupdate(void)
{
	cursoron(0);
}

static void
swenable(VGAscr*)
{
	if(!swenabled){
		swenabled = 1;
		swcursorinit();
		swcursorload(&cursor);
		swtimer = addclock0link(ptrupdate, 10);
	}
	else
		print("attempt to enable cursor multiple times\n");
}

static void
swdisable(VGAscr*)
{
	swenabled = 0;
	timerdel(swtimer);
	swcursorhide(1);
}

static void
swload(VGAscr*, Cursor *curs)
{
	swcursorload(curs);
}

static int
swmove(VGAscr *scr, Point p)
{
	swcursorhide(0);
	swcursordraw(p);
	return 0;
}

VGAcur vgasoftcur =
{
	"soft",
	swenable,
	swdisable,
	swload,
	swmove,
};
