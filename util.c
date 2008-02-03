/*****************************************************************************/
/**       Copyright 1988 by Evans & Sutherland Computer Corporation,        **/
/**                          Salt Lake City, Utah                           **/
/**  Portions Copyright 1989 by the Massachusetts Institute of Technology   **/
/**                        Cambridge, Massachusetts                         **/
/**                                                                         **/
/**                           All Rights Reserved                           **/
/**                                                                         **/
/**    Permission to use, copy, modify, and distribute this software and    **/
/**    its documentation  for  any  purpose  and  without  fee is hereby    **/
/**    granted, provided that the above copyright notice appear  in  all    **/
/**    copies and that both  that  copyright  notice  and  this  permis-    **/
/**    sion  notice appear in supporting  documentation,  and  that  the    **/
/**    names of Evans & Sutherland and M.I.T. not be used in advertising    **/
/**    in publicity pertaining to distribution of the  software  without    **/
/**    specific, written prior permission.                                  **/
/**                                                                         **/
/**    EVANS & SUTHERLAND AND M.I.T. DISCLAIM ALL WARRANTIES WITH REGARD    **/
/**    TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES  OF  MERCHANT-    **/
/**    ABILITY  AND  FITNESS,  IN  NO  EVENT SHALL EVANS & SUTHERLAND OR    **/
/**    M.I.T. BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL  DAM-    **/
/**    AGES OR  ANY DAMAGES WHATSOEVER  RESULTING FROM LOSS OF USE, DATA    **/
/**    OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER    **/
/**    TORTIOUS ACTION, ARISING OUT OF OR IN  CONNECTION  WITH  THE  USE    **/
/**    OR PERFORMANCE OF THIS SOFTWARE.                                     **/
/*****************************************************************************/

/*

Portions Copyright 1989, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

*/


/***********************************************************************
 *
 * $XConsortium: util.c,v 1.47 91/07/14 13:40:37 rws Exp $
 *
 * utility routines for twm
 *
 * 28-Oct-87 Thomas E. LaStrange	File created
 *
 ***********************************************************************/

#include "twm.h"
#include "util.h"
#include "gram.h"
#include "screen.h"
#include "list.h"
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/Xmu/Drawing.h>
#include <X11/Xmu/CharSet.h>
#ifndef NO_XPM_SUPPORT
#include <X11/xpm.h>
#endif

#include <stdio.h>
#include <string.h>

/* see Zoom() - djhjr - 10/11/01 */
#ifdef NEED_SELECT_H
#include <sys/select.h>
#else
#include <sys/time.h>
#include <sys/types.h>
#endif
#ifndef ZOOMSLEEP
#define ZOOMSLEEP 50000 /* arbitrary, but pleasing, msec value */
#endif

/*
 * All instances of Scr->TitleBevelWidth and Scr->BorderBevelWidth
 * were a hard value of 2 - djhjr - 4/29/98
 */

/* djhjr - 4/18/96 10/29/02 */
static void Draw3DMenuImage();
static void Draw3DDotImage();
static void Draw3DResizeImage();
static void Draw3DZoomImage();
static void Draw3DBarImage();

/* djhjr - 6/4/00 10/29/02 */
static void Draw3DRArrowImage();
static void Draw3DDArrowImage();

/* djhjr - 1/13/98 10/20/02 */
void setBorderGC();
#ifdef USE_ORIGINAL_CORNERS
void Draw3DCorner();
#else
GC setBevelGC();
void Draw3DBevel();
void Draw3DNoBevel();
#endif

/* sjr - 9/25/06 */
#if 0
/* djhjr - 4/19/96 */
static GC     rootGC = (GC) 0;
#endif

/* for trying to clean up BeNiceToColormap - djhjr - 10/20/02 */
static int borderdashoffset;

int HotX, HotY;

#define questionmark_width (unsigned int)(8)
#define questionmark_height (unsigned int)(8)
static char questionmark_bits[] = {
   0x38, 0x7c, 0x64, 0x30, 0x18, 0x00, 0x18, 0x18};

/***********************************************************************
 *
 *  Procedure:
 *	MoveOutline - move a window outline
 *
 *  Inputs:
 *	root	    - the window we are outlining
 *	x	    - upper left x coordinate
 *	y	    - upper left y coordinate
 *	width	    - the width of the rectangle
 *	height	    - the height of the rectangle
 *      bw          - the border width of the frame
 *      th          - title height
 *
 ***********************************************************************
 */

/* ARGSUSED */
void MoveOutline(root, x, y, width, height, bw, th)
    Window root;
    int x, y, width, height, bw, th;
{
    static int	lastx = 0;
    static int	lasty = 0;
    static int	lastWidth = 0;
    static int	lastHeight = 0;
    static int	lastBW = 0;
    static int	lastTH = 0;
    int		xl, xr, yt, yb, xinnerl, xinnerr, yinnert, yinnerb;
    int		xthird, ythird;
    XSegment	outline[18];
    register XSegment	*r;

    if (x == lastx && y == lasty && width == lastWidth && height == lastHeight
	&& lastBW == bw && th == lastTH)
	return;

    r = outline;

#ifdef ORIGINAL_DRAWIT
#define DRAWIT() \
    if (lastWidth || lastHeight)			\
    {							\
	xl = lastx;					\
	xr = lastx + lastWidth - 1;			\
	yt = lasty;					\
	yb = lasty + lastHeight - 1;			\
	xinnerl = xl + lastBW;				\
	xinnerr = xr - lastBW;				\
	yinnert = yt + lastTH + lastBW;			\
	yinnerb = yb - lastBW;				\
	xthird = (xinnerr - xinnerl) / 3;		\
	ythird = (yinnerb - yinnert) / 3;		\
							\
	/* frame outline */						\
	r->x1 = xl;					\
	r->y1 = yt;					\
	r->x2 = xr;					\
	r->y2 = yt;					\
	r++;						\
							\
	r->x1 = xl;					\
	r->y1 = yb;					\
	r->x2 = xr;					\
	r->y2 = yb;					\
	r++;						\
							\
	r->x1 = xl;					\
	r->y1 = yt;					\
	r->x2 = xl;					\
	r->y2 = yb;					\
	r++;						\
							\
	r->x1 = xr;					\
	r->y1 = yt;					\
	r->x2 = xr;					\
	r->y2 = yb;					\
	r++;						\
							\
	/* left vertical */						\
	r->x1 = xinnerl + xthird;			\
	r->y1 = yinnert;				\
	r->x2 = r->x1;					\
	r->y2 = yinnerb;				\
	r++;						\
							\
	/* right vertical */						\
	r->x1 = xinnerl + (2 * xthird);			\
	r->y1 = yinnert;				\
	r->x2 = r->x1;					\
	r->y2 = yinnerb;				\
	r++;						\
							\
	/* top horizontal */						\
	r->x1 = xinnerl;				\
	r->y1 = yinnert + ythird;			\
	r->x2 = xinnerr;				\
	r->y2 = r->y1;					\
	r++;						\
							\
	/* bottom horizontal */						\
	r->x1 = xinnerl;				\
	r->y1 = yinnert + (2 * ythird);			\
	r->x2 = xinnerr;				\
	r->y2 = r->y1;					\
	r++;						\
							\
	/* title bar */								\
	if (lastTH != 0) {				\
	    r->x1 = xl;					\
	    r->y1 = yt + lastTH;			\
	    r->x2 = xr;					\
	    r->y2 = r->y1;				\
	    r++;					\
	}						\
    }

    /* undraw the old one, if any */
    DRAWIT ();

    lastx = x;
    lasty = y;
    lastWidth = width;
    lastHeight = height;
    lastBW = bw;
    lastTH = th;

    /* draw the new one, if any */
    DRAWIT ();
#else
#define DRAWIT() \
    if (lastWidth || lastHeight)			\
    {							\
	xl = lastx;					\
	xr = lastx + lastWidth - 1;			\
	yt = lasty;					\
	yb = lasty + lastHeight - 1;			\
	xinnerl = xl + lastBW;				\
	xinnerr = xr - lastBW;				\
	yinnert = yt + lastTH + lastBW;			\
	yinnerb = yb - lastBW;				\
	xthird = (xinnerr - xinnerl) / 3;		\
	ythird = (yinnerb - yinnert) / 3;		\
							\
	/* frame outline */						\
	r->x1 = xl;					\
	r->y1 = yt;					\
	r->x2 = xr;					\
	r->y2 = yt;					\
	r++;						\
							\
	r->x1 = xl;					\
	r->y1 = yb;					\
	r->x2 = xr;					\
	r->y2 = yb;					\
	r++;						\
							\
	r->x1 = xl;					\
	r->y1 = yt;					\
	r->x2 = xl;					\
	r->y2 = yb;					\
	r++;						\
							\
	r->x1 = xr;					\
	r->y1 = yt;					\
	r->x2 = xr;					\
	r->y2 = yb;					\
	r++;						\
							\
	/* top-left to bottom-right */		\
	r->x1 = xinnerl;			\
	r->y1 = yinnert;			\
	r->x2 = xinnerr;			\
	r->y2 = yinnerb;			\
	r++;				\
							\
	/* bottom-left to top-right */		\
	r->x1 = xinnerl;			\
	r->y1 = yinnerb;			\
	r->x2 = xinnerr;			\
	r->y2 = yinnert;			\
	r++;				\
							\
	/* title bar */						\
	if (lastTH != 0) {				\
	    r->x1 = xl;					\
	    r->y1 = yt + lastTH;			\
	    r->x2 = xr;					\
	    r->y2 = r->y1;				\
	    r++;					\
	}						\
    }

    /* undraw the old one, if any */
    DRAWIT ();

    lastx = x;
    lasty = y;
    lastWidth = width;
    lastHeight = height;
    lastBW = bw;
    lastTH = th;

    /* draw the new one, if any */
    DRAWIT ();
#endif

#undef DRAWIT

    if (r != outline)
    {
	XDrawSegments(dpy, root, Scr->DrawGC, outline, r - outline);
    }
}

/***********************************************************************
 *
 *  Procedure:
 *	Zoom - zoom in or out of an icon
 *
 *  Inputs:
 *	wf	- window to zoom from
 *	ipf	- icon manager of window to zoom from
 *	wt	- window to zoom to
 *	ipt	- icon manager of window to zoom to
 *
 *	Patched to make sure a "None" window generates coordinates *INSIDE*
 *	the screen. -- DSE 
 *
 *	Added args to icon managers so zooms from and to the window's
 *	entry use that icon manager's coordinates - djhjr - 10/11/01 
 *
 ***********************************************************************
 */

void
Zoom(wf, ipf, wt, ipt)
    Window wf, wt;
    IconMgr *ipf, *ipt;
{
    int fx, fy, tx, ty;			/* from, to */
    unsigned int fw, fh, tw, th;	/* from, to */
    long dx, dy, dw, dh;
    long z;
    int j;

    /* djhjr - 10/11/01 */
    static struct timeval timeoutval = {0, ZOOMSLEEP};
    struct timeval timeout;
    
    void draw_rect(); /* DSE */
    
    if (!Scr->DoZoom || Scr->ZoomCount < 1) return;

#if (0)
    if (wf == None || wt == None) return;

    XGetGeometry (dpy, wf, &JunkRoot, &fx, &fy, &fw, &fh, &JunkBW, &JunkDepth);
    XGetGeometry (dpy, wt, &JunkRoot, &tx, &ty, &tw, &th, &JunkBW, &JunkDepth);
#else

    /* if (wf == None && wt == None) return; */

	if ( wt == None)
		{
		if (Scr->LessRandomZoomZoom) /* DSE */
			{
			int temp,x1,y1,x2,y2;
			x1 = ( (int)rand() % (Scr->MyDisplayWidth >> 3) );
			x2 = Scr->MyDisplayWidth - 
			       ( (int)rand() % (Scr->MyDisplayWidth >> 3) );
			y1 = ( (int)rand() % (Scr->MyDisplayHeight >> 3) );
			y2 = Scr->MyDisplayHeight - 
			       ( (int)rand() % (Scr->MyDisplayHeight >> 3) );
			if(x1>x2){temp=x1;x1=x2;x2=temp;}
			if(y1>y2){temp=y1;y1=y2;y2=temp;}
			tx = x1; ty = y1;
			tw = x2 - x1; th = y2 - y1;
			}
		else
			{
			/* Zoom from nowhere, RFBZOOM */
			tx = ( (int)rand() % Scr->MyDisplayWidth );
			tw = ( (int)rand() % Scr->MyDisplayWidth );
			ty = ( (int)rand() % Scr->MyDisplayWidth );
			th = ( (int)rand() % Scr->MyDisplayWidth );
			}
		}
	else
		{	/* Normal. */
		XGetGeometry (dpy, wt, &JunkRoot,
			&tx, &ty, &tw, &th, &JunkBW, &JunkDepth);

		if (ipt)
		{
			tx += (ipt->twm_win->frame_x + ipt->twm_win->title_x);
			ty += (ipt->twm_win->frame_y + ipt->twm_win->title_y +
					ipt->twm_win->title_height);
		}
		}

	if ( wf == None )
		{
		if (Scr->LessRandomZoomZoom) /* DSE */
			{
			/* zoom from somewhere on the screen, DSE */
			int temp,x1,y1,x2,y2;
			do
				{
				x1 = ( (int)rand() % Scr->MyDisplayWidth );
				x2 = ( (int)rand() % Scr->MyDisplayWidth );
				y1 = ( (int)rand() % Scr->MyDisplayHeight );
				y2 = ( (int)rand() % Scr->MyDisplayHeight );
				if(x1>x2){temp=x1;x1=x2;x2=temp;}
				if(y1>y2){temp=y1;y1=y2;y2=temp;}
				fx = x1; fy = y1;
				fw = x2 - x1; fh = y2 - y1;
				}
			while ( fw > (Scr->MyDisplayWidth >> 2) || fh > (Scr->MyDisplayHeight >> 2) );
			}
		else
			{
			/* Zoom from nowhere, RFB */
			/* fx = ( rand() & 1 ) * Scr->MyDisplayWidth; */
			fx = ( (int)rand() % Scr->MyDisplayWidth );
			fw = ( (int)rand() % Scr->MyDisplayWidth );
			fy = ( (int)rand() % Scr->MyDisplayWidth );
			fh = ( (int)rand() % Scr->MyDisplayWidth );
			}
		}

	else
		{	/* Normal. */
		XGetGeometry (dpy, wf, &JunkRoot,
			&fx, &fy, &fw, &fh, &JunkBW, &JunkDepth);

		if (ipf)
		{
			fx += (ipf->twm_win->frame_x + ipf->twm_win->title_x);
			fy += (ipf->twm_win->frame_y + ipf->twm_win->title_y +
					ipf->twm_win->title_height);
		}
		}
#endif

    dx = ((long) (tx - fx));	/* going from -> to */
    dy = ((long) (ty - fy));	/* going from -> to */
    dw = ((long) (tw - fw));	/* going from -> to */
    dh = ((long) (th - fh));	/* going from -> to */
    z = (long) (Scr->ZoomCount + 1);

    for (j = 0; j < 2; j++) {
	long i;

	draw_rect (dpy, Scr->Root, Scr->DrawGC, fx, fy, fw, fh); /* DSE */
	for (i = 1; i < z; i++)
		{
	    int x = fx + (int) ((dx * i) / z);
	    int y = fy + (int) ((dy * i) / z);
	    unsigned width = (unsigned) (((long) fw) + (dw * i) / z);
	    unsigned height = (unsigned) (((long) fh) + (dh * i) / z);

	    draw_rect (dpy, Scr->Root, Scr->DrawGC, x, y, width, height); /* DSE */
		}
	draw_rect (dpy, Scr->Root, Scr->DrawGC, tx, ty, tw, th); /* DSE */

	/* djhjr - 10/11/01 */
	timeout = timeoutval;
	select(0, 0, 0, 0, &timeout);
    }
}


