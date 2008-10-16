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
#include "image_formats.h"
#include "util.h"
#include "gram.h"
#include "screen.h"
#include "list.h"
#include "prototypes.h"
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
#define ZOOMSLEEP 50000		/* arbitrary, but pleasing, msec value */
#endif

/*
 * All instances of Scr->TitleBevelWidth and Scr->BorderBevelWidth
 * were a hard value of 2 - djhjr - 4/29/98
 */

static void Draw3DMenuImage(Drawable d, int x, int y, int w, int h, int pad, ColorPair cp, int state);
static void Draw3DDotImage(Drawable d, int x, int y, int w, int h, int pad, ColorPair cp, int state);
static void Draw3DResizeImage(Drawable d, int x, int y, int w, int h, int pad, ColorPair cp, int state);
static void Draw3DZoomImage(Drawable d, int x, int y, int w, int h, int pad, ColorPair cp, int state);
static void Draw3DBarImage(Drawable d, int x, int y, int w, int h, int pad, ColorPair cp, int state);
static void Draw3DRArrowImage(Drawable d, int x, int y, int w, int h, int pad, ColorPair cp, int state);
static void Draw3DDArrowImage(Drawable d, int x, int y, int w, int h, int pad, ColorPair cp, int state);
static void setBorderGC(int type, GC gc, ColorPair cp, int state, int forcebw);
static GC setBevelGC(int type, int state, ColorPair cp);
static void Draw3DBevel(Drawable w, int x, int y, int bw, ColorPair cp, int state, int type);
static void Draw3DNoBevel(Drawable w, int x, int y, int bw, ColorPair cp, int state, int forcebw);
static void draw_rect(Display * display, Drawable drawable, GC gc, int x, int y, unsigned int width, unsigned int height);
static void draw_scaled_rect(Display * display, Drawable drawable, GC gc, int x, int y, unsigned int width, unsigned int height,
			     unsigned int scale, unsigned int over);

/* for trying to clean up BeNiceToColormap - djhjr - 10/20/02 */
static int borderdashoffset;

int HotX, HotY;

#define questionmark_width (unsigned int)(8)
#define questionmark_height (unsigned int)(8)
static char questionmark_bits[] = {
  0x38, 0x7c, 0x64, 0x30, 0x18, 0x00, 0x18, 0x18
};

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