/*
 *	Use any routine to draw your own rectangles here. -- DSE
 */
void draw_rect (display,drawable,gc,x,y,width,height) /* DSE */
		Display *display;
		Drawable drawable;
		GC gc;
		int x,y;
		unsigned int width,height;
	{
	void draw_scaled_rect();
	draw_scaled_rect (display,drawable,gc,x,y,width,height, 20,20);
	if (Scr->PrettyZoom)
		{
		draw_scaled_rect (display,drawable,gc,x,y,width,height, 18,20);
		draw_scaled_rect (display,drawable,gc,x,y,width,height, 16,20);
		}
	}
void draw_scaled_rect (display,drawable,gc,x,y,
                       width,height,scale,over) /* DSE */
		Display *display;
		Drawable drawable;
		GC gc;
		int x,y;
		unsigned int width,height;
		unsigned int scale,over;
	{
	XDrawRectangle(dpy,drawable,gc,
		x + ( over + width * (over - scale) ) / (2 * over),
		  y + ( over  + width * (over - scale) ) / (2 * over),
		( (over / 2) + width * scale ) / over,
		  ( (over / 2) + height * scale ) / over
		);
	}

/***********************************************************************
 *
 *  Procedure:
 *	ExpandFilename - expand the tilde character to HOME
 *		if it is the first character of the filename
 *
 *  Returned Value:
 *	a pointer to the new name
 *
 *  Inputs:
 *	name	- the filename to expand
 *
 ***********************************************************************
 */

char *
ExpandFilename(name)
char *name;
{
    char *newname;

    if (name[0] != '~') return name;

    newname = (char *) malloc (HomeLen + strlen(name) + 2);
    if (!newname) {
	fprintf (stderr,
		 "%s:  unable to allocate %d bytes to expand filename %s/%s\n",
		 ProgramName, HomeLen + strlen(name) + 2, Home, &name[1]);
    } else {
	(void) sprintf (newname, "%s/%s", Home, &name[1]);
    }

    return newname;
}

/***********************************************************************
 *
 *  Procedure:
 *	GetUnknownIcon - read in the bitmap file for the unknown icon
 *
 *  Inputs:
 *	name - the filename to read
 *
 ***********************************************************************
 */

void
GetUnknownIcon(name)
char *name;
{
/* djhjr - 8/13/98 */
#ifdef ORIGINAL_PIXMAPS
    if ((Scr->UnknownPm = GetBitmap(name)) != None)
    {
	XGetGeometry(dpy, Scr->UnknownPm, &JunkRoot, &JunkX, &JunkY,
	    (unsigned int *)&Scr->UnknownWidth, (unsigned int *)&Scr->UnknownHeight, &JunkBW, &JunkDepth);
    }
#else
	Scr->unknownName = name;
#endif
}

/***********************************************************************
 *
 *  Procedure:
 *	FindBitmap - read in a bitmap file and return size
 *
 *  Returned Value:
 *	the pixmap associated with the bitmap
 *      widthp	- pointer to width of bitmap
 *      heightp	- pointer to height of bitmap
 *
 *  Inputs:
 *	name	- the filename to read
 *
 ***********************************************************************
 */

Pixmap FindBitmap (name, widthp, heightp)
    char *name;
    unsigned int *widthp, *heightp;
{
    char *bigname;
    Pixmap pm;

    if (!name) return None;

    /*
     * Generate a full pathname if any special prefix characters (such as ~)
     * are used.  If the bigname is different from name, bigname will need to
     * be freed.
     */
    bigname = ExpandFilename (name);
    if (!bigname) return None;

    /*
     * look along bitmapFilePath resource same as toolkit clients
     */
    pm = XmuLocateBitmapFile (ScreenOfDisplay(dpy, Scr->screen), bigname, NULL,
			      0, (int *)widthp, (int *)heightp, &HotX, &HotY);
    if (pm == None && Scr->IconDirectory && bigname[0] != '/') {
	if (bigname != name) free (bigname);
	/*
	 * Attempt to find icon in old IconDirectory (now obsolete)
	 */
	bigname = (char *) malloc (strlen(name) + strlen(Scr->IconDirectory) +
				   2);
	if (!bigname) {
	    fprintf (stderr,
		     "%s:  unable to allocate memory for \"%s/%s\"\n",
		     ProgramName, Scr->IconDirectory, name);
	    return None;
	}
	(void) sprintf (bigname, "%s/%s", Scr->IconDirectory, name);
	if (XReadBitmapFile (dpy, Scr->Root, bigname, widthp, heightp, &pm,
			     &HotX, &HotY) != BitmapSuccess) {
	    pm = None;
	}
    }
    if (bigname != name) free (bigname);

#ifdef DEBUG
    if (pm == None) {
	fprintf (stderr, "%s:  unable to find bitmap \"%s\"\n",
		 ProgramName, name);
    }
#endif

    return pm;
}

Pixmap GetBitmap (name)
    char *name;
{
    return FindBitmap (name, &JunkWidth, &JunkHeight);
}

#ifndef NO_XPM_SUPPORT
/*
 * Submitted by Jason Gloudon
 * added color argument - djhjr - 9/28/99
 */
Image *FindImage (name, color)
    char *name;
    Pixel color;
{
    char *bigname;
    Pixmap pixmap,mask;
    Image *newimage = None; /* added intialization - djhjr - 3/21/98 */
    XpmAttributes attributes;
	XpmColorSymbol xpmcolor[1]; /* djhjr - 9/28/99 */
    int ErrorStatus;

    /*
     * Generate a full pathname if any special prefix characters (such as ~)
     * are used.  If the bigname is different from name, bigname will need to
     * be freed.
     */
    bigname = ExpandFilename (name);
    if (!bigname) return None;

    /* was 'XpmReturnAllocPixels' - Submitted by Takeharu Kato */
    attributes.valuemask = XpmReturnPixels;

    /* djhjr - 5/10/99 */
    attributes.valuemask |= XpmCloseness;
    attributes.closeness = 32768;

	/* djhjr - 9/28/99 */
	if (color)
	{
		xpmcolor[0].name = NULL;
		xpmcolor[0].value = "none";
		xpmcolor[0].pixel = color;

		attributes.colorsymbols = xpmcolor;
		attributes.numsymbols = 1;
		attributes.valuemask |= XpmColorSymbols;
	}

    /*
     * By default, the XPM library assumes screen 0, so we have
     * to pass in the real values. Submitted by Caveh Frank Jalali
     */
    attributes.valuemask |= XpmVisual | XpmColormap | XpmDepth;
    attributes.visual = Scr->d_visual;
    attributes.colormap = XDefaultColormap(dpy, Scr->screen);
    attributes.depth = Scr->d_depth;

    if( (ErrorStatus = XpmReadFileToPixmap(dpy, Scr->Root, bigname, &pixmap,
					   &mask, &attributes)) != XpmSuccess){
      pixmap = None;
    }

	/*
	 * Do for pixmaps what XmuLocateBitmapFile() does for bitmaps,
	 * because, apparently, XmuLocatePixmapFile() doesn't!
	 *
	 * djhjr - 12/26/98
	 */
	if (pixmap == None && Scr->BitmapFilePath && bigname[0] != '/')
	{
		char *path = Scr->BitmapFilePath, *term;

		do
		{
			if ((term = strchr(path, ':'))) *term = 0;

			if (bigname != name) free(bigname);
			if (!(bigname = (char *)malloc(strlen(name) + strlen(path) + 2)))
				fprintf(stderr, "%s:  unable to allocate memory for \"%s/%s\"\n",
						ProgramName, path, name);
			else
			{
				(void)sprintf(bigname, "%s/%s", path, name);

#ifdef NEVER /* once, at the top, is enough? - djhjr - 12/26/98 */
				/* was 'XpmReturnAllocPixels' - Submitted by Takeharu Kato */
				attributes.valuemask = XpmReturnPixels;

				/* djhjr - 5/10/99 */
				attributes.valuemask |= XpmCloseness;
				attributes.closeness = 32768;

				/* djhjr - 9/28/99 */
				if (color)
				{
					xpmcolor[0].name = NULL;
					xpmcolor[0].value = "none";
					xpmcolor[0].pixel = color;

					attributes.colorsymbols = xpmcolor;
					attributes.numsymbols = 1;
					attributes.valuemask |= XpmColorSymbols;
				}

				/*
				 * By default, the XPM library assumes screen 0, so we have
				 * to pass in the real values. Submitted by Caveh Frank Jalali
				 */
				attributes.valuemask |= XpmVisual | XpmColormap | XpmDepth;
				attributes.visual = Scr->d_visual;
				attributes.colormap = XDefaultColormap(dpy, Scr->screen);
				attributes.depth = Scr->d_depth;
#endif /* NEVER */

				ErrorStatus = XpmReadFileToPixmap(dpy, Scr->Root, bigname,
						&pixmap, &mask, &attributes);
			}

			if (term)
			{
				*term = ':';
				path = term + 1;
			}
			else
				path = NULL;

			if (ErrorStatus != XpmSuccess)
				pixmap = None;
			else
				break;

		} while (path);
	}

    if (pixmap == None && Scr->IconDirectory && bigname[0] != '/') {
      if (bigname != name) free (bigname);
      /*
       * Attempt to find icon pixmap in old IconDirectory (now obsolete)
       */
      bigname = (char *) malloc (strlen(name) + strlen(Scr->IconDirectory) + 2);
      if (!bigname) {
	fprintf (stderr,
		 "%s:  unable to allocate memory for \"%s/%s\"\n",
		 ProgramName, Scr->IconDirectory, name);
	return None;
      }
      (void) sprintf (bigname, "%s/%s", Scr->IconDirectory, name);

#ifdef NEVER /* once, at the top, is enough? - djhjr - 12/26/98 */
      /* was 'XpmReturnAllocPixels' - Submitted by Takeharu Kato */
      attributes.valuemask = XpmReturnPixels;

      /* djhjr - 5/10/99 */
      attributes.valuemask |= XpmCloseness;
      attributes.closeness = 32768;

      /*
       * By default, the XPM library assumes screen 0, so we have
       * to pass in the real values. Submitted by Caveh Frank Jalali
       */
      attributes.valuemask |= XpmVisual | XpmColormap | XpmDepth;
      attributes.visual = Scr->d_visual;
      attributes.colormap = XDefaultColormap(dpy, Scr->screen);
      attributes.depth = Scr->d_depth;
#endif /* NEVER */

      ErrorStatus = XpmReadFileToPixmap(dpy, Scr->Root, bigname, &pixmap,
					&mask, &attributes);
    }

    if(ErrorStatus == XpmSuccess){
      newimage = (Image *)malloc(sizeof(Image));
      newimage->pixmap = pixmap;
      newimage->mask = mask;
      newimage->height = attributes.height;
      newimage->width = attributes.width;
    }
    else {
      fprintf (stderr, "%s:  unable to find pixmap \"%s\"\n",
	       ProgramName, name);
    }

    if (bigname != name) free (bigname);
    return newimage;
}
#endif /* NO_XPM_SUPPORT */

void InsertRGBColormap (a, maps, nmaps, replace)
    Atom a;
    XStandardColormap *maps;
    int nmaps;
    Bool replace;
{
    StdCmap *sc = NULL;

    if (replace) {			/* locate existing entry */
	for (sc = Scr->StdCmapInfo.head; sc; sc = sc->next) {
	    if (sc->atom == a) break;
	}
    }

    if (!sc) {				/* no existing, allocate new */
	sc = (StdCmap *) malloc (sizeof (StdCmap));
	if (!sc) {
	    fprintf (stderr, "%s:  unable to allocate %d bytes for StdCmap\n",
		     ProgramName, sizeof (StdCmap));
	    return;
	}
    }

    if (replace) {			/* just update contents */
	if (sc->maps) XFree ((char *) maps);
	if (sc == Scr->StdCmapInfo.mru) Scr->StdCmapInfo.mru = NULL;
    } else {				/* else appending */
	sc->next = NULL;
	sc->atom = a;
	if (Scr->StdCmapInfo.tail) {
	    Scr->StdCmapInfo.tail->next = sc;
	} else {
	    Scr->StdCmapInfo.head = sc;
	}
	Scr->StdCmapInfo.tail = sc;
    }
    sc->nmaps = nmaps;
    sc->maps = maps;

    return;
}

/*
 * SetPixmapsPixmap - load the Image structure for the Pixmaps resource images
 *
 * djhjr - 5/17/98
 */
Image *SetPixmapsPixmap(filename)
char *filename;
{
	Pixmap bm;
	Image *image = NULL;

	bm = FindBitmap(filename, &JunkWidth, &JunkHeight);
	if (bm != None)
	{
		image = (Image *)malloc(sizeof(Image));
	    
		image->width = JunkWidth;
		image->height = JunkHeight;
		image->mask = None;

		image->pixmap = bm;
	}
#ifndef NO_XPM_SUPPORT
	else /* Try to find a pixmap file with this name */

		/* added color argument - djhjr - 9/28/99 */
		image = FindImage(filename, 0);
#endif

	return image;
}

/*
 * SetPixmapsBackground - set the background for the Pixmaps resource images
 *
 * djhjr - 5/23/98
 * used to receive (int *)numcolors and return (Image *)image - djhjr - 9/2/98
 */
#ifndef NO_XPM_SUPPORT
int SetPixmapsBackground(image, drawable, color)
Image *image;
Drawable drawable;
Pixel color;
{
	XpmImage xpmimage;
	XpmAttributes xpmattr;
	XpmColorSymbol xpmcolor[1];
	unsigned int i;

	/* djhjr - 5/10/99 */
	xpmattr.valuemask = XpmCloseness;
	xpmattr.closeness = 32768;

	/*
	 * By default, the XPM library assumes screen 0, so we have
	 * to pass in the real values. Submitted by Caveh Frank Jalali
	 */
	xpmattr.valuemask |= XpmVisual | XpmColormap | XpmDepth;
	xpmattr.visual = Scr->d_visual;
	xpmattr.colormap = XDefaultColormap(dpy, Scr->screen);
	xpmattr.depth = Scr->d_depth;

	if (XpmCreateXpmImageFromPixmap(dpy, image->pixmap, image->mask,
			&xpmimage, &xpmattr) != XpmSuccess)
		return (0);

	for (i = 0; i < xpmimage.ncolors; i++)
		if (!strcmp(xpmimage.colorTable[i].c_color, "None"))
			break;

	if (i < xpmimage.ncolors)
	{
		/* djhjr - 9/2/98 */
		XFreePixmap(dpy, image->pixmap);
		XFreePixmap(dpy, image->mask);

		xpmcolor[0].name = NULL;
		xpmcolor[0].value = "none";
		xpmcolor[0].pixel = color;

#ifdef NEVER /* once, at the top, is enough? - djhjr - 2/13/99 */
		xpmattr.colorsymbols = xpmcolor;
		xpmattr.numsymbols = 1;
		xpmattr.valuemask = XpmColorSymbols;

		/* djhjr - 5/10/99 */
		xpmattr.valuemask |= XpmCloseness;
		xpmattr.closeness = 32768;

		/*
		 * By default, the XPM library assumes screen 0, so we have
		 * to pass in the real values. Submitted by Caveh Frank Jalali
		 */
		xpmattr.valuemask |= XpmVisual | XpmColormap | XpmDepth;
		xpmattr.visual = Scr->d_visual;
		xpmattr.colormap = XDefaultColormap(dpy, Scr->screen);
		xpmattr.depth = Scr->d_depth;
#else
		xpmattr.colorsymbols = xpmcolor;
		xpmattr.numsymbols = 1;
		xpmattr.valuemask |= XpmColorSymbols;
#endif /* NEVER */

		XpmCreatePixmapFromXpmImage(dpy, drawable, &xpmimage,
				&image->pixmap, &image->mask, &xpmattr);
	}

	i = xpmimage.ncolors;
	XpmFreeXpmImage(&xpmimage);

	return (i);
}
#endif /* NO_XPM_SUPPORT */

void RemoveRGBColormap (a)
    Atom a;
{
    StdCmap *sc, *prev;

    prev = NULL;
    for (sc = Scr->StdCmapInfo.head; sc; sc = sc->next) {
	if (sc->atom == a) break;
	prev = sc;
    }
    if (sc) {				/* found one */
	if (sc->maps) XFree ((char *) sc->maps);
	if (prev) prev->next = sc->next;
	if (Scr->StdCmapInfo.head == sc) Scr->StdCmapInfo.head = sc->next;
	if (Scr->StdCmapInfo.tail == sc) Scr->StdCmapInfo.tail = prev;
	if (Scr->StdCmapInfo.mru == sc) Scr->StdCmapInfo.mru = NULL;
    }
    return;
}

void LocateStandardColormaps()
{
    Atom *atoms;
    int natoms;
    int i;

    atoms = XListProperties (dpy, Scr->Root, &natoms);
    for (i = 0; i < natoms; i++) {
	XStandardColormap *maps = NULL;
	int nmaps;

	if (XGetRGBColormaps (dpy, Scr->Root, &maps, &nmaps, atoms[i])) {
	    /* if got one, then append to current list */
	    InsertRGBColormap (atoms[i], maps, nmaps, False);
	}
    }
    if (atoms) XFree ((char *) atoms);
    return;
}

void GetColor(kind, what, name)
int kind;
Pixel *what;
char *name;
{
    XColor color, junkcolor;
    Status stat = 0;
    Colormap cmap = Scr->TwmRoot.cmaps.cwins[0]->colormap->c;

#ifndef TOM
    if (!Scr->FirstTime) return;
#endif

    if (Scr->Monochrome != kind) return;

#if ( XlibSpecificationRelease < 5 )
    /* eyckmans@imec.be */
	if ( ! ( ( name[0] == '#')
			? ( (stat = XParseColor (dpy, cmap, name, &color))
				&& XAllocColor (dpy, cmap, &color))
			: XAllocNamedColor (dpy, cmap, name, &color, &junkcolor)))
#else
    if (!XAllocNamedColor (dpy, cmap, name, &color, &junkcolor))
#endif
    {
	/* if we could not allocate the color, let's see if this is a
	 * standard colormap
	 */
	XStandardColormap *stdcmap = NULL;

	/* parse the named color */
	if (name[0] != '#')
	    stat = XParseColor (dpy, cmap, name, &color);
	if (!stat)
	{
	    fprintf (stderr, "%s:  invalid color name \"%s\"\n",
		     ProgramName, name);
	    return;
	}

	/*
	 * look through the list of standard colormaps (check cache first)
	 */
	if (Scr->StdCmapInfo.mru && Scr->StdCmapInfo.mru->maps &&
	    (Scr->StdCmapInfo.mru->maps[Scr->StdCmapInfo.mruindex].colormap ==
	     cmap)) {
	    stdcmap = &(Scr->StdCmapInfo.mru->maps[Scr->StdCmapInfo.mruindex]);
	} else {
	    StdCmap *sc;

	    for (sc = Scr->StdCmapInfo.head; sc; sc = sc->next) {
		int i;

		for (i = 0; i < sc->nmaps; i++) {
		    if (sc->maps[i].colormap == cmap) {
			Scr->StdCmapInfo.mru = sc;
			Scr->StdCmapInfo.mruindex = i;
			stdcmap = &(sc->maps[i]);
			goto gotit;
		    }
		}
	    }
	}

      gotit:
	if (stdcmap) {
            color.pixel = (stdcmap->base_pixel +
			   ((Pixel)(((float)color.red / 65535.0) *
				    stdcmap->red_max + 0.5) *
			    stdcmap->red_mult) +
			   ((Pixel)(((float)color.green /65535.0) *
				    stdcmap->green_max + 0.5) *
			    stdcmap->green_mult) +
			   ((Pixel)(((float)color.blue  / 65535.0) *
				    stdcmap->blue_max + 0.5) *
			    stdcmap->blue_mult));
        } else {
	    fprintf (stderr, "%s:  unable to allocate color \"%s\"\n",
		     ProgramName, name);
	    return;
	}
    }

    *what = color.pixel;
}

void GetShadeColors (cp)
ColorPair *cp;
{
    XColor	xcol;
    Colormap	cmap = Scr->TwmRoot.cmaps.cwins[0]->colormap->c;
    int		save;
    float	clearfactor;
    float	darkfactor;
    char	clearcol [32], darkcol [32];

    clearfactor = (float) Scr->ClearBevelContrast / 100.0;
    darkfactor  = (100.0 - (float) Scr->DarkBevelContrast)  / 100.0;
    xcol.pixel = cp->back;
    XQueryColor (dpy, cmap, &xcol);

    sprintf (clearcol, "#%04x%04x%04x",
		xcol.red   + (unsigned short) ((65535 -   xcol.red) * clearfactor),
		xcol.green + (unsigned short) ((65535 - xcol.green) * clearfactor),
		xcol.blue  + (unsigned short) ((65535 -  xcol.blue) * clearfactor));
    sprintf (darkcol,  "#%04x%04x%04x",
		(unsigned short) (xcol.red   * darkfactor),
		(unsigned short) (xcol.green * darkfactor),
		(unsigned short) (xcol.blue  * darkfactor));

    save = Scr->FirstTime;
    Scr->FirstTime = True;
    GetColor (Scr->Monochrome, &cp->shadc, clearcol);
    GetColor (Scr->Monochrome, &cp->shadd,  darkcol);
    Scr->FirstTime = save;
}

/* 
 * The following I18N-oriented functions are adapted from TWM
 * as distributed with XFree86 4.2.0 - djhjr - 9/14/03
 */

/*
 * The following functions are sensible to 'use_fontset'.
 * When 'use_fontset' is True,
 *  - XFontSet-related internationalized functions are used
 *     so as multibyte languages can be displayed.
 * When 'use_fontset' is False,
 *  - XFontStruct-related conventional functions are used
 *     so as 8-bit characters can be displayed even when
 *     locale is not set properly.
 */
void
GetFont(font)
MyFont *font;
{
    char **missing_charset_list_return;
    int missing_charset_count_return;
    char *def_string_return;
    XFontSetExtents *font_extents;
    XFontStruct **xfonts;
    char **font_names;
    register int i;
    int ascent;
    int descent;
    int fnum;
    char *basename2, *basename3 = NULL;

    if (use_fontset)
    {
	if (font->fontset != NULL)
	    XFreeFontSet(dpy, font->fontset);

	if (!font->name)
	    font->name = Scr->DefaultFont.name;
	if ((basename2 = (char *)malloc(strlen(font->name) + 3)))
	    sprintf(basename2, "%s,*", font->name);
	else
	    basename2 = font->name;
	if ((font->fontset = XCreateFontSet(dpy, basename2,
					    &missing_charset_list_return,
					    &missing_charset_count_return,
					    &def_string_return)) == NULL)
	{
	    /* idea from Seth Robertson - djhjr - 9/17/03 */

	    if ((basename3 = (char *)realloc(basename2,
					strlen(Scr->DefaultFont.name) + 3)))
		sprintf(basename3, "%s,*", Scr->DefaultFont.name);
	    else
	    {
		basename3 = Scr->DefaultFont.name;
		if (basename2 != font->name)
		    free(basename2);
	    }
	    if ((font->fontset = XCreateFontSet(dpy, basename3,
						&missing_charset_list_return,
						&missing_charset_count_return,
						&def_string_return)) == NULL)
	    {
		fprintf(stderr,
			"%s: unable to open fontsets \"%s\" or \"%s\"\n",
			ProgramName, font->name, Scr->DefaultFont.name);
		if (basename3 != Scr->DefaultFont.name)
		    free(basename3);
		exit(1);
	    }
	    basename2 = basename3;
	}

	if (basename2 != ((basename3) ? Scr->DefaultFont.name : font->name))
	    free(basename2);

	for (i = 0; i < missing_charset_count_return; i++)
	    fprintf(stderr, "%s: font for charset %s is lacking\n",
		   ProgramName, missing_charset_list_return[i]);

	font_extents = XExtentsOfFontSet(font->fontset);
	fnum = XFontsOfFontSet(font->fontset, &xfonts, &font_names);
	for (i = 0, ascent = 0, descent = 0; i<fnum; i++)
	{
	    if (ascent < (*xfonts)->ascent)
		ascent = (*xfonts)->ascent;
	    if (descent < (*xfonts)->descent)
		descent = (*xfonts)->descent;
	    xfonts++;
	}

	font->height = font_extents->max_logical_extent.height;
	font->y = ascent;
	font->ascent = ascent;
	font->descent = descent;
	return;
    }

    if (font->font != NULL)
	XFreeFont(dpy, font->font);

    if ((font->font = XLoadQueryFont(dpy, font->name)) == NULL)
	if ((font->font = XLoadQueryFont(dpy, Scr->DefaultFont.name)) == NULL)
	{
	    fprintf(stderr, "%s:  unable to open fonts \"%s\" or \"%s\"\n",
		    ProgramName, font->name, Scr->DefaultFont.name);
	    exit(1);
	}

    font->height = font->font->ascent + font->font->descent;
    font->y = font->font->ascent;
    font->ascent = font->font->ascent;
    font->descent = font->font->descent;
}

int
MyFont_TextWidth(font, string, len)
    MyFont *font;
    char *string;
    int len;
{
    XRectangle ink_rect;
    XRectangle logical_rect;

    if (use_fontset) {
	XmbTextExtents(font->fontset, string, len, &ink_rect, &logical_rect);
	return logical_rect.width;
    }
    return XTextWidth(font->font, string, len);
}

void
MyFont_DrawImageString(dpy, d, font, gc, x, y, string, len)
    Display *dpy;
    Drawable d;
    MyFont *font;
    GC gc;
    int x,y;
    char *string;
    int len;
{
    if (use_fontset) {
	XmbDrawImageString(dpy, d, font->fontset, gc, x, y, string, len);
	return;
    }
    XDrawImageString (dpy, d, gc, x, y, string, len);
}

void
MyFont_DrawString(dpy, d, font, gc, x, y, string, len)
    Display *dpy;
    Drawable d;
    MyFont *font;
    GC gc;
    int x,y;
    char *string;
    int len;
{
    if (use_fontset) {
	XmbDrawString(dpy, d, font->fontset, gc, x, y, string, len);
	return;
    }
    XDrawString (dpy, d, gc, x, y, string, len);
}

void
MyFont_ChangeGC(fix_fore, fix_back, fix_font)
    unsigned long fix_fore, fix_back;
    MyFont *fix_font;
{
    Gcv.foreground = fix_fore;
    Gcv.background = fix_back;
    if (use_fontset) {
	XChangeGC(dpy, Scr->NormalGC, GCForeground|GCBackground, &Gcv);
	return;
    }
    Gcv.font = fix_font->font->fid;
    XChangeGC(dpy, Scr->NormalGC, GCFont|GCForeground|GCBackground,&Gcv);
}

/*
 * The following functions are internationalized substitutions
 * for XFetchName and XGetIconName using XGetWMName and
 * XGetWMIconName.  
 *
 * Please note that the third arguments have to be freed using free(), 
 * not XFree().
 */
Status
I18N_FetchName (Display *dpy, Window w, char **winname)
{
    int    status;
    XTextProperty text_prop;
    char **list;
    int    num;

    status = XGetWMName(dpy, w, &text_prop);
    if (!status || !text_prop.value || !text_prop.nitems) {
	*winname = NULL;
	return 0;
    }
    status = XmbTextPropertyToTextList(dpy, &text_prop, &list, &num);
    if (status != Success || !num || !*list)
	*winname = strdup((char *)text_prop.value);
    else {
	*winname = strdup(*list);
	XFreeStringList(list);
    }
    XFree(text_prop.value);
    return 1;
}

Status
I18N_GetIconName (Display *dpy, Window w, char **iconname)
{
    int    status;
    XTextProperty text_prop;
    char **list;
    int    num;

    status = XGetWMIconName(dpy, w, &text_prop);
    if (!status || !text_prop.value || !text_prop.nitems) {
	*iconname = NULL;
	return 0;
    }
    status = XmbTextPropertyToTextList(dpy, &text_prop, &list, &num);
    if (status != Success || !num || !*list)
	*iconname = strdup((char *)text_prop.value);
    else {
	*iconname = strdup(*list);
	XFreeStringList(list);
    }
    XFree(text_prop.value);
    return 1;
}