void
MoveOutline(Window root, int x, int y, int width, int height, int bw, int th)
{
  static int lastx = 0;
  static int lasty = 0;
  static int lastWidth = 0;
  static int lastHeight = 0;
  static int lastBW = 0;
  static int lastTH = 0;
  int xl, xr, yt, yb, xinnerl, xinnerr, yinnert, yinnerb;
  int xthird, ythird;
  XSegment outline[18];
  register XSegment *r;

  if (x == lastx && y == lasty && width == lastWidth && height == lastHeight && lastBW == bw && th == lastTH)
    return;

  r = outline;

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
  DRAWIT();

  lastx = x;
  lasty = y;
  lastWidth = width;
  lastHeight = height;
  lastBW = bw;
  lastTH = th;

  /* draw the new one, if any */
  DRAWIT();
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
Zoom(Window wf, IconMgr * ipf, Window wt, IconMgr * ipt)
{
  int fx, fy, tx, ty;		/* from, to */
  unsigned int fw, fh, tw, th;	/* from, to */
  long dx, dy, dw, dh;
  long z;
  int j;

  static struct timeval timeoutval = { 0, ZOOMSLEEP };
  struct timeval timeout;

  if (!Scr->DoZoom || Scr->ZoomCount < 1)
    return;

  if (wt == None)
  {
    if (Scr->LessRandomZoomZoom)
    {
      int temp, x1, y1, x2, y2;

      x1 = ((int)rand() % (Scr->MyDisplayWidth >> 3));
      x2 = Scr->MyDisplayWidth - ((int)rand() % (Scr->MyDisplayWidth >> 3));
      y1 = ((int)rand() % (Scr->MyDisplayHeight >> 3));
      y2 = Scr->MyDisplayHeight - ((int)rand() % (Scr->MyDisplayHeight >> 3));
      if (x1 > x2)
      {
	temp = x1;
	x1 = x2;
	x2 = temp;
      }
      if (y1 > y2)
      {
	temp = y1;
	y1 = y2;
	y2 = temp;
      }
      tx = x1;
      ty = y1;
      tw = x2 - x1;
      th = y2 - y1;
    }
    else
    {
      /* Zoom from nowhere, RFBZOOM */
      tx = ((int)rand() % Scr->MyDisplayWidth);
      tw = ((int)rand() % Scr->MyDisplayWidth);
      ty = ((int)rand() % Scr->MyDisplayWidth);
      th = ((int)rand() % Scr->MyDisplayWidth);
    }
  }
  else
  {				/* Normal. */
    XGetGeometry(dpy, wt, &JunkRoot, &tx, &ty, &tw, &th, &JunkBW, &JunkDepth);

    if (ipt)
    {
      tx += (ipt->twm_win->frame_x + ipt->twm_win->title_x);
      ty += (ipt->twm_win->frame_y + ipt->twm_win->title_y + ipt->twm_win->title_height);
    }
  }

  if (wf == None)
  {
    if (Scr->LessRandomZoomZoom)
    {
      /* zoom from somewhere on the screen, DSE */
      int temp, x1, y1, x2, y2;

      do
      {
	x1 = ((int)rand() % Scr->MyDisplayWidth);
	x2 = ((int)rand() % Scr->MyDisplayWidth);
	y1 = ((int)rand() % Scr->MyDisplayHeight);
	y2 = ((int)rand() % Scr->MyDisplayHeight);
	if (x1 > x2)
	{
	  temp = x1;
	  x1 = x2;
	  x2 = temp;
	}
	if (y1 > y2)
	{
	  temp = y1;
	  y1 = y2;
	  y2 = temp;
	}
	fx = x1;
	fy = y1;
	fw = x2 - x1;
	fh = y2 - y1;
      }
      while (fw > (Scr->MyDisplayWidth >> 2) || fh > (Scr->MyDisplayHeight >> 2));
    }
    else
    {
      /* Zoom from nowhere, RFB */
      /* fx = ( rand() & 1 ) * Scr->MyDisplayWidth; */
      fx = ((int)rand() % Scr->MyDisplayWidth);
      fw = ((int)rand() % Scr->MyDisplayWidth);
      fy = ((int)rand() % Scr->MyDisplayWidth);
      fh = ((int)rand() % Scr->MyDisplayWidth);
    }
  }

  else
  {				/* Normal. */
    XGetGeometry(dpy, wf, &JunkRoot, &fx, &fy, &fw, &fh, &JunkBW, &JunkDepth);

    if (ipf)
    {
      fx += (ipf->twm_win->frame_x + ipf->twm_win->title_x);
      fy += (ipf->twm_win->frame_y + ipf->twm_win->title_y + ipf->twm_win->title_height);
    }
  }

  dx = ((long)(tx - fx));	/* going from -> to */
  dy = ((long)(ty - fy));	/* going from -> to */
  dw = ((long)(tw - fw));	/* going from -> to */
  dh = ((long)(th - fh));	/* going from -> to */
  z = (long)(Scr->ZoomCount + 1);

  for (j = 0; j < 2; j++)
  {
    long i;

    draw_rect(dpy, Scr->Root, Scr->DrawGC, fx, fy, fw, fh);
    for (i = 1; i < z; i++)
    {
      int x = fx + (int)((dx * i) / z);
      int y = fy + (int)((dy * i) / z);
      unsigned width = (unsigned)(((long)fw) + (dw * i) / z);
      unsigned height = (unsigned)(((long)fh) + (dh * i) / z);

      draw_rect(dpy, Scr->Root, Scr->DrawGC, x, y, width, height);
    }
    draw_rect(dpy, Scr->Root, Scr->DrawGC, tx, ty, tw, th);

    timeout = timeoutval;
    select(0, 0, 0, 0, &timeout);
  }
}


/*
 *	Use any routine to draw your own rectangles here. -- DSE
 */
void
draw_rect(Display * display, Drawable drawable, GC gc, int x, int y, unsigned int width, unsigned int height)
{
  draw_scaled_rect(display, drawable, gc, x, y, width, height, 20, 20);
  if (Scr->PrettyZoom)
  {
    draw_scaled_rect(display, drawable, gc, x, y, width, height, 18, 20);
    draw_scaled_rect(display, drawable, gc, x, y, width, height, 16, 20);
  }
}
void
draw_scaled_rect(Display * display, Drawable drawable, GC gc, int x, int y, unsigned int width, unsigned int height,
		 unsigned int scale, unsigned int over)
{
  XDrawRectangle(dpy, drawable, gc,
		 x + (over + width * (over - scale)) / (2 * over),
		 y + (over + width * (over - scale)) / (2 * over),
		 ((over / 2) + width * scale) / over, ((over / 2) + height * scale) / over);
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
ExpandFilename(char *name)
{
  char *newname;

  if (name[0] != '~')
    return name;

  newname = (char *)malloc(HomeLen + strlen(name) + 2);
  if (!newname)
  {
    fprintf(stderr,
	    "%s:  unable to allocate %lu bytes to expand filename %s/%s\n",
	    ProgramName, HomeLen + strlen(name) + 2, Home, &name[1]);
  }
  else
  {
    (void)sprintf(newname, "%s/%s", Home, &name[1]);
  }

  return newname;
}


void
InsertRGBColormap(Atom a, XStandardColormap * maps, int nmaps, Bool replace)
{
  StdCmap *sc = NULL;

  if (replace)
  {				/* locate existing entry */
    for (sc = Scr->StdCmapInfo.head; sc; sc = sc->next)
    {
      if (sc->atom == a)
	break;
    }
  }

  if (!sc)
  {				/* no existing, allocate new */
    sc = (StdCmap *) malloc(sizeof(StdCmap));
    if (!sc)
    {
      fprintf(stderr, "%s:  unable to allocate %lu bytes for StdCmap\n", ProgramName, sizeof(StdCmap));
      return;
    }
  }

  if (replace)
  {				/* just update contents */
    if (sc->maps)
      XFree((char *)maps);
    if (sc == Scr->StdCmapInfo.mru)
      Scr->StdCmapInfo.mru = NULL;
  }
  else
  {				/* else appending */
    sc->next = NULL;
    sc->atom = a;
    if (Scr->StdCmapInfo.tail)
    {
      Scr->StdCmapInfo.tail->next = sc;
    }
    else
    {
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
 */
Image *
SetPixmapsPixmap(char *filename)
{
  Pixmap bm;
  Image *image = NULL;

  bm = FindBitmap(filename, &JunkWidth, &JunkHeight);
  if (bm != None)
  {
    image = (Image *) malloc(sizeof(Image));

    image->width = JunkWidth;
    image->height = JunkHeight;
    image->mask = None;

    image->pixmap = bm;
  }
#ifndef NO_XPM_SUPPORT
  else				/* Try to find a pixmap file with this name */
    image = FindImage(filename, 0);
#endif

  return image;
}


void
RemoveRGBColormap(Atom a)
{
  StdCmap *sc, *prev;

  prev = NULL;
  for (sc = Scr->StdCmapInfo.head; sc; sc = sc->next)
  {
    if (sc->atom == a)
      break;
    prev = sc;
  }
  if (sc)
  {				/* found one */
    if (sc->maps)
      XFree((char *)sc->maps);
    if (prev)
      prev->next = sc->next;
    if (Scr->StdCmapInfo.head == sc)
      Scr->StdCmapInfo.head = sc->next;
    if (Scr->StdCmapInfo.tail == sc)
      Scr->StdCmapInfo.tail = prev;
    if (Scr->StdCmapInfo.mru == sc)
      Scr->StdCmapInfo.mru = NULL;
  }
  return;
}

void
LocateStandardColormaps(void)
{
  Atom *atoms;
  int natoms;
  int i;

  atoms = XListProperties(dpy, Scr->Root, &natoms);
  for (i = 0; i < natoms; i++)
  {
    XStandardColormap *maps = NULL;
    int nmaps;

    if (XGetRGBColormaps(dpy, Scr->Root, &maps, &nmaps, atoms[i]))
    {
      /* if got one, then append to current list */
      InsertRGBColormap(atoms[i], maps, nmaps, False);
    }
  }
  if (atoms)
    XFree((char *)atoms);
  return;
}

void
GetColor(int kind, Pixel * what, char *name)
{
  XColor color, junkcolor;
  Status stat = 0;
  Colormap cmap = Scr->TwmRoot.cmaps.cwins[0]->colormap->c;

  if (!Scr->FirstTime)
    return;

  if (Scr->Monochrome != kind)
    return;

#if ( XlibSpecificationRelease < 5 )
  if (!((name[0] == '#')
	? ((stat = XParseColor(dpy, cmap, name, &color))
	   && XAllocColor(dpy, cmap, &color)) : XAllocNamedColor(dpy, cmap, name, &color, &junkcolor)))
#else
  if (!XAllocNamedColor(dpy, cmap, name, &color, &junkcolor))
#endif
  {
    /* if we could not allocate the color, let's see if this is a
     * standard colormap
     */
    XStandardColormap *stdcmap = NULL;

    /* parse the named color */
    if (name[0] != '#')
      stat = XParseColor(dpy, cmap, name, &color);
    if (!stat)
    {
      fprintf(stderr, "%s:  invalid color name \"%s\"\n", ProgramName, name);
      return;
    }

    /*
     * look through the list of standard colormaps (check cache first)
     */
    if (Scr->StdCmapInfo.mru && Scr->StdCmapInfo.mru->maps &&
	(Scr->StdCmapInfo.mru->maps[Scr->StdCmapInfo.mruindex].colormap == cmap))
    {
      stdcmap = &(Scr->StdCmapInfo.mru->maps[Scr->StdCmapInfo.mruindex]);
    }
    else
    {
      StdCmap *sc;

      for (sc = Scr->StdCmapInfo.head; sc; sc = sc->next)
      {
	int i;

	for (i = 0; i < sc->nmaps; i++)
	{
	  if (sc->maps[i].colormap == cmap)
	  {
	    Scr->StdCmapInfo.mru = sc;
	    Scr->StdCmapInfo.mruindex = i;
	    stdcmap = &(sc->maps[i]);
	    goto gotit;
	  }
	}
      }
    }

  gotit:
    if (stdcmap)
    {
      color.pixel = (stdcmap->base_pixel +
		     ((Pixel) (((float)color.red / 65535.0) *
			       stdcmap->red_max + 0.5) *
		      stdcmap->red_mult) +
		     ((Pixel) (((float)color.green / 65535.0) *
			       stdcmap->green_max + 0.5) *
		      stdcmap->green_mult) +
		     ((Pixel) (((float)color.blue / 65535.0) * stdcmap->blue_max + 0.5) * stdcmap->blue_mult));
    }
    else
    {
      fprintf(stderr, "%s:  unable to allocate color \"%s\"\n", ProgramName, name);
      return;
    }
  }

  *what = color.pixel;
}

void
GetShadeColors(ColorPair * cp)
{
  XColor xcol;
  Colormap cmap = Scr->TwmRoot.cmaps.cwins[0]->colormap->c;
  int save;
  float clearfactor;
  float darkfactor;
  char clearcol[32], darkcol[32];

  clearfactor = (float)Scr->ClearBevelContrast / 100.0;
  darkfactor = (100.0 - (float)Scr->DarkBevelContrast) / 100.0;
  xcol.pixel = cp->back;
  XQueryColor(dpy, cmap, &xcol);

  sprintf(clearcol, "#%04x%04x%04x",
	  xcol.red + (unsigned short)((65535 - xcol.red) * clearfactor),
	  xcol.green + (unsigned short)((65535 - xcol.green) * clearfactor),
	  xcol.blue + (unsigned short)((65535 - xcol.blue) * clearfactor));
  sprintf(darkcol, "#%04x%04x%04x",
	  (unsigned short)(xcol.red * darkfactor),
	  (unsigned short)(xcol.green * darkfactor), (unsigned short)(xcol.blue * darkfactor));

  save = Scr->FirstTime;
  Scr->FirstTime = True;
  GetColor(Scr->Monochrome, &cp->shadc, clearcol);
  GetColor(Scr->Monochrome, &cp->shadd, darkcol);
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
GetFont(MyFont * font)
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

#ifdef TWM_USE_XFT

  XFontStruct *xlfd;
  Atom atom;
  char *atom_name;
  char *deffontname;

  if (Scr->use_xft > 0)
  {

    if (font->xft != NULL)
    {
      XftFontClose(dpy, font->xft);
      font->xft = NULL;
    }

    /* test if core font: */
    fnum = 0;
    xlfd = NULL;
    font_names = XListFontsWithInfo(dpy, font->name, 5, &fnum, &xlfd);
    if (font_names != NULL && xlfd != NULL)
    {
      for (i = 0; i < fnum; ++i)
	if (XGetFontProperty(xlfd + i, XA_FONT, &atom) == True)
	{
	  atom_name = XGetAtomName(dpy, atom);
	  if (atom_name)
	  {
	    font->xft = XftFontOpenXlfd(dpy, Scr->screen, atom_name);
	    XFree(atom_name);
	    break;
	  }
	}
      XFreeFontInfo(font_names, xlfd, fnum);
    }

    /* next, try Xft font: */
    if (font->xft == NULL)
      font->xft = XftFontOpenName(dpy, Scr->screen, font->name);

    /* fallback: */
    if (font->xft == NULL)
    {
      if (Scr->DefaultFont.name != NULL)
	deffontname = Scr->DefaultFont.name;
      else
	deffontname = "fixed";

      fnum = 0;
      xlfd = NULL;
      font_names = XListFontsWithInfo(dpy, deffontname, 5, &fnum, &xlfd);
      if (font_names != NULL && xlfd != NULL)
      {
	for (i = 0; i < fnum; ++i)
	  if (XGetFontProperty(xlfd + i, XA_FONT, &atom) == True)
	  {
	    atom_name = XGetAtomName(dpy, atom);
	    if (atom_name)
	    {
	      font->xft = XftFontOpenXlfd(dpy, Scr->screen, atom_name);
	      XFree(atom_name);
	      break;
	    }
	  }
	XFreeFontInfo(font_names, xlfd, fnum);
      }

      if (font->xft == NULL)
	font->xft = XftFontOpenName(dpy, Scr->screen, deffontname);

      if (font->xft == NULL)
      {
	fprintf(stderr, "%s:  unable to open fonts \"%s\" or \"%s\"\n", ProgramName, font->name, deffontname);
	exit(1);
      }
    }

    font->height = font->xft->ascent + font->xft->descent;
    font->y = font->xft->ascent;
    font->ascent = font->xft->ascent;
    font->descent = font->xft->descent;

    return;
  }

#endif /*TWM_USE_XFT */

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
					&missing_charset_list_return, &missing_charset_count_return, &def_string_return)) == NULL)
    {

      if ((basename3 = (char *)realloc(basename2, strlen(Scr->DefaultFont.name) + 3)))
	sprintf(basename3, "%s,*", Scr->DefaultFont.name);
      else
      {
	basename3 = Scr->DefaultFont.name;
	if (basename2 != font->name)
	  free(basename2);
      }
      if ((font->fontset = XCreateFontSet(dpy, basename3,
					  &missing_charset_list_return, &missing_charset_count_return, &def_string_return)) == NULL)
      {
	fprintf(stderr, "%s: unable to open fontsets \"%s\" or \"%s\"\n", ProgramName, font->name, Scr->DefaultFont.name);
	if (basename3 != Scr->DefaultFont.name)
	  free(basename3);
	exit(1);
      }
      basename2 = basename3;
    }

    if (basename2 != ((basename3) ? Scr->DefaultFont.name : font->name))
      free(basename2);

    for (i = 0; i < missing_charset_count_return; i++)
      fprintf(stderr, "%s: font for charset %s is lacking\n", ProgramName, missing_charset_list_return[i]);

    font_extents = XExtentsOfFontSet(font->fontset);
    fnum = XFontsOfFontSet(font->fontset, &xfonts, &font_names);
    for (i = 0, ascent = 0, descent = 0; i < fnum; i++)
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
      fprintf(stderr, "%s:  unable to open fonts \"%s\" or \"%s\"\n", ProgramName, font->name, Scr->DefaultFont.name);
      exit(1);
    }

  font->height = font->font->ascent + font->font->descent;
  font->y = font->font->ascent;
  font->ascent = font->font->ascent;
  font->descent = font->font->descent;
}

int
MyFont_TextWidth(MyFont * font, char *string, int len)
{
  XRectangle ink_rect;
  XRectangle logical_rect;

#ifdef TWM_USE_XFT
  XGlyphInfo size;

  if (Scr->use_xft > 0)
  {
#ifdef X_HAVE_UTF8_STRING
    if (use_fontset)
      XftTextExtentsUtf8(dpy, font->xft, (XftChar8 *) (string), len, &size);
    else
#endif
      XftTextExtents8(dpy, font->xft, (XftChar8 *) (string), len, &size);
    return size.width;
  }
#endif

  if (use_fontset)
  {
    XmbTextExtents(font->fontset, string, len, &ink_rect, &logical_rect);
    return logical_rect.width;
  }
  return XTextWidth(font->font, string, len);
}

void
MyFont_DrawImageString(Display * dpy, MyWindow * win, MyFont * font, ColorPair * col, int x, int y, char *string, int len)
{
#ifdef TWM_USE_XFT
  if (Scr->use_xft > 0)
  {
    if (win->xft)
    {
      XClearArea(dpy, win->win, 0, 0, 0, 0, False);
#ifdef X_HAVE_UTF8_STRING
      if (use_fontset)
	XftDrawStringUtf8(win->xft, &col->xft, font->xft, x, y, (XftChar8 *) (string), len);
      else
#endif
	XftDrawString8(win->xft, &col->xft, font->xft, x, y, (XftChar8 *) (string), len);
    }
    return;
  }
#endif

  Gcv.foreground = col->fore;
  Gcv.background = col->back;
  if (use_fontset)
  {
    XChangeGC(dpy, Scr->NormalGC, GCForeground | GCBackground, &Gcv);
    XmbDrawImageString(dpy, win->win, font->fontset, Scr->NormalGC, x, y, string, len);
  }
  else
  {
    Gcv.font = font->font->fid;
    XChangeGC(dpy, Scr->NormalGC, GCFont | GCForeground | GCBackground, &Gcv);
    XDrawImageString(dpy, win->win, Scr->NormalGC, x, y, string, len);
  }
}

void
MyFont_DrawString(Display * dpy, MyWindow * win, MyFont * font, ColorPair * col, int x, int y, char *string, int len)
{
#ifdef TWM_USE_XFT
  if (Scr->use_xft > 0)
  {
    if (win->xft)
    {
#ifdef X_HAVE_UTF8_STRING
      if (use_fontset)
	XftDrawStringUtf8(win->xft, &col->xft, font->xft, x, y, (XftChar8 *) (string), len);
      else
#endif
	XftDrawString8(win->xft, &col->xft, font->xft, x, y, (XftChar8 *) (string), len);
    }
    return;
  }
#endif

  Gcv.foreground = col->fore;
  Gcv.background = col->back;
  if (use_fontset)
  {
    XChangeGC(dpy, Scr->NormalGC, GCForeground | GCBackground, &Gcv);
    XmbDrawString(dpy, win->win, font->fontset, Scr->NormalGC, x, y, string, len);
  }
  else
  {
    Gcv.font = font->font->fid;
    XChangeGC(dpy, Scr->NormalGC, GCFont | GCForeground | GCBackground, &Gcv);
    XDrawString(dpy, win->win, Scr->NormalGC, x, y, string, len);
  }
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
I18N_FetchName(Display * dpy, Window w, char **winname)
{
  int status;
  XTextProperty text_prop;
  char **list;
  int num;

  text_prop.value = NULL;
  status = XGetWMName(dpy, w, &text_prop);
  if (!status || !text_prop.value || !text_prop.nitems)
  {
    if (text_prop.value)
      XFree(text_prop.value);
    *winname = NULL;
    return 0;
  }
  list = NULL;
#if defined TWM_USE_XFT && defined X_HAVE_UTF8_STRING
  if (use_fontset)
    status = Xutf8TextPropertyToTextList(dpy, &text_prop, &list, &num);
  else
#endif
    status = XmbTextPropertyToTextList(dpy, &text_prop, &list, &num);
  if (status < Success || !list || !*list)
    *winname = strdup((char *)text_prop.value);
  else
    *winname = strdup(*list);
  if (list)
    XFreeStringList(list);
  XFree(text_prop.value);
  return 1;
}

Status
I18N_GetIconName(Display * dpy, Window w, char **iconname)
{
  int status;
  XTextProperty text_prop;
  char **list;
  int num;

  text_prop.value = NULL;
  status = XGetWMIconName(dpy, w, &text_prop);
  if (!status || !text_prop.value || !text_prop.nitems)
  {
    if (text_prop.value)
      XFree(text_prop.value);
    *iconname = NULL;
    return 0;
  }
  list = NULL;
#if defined TWM_USE_XFT && defined X_HAVE_UTF8_STRING
  if (use_fontset)
    status = Xutf8TextPropertyToTextList(dpy, &text_prop, &list, &num);
  else
#endif
    status = XmbTextPropertyToTextList(dpy, &text_prop, &list, &num);
  if (status < Success || !list || !*list)
    *iconname = strdup((char *)text_prop.value);
  else
    *iconname = strdup(*list);
  if (list)
    XFreeStringList(list);
  XFree(text_prop.value);
  return 1;
}


/*
 * SetFocus - separate routine to set focus to make things more understandable
 * and easier to debug
 */
void
SetFocus(TwmWindow * tmp_win, Time time)
{
  Window w = (tmp_win ? tmp_win->w : PointerRoot);

#ifdef TRACE
  if (tmp_win)
  {
    printf("Focusing on window \"%s\"\n", tmp_win->full_name);
  }
  else
  {
    printf("Unfocusing; Focus was \"%s\"\n", Focus ? Focus->full_name : "(nil)");
  }
#endif

  XSetInputFocus(dpy, w, RevertToPointerRoot, time);
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
     const char *s;
{
  char *v;
  int varlen, idx;
  extern char **environ;
  char **newenv;
  static int virgin = 1;	/* true while "environ" is a virgin */

  v = index(s, '=');
  if (v == 0)
    return 0;			/* punt if it's not of the right form */
  varlen = (v + 1) - s;

  for (idx = 0; environ[idx] != 0; idx++)
  {
    if (strncmp(environ[idx], s, varlen) == 0)
    {
      if (v[1] != 0)
      {				/* true if there's a value */
	environ[idx] = (char *)s;
	return 0;
      }
      else
      {
	do
	{
	  environ[idx] = environ[idx + 1];
	} while (environ[++idx] != 0);
	return 0;
      }
    }
  }

  /* add to environment (unless no value; then just return) */
  if (v[1] == 0)
    return 0;
  if (virgin)
  {
    register i;

    newenv = (char **)malloc((unsigned)((idx + 2) * sizeof(char *)));
    if (newenv == 0)
      return -1;
    for (i = idx - 1; i >= 0; --i)
      newenv[i] = environ[i];
    virgin = 0;			/* you're not a virgin anymore, sweety */
  }
  else
  {
    newenv = (char **)realloc((char *)environ, (unsigned)((idx + 2) * sizeof(char *)));
    if (newenv == 0)
      return -1;
  }

  environ = newenv;
  environ[idx] = (char *)s;
  environ[idx + 1] = 0;

  return 0;
}
#endif /* NEED_PUTENV_F */


/* Returns a blank cursor */
Cursor
NoCursor(void)
{
  static Cursor blank = (Cursor) NULL;
  Pixmap nopixmap;
  XColor nocolor;

  if (!blank)
  {
    nopixmap = XCreatePixmap(dpy, Scr->Root, 1, 1, 1);
    nocolor.red = nocolor.blue = nocolor.green = 0;
    blank = XCreatePixmapCursor(dpy, nopixmap, nopixmap, &nocolor, &nocolor, 1, 1);
  }
  return (blank);
}

static void
DrawDotImage(Drawable d, int x, int y, int w, int h, int pad, ColorPair cp, int state)
{
  XPoint points[5];
  int wb, hb, ws, hs, lw;

  lw = (w > h) ? h / 16 : w / 16;
  if (lw == 1)
    lw = 0;
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

static void
DrawResizeImage(Drawable d, int x, int y, int w, int h, int pad, ColorPair cp, int state)
{
  XPoint points[3];
  int wb, hb, ws, hs, lw;

  lw = (w > h) ? h / 16 : w / 16;
  if (lw == 1)
    lw = 0;
  XSetForeground(dpy, Scr->RootGC, cp.fore);
  XSetLineAttributes(dpy, Scr->RootGC, lw, LineSolid, CapButt, JoinMiter);

  y--;
  wb = w / 4;			/* bigger width */
  hb = h / 4;			/* bigger width */
  ws = w / 2;			/* smaller width */
  hs = h / 2;			/* smaller width */

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

static void
DrawMenuImage(Drawable d, int x, int y, int w, int h, int pad, ColorPair cp, int state)
{
  int ih, iw;
  int ix, iy;
  int mh, mw;
  int tw, th;
  int lw, lh;
  int lx, ly;
  int lines, dly;
  int bw;

  if (h < 1)
    h = 1;
  if (w < 1)
    w = 1;

  ix = iy = pad + 1;
  ih = h - iy * 2;
  iw = w - ix * 2;
  mh = ih - ih / 8;
  mw = iw - iw / 8;
  bw = mh / 16;
  if (bw == 0 && mw > 2)
    bw = 1;
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
  if ((tw & 1) ^ (lw & 1))
    lw++;
  lx = ix + bw + (tw - lw) / 2;
  lh = th / 2 - bw;
  if ((lh & 1) ^ ((th - bw) & 1))
    lh++;
  ly = iy + bw + (th - bw - lh) / 2;
  lines = 3;
  if ((lh & 1) && lh < 6)
    lines--;
  dly = lh / (lines - 1);
  XSetForeground(dpy, Scr->RootGC, cp.fore);
  while (lines--)
  {
    XFillRectangle(dpy, d, Scr->RootGC, lx, ly, lw, bw);
    ly += dly;
  }
}

static void
DrawXLogoImage(Drawable d, int x, int y, int w, int h, int pad, ColorPair cp, int state)
{
  GC gcBack;
  XGCValues gcvalues;
  int lw;

#define siconify_width 11
#define siconify_height 11
  static unsigned char siconify_bits[] = {
    0xff, 0x07, 0x01, 0x04, 0x0d, 0x05, 0x9d, 0x05, 0xb9, 0x04, 0x51, 0x04,
    0xe9, 0x04, 0xcd, 0x05, 0x85, 0x05, 0x01, 0x04, 0xff, 0x07
  };

  if (w <= siconify_width || h <= siconify_height)
  {
    Pixmap p;

    p = XCreatePixmapFromBitmapData(dpy, Scr->Root, (char *)siconify_bits, siconify_width, siconify_height, 1, 0, 1);
    if (p != None)
    {
      FB(Scr, cp.fore, cp.back);
      XCopyPlane(dpy, p, d, Scr->NormalGC, 0, 0, siconify_width, siconify_height, 0, 0, 1);
      XFreePixmap(dpy, p);
      return;
    }
  }

  gcBack = XCreateGC(dpy, Scr->Root, 0, &gcvalues);
  gcvalues.background = cp.back;
  gcvalues.foreground = cp.fore;
  XChangeGC(dpy, gcBack, GCForeground | GCBackground, &gcvalues);

  lw = (w > h) ? h / 16 : w / 16;
  if (lw < 3)
    lw = 3;
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

static void
DrawQuestionImage(Drawable d, int x, int y, int w, int h, int pad, ColorPair cp, int state)
{
  Pixmap p;

  p = XCreateBitmapFromData(dpy, Scr->Root, questionmark_bits, questionmark_width, questionmark_height);

  XSetForeground(dpy, Scr->RootGC, cp.fore);
  XCopyPlane(dpy, p, d, Scr->RootGC, 0, 0,
	     questionmark_width, questionmark_height,
	     x + (w - questionmark_width) / 2, y + (h - questionmark_height) / 2, (unsigned long)1);

  XFreePixmap(dpy, p);
}

static void
DrawRArrowImage(Drawable d, int x, int y, int w, int h, int pad, ColorPair cp, int state)
{
  XPoint points[4];
  int lw, mw, mh;

  if (!(h & 1))
    h--;
  if (h < 1)
    h = 1;

  lw = (w > h) ? h / 16 : w / 16;
  if (lw == 1)
    lw = 0;
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

static void
DrawDArrowImage(Drawable d, int x, int y, int w, int h, int pad, ColorPair cp, int state)
{
  XPoint points[4];
  int lw, mw, mh;

  if (!(h & 1))
    h--;
  if (h < 1)
    h = 1;

  lw = (w > h) ? h / 16 : w / 16;
  if (lw == 1)
    lw = 0;
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

static void
Draw3DDotImage(Drawable d, int x, int y, int w, int h, int pad, ColorPair cp, int state)
{
  Draw3DBorder(d, x + (w / 2) - 2, y + (h / 2) - 2, 5, 5, Scr->ShallowReliefWindowButton, cp, state, True, False);
}

static void
Draw3DBarImage(Drawable d, int x, int y, int w, int h, int pad, ColorPair cp, int state)
{
  Draw3DBorder(d, x + pad + 1, y + (h - 5) / 2, w - pad * 2 - 2, 5, Scr->ShallowReliefWindowButton, cp, state, True, False);
}

static void
Draw3DMenuImage(Drawable d, int x, int y, int w, int h, int pad, ColorPair cp, int state)
{
  int i, lines, width, height;

  height = Scr->ShallowReliefWindowButton * 2;

  /* count the menu lines */
  lines = (h - pad * 2 - 2) / height;
  /* center 'em */
  y += (h - lines * height) / 2;
  if (!(y & 1))
    y += 1;
  /* now draw 'em */
  x += pad + 1;
  lines = y + lines * height;
  width = w - pad * 2 - 2;
  for (i = y; i < lines; i += height)
    Draw3DBorder(d, x, i, width, height, Scr->ShallowReliefWindowButton, cp, state, True, False);
}

static void
Draw3DResizeImage(Drawable d, int x, int y, int w, int h, int pad, ColorPair cp, int state)
{
  int i, j;

  i = w - Scr->ButtonBevelWidth * 2;

  /*
   * Extend the left and bottom "off-window" by the
   * line width for "thick" boxes on "thin" buttons.
   */

  j = Scr->ButtonBevelWidth + (i / 4);
  Draw3DBorder(d,
	       x + -Scr->ShallowReliefWindowButton, y + j - 1,
	       x + w - j + 1 + Scr->ShallowReliefWindowButton,
	       y + h - j + 1 + Scr->ShallowReliefWindowButton, Scr->ShallowReliefWindowButton, cp, state, True, False);

  j = Scr->ButtonBevelWidth + (i / 2);
  Draw3DBorder(d,
	       x + -Scr->ShallowReliefWindowButton, y + j,
	       x + w - j + Scr->ShallowReliefWindowButton,
	       y + h - j + Scr->ShallowReliefWindowButton, Scr->ShallowReliefWindowButton, cp, state, True, False);
}

static void
Draw3DZoomImage(Drawable d, int x, int y, int w, int h, int pad, ColorPair cp, int state)
{
  Draw3DBorder(d,
	       x + pad + 1, y + pad + 1, w - 2 * pad - 2, h - 2 * pad - 2, Scr->ShallowReliefWindowButton, cp, state, True, False);
}

static void
Draw3DRArrowImage(Drawable d, int x, int y, int w, int h, int pad, ColorPair cp, int state)
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
      XDrawLine(dpy, d, Scr->GreyGC, x + w - mw - i, y + h / 2, x + mw - 1 + i, y + mh - 1 + i);
      XDrawLine(dpy, d, Scr->GreyGC, x + mw - 1 + i, y + mh - 1 + i, x + mw - 1 + i, y + h - mh - i);
    }

    /* draw shadows */
    setBorderGC(2, Scr->GreyGC, cp, state, False);
    for (i = 0; i < Scr->ShallowReliefWindowButton; i++)
      XDrawLine(dpy, d, Scr->GreyGC, x + mw - 1 + i, y + h - mh - i, x + w - mw - i, y + h / 2);
  }
  else if (Scr->BeNiceToColormap)
  {
    int dashoffset = 0;

    setBorderGC(3, Scr->ShadGC, cp, state, False);

    /* draw highlights */
    XSetForeground(dpy, Scr->ShadGC, Scr->White);
    for (i = 0; i < Scr->ShallowReliefWindowButton; i++)
    {
      XDrawLine(dpy, d, Scr->ShadGC, x + w - mw - i, y + h / 2, x + mw - 1 + i, y + mh - 1 + i);
      XDrawLine(dpy, d, Scr->ShadGC, x + mw - 1 + i, y + mh - 1 + i, x + mw - 1 + i, y + h - mh - i);
    }

    /* draw shadows */
    XSetForeground(dpy, Scr->ShadGC, Scr->Black);
    for (i = 0; i < Scr->ShallowReliefWindowButton; i++)
    {
      XDrawLine(dpy, d, Scr->ShadGC, x + mw - 1 + i, y + h - mh - i + dashoffset, x + w - mw - i, y + h / 2 + dashoffset);
      dashoffset = 1 - dashoffset;
    }
  }
  else
  {
    /* draw highlights */
    if (state)
    {
      FB(Scr, cp.shadc, cp.shadd);
    }
    else
    {
      FB(Scr, cp.shadd, cp.shadc);
    }
    for (i = 0; i < Scr->ShallowReliefWindowButton; i++)
    {
      XDrawLine(dpy, d, Scr->NormalGC, x + w - mw - i, y + h / 2, x + mw - 1 + i, y + mh - 1 + i);
      XDrawLine(dpy, d, Scr->NormalGC, x + mw - 1 + i, y + mh - 1 + i, x + mw - 1 + i, y + h - mh - i);
    }

    /* draw shadows */
    if (state)
    {
      FB(Scr, cp.shadd, cp.shadc);
    }
    else
    {
      FB(Scr, cp.shadc, cp.shadd);
    }
    for (i = 0; i < Scr->ShallowReliefWindowButton; i++)
      XDrawLine(dpy, d, Scr->NormalGC, x + mw - 1 + i, y + h - mw - i, x + w - mw - i, y + h / 2);
  }
}

static void
Draw3DDArrowImage(Drawable d, int x, int y, int w, int h, int pad, ColorPair cp, int state)
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
      XDrawLine(dpy, d, Scr->GreyGC, x + w - mw - i, y + mh - 1 + i, x + mw - 1 + i, y + mh - 1 + i);
      XDrawLine(dpy, d, Scr->GreyGC, x + mw - 1 + i, y + mh - 1 + i, x + w / 2, y + mh - 1 - i + h / 2);
    }

    /* draw shadows */
    setBorderGC(2, Scr->GreyGC, cp, state, False);
    for (i = 0; i < Scr->ShallowReliefWindowButton; i++)
      XDrawLine(dpy, d, Scr->GreyGC, x + w / 2, y + mh - 1 - i + h / 2, x + w - mw - i, y + mh + i);
  }
  else if (Scr->BeNiceToColormap)
  {
    int dashoffset = 0;

    setBorderGC(3, Scr->ShadGC, cp, state, False);

    /* draw highlights */
    XSetForeground(dpy, Scr->ShadGC, Scr->White);
    for (i = 0; i < Scr->ShallowReliefWindowButton; i++)
    {
      XDrawLine(dpy, d, Scr->ShadGC, x + w - mw - i, y + mh - 1 + i, x + mw - 1 + i, y + mh - 1 + i);
      XDrawLine(dpy, d, Scr->ShadGC, x + mw - 1 + i, y + mh - 1 + i + dashoffset, x + w / 2, y + mh - 1 - i + h / 2 + dashoffset);
      dashoffset = 1 - dashoffset;
    }

    /* draw shadows */
    XSetForeground(dpy, Scr->ShadGC, Scr->Black);
    for (i = 0; i < Scr->ShallowReliefWindowButton; i++)
      XDrawLine(dpy, d, Scr->ShadGC, x + w / 2, y + mh - 1 - i + h / 2, x + w - mw - i, y + mh + i);
  }
  else
  {
    /* draw highlights */
    if (state)
    {
      FB(Scr, cp.shadc, cp.shadd);
    }
    else
    {
      FB(Scr, cp.shadd, cp.shadc);
    }
    for (i = 0; i < Scr->ShallowReliefWindowButton; i++)
    {
      XDrawLine(dpy, d, Scr->NormalGC, x + w - mw - i, y + mh - 1 + i, x + mw - 1 + i, y + mh - 1 + i);
      XDrawLine(dpy, d, Scr->NormalGC, x + mw - 1 + i, y + mh - 1 + i, x + w / 2, y + mh - 1 - i + h / 2);
    }

    /* draw shadows */
    if (state)
    {
      FB(Scr, cp.shadd, cp.shadc);
    }
    else
    {
      FB(Scr, cp.shadc, cp.shadd);
    }
    for (i = 0; i < Scr->ShallowReliefWindowButton; i++)
      XDrawLine(dpy, d, Scr->NormalGC, x + w / 2, y + mh - 1 - i + h / 2, x + w - mw - i, y + mh + i);
  }
}

static void
Draw3DBoxHighlight(Drawable d, int x, int y, int w, int h, int pad, ColorPair cp, int state)
{
  Draw3DBorder(d, x, y, w, h, Scr->ShallowReliefWindowButton, cp, state, True, False);
}

static void
Draw3DLinesHighlight(Drawable d, int x, int y, int w, int h, int pad, ColorPair cp, int state)
{
  int p;

  p = (Scr->ShallowReliefWindowButton & 1) ? 3 : 2;
  p = (h - Scr->ShallowReliefWindowButton * p - 2) / 2;

  y += h / 2 - 1;
  y -= (Scr->ShallowReliefWindowButton & 1) ? 0 : 1;
  y += (state == on) ? 0 : 1;

  h = Scr->ShallowReliefWindowButton * 2;

  Draw3DBorder(d, x, y - p, w, h, Scr->ShallowReliefWindowButton, cp, state, False, False);

  if ((Scr->ShallowReliefWindowButton & 1))
    Draw3DBorder(d, x, y, w, h, Scr->ShallowReliefWindowButton, cp, state, False, False);

  Draw3DBorder(d, x, y + p, w, h, Scr->ShallowReliefWindowButton, cp, state, False, False);
}

static void
DrawBackground(Drawable d, int x, int y, int w, int h, ColorPair cp, int use_rootGC)
{
  XGCValues gcvalues;

  if (use_rootGC)
  {
    if (Scr->RootGC == (GC) 0)
      Scr->RootGC = XCreateGC(dpy, Scr->Root, 0, &gcvalues);

    gcvalues.background = cp.back;
    gcvalues.foreground = cp.fore;
    XChangeGC(dpy, Scr->RootGC, GCForeground | GCBackground, &gcvalues);

    XSetForeground(dpy, Scr->RootGC, cp.back);
    XFillRectangle(dpy, d, Scr->RootGC, x, y, w, h);
  }
  else
  {
    FB(Scr, cp.back, cp.fore);
    XFillRectangle(dpy, d, Scr->NormalGC, x, y, w, h);
  }
}

static void
DrawTitleHighlight(TwmWindow * t, int state)
{
  static const struct
  {
    char *name;
    void (*proc) (Drawable d, int x, int y, int w, int h, int pad, ColorPair cp, int state);
    Bool use_rootGC;
    int state;
  } pmtab[] =
  {
    {
    TBPM_DOT, DrawDotImage, True, off},
    {
    TBPM_ICONIFY, DrawDotImage, True, off},
    {
    TBPM_RESIZE, DrawResizeImage, True, off},
    {
    TBPM_MENU, DrawMenuImage, True, off},
    {
    TBPM_XLOGO, DrawXLogoImage, True, off},
    {
    TBPM_DELETE, DrawXLogoImage, True, off},
    {
    TBPM_QUESTION, DrawQuestionImage, True, off},
    {
    TBPM_3DDOT, Draw3DDotImage, False, off},
    {
    TBPM_3DRESIZE, Draw3DResizeImage, False, off},
    {
    TBPM_3DMENU, Draw3DMenuImage, False, off},
    {
    TBPM_3DZOOM, Draw3DZoomImage, False, off},
    {
    TBPM_3DBAR, Draw3DBarImage, False, off},
    {
    TBPM_3DBOX, Draw3DBoxHighlight, False, off},
    {
    TBPM_3DLINES, Draw3DLinesHighlight, False, off},
    {
    TBPM_3DRAISEDBOX, Draw3DBoxHighlight, False, off},
    {
    TBPM_3DSUNKENBOX, Draw3DBoxHighlight, False, on},
    {
    TBPM_3DRAISEDLINES, Draw3DLinesHighlight, False, off},
    {
  TBPM_3DSUNKENLINES, Draw3DLinesHighlight, False, on},};

  XGCValues gcvalues;
  ColorPair cp;
  register int i;
  int h, w;

  ScreenInfo *scr;

  scr = (ClientIsOnScreen(t, Scr) ? Scr : FindWindowScreenInfo(&t->attr));

  cp = t->title;
  w = ComputeHighlightWindowWidth(t);
  h = scr->TitleHeight - 2 * scr->FramePadding - 2;

  for (i = 0; i < sizeof(pmtab) / sizeof(pmtab[0]); i++)
  {
    if (XmuCompareISOLatin1(pmtab[i].name, scr->hiliteName) == 0)
    {
      if (state == off)
      {
	XClearArea(dpy, t->title_w.win, t->highlightx, 0, 0, 0, False);
      }
      else if (scr == Scr)	/* draw only onto the 'event screen' */
      {
	if (pmtab[i].use_rootGC)
	{
	  if (Scr->RootGC == (GC) 0)
	    Scr->RootGC = XCreateGC(dpy, Scr->Root, 0, &gcvalues);

	  gcvalues.background = cp.back;
	  gcvalues.foreground = cp.fore;
	  XChangeGC(dpy, Scr->RootGC, GCForeground | GCBackground, &gcvalues);
	}

	(*pmtab[i].proc) (t->title_w.win, t->highlightx, Scr->FramePadding + 1, w, h, 0, cp, pmtab[i].state);
      }

      break;
    }
  }
}

void
setBorderGC(int type, GC gc, ColorPair cp, int state, int forcebw)
{
  XGCValues gcv;
  unsigned long gcm;

  switch (type)
  {
  case 0:			/* Monochrome main */
    gcm = GCFillStyle;
    gcv.fill_style = FillOpaqueStippled;
    break;
  case 1:			/* Monochrome highlight */
    gcm = 0;
    gcm |= GCLineStyle;
    gcv.line_style = (state == on) ? LineSolid : LineDoubleDash;
    gcm |= GCFillStyle;
    gcv.fill_style = FillSolid;
    break;
  case 2:			/* Monochrome shadow */
    gcm = 0;
    gcm |= GCLineStyle;
    gcv.line_style = (state == on) ? LineDoubleDash : LineSolid;
    gcm |= GCFillStyle;
    gcv.fill_style = FillSolid;
    break;
  case 3:			/* BeNiceToColormap */
    gcm = 0;
    gcm |= GCLineStyle;
    gcv.line_style = (forcebw) ? LineSolid : LineDoubleDash;
    gcm |= GCBackground;
    gcv.background = cp.back;
    break;
  default:
    return;
  }

  XChangeGC(dpy, gc, gcm, &gcv);
}

void
Draw3DBorder(Drawable w, int x, int y, int width, int height, int bw, ColorPair cp, int state, int fill, int forcebw)
{
  int i;
  ScreenInfo *scr;

  if (width < 1 || height < 1)
    return;

  scr = FindDrawableScreenInfo(w);	/* frame, pixmap, client */

  if (scr->Monochrome != COLOR)
  {
    /* set main color */
    if (fill)
    {
      setBorderGC(0, scr->GreyGC, cp, state, forcebw);
      XFillRectangle(dpy, w, scr->GreyGC, x, y, width, height);
    }

    /* draw highlights */
    setBorderGC(1, scr->GreyGC, cp, state, forcebw);
    for (i = 0; i < bw; i++)
    {
      XDrawLine(dpy, w, scr->GreyGC, x, y + i, x + width - i - 1, y + i);
      XDrawLine(dpy, w, scr->GreyGC, x + i, y, x + i, y + height - i - 1);
    }

    /* draw shadows */
    setBorderGC(2, scr->GreyGC, cp, state, forcebw);
    for (i = 0; i < bw; i++)
    {
      XDrawLine(dpy, w, scr->GreyGC, x + width - i - 1, y + i, x + width - i - 1, y + height - 1);
      XDrawLine(dpy, w, scr->GreyGC, x + i, y + height - i - 1, x + width - 1, y + height - i - 1);
    }

    return;
  }

  /* set main color */
  if (fill)
  {
    FB(scr, cp.back, cp.fore);
    XFillRectangle(dpy, w, scr->NormalGC, x, y, width, height);
  }

  if (scr->BeNiceToColormap)
  {
    setBorderGC(3, scr->ShadGC, cp, state, forcebw);

    /* draw highlights */
    if (state == on)
      XSetForeground(dpy, scr->ShadGC, scr->Black);
    else
      XSetForeground(dpy, scr->ShadGC, scr->White);
    for (i = 0; i < bw; i++)
    {
      XDrawLine(dpy, w, scr->ShadGC, x + i, y + borderdashoffset, x + i, y + height - i - 1);
      XDrawLine(dpy, w, scr->ShadGC, x + borderdashoffset, y + i, x + width - i - 1, y + i);
      borderdashoffset = 1 - borderdashoffset;
    }

    /* draw shadows */
    if (state == on)
      XSetForeground(dpy, scr->ShadGC, scr->White);
    else
      XSetForeground(dpy, scr->ShadGC, scr->Black);
    for (i = 0; i < bw; i++)
    {
      XDrawLine(dpy, w, scr->ShadGC, x + i, y + height - i - 1, x + width - 1, y + height - i - 1);
      XDrawLine(dpy, w, scr->ShadGC, x + width - i - 1, y + i, x + width - i - 1, y + height - 1);
    }

    return;
  }

  /* draw highlights */
  if (state == on)
  {
    FB(scr, cp.shadd, cp.shadc);
  }
  else
  {
    FB(scr, cp.shadc, cp.shadd);
  }
  for (i = 0; i < bw; i++)
  {
    XDrawLine(dpy, w, scr->NormalGC, x, y + i, x + width - i - 1, y + i);
    XDrawLine(dpy, w, scr->NormalGC, x + i, y, x + i, y + height - i - 1);
  }

  /* draw shadows */
  if (state == on)
  {
    FB(scr, cp.shadc, cp.shadd);
  }
  else
  {
    FB(scr, cp.shadd, cp.shadc);
  }
  for (i = 0; i < bw; i++)
  {
    XDrawLine(dpy, w, scr->NormalGC, x + width - i - 1, y + i, x + width - i - 1, y + height - 1);
    XDrawLine(dpy, w, scr->NormalGC, x + i, y + height - i - 1, x + width - 1, y + height - i - 1);
  }
}

GC
setBevelGC(int type, int state, ColorPair cp)
{
  GC gc;

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
      XSetForeground(dpy, gc, Scr->Black);
    else
      XSetForeground(dpy, gc, Scr->White);
  }
  else
  {
    gc = Scr->NormalGC;
    if (state == on)
    {
      FB(Scr, cp.shadc, cp.shadd);
    }
    else
    {
      FB(Scr, cp.shadd, cp.shadc);
    }
  }

  return (gc);
}

void
Draw3DBevel(Drawable w, int x, int y, int bw, ColorPair cp, int state, int type)
{
  int i;
  GC gc;

  switch (type)
  {
    /* vertical */
  case 1:
  case 11:
    gc = setBevelGC(1, state, cp);
    for (i = 0; i < Scr->BorderBevelWidth; i++)
      XDrawLine(dpy, w, gc, x - i - 1, y + i, x - i - 1, y + bw - 2 * i + i);
    if (type == 11)
      break;
  case 111:
    gc = setBevelGC(2, !state, cp);
    for (i = 0; i < Scr->BorderBevelWidth; i++)
      XDrawLine(dpy, w, gc, x + i, y + i - 1, x + i, y + bw - 2 * i + i - 1);
    break;
    /* horizontal */
  case 2:
  case 22:
    gc = setBevelGC(1, state, cp);
    for (i = 0; i < Scr->BorderBevelWidth; i++)
      XDrawLine(dpy, w, gc, x + i, y - i - 1, x + bw - 2 * i + i, y - i - 1);
    if (type == 22)
      break;
  case 222:
    gc = setBevelGC(2, !state, cp);
    for (i = 0; i < Scr->BorderBevelWidth; i++)
      XDrawLine(dpy, w, gc, x + i - 1, y + i, x + bw - 2 * i + i - 1, y + i);
    break;
    /* ulc to lrc */
  case 3:
    if (Scr->Monochrome != COLOR)
    {
      gc = Scr->GreyGC;
      setBorderGC(0, gc, cp, state, False);
    }
    else
    {
      gc = Scr->NormalGC;
      FB(Scr, cp.back, cp.fore);
    }
    XFillRectangle(dpy, w, gc, x, y, bw, bw);
    gc = setBevelGC(1, (Scr->BeNiceToColormap) ? state : !state, cp);
    for (i = 0; i < Scr->BorderBevelWidth; i++)
      XDrawLine(dpy, w, gc, x + i, y, x + i, y + Scr->BorderBevelWidth - 1);
    gc = setBevelGC(2, (Scr->BeNiceToColormap) ? !state : state, cp);
    for (i = 0; i < Scr->BorderBevelWidth; i++)
      XDrawLine(dpy, w, gc, x, y + bw - i - 1, x + bw - 1, y + bw - i - 1);
    for (i = 0; i < Scr->BorderBevelWidth; i++)
      XDrawLine(dpy, w, gc, x + bw - i - 1, y, x + bw - i - 1, y + bw - 1);
    break;
    /* urc to llc */
  case 4:
    if (Scr->Monochrome != COLOR)
    {
      gc = Scr->GreyGC;
      setBorderGC(0, gc, cp, state, False);
    }
    else
    {
      gc = Scr->NormalGC;
      FB(Scr, cp.back, cp.fore);
    }
    XFillRectangle(dpy, w, gc, x, y, bw, bw);
    gc = setBevelGC(1, (Scr->BeNiceToColormap) ? state : !state, cp);
    /* top light */
    for (i = 0; i < Scr->BorderBevelWidth; i++)
      XDrawLine(dpy, w, gc, x + i, y, x + i, y + bw - i);
    /* bottom light */
    for (i = 0; i < Scr->BorderBevelWidth; i++)
      XDrawLine(dpy, w, gc, x + bw - 1, y + i, x + bw - i - 1, y + i);
    gc = setBevelGC(2, (Scr->BeNiceToColormap) ? !state : state, cp);
    /* top dark */
    for (i = 0; i < Scr->BorderBevelWidth - 1; i++)
      XDrawLine(dpy, w, gc, x + bw - Scr->BorderBevelWidth, y + i, x + bw - i - 2, y + i);
    /* bottom dark */
    for (i = 0; i < Scr->BorderBevelWidth; i++)
      XDrawLine(dpy, w, gc, x + i, y + bw - i - 1, x + bw + 1, y + bw - i - 1);
    break;
  }
}

void
Draw3DNoBevel(Drawable w, int x, int y, int bw, ColorPair cp, int state, int forcebw)
{
  int i, upr, lwr;

  if (bw < 1)
    return;

  upr = y - 2 * Scr->BorderBevelWidth;
  if ((upr & 1))
    upr--;
  lwr = y + 2 * Scr->BorderBevelWidth;
  if ((lwr & 1))
    lwr++;

  if (Scr->Monochrome != COLOR)
  {
    /* set main color */
    setBorderGC(0, Scr->GreyGC, cp, state, forcebw);
    XFillRectangle(dpy, w, Scr->GreyGC,
		   x + Scr->BorderBevelWidth, upr, (unsigned int)(bw - Scr->BorderBevelWidth * 2), (unsigned int)(upr * 2));

    /* draw highlight */
    setBorderGC(1, Scr->GreyGC, cp, state, forcebw);
    for (i = 0; i < Scr->BorderBevelWidth; i++)
      XDrawLine(dpy, w, Scr->GreyGC, x + i, upr, x + i, lwr);

    /* draw shadow */
    setBorderGC(2, Scr->GreyGC, cp, state, forcebw);
    for (i = bw - Scr->BorderBevelWidth; i < bw; i++)
      XDrawLine(dpy, w, Scr->GreyGC, x + i, upr, x + i, lwr);

    return;
  }

  /* set main color */
  FB(Scr, cp.back, cp.fore);
  XFillRectangle(dpy, w, Scr->NormalGC,
		 x + Scr->BorderBevelWidth, upr, (unsigned int)(bw - Scr->BorderBevelWidth * 2), (unsigned int)(upr * 2));

  if (Scr->BeNiceToColormap)
  {
    int dashoffset;

    setBorderGC(3, Scr->ShadGC, cp, state, forcebw);

    /* draw highlight */
    if (state == on)
      XSetForeground(dpy, Scr->ShadGC, Scr->Black);
    else
      XSetForeground(dpy, Scr->ShadGC, Scr->White);
    dashoffset = 0;
    for (i = 0; i < Scr->BorderBevelWidth; i++)
    {
      XDrawLine(dpy, w, Scr->ShadGC, x + i, upr + dashoffset, x + i, lwr);
      dashoffset = 1 - dashoffset;
    }

    /* draw shadow */
    if (state == on)
      XSetForeground(dpy, Scr->ShadGC, Scr->White);
    else
      XSetForeground(dpy, Scr->ShadGC, Scr->Black);
    dashoffset = 0;
    for (i = bw - Scr->BorderBevelWidth; i < bw; i++)
    {
      XDrawLine(dpy, w, Scr->ShadGC, x + i, upr + dashoffset, x + i, lwr);
      dashoffset = 1 - dashoffset;
    }

    return;
  }

  /* draw highlight */
  if (state == on)
  {
    FB(Scr, cp.shadc, cp.shadd);
  }
  else
  {
    FB(Scr, cp.shadd, cp.shadc);
  }
  for (i = 0; i < Scr->BorderBevelWidth; i++)
    XDrawLine(dpy, w, Scr->NormalGC, x + i, upr, x + i, lwr);

  /* draw shadow */
  if (state == on)
  {
    FB(Scr, cp.shadd, cp.shadc);
  }
  else
  {
    FB(Scr, cp.shadc, cp.shadd);
  }
  for (i = bw - Scr->BorderBevelWidth; i < bw; i++)
    XDrawLine(dpy, w, Scr->NormalGC, x + i, upr, x + i, lwr);
}

static Image *
LoadBitmapImage(char *name, ColorPair cp)
{
  Image *image;
  Pixmap bm;
  int width, height;
  XGCValues gcvalues;

  if (Scr->RootGC == (GC) 0)
    Scr->RootGC = XCreateGC(dpy, Scr->Root, 0, &gcvalues);
  bm = FindBitmap(name, (unsigned int *)&width, (unsigned int *)&height);
  if (bm == None)
    return (None);

  image = (Image *) malloc(sizeof(struct _Image));
  image->pixmap = XCreatePixmap(dpy, Scr->Root, width, height, Scr->d_depth);
  gcvalues.background = cp.back;
  gcvalues.foreground = cp.fore;
  XChangeGC(dpy, Scr->RootGC, GCForeground | GCBackground, &gcvalues);
  XCopyPlane(dpy, bm, image->pixmap, Scr->RootGC, 0, 0, width, height, 0, 0, (unsigned long)1);
  XFreePixmap(dpy, bm);
  image->mask = None;
  image->width = width;
  image->height = height;
  image->next = None;
  return (image);
}

static Image *
CreateImagePixmap(char *name, int w, int h, int depth)
{
  Image *image;

  image = (Image *) malloc(sizeof(struct _Image));
  if (!image)
  {
    fprintf(stderr, "%s: cannot allocate %lu bytes for Image \"%s\"\n", ProgramName, sizeof(struct _Image), name);
    return (None);
  }

  image->pixmap = XCreatePixmap(dpy, Scr->Root, w, h, depth);
  if (image->pixmap == None)
  {
    fprintf(stderr, "%s: cannot allocate %lu bytes for pixmap \"%s\"\n", ProgramName, sizeof(image->pixmap), name);
    free((void *)image);
    return (None);
  }

  return (image);
}

static Image *
ReallyGetImage(char *name, int w, int h, int pad, ColorPair cp)
{
  static const struct
  {
    char *name;
    void (*proc) (Drawable d, int x, int y, int w, int h, int pad, ColorPair cp, int state);
    Bool use_rootGC;
    int state;
  } pmtab[] =
  {
    {
    TBPM_DOT, DrawDotImage, True, off},
    {
    TBPM_ICONIFY, DrawDotImage, True, off},
    {
    TBPM_RESIZE, DrawResizeImage, True, off},
    {
    TBPM_MENU, DrawMenuImage, True, off},
    {
    TBPM_XLOGO, DrawXLogoImage, True, off},
    {
    TBPM_DELETE, DrawXLogoImage, True, off},
    {
    TBPM_QUESTION, DrawQuestionImage, True, off},
    {
    TBPM_RARROW, DrawRArrowImage, True, off},
    {
    TBPM_DARROW, DrawDArrowImage, True, off},
    {
    TBPM_3DDOT, Draw3DDotImage, False, off},
    {
    TBPM_3DRESIZE, Draw3DResizeImage, False, off},
    {
    TBPM_3DMENU, Draw3DMenuImage, False, off},
    {
    TBPM_3DZOOM, Draw3DZoomImage, False, off},
    {
    TBPM_3DBAR, Draw3DBarImage, False, off},
    {
    TBPM_3DRARROW, Draw3DRArrowImage, False, off},
    {
    TBPM_3DDARROW, Draw3DDArrowImage, False, off},
    {
    TBPM_3DBOX, Draw3DBoxHighlight, False, off},
    {
  TBPM_3DLINES, Draw3DLinesHighlight, False, off},};

  Image *image = NULL;
  name_list **list;
  register int i;
  char fullname[256];

  if (name == NULL)
    return (NULL);

  list = &Scr->ImageCache;

  if (name[0] == ':')
  {
    sprintf(fullname, "%s.%dx%d.%Xx%X.%d", name, w, h, (int)cp.fore, (int)cp.back, Scr->screen);
    if ((image = (Image *) LookInNameList(*list, fullname)) == NULL)
    {
      for (i = 0; i < sizeof(pmtab) / sizeof(pmtab[0]); i++)
      {
	if (XmuCompareISOLatin1(pmtab[i].name, name) == 0)
	{
	  if (!(image = CreateImagePixmap(name, w, h, Scr->d_depth)))
	    return (None);

	  DrawBackground(image->pixmap, 0, 0, w, h, cp, pmtab[i].use_rootGC | (Scr->Monochrome != COLOR));

	  (*pmtab[i].proc) (image->pixmap, 0, 0, w, h, pad, cp, pmtab[i].state);

	  image->mask = None;
	  image->width = w;
	  image->height = h;
	  image->next = None;
	  break;
	}
      }

      if (!image)
      {
	fprintf(stderr, "%s:  no such built-in pixmap \"%s\"\n", ProgramName, name);
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
     */
    sprintf(fullname, "%s.%Xx%X.%d", name, (int)cp.fore, (int)cp.back, (int)Scr->screen);
    if ((image = (Image *) LookInNameList(*list, fullname)) == NULL)
    {
      if ((image = LoadBitmapImage(name, cp)) == NULL)
#ifndef NO_XPM_SUPPORT
	image = FindImage(name, cp.back);
#else
	;
#endif

      if (!image)
	return (NULL);
    }
    else
      return (image);
  }

  AddToList(list, fullname, LTYPE_EXACT_NAME, (char *)image);

  return (image);
}

/*
 * Wrapper to guarantee something is returned - djhjr - 10/30/02
 */
Image *
GetImage(char *name, int w, int h, int pad, ColorPair cp)
{
  Image *image = NULL;

  if (!(image = ReallyGetImage(name, w, h, pad, cp)))
    image = ReallyGetImage(TBPM_QUESTION, w, h, pad, cp);

  return (image);
}

void
PaintBorders(TwmWindow * tmp_win, Bool focus)
{
  GC gc;
  ColorPair cp;
  int i, j, cw, ch, cwbw, chbw;

  if (!ClientIsOnScreen(tmp_win, Scr))
    return;

  cp = (focus && tmp_win->highlight) ? tmp_win->border : tmp_win->border_tile;

  /* no titlebar, no corners */
  if (tmp_win->title_height == 0)
  {
    Draw3DBorder(tmp_win->frame, 0, 0, tmp_win->frame_width, tmp_win->frame_height, Scr->BorderBevelWidth, cp, off, True, False);
    Draw3DBorder(tmp_win->frame,
		 tmp_win->frame_bw3D - Scr->BorderBevelWidth,
		 tmp_win->frame_bw3D - Scr->BorderBevelWidth,
		 tmp_win->frame_width - 2 * tmp_win->frame_bw3D + 2 * Scr->BorderBevelWidth,
		 tmp_win->frame_height - 2 * tmp_win->frame_bw3D + 2 * Scr->BorderBevelWidth,
		 Scr->BorderBevelWidth, cp, on, True, False);

    return;
  }

  cw = ch = Scr->TitleHeight;
  if (cw * 2 > tmp_win->attr.width)
    cw = tmp_win->attr.width / 2;
  if (ch * 2 > tmp_win->attr.height)
    ch = tmp_win->attr.height / 2;
  cwbw = cw + tmp_win->frame_bw3D;
  chbw = ch + tmp_win->frame_bw3D;

  /* client */
  borderdashoffset = 1;
  Draw3DBorder(tmp_win->frame,
	       0,
	       Scr->TitleHeight,
	       tmp_win->frame_width, tmp_win->frame_height - Scr->TitleHeight, Scr->BorderBevelWidth, cp, off, True, False);
  borderdashoffset = 1;
  Draw3DBorder(tmp_win->frame,
	       tmp_win->frame_bw3D - Scr->BorderBevelWidth,
	       Scr->TitleHeight + tmp_win->frame_bw3D - Scr->BorderBevelWidth,
	       tmp_win->frame_width - 2 * tmp_win->frame_bw3D + 2 * Scr->BorderBevelWidth,
	       tmp_win->frame_height - 2 * tmp_win->frame_bw3D + 2 * Scr->BorderBevelWidth - Scr->TitleHeight,
	       Scr->BorderBevelWidth, cp, on, True, False);
  if (!Scr->NoBorderDecorations)
  {
    /* upper left corner */
    if (tmp_win->title_x == tmp_win->frame_bw3D)
      Draw3DBevel(tmp_win->frame,
		  0, Scr->TitleHeight + tmp_win->frame_bw3D, tmp_win->frame_bw3D, cp, (Scr->BeNiceToColormap) ? on : off, 222);
    else
    {
      if (tmp_win->title_x > tmp_win->frame_bw3D + cwbw)
	Draw3DBevel(tmp_win->frame, cwbw, Scr->TitleHeight, tmp_win->frame_bw3D, cp, (Scr->BeNiceToColormap) ? on : off, 1);
      Draw3DBevel(tmp_win->frame, 0, Scr->TitleHeight + chbw, tmp_win->frame_bw3D, cp, (Scr->BeNiceToColormap) ? on : off, 2);
    }
  }
  /* upper right corner */
  if ((i = tmp_win->title_x + tmp_win->title_width + tmp_win->frame_bw3D) == tmp_win->frame_width)
    Draw3DBevel(tmp_win->frame,
		tmp_win->frame_width - tmp_win->frame_bw3D, Scr->TitleHeight + tmp_win->frame_bw3D,
		tmp_win->frame_bw3D, cp, (Scr->BeNiceToColormap) ? on : off, 222);
  else
  {
    if (!Scr->NoBorderDecorations)
    {
      if (i < tmp_win->frame_width - cwbw)
	Draw3DBevel(tmp_win->frame,
		    tmp_win->frame_width - cwbw, Scr->TitleHeight, tmp_win->frame_bw3D, cp, (Scr->BeNiceToColormap) ? on : off, 1);
      Draw3DBevel(tmp_win->frame,
		  tmp_win->frame_width - tmp_win->frame_bw3D,
		  Scr->TitleHeight + chbw, tmp_win->frame_bw3D, cp, (Scr->BeNiceToColormap) ? on : off, 2);
    }
  }
  if (!Scr->NoBorderDecorations)
  {
    /* lower left corner */
    Draw3DBevel(tmp_win->frame,
		cwbw, tmp_win->frame_height - tmp_win->frame_bw3D, tmp_win->frame_bw3D, cp, (Scr->BeNiceToColormap) ? on : off, 1);
    Draw3DBevel(tmp_win->frame, 0, tmp_win->frame_height - chbw, tmp_win->frame_bw3D, cp, (Scr->BeNiceToColormap) ? on : off, 2);
    /* lower right corner */
    Draw3DBevel(tmp_win->frame,
		tmp_win->frame_width - cwbw,
		tmp_win->frame_height - tmp_win->frame_bw3D, tmp_win->frame_bw3D, cp, (Scr->BeNiceToColormap) ? on : off, 1);
    Draw3DBevel(tmp_win->frame,
		tmp_win->frame_width - tmp_win->frame_bw3D,
		tmp_win->frame_height - chbw, tmp_win->frame_bw3D, cp, (Scr->BeNiceToColormap) ? on : off, 2);
  }
  /* title */
  borderdashoffset = 0;
  Draw3DBorder(tmp_win->frame,
	       tmp_win->title_x - tmp_win->frame_bw3D,
	       tmp_win->title_y - tmp_win->frame_bw3D,
	       tmp_win->title_width + 2 * tmp_win->frame_bw3D,
	       Scr->TitleHeight + tmp_win->frame_bw3D, Scr->BorderBevelWidth, cp, off, True, False);
  borderdashoffset = 0;
  Draw3DBorder(tmp_win->frame,
	       tmp_win->title_x - Scr->BorderBevelWidth,
	       tmp_win->title_y - Scr->BorderBevelWidth,
	       tmp_win->title_width + 2 * Scr->BorderBevelWidth, Scr->TitleHeight, Scr->BorderBevelWidth, cp, on, True, False);
  /* upper left corner */
  if (tmp_win->title_x == tmp_win->frame_bw3D)
  {
    if (Scr->NoBorderDecorations)
    {
      Draw3DNoBevel(tmp_win->frame,
		    tmp_win->title_x - tmp_win->frame_bw3D,
		    Scr->TitleHeight + tmp_win->frame_bw3D, tmp_win->frame_bw3D, cp, (Scr->BeNiceToColormap) ? off : on, False);
    }
    else
    {
      gc = setBevelGC(2, (Scr->BeNiceToColormap) ? on : off, cp);

      for (j = 1; j <= Scr->BorderBevelWidth; j++)
	XDrawLine(dpy, tmp_win->frame, gc,
		  tmp_win->title_x - j,
		  Scr->TitleHeight + tmp_win->frame_bw3D - 2 * Scr->BorderBevelWidth,
		  tmp_win->title_x - j, Scr->TitleHeight + tmp_win->frame_bw3D - Scr->BorderBevelWidth - 1);

      Draw3DBevel(tmp_win->frame, tmp_win->title_x + cw, 0, tmp_win->frame_bw3D, cp, (Scr->BeNiceToColormap) ? on : off, 1);
    }
  }
  else
  {
    Draw3DBevel(tmp_win->frame, tmp_win->title_x - tmp_win->frame_bw3D, Scr->TitleHeight, tmp_win->frame_bw3D, cp, off, 3);
  }
  /* upper right corner */
  if (i == tmp_win->frame_width)
  {
    if (Scr->NoBorderDecorations)
    {
      Draw3DNoBevel(tmp_win->frame,
		    tmp_win->frame_width - tmp_win->frame_bw3D,
		    Scr->TitleHeight + tmp_win->frame_bw3D, tmp_win->frame_bw3D, cp, (Scr->BeNiceToColormap) ? off : on, False);
    }
    else
    {
      gc = setBevelGC(1, (Scr->BeNiceToColormap) ? off : on, cp);

      for (j = 0; j < Scr->BorderBevelWidth; j++)
	XDrawLine(dpy, tmp_win->frame, gc,
		  tmp_win->title_x + tmp_win->title_width - 1,
		  Scr->TitleHeight + tmp_win->frame_bw3D - Scr->BorderBevelWidth + j - 1,
		  tmp_win->title_x + tmp_win->title_width + Scr->BorderBevelWidth - j - 1,
		  Scr->TitleHeight + tmp_win->frame_bw3D - Scr->BorderBevelWidth + j - 1);

      Draw3DBevel(tmp_win->frame,
		  tmp_win->title_x + tmp_win->title_width - cw, 0, tmp_win->frame_bw3D, cp, (Scr->BeNiceToColormap) ? on : off, 1);
    }
  }
  else
  {
    Draw3DBevel(tmp_win->frame, tmp_win->title_x + tmp_win->title_width, Scr->TitleHeight, tmp_win->frame_bw3D, cp, off, 4);
  }
}

void
PaintIcon(TwmWindow * tmp_win)
{
  if (!ClientIsOnScreen(tmp_win, Scr))
    return;

  if (Scr->IconBevelWidth > 0)
  {

    Draw3DBorder(tmp_win->icon_w.win, 0, 0,
		 tmp_win->icon_w_width, tmp_win->icon_w_height, Scr->IconBevelWidth, tmp_win->iconc, off, False, False);
  }

  MyFont_DrawImageString(dpy, &tmp_win->icon_w, &Scr->IconFont,
			 &tmp_win->iconc, tmp_win->icon_x, tmp_win->icon_y, tmp_win->icon_name, strlen(tmp_win->icon_name));
}

void
PaintTitle(TwmWindow * tmp_win)
{
  int en, dots;

  int bwidth = Scr->TBInfo.width + Scr->TBInfo.pad;
  int left = (Scr->TBInfo.nleft) ? Scr->TBInfo.leftx + (Scr->TBInfo.nleft * bwidth) - Scr->TBInfo.pad : 0;
  int right = (Scr->TBInfo.nright) ? (Scr->TBInfo.nright * bwidth) - Scr->TBInfo.pad : 0;

  int cur_computed_scrlen;
  int max_avail_scrlen;
  int cur_string_len = strlen(tmp_win->name);
  char *a = NULL;

  if (!ClientIsOnScreen(tmp_win, Scr))
    return;

  en = Scr->TitleBarFont.height / 2;

  /*
   * clip the title a couple of characters less than the width of
   * the titlebar plus padding, and tack on ellipses - this is a
   * little different than the icon manager's...
   */
  if (Scr->NoPrettyTitles == FALSE)
  {
    cur_computed_scrlen = MyFont_TextWidth(&Scr->TitleBarFont, tmp_win->name, cur_string_len);

    dots = MyFont_TextWidth(&Scr->TitleBarFont, "...", 3);

    max_avail_scrlen = tmp_win->title_width - Scr->TBInfo.titlex - Scr->TBInfo.rightoff - en;

    if (dots >= max_avail_scrlen)
      cur_string_len = 0;
    else if (cur_computed_scrlen > max_avail_scrlen)
    {
      while (cur_string_len > 0)
      {
	if (MyFont_TextWidth(&Scr->TitleBarFont, tmp_win->name, cur_string_len) + dots < max_avail_scrlen)
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

  MyFont_DrawImageString(dpy, &tmp_win->title_w, &Scr->TitleBarFont,
			 &tmp_win->title, Scr->TBInfo.titlex, Scr->TitleBarFont.y, (a) ? a : tmp_win->name, cur_string_len);

  /* free the clipped title - djhjr - 3/29/98 */
  if (a)
    free(a);

  if (Scr->TitleBevelWidth > 0)
  {
    if (Scr->FramePadding + Scr->ButtonIndent > 0)
    {
      Draw3DBorder(tmp_win->title_w.win, 0, 0,
		   tmp_win->title_width, Scr->TitleHeight, Scr->TitleBevelWidth, tmp_win->title, off, False, False);
    }
    else
    {
      Draw3DBorder(tmp_win->title_w.win, left, 0,
		   tmp_win->title_width - (left + right),
		   Scr->TitleHeight, Scr->TitleBevelWidth, tmp_win->title, off, False, False);
    }
  }
}

void
PaintTitleButton(TwmWindow * tmp_win, TBWindow * tbw, int onoroff	/* 0 = no hilite    1 = hilite off    2 = hilite on */
  )
{
  Image *image;
  TitleButton *tb;
  ColorPair cp;

  if (!ClientIsOnScreen(tmp_win, Scr))
    return;

  if (!tbw->window)
    return;

  tb = tbw->info;

  if (Scr->ButtonColorIsFrame)
    cp = (onoroff == 2) ? tmp_win->border : tmp_win->border_tile;
  else
    cp = tmp_win->title;
  cp.fore = tmp_win->title.fore;

  image = GetImage(tb->name, tb->width, tb->height, Scr->ButtonBevelWidth * 2, cp);

  XCopyArea(dpy, image->pixmap, tbw->window, Scr->NormalGC, tb->srcx, tb->srcy, tb->width, tb->height, tb->dstx, tb->dsty);

  if (Scr->ButtonBevelWidth > 0)
    Draw3DBorder(tbw->window, 0, 0, tb->width, tb->height, Scr->ButtonBevelWidth, cp, off, False, False);
}

void
PaintTitleHighlight(TwmWindow * tmp_win, Bool onoroff)
{
  if (!tmp_win->titlehighlight)
    return;

  if (tmp_win->hilite_w)
  {
    if (onoroff == on)
      XMapWindow(dpy, tmp_win->hilite_w);
    else
      XUnmapWindow(dpy, tmp_win->hilite_w);
  }
  else if (Scr->hiliteName && tmp_win->title_height != 0)
    DrawTitleHighlight(tmp_win, onoroff);
}

int
ComputeHighlightWindowWidth(TwmWindow * tmp_win)
{
  int en = MyFont_TextWidth(&Scr->TitleBarFont, "n", 1);

  return (tmp_win->rightx - tmp_win->highlightx - en);
}


#ifdef TWM_USE_XFT
XftDraw *
MyXftDrawCreate(Window win)
{
  Colormap cmap = Scr->TwmRoot.cmaps.cwins[0]->colormap->c;
  XftDraw *draw = XftDrawCreate(dpy, win, Scr->d_visual, cmap);

  if (!draw)
    fprintf(stderr, "%s: XftDrawCreate() failed.\n", ProgramName);
  return draw;
}

void
MyXftDrawDestroy(XftDraw * draw)
{
  if (draw)
    XftDrawDestroy(draw);
}

void
CopyPixelToXftColor(unsigned long pixel, XftColor * col)
{
  /* color already allocated, extract RGB values (for Xft rendering): */
  Colormap cmap = Scr->TwmRoot.cmaps.cwins[0]->colormap->c;
  XColor tmp;

  tmp.pixel = col->pixel = pixel;
  XQueryColor(dpy, cmap, &tmp);
  col->color.red = tmp.red;
  col->color.green = tmp.green;
  col->color.blue = tmp.blue;
  col->color.alpha = 0xffff;
}
#endif

#ifdef TWM_USE_OPACITY
void
SetWindowOpacity(Window win, unsigned int opacity)
{
  /* rescale opacity from  0...255  to  0x00000000...0xffffffff */
  opacity *= 0x01010101;
  if (opacity == 0xffffffff)
    XDeleteProperty(dpy, win, _XA_NET_WM_WINDOW_OPACITY);
  else
    XChangeProperty(dpy, win, _XA_NET_WM_WINDOW_OPACITY, XA_CARDINAL, 32, PropModeReplace, (unsigned char *)(&opacity), 1);
}

void
PropagateWindowOpacity(TwmWindow * tmp)
{
  Atom type;
  int fmt;
  unsigned char *data;
  unsigned long n, left;

  /* propagate 'opacity' property from 'client' to 'frame' window: */
  if (XGetWindowProperty(dpy, tmp->w, _XA_NET_WM_WINDOW_OPACITY, 0, 1, False,
			 XA_CARDINAL, &type, &fmt, &n, &left, &data) == Success && data != NULL)
  {
    XChangeProperty(dpy, tmp->frame, _XA_NET_WM_WINDOW_OPACITY, XA_CARDINAL, 32, PropModeReplace, data, 1);
    XFree((void *)data);
  }
}
#endif


/*
  Local Variables:
  mode:c
  c-file-style:"GNU"
  c-file-offsets:((substatement-open 0)(brace-list-open 0)(c-hanging-comment-ender-p . nil)(c-hanging-comment-beginner-p . nil)(comment-start . "// ")(comment-end . "")(comment-column . 48))
  End:
*/
/* vim: sw=2: */