/*
 * SetFocus - separate routine to set focus to make things more understandable
 * and easier to debug
 */
void SetFocus (tmp_win, time)
    TwmWindow *tmp_win;
    Time	time;
{
    Window w = (tmp_win ? tmp_win->w : PointerRoot);

#ifdef TRACE
    if (tmp_win) {
	printf ("Focusing on window \"%s\"\n", tmp_win->full_name);
    } else {
	printf ("Unfocusing; Scr->Focus was \"%s\"\n",
		Scr->Focus ? Scr->Focus->full_name : "(nil)");
    }
#endif

    XSetInputFocus (dpy, w, RevertToPointerRoot, time);
}


#ifdef NEED_PUTENV_F
/*
 * define our own putenv() if the system doesn't have one.
 * putenv(s): place s (a string of the form "NAME=value") in
 * the environment; replacing any existing NAME.  s is placed in
 * environment, so if you change s, the environment changes (like
 * putenv on a sun).  Binding removed if you putenv something else
 * called NAME.
 */
int
putenv(s)
/* djhjr - 4/22/98
char *s;
*/
const char *s;
{
    char *v;
    int varlen, idx;
    extern char **environ;
    char **newenv;
    static int virgin = 1; /* true while "environ" is a virgin */

    v = index(s, '=');
    if(v == 0)
	return 0; /* punt if it's not of the right form */
    varlen = (v + 1) - s;

    for (idx = 0; environ[idx] != 0; idx++) {
	if (strncmp(environ[idx], s, varlen) == 0) {
	    if(v[1] != 0) { /* true if there's a value */
		environ[idx] = (char *)s;
		return 0;
	    } else {
		do {
		    environ[idx] = environ[idx+1];
		} while(environ[++idx] != 0);
		return 0;
	    }
	}
    }

    /* add to environment (unless no value; then just return) */
    if(v[1] == 0)
	return 0;
    if(virgin) {
	register i;

	newenv = (char **) malloc((unsigned) ((idx + 2) * sizeof(char*)));
	if(newenv == 0)
	    return -1;
	for(i = idx-1; i >= 0; --i)
	    newenv[i] = environ[i];
	virgin = 0;     /* you're not a virgin anymore, sweety */
    } else {
	newenv = (char **) realloc((char *) environ,
				   (unsigned) ((idx + 2) * sizeof(char*)));
	if (newenv == 0)
	    return -1;
    }

    environ = newenv;
    environ[idx] = (char *)s;
    environ[idx+1] = 0;

    return 0;
}
#endif /* NEED_PUTENV_F */


/* Returns a blank cursor */
Cursor NoCursor()
{
	static Cursor blank = (Cursor) NULL;
	Pixmap nopixmap;
	XColor nocolor;

	if (!blank) {
		nopixmap = XCreatePixmap(dpy, Scr->Root, 1, 1, 1);
		nocolor.red = nocolor.blue = nocolor.green = 0;
		blank = XCreatePixmapCursor(dpy, nopixmap, nopixmap, &nocolor, &nocolor, 1, 1);
	}
	return(blank);
}

/* djhjr - 10/30/02 */
static void DrawDotImage(d, x, y, w, h, pad, cp, state)
Drawable d;
int x, y, w, h, pad;
ColorPair cp;
int state;
{
    XPoint points[5];
    int wb, hb, ws, hs, lw;

    lw = (w > h) ? h / 16 : w / 16;
    if (lw == 1) lw = 0;
    XSetForeground(dpy, Scr->RootGC, cp.fore);
    XSetLineAttributes(dpy, Scr->RootGC, lw, LineSolid, CapButt, JoinMiter);

    ws = x + (w / 2) - 2;
    hs = y + (h / 2) - 2;
    wb = ws + 4;
    hb = hs + 4;
    
    points[0].x = points[3].x = points[4].x = ws;
    points[0].y = points[1].y = points[4].y = hs;
    points[1].x = points[2].x = wb;
    points[2].y = points[3].y = hb;
    XDrawLines(dpy, d, Scr->RootGC, points, 5, CoordModeOrigin);
}

/* djhjr - 10/30/02 */
static void DrawResizeImage(d, x, y, w, h, pad, cp, state)
Drawable d;
int x, y, w, h, pad;
ColorPair cp;
int state;
{
    XPoint points[3];
    int wb, hb, ws, hs, lw;

    lw = (w > h) ? h / 16 : w / 16;
    if (lw == 1) lw = 0;
    XSetForeground(dpy, Scr->RootGC, cp.fore);
    XSetLineAttributes(dpy, Scr->RootGC, lw, LineSolid, CapButt, JoinMiter);

    y--;
    wb = w / 4; /* bigger width */
    hb = h / 4; /* bigger width */
    ws = w / 2; /* smaller width */
    hs = h / 2; /* smaller width */

    points[0].x = x;
    points[0].y = points[1].y = y + hb;
    points[1].x = points[2].x = x + w - wb - 1;
    points[2].y = y + h;
    XDrawLines(dpy, d, Scr->RootGC, points, 3, CoordModeOrigin);

    points[0].x = x;
    points[0].y = points[1].y = y + hs;
    points[1].x = points[2].x = x + ws;
    points[2].y = y + h;
    XDrawLines(dpy, d, Scr->RootGC, points, 3, CoordModeOrigin);
}

/* djhjr - 10/30/02 */
static void DrawMenuImage(d, x, y, w, h, pad, cp, state)
Drawable d;
int x, y, w, h, pad;
ColorPair cp;
int state;
{
    int ih, iw;
    int	ix, iy;
    int	mh, mw;
    int	tw, th;
    int	lw, lh;
    int	lx, ly;
    int	lines, dly;
    /*int off;*/
    int	bw;

    if (h < 1) h = 1;
    if (w < 1) w = 1;

    ix = iy = pad + 1;
    ih = h - iy * 2;
    iw = w - ix * 2;
    /*off = ih / 8;*/
    mh = ih - ih / 8/*off*/;
    mw = iw - iw / 8/*off*/;
    bw = mh / 16;
    if (bw == 0 && mw > 2) bw = 1;
    tw = mw - bw * 2;
    th = mh - bw * 2;
    ix += x;
    iy += y;
    XSetForeground(dpy, Scr->RootGC, cp.fore);
    XFillRectangle(dpy, d, Scr->RootGC, ix, iy, mw, mh);
    XFillRectangle(dpy, d, Scr->RootGC, ix + iw - mw, iy + ih - mh, mw, mh);
    XSetForeground(dpy, Scr->RootGC, cp.back);
    XFillRectangle(dpy, d, Scr->RootGC, ix + bw, iy + bw, tw, th);

    lw = tw / 2;
    if ((tw & 1) ^ (lw & 1)) lw++;
    lx = ix + bw + (tw - lw) / 2;
    lh = th / 2 - bw;
    if ((lh & 1) ^ ((th - bw) & 1)) lh++;
    ly = iy + bw + (th - bw - lh) / 2;
    lines = 3;
    if ((lh & 1) && lh < 6) lines--;
    dly = lh / (lines - 1);
    XSetForeground(dpy, Scr->RootGC, cp.fore);
    while (lines--)
    {
	XFillRectangle(dpy, d, Scr->RootGC, lx, ly, lw, bw);
	ly += dly;
    }
}

/* djhjr - 10/30/02 */
static void DrawXLogoImage(d, x, y, w, h, pad, cp, state)
Drawable d;
int x, y, w, h, pad;
ColorPair cp;
int state;
{
    GC gcBack;
    XGCValues gcvalues;
    int lw;

    gcBack = XCreateGC(dpy, Scr->Root, 0, &gcvalues);
    gcvalues.background = cp.back;
    gcvalues.foreground = cp.fore;
    XChangeGC(dpy, gcBack, GCForeground | GCBackground, &gcvalues);

    lw = (w > h) ? h / 16 : w / 16;
    if (lw < 3) lw = 3;
    XSetLineAttributes(dpy, gcBack, lw, LineSolid, CapButt, JoinMiter);

    /*
     * Draw the logo large so that it gets as dense as possible,
     * then blank out the edges so that they look crisp.
     */

    x += pad;
    y += pad;
    w -= pad * 2;
    h -= pad * 2;
    XSetForeground(dpy, Scr->RootGC, cp.fore);
    XSetForeground(dpy, gcBack, cp.back);
    XmuDrawLogo(dpy, d, Scr->RootGC, gcBack, x - 1, y - 1, w + 2, h + 2);
    XDrawRectangle(dpy, d, gcBack, x - 1, y - 1, w + 1, h + 1);

    XFreeGC(dpy, gcBack);
}

/* djhjr - 10/30/02 */
static void DrawQuestionImage(d, x, y, w, h, pad, cp, state)
Drawable d;
int x, y, w, h, pad;
ColorPair cp;
int state;
{
    Pixmap p;

    p = XCreateBitmapFromData(dpy, Scr->Root, questionmark_bits,
			      questionmark_width, questionmark_height);

    XSetForeground(dpy, Scr->RootGC, cp.fore);
    XCopyPlane(dpy, p, d, Scr->RootGC, 0, 0,
	       questionmark_width, questionmark_height,
	       x + (w - questionmark_width) / 2,
	       y + (h - questionmark_height) / 2,
	       (unsigned long)1);

    XFreePixmap(dpy, p);
}

/* djhjr - 10/30/02 */
static void DrawRArrowImage(d, x, y, w, h, pad, cp, state)
Drawable d;
int x, y, w, h, pad;
ColorPair cp;
int state;
{
    XPoint points[4];
    int lw, mw, mh;

    if (!(h & 1)) h--;
    if (h < 1) h = 1;

    lw = (w > h) ? h / 16 : w / 16;
    if (lw == 1) lw = 0;
    XSetForeground(dpy, Scr->RootGC, cp.fore);
    XSetLineAttributes(dpy, Scr->RootGC, lw, LineSolid, CapButt, JoinMiter);

    mw = w / 3;
    mh = h / 3;
    points[0].x = w - mw;
    points[0].y = h / 2;
    points[1].x = mw - 1;
    points[1].y = mh - 1;
    points[2].x = mw - 1;
    points[2].y = h - mh;
    points[3] = points[0];
    XDrawLines(dpy, d, Scr->RootGC, points, 4, CoordModeOrigin);
}

/* djhjr - 10/30/02 */
static void DrawDArrowImage(d, x, y, w, h, pad, cp, state)
Drawable d;
int x, y, w, h, pad;
ColorPair cp;
int state;
{
    XPoint points[4];
    int lw, mw, mh;

    if (!(h & 1)) h--;
    if (h < 1) h = 1;

    lw = (w > h) ? h / 16 : w / 16;
    if (lw == 1) lw = 0;
    XSetForeground(dpy, Scr->RootGC, cp.fore);
    XSetLineAttributes(dpy, Scr->RootGC, lw, LineSolid, CapButt, JoinMiter);

    mw = h / 3;
    mh = h / 3;
    points[0].x = w / 2;
    points[0].y = h - mh;
    points[1].x = w - mw;
    points[1].y = mh - 1;
    points[2].x = mw - 1;
    points[2].y = mh - 1;
    points[3] = points[0];
    XDrawLines(dpy, d, Scr->RootGC, points, 4, CoordModeOrigin);
}

/* djhjr - 4/19/96 */
/* added d, x, y, w, h, state - djhjr - 10/29/02 */
static void Draw3DDotImage(d, x, y, w, h, pad, cp, state)
Drawable d;
int x, y, w, h, pad;
ColorPair cp;
int state;
{
    Draw3DBorder(d, x + (w / 2) - 2, y + (h / 2) - 2, 5, 5,
		 Scr->ShallowReliefWindowButton, cp, state, True, False);
}

/* djhjr - 4/19/96 */
/* added d, x, y, w, h, state - djhjr - 10/29/02 */
static void Draw3DBarImage(d, x, y, w, h, pad, cp, state)
Drawable d;
int x, y, w, h, pad;
ColorPair cp;
int state;
{
	/* was 'Scr->TitleBevelWidth' - djhjr - 8/11/98 */
	Draw3DBorder (d, x + pad + 1, y + (h - 5) / 2,
		w - pad * 2 - 2, 5,
		Scr->ShallowReliefWindowButton, cp, state, True, False);
}

/* djhjr - 4/19/96 */
/* added d, x, y, w, h, state - djhjr - 10/29/02 */
static void Draw3DMenuImage(d, x, y, w, h, pad, cp, state)
Drawable d;
int x, y, w, h, pad;
ColorPair cp;
int state;
{
	int i, lines, width, height;

	height = Scr->ShallowReliefWindowButton * 2;

	/* count the menu lines */
	lines = (h - pad * 2 - 2) / height;
	/* center 'em */
	y += (h - lines * height) / 2;
	if (!(y & 1)) y += 1;
	/* now draw 'em */
	x += pad + 1;
	lines = y + lines * height;
	width = w - pad * 2 - 2;
	for (i = y; i < lines; i += height)
		Draw3DBorder(d, x, i, width, height,
			     Scr->ShallowReliefWindowButton,
			     cp, state, True, False);
}

/* djhjr - 4/19/96 */
/* added d, x, y, w, h, state - djhjr - 10/29/02 */
static void Draw3DResizeImage(d, x, y, w, h, pad, cp, state)
Drawable d;
int x, y, w, h, pad;
ColorPair cp;
int state;
{
    int	  i, j;

    /* djhjr - 4/29/98 */
    /* was 'Scr->TitleBevelWidth' - djhjr - 8/11/98 */
    i = w - Scr->ButtonBevelWidth * 2;

    /*
     * Extend the left and bottom "off-window" by the
     * line width for "thick" boxes on "thin" buttons.
     */

    /* was 'Scr->TitleBevelWidth' - djhjr - 8/11/98 */
    j = Scr->ButtonBevelWidth + (i / 4);
    Draw3DBorder(d,
		x + -Scr->ShallowReliefWindowButton, y + j - 1,
		x + w - j + 1 + Scr->ShallowReliefWindowButton,
		y + h - j + 1 + Scr->ShallowReliefWindowButton,
		Scr->ShallowReliefWindowButton, cp, state, True, False);

    j = Scr->ButtonBevelWidth + (i / 2);
    Draw3DBorder(d,
		x + -Scr->ShallowReliefWindowButton, y + j,
		x + w - j + Scr->ShallowReliefWindowButton,
		y + h - j + Scr->ShallowReliefWindowButton,
		Scr->ShallowReliefWindowButton, cp, state, True, False);
}

/* djhjr - 4/19/96 */
/* added d, x, y, w, h, state - djhjr - 10/29/02 */
static void Draw3DZoomImage(d, x, y, w, h, pad, cp, state)
Drawable d;
int x, y, w, h, pad;
ColorPair cp;
int state;
{
	/* was 'Scr->TitleBevelWidth' - djhjr - 8/11/98 */
	Draw3DBorder (d,
		x + pad + 1, y + pad + 1,
		w - 2 * pad - 2,
		h - 2 * pad - 2,
		Scr->ShallowReliefWindowButton, cp, state, True, False);
}

/* djhjr - 6/4/00 */
/* added d, x, y, w, h, state - djhjr - 10/29/02 */
static void Draw3DRArrowImage(d, x, y, w, h, pad, cp, state)
Drawable d;
int x, y, w, h, pad;
ColorPair cp;
int state;
{
    int i, mw, mh;

	mw = w / 3;
	mh = h / 3;

	if (Scr->Monochrome != COLOR)
	{
		/* draw highlights */
		setBorderGC(1, Scr->GreyGC, cp, state, False);
		for (i = 0; i < Scr->ShallowReliefWindowButton; i++)
		{
			XDrawLine(dpy, d, Scr->GreyGC,
				x + w - mw - i, y + h / 2,
				x + mw - 1 + i, y + mh - 1 + i);
			XDrawLine (dpy, d, Scr->GreyGC,
				x + mw - 1 + i, y + mh - 1 + i,
				x + mw - 1 + i, y + h - mh - i);
		}

		/* draw shadows */
		setBorderGC(2, Scr->GreyGC, cp, state, False);
		for (i = 0; i < Scr->ShallowReliefWindowButton; i++)
			XDrawLine (dpy, d, Scr->GreyGC,
				x + mw - 1 + i, y + h - mh - i,
				x + w - mw - i, y + h / 2);
    }
	else if (Scr->BeNiceToColormap)
	{
		int dashoffset = 0;

		setBorderGC(3, Scr->ShadGC, cp, state, False);
	    
		/* draw highlights */
		XSetForeground (dpy, Scr->ShadGC, Scr->White);
		for (i = 0; i < Scr->ShallowReliefWindowButton; i++)
		{
			XDrawLine(dpy, d, Scr->ShadGC,
				x + w - mw - i, y + h / 2,
				x + mw - 1 + i, y + mh - 1 + i);
			XDrawLine (dpy, d, Scr->ShadGC,
				x + mw - 1 + i, y + mh - 1 + i,
				x + mw - 1 + i, y + h - mh - i);
		}

		/* draw shadows */
		XSetForeground (dpy, Scr->ShadGC, Scr->Black);
		for (i = 0; i < Scr->ShallowReliefWindowButton; i++)
		{
			XDrawLine (dpy, d, Scr->ShadGC,
				x + mw - 1 + i, y + h - mh - i + dashoffset,
				x + w - mw - i, y + h / 2 + dashoffset);
			dashoffset = 1 - dashoffset;
		}
	}
	else
	{
		/* draw highlights */
		if (state)
		{ FB (cp.shadc, cp.shadd); }
		else
		{ FB (cp.shadd, cp.shadc); }
		for (i = 0; i < Scr->ShallowReliefWindowButton; i++)
		{
			XDrawLine(dpy, d, Scr->NormalGC,
				x + w - mw - i, y + h / 2,
				x + mw - 1 + i, y + mh - 1 + i);
			XDrawLine (dpy, d, Scr->NormalGC,
				x + mw - 1 + i, y + mh - 1 + i,
				x + mw - 1 + i, y + h - mh - i);
		}

		/* draw shadows */
		if (state)
		{ FB (cp.shadd, cp.shadc); }
		else
		{ FB (cp.shadc, cp.shadd); }
		for (i = 0; i < Scr->ShallowReliefWindowButton; i++)
			XDrawLine (dpy, d, Scr->NormalGC,
				x + mw - 1 + i, y + h - mw - i,
				x + w - mw - i, y + h / 2);
	}
}

/* djhjr - 6/4/00 */
/* added d, x, y, w, h, state - djhjr - 10/29/02 */
static void Draw3DDArrowImage(d, x, y, w, h, pad, cp, state)
Drawable d;
int x, y, w, h, pad;
ColorPair cp;
int state;
{
    int i, mw, mh;

	mw = w / 3;
	mh = h / 3;

	if (Scr->Monochrome != COLOR)
	{
		/* draw highlights */
		setBorderGC(1, Scr->GreyGC, cp, state, False);
		for (i = 0; i < Scr->ShallowReliefWindowButton; i++)
		{
			XDrawLine(dpy, d, Scr->GreyGC,
				x + w - mw - i, y + mh - 1 + i,
				x + mw - 1 + i, y + mh - 1 + i);
			XDrawLine (dpy, d, Scr->GreyGC,
				x + mw - 1 + i, y + mh - 1 + i,
				x + w / 2, y + mh - 1 - i + h / 2);
		}

		/* draw shadows */
		setBorderGC(2, Scr->GreyGC, cp, state, False);
		for (i = 0; i < Scr->ShallowReliefWindowButton; i++)
			XDrawLine (dpy, d, Scr->GreyGC,
				x + w / 2, y + mh - 1 - i + h / 2,
				x + w - mw - i, y + mh + i);
    }
	else if (Scr->BeNiceToColormap)
	{
		int dashoffset = 0;

		setBorderGC(3, Scr->ShadGC, cp, state, False);
	    
		/* draw highlights */
		XSetForeground (dpy, Scr->ShadGC, Scr->White);
		for (i = 0; i < Scr->ShallowReliefWindowButton; i++)
		{
			XDrawLine(dpy, d, Scr->ShadGC,
				x + w - mw - i, y + mh - 1 + i,
				x + mw - 1 + i, y + mh - 1 + i);
			XDrawLine (dpy, d, Scr->ShadGC,
				x + mw - 1 + i, y + mh - 1 + i + dashoffset,
				x + w / 2, y + mh - 1 - i + h / 2 + dashoffset);
			dashoffset = 1 - dashoffset;
		}

		/* draw shadows */
		XSetForeground (dpy, Scr->ShadGC, Scr->Black);
		for (i = 0; i < Scr->ShallowReliefWindowButton; i++)
			XDrawLine (dpy, d, Scr->ShadGC,
				x + w / 2, y + mh - 1 - i + h / 2,
				x + w - mw - i, y + mh + i);
	}
	else
	{
		/* draw highlights */
		if (state)
		{ FB (cp.shadc, cp.shadd); }
		else
		{ FB (cp.shadd, cp.shadc); }
		for (i = 0; i < Scr->ShallowReliefWindowButton; i++)
		{
			XDrawLine(dpy, d, Scr->NormalGC,
				x + w - mw - i, y + mh - 1 + i,
				x + mw - 1 + i, y + mh - 1 + i);
			XDrawLine (dpy, d, Scr->NormalGC,
				x + mw - 1 + i, y + mh - 1 + i,
				x + w / 2, y + mh - 1 - i + h / 2);
		}

		/* draw shadows */
		if (state)
		{ FB (cp.shadd, cp.shadc); }
		else
		{ FB (cp.shadc, cp.shadd); }
		for (i = 0; i < Scr->ShallowReliefWindowButton; i++)
			XDrawLine (dpy, d, Scr->NormalGC,
				x + w / 2, y + mh - 1 - i + h / 2,
				x + w - mw - i, y + mh + i);
	}
}

/* djhjr - 10/25/02 */
static void Draw3DBoxHighlight(d, x, y, w, h, pad, cp, state)
Drawable d;
int x, y, w, h, pad;
ColorPair cp;
int state;
{
	Draw3DBorder(d, x, y, w, h,
		     Scr->ShallowReliefWindowButton, cp,
		     state, True, False);
}

/* djhjr - 10/25/02 */
static void Draw3DLinesHighlight(d, x, y, w, h, pad, cp, state)
Drawable d;
int x, y, w, h, pad;
ColorPair cp;
int state;
{
	int p;
	
	p = (Scr->ShallowReliefWindowButton & 1) ? 3 : 2;
	p = (h - Scr->ShallowReliefWindowButton * p - 2) / 2;

	y += h / 2 - 1;
	y -= (Scr->ShallowReliefWindowButton & 1) ? 0 : 1;
	y += (state == on) ? 0 : 1;

	h = Scr->ShallowReliefWindowButton * 2;

	Draw3DBorder(d, x, y - p, w, h,
		     Scr->ShallowReliefWindowButton, cp,
		     state, False, False);

	if ((Scr->ShallowReliefWindowButton & 1))
		Draw3DBorder(d, x, y, w, h,
			     Scr->ShallowReliefWindowButton, cp,
			     state, False, False);

	Draw3DBorder(d, x, y + p, w, h,
		     Scr->ShallowReliefWindowButton, cp,
		     state, False, False);
}

/* ick - djhjr - 10/30/02 */
static void DrawBackground(d, x, y, w, h, cp, use_rootGC)
Drawable d;
int x, y, w, h;
ColorPair cp;
int use_rootGC;
{
    XGCValues gcvalues;

    if (use_rootGC)
    {
	if (Scr->RootGC == (GC)0)
	    Scr->RootGC = XCreateGC(dpy, Scr->Root, 0, &gcvalues);

	gcvalues.background = cp.back;
	gcvalues.foreground = cp.fore;
	XChangeGC(dpy, Scr->RootGC, GCForeground | GCBackground, &gcvalues);

	XSetForeground(dpy, Scr->RootGC, cp.back);
	XFillRectangle(dpy, d, Scr->RootGC, x, y, w, h);
    }
    else
    {
	FB(cp.back, cp.fore);
	XFillRectangle(dpy, d, Scr->NormalGC, x, y, w, h);
    }
}

/* djhjr - 10/25/02 */
static void DrawTitleHighlight(t, state)
TwmWindow *t;
int state;
{
	static const struct {
		char *name;
		void (*proc)();
		Bool use_rootGC;
		int state;
	} pmtab[] = {
		/* djhjr - 10/30/02 */
		{ TBPM_DOT,		DrawDotImage,		True, off  },
		{ TBPM_ICONIFY,		DrawDotImage,		True, off  },
		{ TBPM_RESIZE,		DrawResizeImage,	True, off  },
		{ TBPM_MENU,		DrawMenuImage,		True, off  },
		{ TBPM_XLOGO,		DrawXLogoImage,		True, off  },
		{ TBPM_DELETE,		DrawXLogoImage,		True, off  },
		{ TBPM_QUESTION,	DrawQuestionImage,	True, off  },

		{ TBPM_3DDOT,		Draw3DDotImage,		False, off },
		{ TBPM_3DRESIZE,	Draw3DResizeImage,	False, off },
		{ TBPM_3DMENU,		Draw3DMenuImage,	False, off },
		{ TBPM_3DZOOM,		Draw3DZoomImage,	False, off },
		{ TBPM_3DBAR,		Draw3DBarImage,		False, off },

		/* djhjr - 10/30/02 */
		{ TBPM_3DBOX,		Draw3DBoxHighlight,	False, off },
		{ TBPM_3DLINES,		Draw3DLinesHighlight,	False, off },

		{ TBPM_3DRAISEDBOX,	Draw3DBoxHighlight,	False, off },
		{ TBPM_3DSUNKENBOX,	Draw3DBoxHighlight,	False, on  },
		{ TBPM_3DRAISEDLINES,	Draw3DLinesHighlight,	False, off },
		{ TBPM_3DSUNKENLINES,	Draw3DLinesHighlight,	False, on  },
	};

	XGCValues gcvalues;
	ColorPair cp;
	register int i;
	int h, w;
	
	cp = t->title;
	w = ComputeHighlightWindowWidth(t);
	h = Scr->TitleHeight - 2 * Scr->FramePadding - 2;
		
	for (i = 0; i < sizeof(pmtab) / sizeof(pmtab[0]); i++)
	{
		if (XmuCompareISOLatin1(pmtab[i].name, Scr->hiliteName) == 0)
		{
			if (state == off)
			{
				DrawBackground(t->title_w,
					       t->highlightx,
					       Scr->FramePadding + 1,
					       w, h, cp,
					       pmtab[i].use_rootGC | (Scr->Monochrome != COLOR));
			}
			else
			{
				/* ick - djhjr - 10/30/02 */
				if (pmtab[i].use_rootGC)
				{
					if (Scr->RootGC == (GC)0)
						Scr->RootGC = XCreateGC(dpy,
								   Scr->Root,
								   0,
								   &gcvalues);

					gcvalues.background = cp.back;
					gcvalues.foreground = cp.fore;
					XChangeGC(dpy, Scr->RootGC,
						  GCForeground | GCBackground,
						  &gcvalues);
				}

				(*pmtab[i].proc)(t->title_w,
						 t->highlightx,
						 Scr->FramePadding + 1,
						 w, h, 0, cp, pmtab[i].state);
			}

			break;
		}
	}
}

/* djhjr - 1/13/98 */
void setBorderGC(type, gc, cp, state, forcebw)
int			type, state, forcebw;
GC			gc;
ColorPair	cp;
{
	XGCValues		gcv;
	unsigned long	gcm;

	switch (type)
	{
		case 0: /* Monochrome main */
			gcm = GCFillStyle;
			gcv.fill_style = FillOpaqueStippled;
			break;
		case 1: /* Monochrome highlight */
			gcm  = 0;
			gcm |= GCLineStyle;		
			gcv.line_style = (state == on) ? LineSolid : LineDoubleDash;
			gcm |= GCFillStyle;
			gcv.fill_style = FillSolid;
			break;
		case 2: /* Monochrome shadow */
			gcm  = 0;
			gcm |= GCLineStyle;		
			gcv.line_style = (state == on) ? LineDoubleDash : LineSolid;
			gcm |= GCFillStyle;
			gcv.fill_style = FillSolid;
			break;
		case 3: /* BeNiceToColormap */
			gcm  = 0;
			gcm |= GCLineStyle;		
			gcv.line_style = (forcebw) ? LineSolid : LineDoubleDash;
			gcm |= GCBackground;
			gcv.background = cp.back;
			break;
		default:
			return;
	}

	XChangeGC (dpy, gc, gcm, &gcv);
}

/* djhjr - 4/19/96 */
void Draw3DBorder (w, x, y, width, height, bw, cp, state, fill, forcebw)
Drawable		w;
int			x, y, width, height, bw;
ColorPair	cp;
int			state, fill, forcebw;
{
	int				i;

	if (width < 1 || height < 1) return;

	if (Scr->Monochrome != COLOR)
	{
		/* set main color */
		if (fill)
		{
			setBorderGC(0, Scr->GreyGC, cp, state, forcebw);
			XFillRectangle (dpy, w, Scr->GreyGC, x, y, width, height);
		}

		/* draw highlights */
		setBorderGC(1, Scr->GreyGC, cp, state, forcebw);
		for (i = 0; i < bw; i++)
		{
			XDrawLine (dpy, w, Scr->GreyGC, x, y + i,
				x + width - i - 1, y + i);
			XDrawLine (dpy, w, Scr->GreyGC, x + i, y,
				x + i, y + height - i - 1);
		}

		/* draw shadows */
		setBorderGC(2, Scr->GreyGC, cp, state, forcebw);
		for (i = 0; i < bw; i++)
		{
			XDrawLine (dpy, w, Scr->GreyGC, x + width - i - 1, y + i,
				x + width - i - 1, y + height - 1);
			XDrawLine (dpy, w, Scr->GreyGC, x + i, y + height - i - 1,
				x + width - 1, y + height - i - 1);
		}

		return;
    }

	/* set main color */
	if (fill)
	{
		FB (cp.back, cp.fore);
		XFillRectangle (dpy, w, Scr->NormalGC, x, y, width, height);
	}

	if (Scr->BeNiceToColormap)
	{
		setBorderGC(3, Scr->ShadGC, cp, state, forcebw);
	    
		/* draw highlights */
		if (state == on)
			XSetForeground (dpy, Scr->ShadGC, Scr->Black);
		else
			XSetForeground (dpy, Scr->ShadGC, Scr->White);
		for (i = 0; i < bw; i++)
		{
			XDrawLine (dpy, w, Scr->ShadGC,
				x + i, y + borderdashoffset,
				x + i, y + height - i - 1);
			XDrawLine (dpy, w, Scr->ShadGC,
				x + borderdashoffset, y + i,
				x + width - i - 1, y + i);
			borderdashoffset = 1 - borderdashoffset;
		}

		/* draw shadows */
		if (state == on)
			XSetForeground (dpy, Scr->ShadGC, Scr->White);
		else
			XSetForeground (dpy, Scr->ShadGC, Scr->Black);
		for (i = 0; i < bw; i++)
		{
			XDrawLine (dpy, w, Scr->ShadGC, x + i, y + height - i - 1,
				x + width - 1, y + height - i - 1);
			XDrawLine (dpy, w, Scr->ShadGC, x + width - i - 1, y + i,
				x + width - i - 1, y + height - 1);
		}

		return;
	}

	/* draw highlights */
	if (state == on)
		{ FB (cp.shadd, cp.shadc); }
	else
		{ FB (cp.shadc, cp.shadd); }
	for (i = 0; i < bw; i++)
	{
		XDrawLine (dpy, w, Scr->NormalGC, x, y + i,
			x + width - i - 1, y + i);
		XDrawLine (dpy, w, Scr->NormalGC, x + i, y,
			x + i, y + height - i - 1);
	}

	/* draw shadows */
	if (state == on)
		{ FB (cp.shadc, cp.shadd); }
	else
		{ FB (cp.shadd, cp.shadc); }
	for (i = 0; i < bw; i++)
	{
		XDrawLine (dpy, w, Scr->NormalGC, x + width - i - 1, y + i,
			x + width - i - 1, y + height - 1);
		XDrawLine (dpy, w, Scr->NormalGC, x + i, y + height - i - 1,
			x + width - 1, y + height - i - 1);
	}
}

#ifdef USE_ORIGINAL_CORNERS
/* djhjr - 4/19/96 */
void Draw3DCorner (w, x, y, width, height, thick, bw, cp, type)
Window		w;
int			x, y, width, height, thick, bw;
ColorPair	cp;
int			type;
{
	Draw3DBorder (w, x, y, width, height, bw, cp, off, True, False);

	switch (type)
	{
		/* upper left */
		case 0 :
			Draw3DBorder (w, x + thick - bw, y + thick - bw,
				width - thick + 2 * bw, height - thick + 2 * bw,
				bw, cp, on, True, False);
			break;
		/* upper right */
		case 1 :
			Draw3DBorder (w, x - bw, y + thick - bw,
				width - thick + 2 * bw, height - thick + 2 * bw,
				bw, cp, on, True, False);
			break;
		/* lower right */
		case 2 :
			Draw3DBorder (w, x - bw, y - bw,
				width - thick + 2 * bw, height - thick + 2 * bw,
				bw, cp, on, True, False);
			break;
		/* lower left */
		case 3 :
			Draw3DBorder (w, x + thick - bw, y - bw,
				width - thick + 2 * bw, height - thick + 2 * bw,
				bw, cp, on, True, False);
			break;
	}
}
#else /* USE_ORIGINAL_CORNERS */
/* djhjr - 1/14/98 */
GC setBevelGC(type, state, cp)
int			type, state;
ColorPair	cp;
{
	GC		gc;

	if (Scr->Monochrome != COLOR)
	{
		gc = Scr->GreyGC;
		setBorderGC(type, gc, cp, state, False);
	}
	else if (Scr->BeNiceToColormap)
	{
		gc = Scr->ShadGC;
		setBorderGC(3, gc, cp, state, False);
		if (state == on)
			XSetForeground (dpy, gc, Scr->Black);
		else
			XSetForeground (dpy, gc, Scr->White);
	}
	else
	{
		gc = Scr->NormalGC;
		if (state == on)
			{ FB (cp.shadc, cp.shadd); }
		else
			{ FB (cp.shadd, cp.shadc); }
	}

	return (gc);
}

/* djhjr - 1/12/98 */
void Draw3DBevel (w, x, y, bw, cp, state, type)
Drawable		w;
int			x, y, bw;
ColorPair	cp;
int			state, type;
{
	int		i;
	GC		gc;

	switch (type)
	{
		/* vertical */
		case 1 :
		case 11 :
			gc = setBevelGC(1, state, cp);
			for (i = 0; i < Scr->BorderBevelWidth; i++)
				XDrawLine (dpy, w, gc, x - i - 1, y + i,
					x - i - 1, y + bw - 2 * i + i);
			if (type == 11) break;
		case 111 :
			gc = setBevelGC(2, !state, cp);
			for (i = 0; i < Scr->BorderBevelWidth; i++)
				XDrawLine (dpy, w, gc, x + i, y + i - 1,
					x + i, y + bw - 2 * i + i - 1);
			break;
		/* horizontal */
		case 2 :
		case 22 :
			gc = setBevelGC(1, state, cp);
			for (i = 0; i < Scr->BorderBevelWidth; i++)
				XDrawLine (dpy, w, gc, x + i, y - i - 1,
					x + bw - 2 * i + i, y - i - 1);
			if (type == 22) break;
		case 222 :
			gc = setBevelGC(2, !state, cp);
			for (i = 0; i < Scr->BorderBevelWidth; i++)
				XDrawLine (dpy, w, gc, x + i - 1, y + i,
					x + bw - 2 * i + i - 1, y + i);
			break;
		/* ulc to lrc */
		case 3 :
			if (Scr->Monochrome != COLOR)
			{
				gc = Scr->GreyGC;
				setBorderGC(0, gc, cp, state, False);
			}
			else
			{
				gc = Scr->NormalGC;
				FB (cp.back, cp.fore);
			}
			XFillRectangle (dpy, w, gc, x, y, bw, bw);
			gc = setBevelGC(1, (Scr->BeNiceToColormap) ? state : !state, cp);
			for (i = 0; i < Scr->BorderBevelWidth; i++)
				XDrawLine (dpy, w, gc, x + i, y,
					x + i, y + Scr->BorderBevelWidth - 1);
			gc = setBevelGC(2, (Scr->BeNiceToColormap) ? !state : state, cp);
			for (i = 0; i < Scr->BorderBevelWidth; i++)
				XDrawLine (dpy, w, gc, x, y + bw - i - 1,
					x + bw - 1, y + bw - i - 1);
			for (i = 0; i < Scr->BorderBevelWidth; i++)
				XDrawLine (dpy, w, gc, x + bw - i - 1, y,
					x + bw - i - 1, y + bw - 1);
			break;
		/* urc to llc */
		case 4 :
			if (Scr->Monochrome != COLOR)
			{
				gc = Scr->GreyGC;
				setBorderGC(0, gc, cp, state, False);
			}
			else
			{
				gc = Scr->NormalGC;
				FB (cp.back, cp.fore);
			}
			XFillRectangle (dpy, w, gc, x, y, bw, bw);
			gc = setBevelGC(1, (Scr->BeNiceToColormap) ? state : !state, cp);
			/* top light */
			for (i = 0; i < Scr->BorderBevelWidth; i++)
				XDrawLine (dpy, w, gc, x + i, y,
					x + i, y + bw - i);
			/* bottom light */
			for (i = 0; i < Scr->BorderBevelWidth; i++)
				XDrawLine (dpy, w, gc, x + bw - 1, y + i,
					x + bw - i - 1, y + i);
			gc = setBevelGC(2, (Scr->BeNiceToColormap) ? !state : state, cp);
			/* top dark */
			for (i = 0; i < Scr->BorderBevelWidth - 1; i++)
				XDrawLine (dpy, w, gc, x + bw - Scr->BorderBevelWidth, y + i,
					x + bw - i - 2, y + i);
			/* bottom dark */
			for (i = 0; i < Scr->BorderBevelWidth; i++)
				XDrawLine (dpy, w, gc, x + i, y + bw - i - 1,
					x + bw + 1, y + bw - i - 1);
			break;
	}
}

/* djhjr - 10/20/02 */
void Draw3DNoBevel(w, x, y, bw, cp, state, forcebw)
Drawable	w;
int		x, y, bw;
ColorPair	cp;
int		state, forcebw;
{
	int		i, upr, lwr;

	if (bw < 1) return;

	upr = y - 2 * Scr->BorderBevelWidth;
	if ((upr & 1)) upr--;
	lwr = y + 2 * Scr->BorderBevelWidth;
	if ((lwr & 1)) lwr++;

	if (Scr->Monochrome != COLOR)
	{
		/* set main color */
		setBorderGC(0, Scr->GreyGC, cp, state, forcebw);
		XFillRectangle (dpy, w, Scr->GreyGC,
			x + Scr->BorderBevelWidth, upr,
			(unsigned int)(bw - Scr->BorderBevelWidth * 2),
			(unsigned int)(upr * 2));

		/* draw highlight */
		setBorderGC(1, Scr->GreyGC, cp, state, forcebw);
		for (i = 0; i < Scr->BorderBevelWidth; i++)
			XDrawLine (dpy, w, Scr->GreyGC,
				x + i, upr, x + i, lwr);
		
		/* draw shadow */
		setBorderGC(2, Scr->GreyGC, cp, state, forcebw);
		for (i = bw - Scr->BorderBevelWidth; i < bw; i++)
			XDrawLine (dpy, w, Scr->GreyGC,
				x + i, upr, x + i, lwr);

		return;
	}

	/* set main color */
	FB (cp.back, cp.fore);
	XFillRectangle (dpy, w, Scr->NormalGC,
		x + Scr->BorderBevelWidth, upr,
		(unsigned int)(bw - Scr->BorderBevelWidth * 2),
		(unsigned int)(upr * 2));

	if (Scr->BeNiceToColormap)
	{
		int dashoffset;

		setBorderGC(3, Scr->ShadGC, cp, state, forcebw);
	    
		/* draw highlight */
		if (state == on)
			XSetForeground (dpy, Scr->ShadGC, Scr->Black);
		else
			XSetForeground (dpy, Scr->ShadGC, Scr->White);
		dashoffset = 0;
		for (i = 0; i < Scr->BorderBevelWidth; i++)
		{
			XDrawLine (dpy, w, Scr->ShadGC,
				x + i, upr + dashoffset, x + i, lwr);
			dashoffset = 1 - dashoffset;
		}

		/* draw shadow */
		if (state == on)
			XSetForeground (dpy, Scr->ShadGC, Scr->White);
		else
			XSetForeground (dpy, Scr->ShadGC, Scr->Black);
		dashoffset = 0;
		for (i = bw - Scr->BorderBevelWidth; i < bw; i++)
		{
			XDrawLine (dpy, w, Scr->ShadGC,
				x + i, upr + dashoffset, x + i, lwr);
			dashoffset = 1 - dashoffset;
		}

		return;
	}

	/* draw highlight */
	if (state == on)
		{ FB (cp.shadc, cp.shadd); }
	else
		{ FB (cp.shadd, cp.shadc); }
	for (i = 0; i < Scr->BorderBevelWidth; i++)
		XDrawLine (dpy, w, Scr->NormalGC,
			x + i, upr, x + i, lwr);

	/* draw shadow */
	if (state == on)
		{ FB (cp.shadd, cp.shadc); }
	else
		{ FB (cp.shadc, cp.shadd); }
	for (i = bw - Scr->BorderBevelWidth; i < bw; i++)
		XDrawLine (dpy, w, Scr->NormalGC,
			x + i, upr, x + i, lwr);
}
#endif /* USE_ORIGINAL_CORNERS */

/* djhjr - 4/19/96 */
static Image *LoadBitmapImage (name, cp)
char  *name;
ColorPair cp;
{
    Image	*image;
    Pixmap	bm;
    int		width, height;
    XGCValues	gcvalues;

    if (Scr->RootGC == (GC) 0) Scr->RootGC = XCreateGC (dpy, Scr->Root, 0, &gcvalues);
    bm = FindBitmap (name, (unsigned int *) &width, (unsigned int *) &height);
    if (bm == None) return (None);

    image = (Image*) malloc (sizeof (struct _Image));
    image->pixmap = XCreatePixmap (dpy, Scr->Root, width, height, Scr->d_depth);
    gcvalues.background = cp.back;
    gcvalues.foreground = cp.fore;
    XChangeGC   (dpy, Scr->RootGC, GCForeground | GCBackground, &gcvalues);
    XCopyPlane  (dpy, bm, image->pixmap, Scr->RootGC, 0, 0, width, height, 0, 0, (unsigned long) 1);
    XFreePixmap (dpy, bm);
    image->mask   = None;
    image->width  = width;
    image->height = height;
    image->next   = None;
    return (image);
}

/* djhjr - 10/30/02 */
static Image *CreateImagePixmap(name, w, h, depth)
char *name;
int w, h, depth;
{
    Image *image;

    image = (Image *)malloc(sizeof(struct _Image));
    if (!image)
    {
	fprintf(stderr, "%s: cannot allocate %d bytes for Image \"%s\"\n",
			ProgramName, sizeof(struct _Image), name);
	return (None);
    }

    image->pixmap = XCreatePixmap(dpy, Scr->Root, w, h, depth);
    if (image->pixmap == None)
    {
	fprintf(stderr, "%s: cannot allocate %d bytes for pixmap \"%s\"\n",
			ProgramName, sizeof(image->pixmap), name);
	free((void *)image);
	return (None);
    }

    return (image);
}

/* djhjr - 4/19/96 10/30/02 */
static Image *ReallyGetImage(name, w, h, pad, cp)
char      *name;
int       w, h, pad;
ColorPair cp;
{
    static const struct {
	char *name;
	void (*proc)();
	Bool use_rootGC;
	int state;
    } pmtab[] = {
	{ TBPM_DOT,		DrawDotImage,		True, off  },
	{ TBPM_ICONIFY,		DrawDotImage,		True, off  },
	{ TBPM_RESIZE,		DrawResizeImage,	True, off  },
	{ TBPM_MENU,		DrawMenuImage,		True, off  },
	{ TBPM_XLOGO,		DrawXLogoImage,		True, off  },
	{ TBPM_DELETE,		DrawXLogoImage,		True, off  },
	{ TBPM_QUESTION,	DrawQuestionImage,	True, off  },

	/* djhjr - 6/4/00 */
	{ TBPM_RARROW,		DrawRArrowImage,	True, off  },
	{ TBPM_DARROW,		DrawDArrowImage,	True, off  },

	{ TBPM_3DDOT,		Draw3DDotImage,		False, off },
	{ TBPM_3DRESIZE,	Draw3DResizeImage,	False, off },
	{ TBPM_3DMENU,		Draw3DMenuImage,	False, off },
	{ TBPM_3DZOOM,		Draw3DZoomImage,	False, off },
	{ TBPM_3DBAR,		Draw3DBarImage,		False, off },

	/* djhjr - 6/4/00 */
	{ TBPM_3DRARROW,	Draw3DRArrowImage,	False, off },
	{ TBPM_3DDARROW,	Draw3DDArrowImage,	False, off },

	/* djhjr - 10/30/02 */
	{ TBPM_3DBOX,		Draw3DBoxHighlight,	False, off },
	{ TBPM_3DLINES,		Draw3DLinesHighlight,	False, off },
    };
	
    Image *image = NULL;
    name_list **list;
    register int i;
    char fullname [256];

    if (name == NULL) return (NULL);

    list = &Scr->ImageCache;

    if (name[0] == ':')
    {
	/* probably need '"%d", Scr->screen' - Caveh Frank Jalali */
	sprintf(fullname, "%s.%dx%d.%Xx%X.%d", name,
		w, h, (int)cp.fore, (int)cp.back, Scr->screen);
	if ((image = (Image *)LookInNameList(*list, fullname)) == NULL)
	{
	    for (i = 0; i < sizeof(pmtab) / sizeof(pmtab[0]); i++)
	    {
		if (XmuCompareISOLatin1(pmtab[i].name, name) == 0)
		{
		    if (!(image = CreateImagePixmap(name, w, h, Scr->d_depth)))
			return (None);

		    DrawBackground(image->pixmap, 0, 0, w, h, cp,
				pmtab[i].use_rootGC | (Scr->Monochrome != COLOR));

		    /* added d, x, y, w, h, pad, state - djhjr - 10/29/02 */
		    (*pmtab[i].proc)(image->pixmap, 0, 0, w, h, pad,
				     cp, pmtab[i].state);

		    image->mask   = None;
		    image->width  = w;
		    image->height = h;
		    image->next   = None;
		    break;
		}
	    }

	    if (!image)
	    {
		fprintf(stderr, "%s:  no such built-in pixmap \"%s\"\n",
				ProgramName, name);
		return (NULL);
	    }
	}
	else
	    return (image);
    }
    else
    {
	/*
	 * Need screen number in fullname since screens may have different GCs.
	 * Submitted by Caveh Frank Jalali
	 */
	sprintf(fullname, "%s.%Xx%X.%d",
		name, (int)cp.fore, (int)cp.back, (int)Scr->screen);
	if ((image = (Image *)LookInNameList(*list, fullname)) == NULL)
	{
	    if ((image = LoadBitmapImage(name, cp)) == NULL)
#ifndef NO_XPM_SUPPORT
		/* djhjr - 3/20/98 */
		/* added color argument - djhjr - 9/28/99 */
		image = FindImage(name, cp.back);
#else
		;
#endif

	    if (!image) return (NULL);
	}
	else
	    return (image);
    }

    /* added 'type' argument - djhjr - 10/20/01 */
    AddToList(list, fullname, LTYPE_EXACT_NAME, (char *)image);

    return (image);
}

/*
 * Wrapper to guarantee something is returned - djhjr - 10/30/02
 */
Image *GetImage(name, w, h, pad, cp)
char *name;
int w, h, pad;
ColorPair cp;
{
    Image *image = NULL;

    if (!(image = ReallyGetImage(name, w, h, pad, cp)))
	image = ReallyGetImage(TBPM_QUESTION, w, h, pad, cp);

    return (image);
}

/* djhjr - 4/21/96 */
/* djhjr - 1/12/98 */
void PaintBorders (tmp_win, focus)
TwmWindow	*tmp_win;
Bool		focus;
{
	GC			gc;
	ColorPair	cp;
	int			i, j, cw, ch, cwbw, chbw;
#ifdef USE_ORIGINAL_CORNERS
	int			THbw, fhbwchbw, fhTHchbw;
#endif

	cp = (focus && tmp_win->highlight) ? tmp_win->border : tmp_win->border_tile;

	/* no titlebar, no corners */
	if (tmp_win->title_height == 0)
	{
		Draw3DBorder (tmp_win->frame,
			0,
			0,
			tmp_win->frame_width,
			tmp_win->frame_height,
			Scr->BorderBevelWidth, cp, off, True, False);
		Draw3DBorder (tmp_win->frame,
			tmp_win->frame_bw3D - Scr->BorderBevelWidth,
			tmp_win->frame_bw3D - Scr->BorderBevelWidth,
			tmp_win->frame_width  - 2 * tmp_win->frame_bw3D + 2 * Scr->BorderBevelWidth,
			tmp_win->frame_height - 2 * tmp_win->frame_bw3D + 2 * Scr->BorderBevelWidth,
			Scr->BorderBevelWidth, cp, on, True, False);

		return;
	}

	cw = ch = Scr->TitleHeight;
	if (cw * 2 > tmp_win->attr.width) cw = tmp_win->attr.width / 2;
	if (ch * 2 > tmp_win->attr.height) ch = tmp_win->attr.height / 2;
	cwbw = cw + tmp_win->frame_bw3D;
	chbw = ch + tmp_win->frame_bw3D;

#ifdef USE_ORIGINAL_CORNERS
	THbw = Scr->TitleHeight + tmp_win->frame_bw3D;
	fhbwchbw = tmp_win->frame_height + tmp_win->frame_bw3D - 3 * chbw;
	fhTHchbw = tmp_win->frame_height - (Scr->TitleHeight + ch + 2 * tmp_win->frame_bw3D);

	/* client upper left corner */
	if (tmp_win->squeeze_info && tmp_win->title_x > tmp_win->frame_bw3D)
		Draw3DCorner (tmp_win->frame,
			0,
			Scr->TitleHeight,
			cwbw,
			chbw,
			tmp_win->frame_bw3D, Scr->BorderBevelWidth, cp, 0);
	/* client top bar */
	if (tmp_win->squeeze_info)
		Draw3DBorder (tmp_win->frame,
			THbw,
			tmp_win->title_height,
			tmp_win->frame_width - 2 * cwbw,
			tmp_win->frame_bw3D,
			Scr->BorderBevelWidth, cp, off, True, False);
	/* client upper right corner */
	if (tmp_win->squeeze_info && tmp_win->title_x + tmp_win->title_width + tmp_win->frame_bw3D < tmp_win->frame_width)
		Draw3DCorner (tmp_win->frame,
			tmp_win->frame_width  - cwbw,
			Scr->TitleHeight,
			cwbw,
			chbw,
			tmp_win->frame_bw3D, Scr->BorderBevelWidth, cp, 1);
	/* client left bar */
	if (tmp_win->squeeze_info && tmp_win->title_x > tmp_win->frame_bw3D)
		Draw3DBorder (tmp_win->frame,
			0,
			THbw + ch,
			tmp_win->frame_bw3D,
			fhbwchbw,
			Scr->BorderBevelWidth, cp, off, True, False);
	else
		Draw3DBorder (tmp_win->frame,
			0,
			THbw,
			tmp_win->frame_bw3D,
			fhTHchbw,
			Scr->BorderBevelWidth, cp, off, True, False);
	/* client right bar */
	if (tmp_win->squeeze_info && tmp_win->title_x + tmp_win->title_width + tmp_win->frame_bw3D < tmp_win->frame_width)
		Draw3DBorder (tmp_win->frame,
			tmp_win->frame_width  - tmp_win->frame_bw3D,
			THbw + ch,
			tmp_win->frame_bw3D,
			fhbwchbw,
			Scr->BorderBevelWidth, cp, off, True, False);
	else
		Draw3DBorder (tmp_win->frame,
			tmp_win->frame_width  - tmp_win->frame_bw3D,
			THbw,
			tmp_win->frame_bw3D,
			fhTHchbw,
			Scr->BorderBevelWidth, cp, off, True, False);
	/* client lower left corner */
	Draw3DCorner (tmp_win->frame,
		0,
		tmp_win->frame_height - chbw,
		cwbw,
		chbw,
		tmp_win->frame_bw3D, Scr->BorderBevelWidth, cp, 3);
	/* client bottom bar */
	Draw3DBorder (tmp_win->frame,
		THbw,
		tmp_win->frame_height - tmp_win->frame_bw3D,
		tmp_win->frame_width - 2 * cwbw,
		tmp_win->frame_bw3D,
		Scr->BorderBevelWidth, cp, off, True, False);
	/* client lower right corner */
	Draw3DCorner (tmp_win->frame,
		tmp_win->frame_width  - cwbw,
		tmp_win->frame_height - chbw,
		cwbw,
		chbw,
		tmp_win->frame_bw3D, Scr->BorderBevelWidth, cp, 2);

	/* titlebar upper left corner */
	Draw3DCorner (tmp_win->frame,
		tmp_win->title_x - tmp_win->frame_bw3D,
		0,
		cwbw,
		THbw,
		tmp_win->frame_bw3D, Scr->BorderBevelWidth, cp, 0);
	/* titlebar top bar */
	Draw3DBorder (tmp_win->frame,
		tmp_win->title_x + cw,
		0,
		tmp_win->title_width - 2 * cw,
		tmp_win->frame_bw3D,
		Scr->BorderBevelWidth, cp, off, True, False);
	/* titlebar upper right corner */
	Draw3DCorner (tmp_win->frame,
		(tmp_win->title_width > 2 * Scr->TitleHeight)
			? tmp_win->title_x + tmp_win->title_width - Scr->TitleHeight
			: tmp_win->frame_width - cwbw,
		0,
		cwbw,
		THbw,
		tmp_win->frame_bw3D, Scr->BorderBevelWidth, cp, 1);
#else /* USE_ORIGINAL_CORNERS */
	/* client */
	borderdashoffset = 1;
	Draw3DBorder (tmp_win->frame,
		0,
		Scr->TitleHeight,
		tmp_win->frame_width,
		tmp_win->frame_height - Scr->TitleHeight,
		Scr->BorderBevelWidth, cp, off, True, False);
	borderdashoffset = 1;
	Draw3DBorder (tmp_win->frame,
		tmp_win->frame_bw3D - Scr->BorderBevelWidth,
		Scr->TitleHeight + tmp_win->frame_bw3D - Scr->BorderBevelWidth,
		tmp_win->frame_width  - 2 * tmp_win->frame_bw3D + 2 * Scr->BorderBevelWidth,
		tmp_win->frame_height - 2 * tmp_win->frame_bw3D + 2 * Scr->BorderBevelWidth - Scr->TitleHeight,
		Scr->BorderBevelWidth, cp, on, True, False);
    if (!Scr->NoBorderDecorations)	/* djhjr - 10/20/02 */
    {
	/* upper left corner */
	if (tmp_win->title_x == tmp_win->frame_bw3D)
		Draw3DBevel (tmp_win->frame,
			0, Scr->TitleHeight + tmp_win->frame_bw3D,
			tmp_win->frame_bw3D, cp,
			(Scr->BeNiceToColormap) ? on : off, 222);
	else
	{
		if (tmp_win->title_x > tmp_win->frame_bw3D + cwbw)
			Draw3DBevel (tmp_win->frame,
				cwbw, Scr->TitleHeight,
				tmp_win->frame_bw3D, cp,
				(Scr->BeNiceToColormap) ? on : off, 1);
		Draw3DBevel (tmp_win->frame,
			0, Scr->TitleHeight + chbw,
			tmp_win->frame_bw3D, cp,
			(Scr->BeNiceToColormap) ? on : off, 2);
	}
    }
	/* upper right corner */
	if ((i = tmp_win->title_x + tmp_win->title_width + tmp_win->frame_bw3D) == tmp_win->frame_width)
		Draw3DBevel (tmp_win->frame,
			tmp_win->frame_width - tmp_win->frame_bw3D, Scr->TitleHeight + tmp_win->frame_bw3D,
			tmp_win->frame_bw3D, cp,
			(Scr->BeNiceToColormap) ? on : off, 222);
	else
	{
	    if (!Scr->NoBorderDecorations)	/* djhjr - 10/20/02 */
	    {
		if (i < tmp_win->frame_width - cwbw)
			Draw3DBevel (tmp_win->frame,
				tmp_win->frame_width - cwbw, Scr->TitleHeight,
				tmp_win->frame_bw3D, cp,
				(Scr->BeNiceToColormap) ? on : off, 1);
		Draw3DBevel (tmp_win->frame,
			tmp_win->frame_width - tmp_win->frame_bw3D,
			Scr->TitleHeight + chbw,
			tmp_win->frame_bw3D, cp,
			(Scr->BeNiceToColormap) ? on : off, 2);
	    }
	}
    if (!Scr->NoBorderDecorations)	/* djhjr - 10/20/02 */
    {
	/* lower left corner */
	Draw3DBevel (tmp_win->frame,
		cwbw, tmp_win->frame_height - tmp_win->frame_bw3D,
		tmp_win->frame_bw3D, cp,
		(Scr->BeNiceToColormap) ? on : off, 1);
	Draw3DBevel (tmp_win->frame,
		0, tmp_win->frame_height - chbw,
		tmp_win->frame_bw3D, cp,
		(Scr->BeNiceToColormap) ? on : off, 2);
	/* lower right corner */
	Draw3DBevel (tmp_win->frame,
		tmp_win->frame_width - cwbw,
		tmp_win->frame_height - tmp_win->frame_bw3D,
		tmp_win->frame_bw3D, cp,
		(Scr->BeNiceToColormap) ? on : off, 1);
	Draw3DBevel (tmp_win->frame,
		tmp_win->frame_width - tmp_win->frame_bw3D,
		tmp_win->frame_height - chbw,
		tmp_win->frame_bw3D, cp,
		(Scr->BeNiceToColormap) ? on : off, 2);
    }
	/* title */
	borderdashoffset = 0;
	Draw3DBorder (tmp_win->frame,
		tmp_win->title_x - tmp_win->frame_bw3D,
		tmp_win->title_y - tmp_win->frame_bw3D,
		tmp_win->title_width + 2 * tmp_win->frame_bw3D,
		Scr->TitleHeight + tmp_win->frame_bw3D,
		Scr->BorderBevelWidth, cp, off, True, False);
	borderdashoffset = 0;
	Draw3DBorder (tmp_win->frame,
		tmp_win->title_x - Scr->BorderBevelWidth,
		tmp_win->title_y - Scr->BorderBevelWidth,
		tmp_win->title_width + 2 * Scr->BorderBevelWidth,
		Scr->TitleHeight,
		Scr->BorderBevelWidth, cp, on, True, False);
	/* upper left corner */
	if (tmp_win->title_x == tmp_win->frame_bw3D)
	{
	    /* this 'if (...) else' - djhjr - 10/20/02 */
	    if (Scr->NoBorderDecorations)
	    {
		Draw3DNoBevel(tmp_win->frame,
			tmp_win->title_x - tmp_win->frame_bw3D,
			Scr->TitleHeight + tmp_win->frame_bw3D,
			tmp_win->frame_bw3D, cp,
			(Scr->BeNiceToColormap) ? off : on, False);
	    }
	    else
	    {
		gc = setBevelGC(2, (Scr->BeNiceToColormap) ? on : off, cp);

/* djhjr - 4/29/98
		XDrawLine (dpy, tmp_win->frame, gc,
			tmp_win->title_x - 2, Scr->TitleHeight + tmp_win->frame_bw3D - 4,
			tmp_win->title_x - 2, Scr->TitleHeight + tmp_win->frame_bw3D - 1);
		XDrawLine (dpy, tmp_win->frame, gc,
			tmp_win->title_x - 1, Scr->TitleHeight + tmp_win->frame_bw3D - 4,
			tmp_win->title_x - 1, Scr->TitleHeight + tmp_win->frame_bw3D - 1);
*/
		for (j = 1; j <= Scr->BorderBevelWidth; j++)
			XDrawLine (dpy, tmp_win->frame, gc,
				tmp_win->title_x - j,
				Scr->TitleHeight + tmp_win->frame_bw3D - 2 * Scr->BorderBevelWidth,
				tmp_win->title_x - j,
				Scr->TitleHeight + tmp_win->frame_bw3D - Scr->BorderBevelWidth - 1);

		Draw3DBevel (tmp_win->frame,
			tmp_win->title_x + cw, 0,
			tmp_win->frame_bw3D, cp,
			(Scr->BeNiceToColormap) ? on : off, 1);
	    }
	}
	else
	{
		Draw3DBevel (tmp_win->frame,
			tmp_win->title_x - tmp_win->frame_bw3D, Scr->TitleHeight,
			tmp_win->frame_bw3D, cp, off, 3);
	}
	/* upper right corner */
	if (i == tmp_win->frame_width)
	{
	    /* this 'if (...) else' - djhjr - 10/20/02 */
	    if (Scr->NoBorderDecorations)
	    {
		Draw3DNoBevel(tmp_win->frame,
			tmp_win->frame_width - tmp_win->frame_bw3D,
			Scr->TitleHeight + tmp_win->frame_bw3D,
			tmp_win->frame_bw3D, cp,
			(Scr->BeNiceToColormap) ? off : on, False);
	    }
	    else
	    {
		gc = setBevelGC(1, (Scr->BeNiceToColormap) ? off : on, cp);

/* djhjr - 4/29/98
		gc = setBevelGC(1, on, cp);
		XDrawLine (dpy, tmp_win->frame, gc,
			tmp_win->title_x + tmp_win->title_width, Scr->TitleHeight + tmp_win->frame_bw3D - 2,
			tmp_win->title_x + tmp_win->title_width, Scr->TitleHeight + tmp_win->frame_bw3D - 2);
*/
		for (j = 0; j < Scr->BorderBevelWidth; j++)
			XDrawLine (dpy, tmp_win->frame, gc,
				tmp_win->title_x + tmp_win->title_width - 1,
				Scr->TitleHeight + tmp_win->frame_bw3D - Scr->BorderBevelWidth + j - 1,
				tmp_win->title_x + tmp_win->title_width + Scr->BorderBevelWidth - j - 1,
				Scr->TitleHeight + tmp_win->frame_bw3D - Scr->BorderBevelWidth + j - 1);

		Draw3DBevel (tmp_win->frame,
			tmp_win->title_x + tmp_win->title_width - cw, 0,
			tmp_win->frame_bw3D, cp,
			(Scr->BeNiceToColormap) ? on : off, 1);
	    }
	}
	else
	{
		Draw3DBevel (tmp_win->frame,
			tmp_win->title_x + tmp_win->title_width, Scr->TitleHeight,
			tmp_win->frame_bw3D, cp, off, 4);
	}
#endif /* USE_ORIGINAL_CORNERS */
}

/* djhjr - 4/21/96 */
void PaintIcon (tmp_win)
TwmWindow *tmp_win;
{
/* djhjr - 5/5/98
    if (Scr->use3Diconmanagers) {
*/
	/* was 'Scr->use3Dicons' - djhjr - 8/11/98 */
    if (Scr->IconBevelWidth > 0) {

/*
	Draw3DBorder (tmp_win->icon_w, 0, tmp_win->icon_height,
		tmp_win->icon_w_width, Scr->IconFont.height + 6,
		Scr->BorderBevelWidth, tmp_win->iconc, off, False, False);
*/
	/* was 'Scr->BorderBevelWidth' - djhjr - 8/11/98 */
	Draw3DBorder (tmp_win->icon_w, 0, 0,
		tmp_win->icon_w_width, tmp_win->icon_w_height,
		Scr->IconBevelWidth, tmp_win->iconc, off, False, False);
    }

    /* font was font.font->fid - djhjr - 9/14/03 */
    FBF(tmp_win->iconc.fore, tmp_win->iconc.back, Scr->IconFont);

	MyFont_DrawString (dpy, tmp_win->icon_w, &Scr->IconFont,
		Scr->NormalGC, tmp_win->icon_x, tmp_win->icon_y, 
		tmp_win->icon_name, strlen(tmp_win->icon_name));
}

/* djhjr - 4/20/96 */
void PaintTitle (tmp_win)
TwmWindow *tmp_win;
{
	/* made static - djhjr - 6/18/99 */
	static int en = 0, dots = 0;

	int bwidth = Scr->TBInfo.width + Scr->TBInfo.pad;
	int left = (Scr->TBInfo.nleft) ? Scr->TBInfo.leftx +
		   (Scr->TBInfo.nleft * bwidth) - Scr->TBInfo.pad :
		   0;
	int right = (Scr->TBInfo.nright) ?
		    (Scr->TBInfo.nright * bwidth) - Scr->TBInfo.pad :
		    0;

	/* djhjr - 3/29/98 */
	int cur_computed_scrlen;
	int max_avail_scrlen;
	int cur_string_len = strlen(tmp_win->name);
	char *a = NULL;

	if (!en) en = MyFont_TextWidth(&Scr->TitleBarFont, "n", 1);

	/*
	 * clip the title a couple of characters less than the width of
	 * the titlebar plus padding, and tack on ellipses - this is a
	 * little different than the icon manager's...
	 *
	 * djhjr - 3/29/98
	 */
	if (Scr->NoPrettyTitles == FALSE) /* for rader - djhjr - 2/9/99 */
	{
		cur_computed_scrlen = MyFont_TextWidth(&Scr->TitleBarFont, tmp_win->name, cur_string_len);

		if (!dots) dots = MyFont_TextWidth(&Scr->TitleBarFont, "...", 3);

		max_avail_scrlen = tmp_win->title_width - Scr->TBInfo.titlex - Scr->TBInfo.rightoff - en;

/* djhjr - 10/18/02
		* djhjr - 5/5/98 *
		* was 'Scr->use3Dtitles' - djhjr - 8/11/98 *
		if (Scr->TitleBevelWidth > 0)
			max_avail_scrlen -= Scr->TitleBevelWidth;
*/
		/* reworked this stuff - djhjr - 10/18/02 */
		if (dots >= max_avail_scrlen)
			cur_string_len = 0;
		else if (cur_computed_scrlen > max_avail_scrlen)
		{
			while (cur_string_len >= 0)
			{
				if (MyFont_TextWidth(&Scr->TitleBarFont,
/* sjr - 10/06/06 */
						tmp_win->name, cur_string_len) + dots < max_avail_scrlen)
				{
					break;
				}

				cur_string_len--;
			}

			a = (char *)malloc(cur_string_len + 4);
			memcpy(a, tmp_win->name, cur_string_len);
			strcpy(a + cur_string_len, "...");
			cur_string_len += 3;
		}
	}

    /* font was font.font->fid - djhjr - 9/14/03 */
    FBF(tmp_win->title.fore, tmp_win->title.back, Scr->TitleBarFont);

/* djhjr - 10/18/02 */
#if 0
	/* was 'Scr->use3Dtitles' - djhjr - 8/11/98 */
    if (Scr->TitleBevelWidth > 0)
	{
	    MyFont_DrawString (dpy, tmp_win->title_w, &Scr->TitleBarFont,
		 Scr->NormalGC,
/* djhjr - 4/29/98
		 Scr->TBInfo.titlex + en, Scr->TitleBarFont.y + 2, 
*/
/* djhjr - 5/5/98
		 Scr->TBInfo.titlex + en, Scr->TitleBarFont.y + Scr->TitleBevelWidth + 1, 
*/
		 Scr->TBInfo.titlex + Scr->TitleBevelWidth + en, Scr->TitleBarFont.y, 

		 (a) ? a : tmp_win->name, cur_string_len);
	}
    else
#endif
        MyFont_DrawString (dpy, tmp_win->title_w, &Scr->TitleBarFont,
		 Scr->NormalGC, Scr->TBInfo.titlex, Scr->TitleBarFont.y,
		 (a) ? a : tmp_win->name, cur_string_len);

	/* free the clipped title - djhjr - 3/29/98 */
	if (a) free(a);

	/* was 'Scr->use3Dtitles' - djhjr - 8/11/98 */
	if (Scr->TitleBevelWidth > 0)
	{
/*
	    Draw3DBorder (tmp_win->title_w, Scr->TBInfo.titlex, 0,
		tmp_win->title_width - Scr->TBInfo.titlex - Scr->TBInfo.rightoff,
		Scr->TitleHeight, Scr->TitleBevelWidth, tmp_win->title, off, True, False);
*/
/* djhjr - 3/12/97
	    Draw3DBorder (tmp_win->title_w, Scr->TBInfo.rightoff, Scr->FramePadding,
		tmp_win->title_width - 2 * Scr->TBInfo.rightoff,
		Scr->TitleHeight - 2 * Scr->FramePadding, Scr->TitleBevelWidth, tmp_win->title, off, True, False);
*/
/* djhjr - 10/17/02
	    Draw3DBorder (tmp_win->title_w, Scr->TBInfo.leftx + left,
		Scr->FramePadding, tmp_win->title_width - (left + right),
		Scr->TitleHeight - 2 * Scr->FramePadding, Scr->TitleBevelWidth, tmp_win->title, off, False, False);
*/
	    if (Scr->FramePadding + Scr->ButtonIndent > 0)
	    {
		Draw3DBorder(tmp_win->title_w, 0, 0,
			     tmp_win->title_width, Scr->TitleHeight,
			     Scr->TitleBevelWidth, tmp_win->title,
			     off, False, False);
	    }
	    else
	    {
		Draw3DBorder(tmp_win->title_w, left, 0,
			     tmp_win->title_width - (left + right),
			     Scr->TitleHeight,
			     Scr->TitleBevelWidth, tmp_win->title,
			     off, False, False);
	    }
	}
}

/* djhjr - 11/17/97 */
/* collapsed the two functions PTB() and PTBH() - djhjr - 8/10/98 */
/* djhjr - 10/30/02 */
void PaintTitleButton(tmp_win, tbw, onoroff)
TwmWindow *tmp_win;
TBWindow  *tbw;
int onoroff; /* 0 = no hilite    1 = hilite off    2 = hilite on */
{
    Image *image;
    TitleButton *tb;
    /* djhjr - 5/23/98 8/10/98 */
    ColorPair cp;

    if (!tbw->window) return;

    tb = tbw->info;

    /* djhjr - 5/23/98 8/10/98 */
    if (Scr->ButtonColorIsFrame)
	cp = (onoroff == 2) ? tmp_win->border : tmp_win->border_tile;
    else
	cp = tmp_win->title;
    cp.fore = tmp_win->title.fore;

    image = GetImage(tb->name, tb->width, tb->height,
		     Scr->ButtonBevelWidth * 2, cp);

    XCopyArea(dpy, image->pixmap, tbw->window, Scr->NormalGC,
	      tb->srcx, tb->srcy, tb->width, tb->height, tb->dstx, tb->dsty);

    /* djhjr - 8/10/98 8/18/98 */
    /* was 'Scr->TitleBevelWidth' - djhjr - 8/11/98 */
    if (Scr->ButtonBevelWidth > 0)
	Draw3DBorder(tbw->window, 0, 0, tb->width, tb->height,
		     Scr->ButtonBevelWidth, cp, off, False, False);
}

/* djhjr - 4/19/96 */
/* re-arranged - djhjr - 4/5/98 */
void PaintTitleHighlight(tmp_win, onoroff)
TwmWindow *tmp_win;
Bool onoroff;
{
	/* djhjr - 4/3/98 */
	if (!tmp_win->titlehighlight) return;

	if (tmp_win->hilite_w)
	{
		if (onoroff == on)
			XMapWindow (dpy, tmp_win->hilite_w);
		else
			XUnmapWindow (dpy, tmp_win->hilite_w);
	}
	else if (Scr->hiliteName && tmp_win->title_height != 0)
		DrawTitleHighlight(tmp_win, onoroff);	/* djhjr - 10/25/02 */
}

/* djhjr - 4/2/98 10/18/02 */
int ComputeHighlightWindowWidth(tmp_win)
TwmWindow *tmp_win;
{
	int en = MyFont_TextWidth(&Scr->TitleBarFont, "n", 1);

	return (tmp_win->rightx - tmp_win->highlightx - en);
}

