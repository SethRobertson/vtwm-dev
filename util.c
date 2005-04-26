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
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <X11/Xmu/Drawing.h>
#include <X11/Xmu/CharSet.h>

static Pixmap CreateXLogoPixmap(), CreateResizePixmap();
static Pixmap CreateQuestionPixmap(), CreateMenuPixmap();
static Pixmap CreateDotPixmap();
static Pixmap Create3DMenuPixmap ();
static Pixmap Create3DDotPixmap ();
static Pixmap Create3DResizePixmap ();
static Pixmap Create3DZoomPixmap ();
static Pixmap Create3DBarPixmap ();
static char* generateRandomColor();
int HotX, HotY;

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
	r->x1 = xinnerl + xthird;			\
	r->y1 = yinnert;				\
	r->x2 = r->x1;					\
	r->y2 = yinnerb;				\
	r++;						\
							\
	r->x1 = xinnerl + (2 * xthird);			\
	r->y1 = yinnert;				\
	r->x2 = r->x1;					\
	r->y2 = yinnerb;				\
	r++;						\
							\
	r->x1 = xinnerl;				\
	r->y1 = yinnert + ythird;			\
	r->x2 = xinnerr;				\
	r->y2 = r->y1;					\
	r++;						\
							\
	r->x1 = xinnerl;				\
	r->y1 = yinnert + (2 * ythird);			\
	r->x2 = xinnerr;				\
	r->y2 = r->y1;					\
	r++;						\
							\
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
 *	wt	- window to zoom to
 *
 ***********************************************************************
 */

void
Zoom(wf, wt)
    Window wf, wt;
{
    int fx, fy, tx, ty;			/* from, to */
    unsigned int fw, fh, tw, th;	/* from, to */
    long dx, dy, dw, dh;
    long z;
    int j;

    if (!Scr->DoZoom || Scr->ZoomCount < 1) return;

#if (0)
    if (wf == None || wt == None) return;

    XGetGeometry (dpy, wf, &JunkRoot, &fx, &fy, &fw, &fh, &JunkBW, &JunkDepth);
    XGetGeometry (dpy, wt, &JunkRoot, &tx, &ty, &tw, &th, &JunkBW, &JunkDepth);
#else
    if (wf == None && wt == None) return;
	if ( wt == None)
	{	/* Zoom from nowhere, RFB */
		tx = ( rand() & 1 ) * Scr->MyDisplayWidth;
		tw = ( rand() & 1 ) * Scr->MyDisplayWidth;
		ty = ( rand() & 1 ) * Scr->MyDisplayHeight;
		th = ( rand() & 1 ) * Scr->MyDisplayHeight;
	}
	else
	{	/* Normal. */
		XGetGeometry (dpy, wt, &JunkRoot,
			&tx, &ty, &tw, &th, &JunkBW, &JunkDepth);
	}
	if ( wf == None )
	{	/* Zoom from nowhere, RFB */
		fx = ( rand() & 1 ) * Scr->MyDisplayWidth;
		fw = ( rand() & 1 ) * Scr->MyDisplayWidth;
		fy = ( rand() & 1 ) * Scr->MyDisplayHeight;
		fh = ( rand() & 1 ) * Scr->MyDisplayHeight;
	}
	else
	{	/* Normal. */
		XGetGeometry (dpy, wf, &JunkRoot,
			&fx, &fy, &fw, &fh, &JunkBW, &JunkDepth);
	}
#endif

    dx = ((long) (tx - fx));	/* going from -> to */
    dy = ((long) (ty - fy));	/* going from -> to */
    dw = ((long) (tw - fw));	/* going from -> to */
    dh = ((long) (th - fh));	/* going from -> to */
    z = (long) (Scr->ZoomCount + 1);

    for (j = 0; j < 2; j++) {
	long i;

	XDrawRectangle (dpy, Scr->Root, Scr->DrawGC, fx, fy, fw, fh);
	for (i = 1; i < z; i++) {
	    int x = fx + (int) ((dx * i) / z);
	    int y = fy + (int) ((dy * i) / z);
	    unsigned width = (unsigned) (((long) fw) + (dw * i) / z);
	    unsigned height = (unsigned) (((long) fh) + (dh * i) / z);

	    XDrawRectangle (dpy, Scr->Root, Scr->DrawGC,
			    x, y, width, height);
	}
	XDrawRectangle (dpy, Scr->Root, Scr->DrawGC, tx, ty, tw, th);
    }
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
#ifdef XPM
    int startn;

    Scr->UnknownXpmIcon = None;
    if ((name [0] == '@') || (strncmp (name, "xpm:", 4) == 0)) {
	if (name [0] == '@') startn = 1; else startn = 4;
	Scr->UnknownXpmIcon = GetXpmPixmap (&(name [startn]));
    }
    else
#endif
    if ((Scr->UnknownPm = GetBitmap(name)) != None)
    {
	XGetGeometry(dpy, Scr->UnknownPm, &JunkRoot, &JunkX, &JunkY,
	    (unsigned int *)&Scr->UnknownWidth, (unsigned int *)&Scr->UnknownHeight, &JunkBW, &JunkDepth);
    }
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
     * Names of the form :name refer to hardcoded images that are scaled to
     * look nice in title buttons.  Eventually, it would be nice to put in a
     * menu symbol as well....
     */
    if (name[0] == ':') {
	int i;
	static struct {
	    char *name;
	    Pixmap (*proc)();
	} pmtab[] = {
	    { TBPM_DOT,		CreateDotPixmap },
	    { TBPM_ICONIFY,	CreateDotPixmap },
	    { TBPM_RESIZE,	CreateResizePixmap },
	    { TBPM_XLOGO,	CreateXLogoPixmap },
	    { TBPM_DELETE,	CreateXLogoPixmap },
	    { TBPM_MENU,	CreateMenuPixmap },
	    { TBPM_QUESTION,	CreateQuestionPixmap },
	};

	for (i = 0; i < (sizeof pmtab)/(sizeof pmtab[0]); i++) {
	    if (XmuCompareISOLatin1 (pmtab[i].name, name) == 0)
	      return (*pmtab[i].proc) (widthp, heightp);
	}
	fprintf (stderr, "%s:  no such built-in bitmap \"%s\"\n",
		 ProgramName, name);
	return None;
    }

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
    if (pm == None) {
	fprintf (stderr, "%s:  unable to find bitmap \"%s\"\n",
		 ProgramName, name);
    }

    return pm;
}

Pixmap FindPixmap (name, widthp, heightp, depthp, cp)
char *name;
unsigned int *widthp, *heightp, *depthp;
ColorPair cp;
{
    if (!name) return None;

    /*
     * Names of the form :name refer to hardcoded images that are scaled to
     * look nice in title buttons.  Eventually, it would be nice to put in a
     * menu symbol as well....
     */
#ifdef XPM
    if ((name [0] == '@') || (strncmp (name, "xpm:", 4) == 0)) {
	XpmIcon *xpmicon;
	int startn;

	if (name [0] == '@') startn = 1; else startn = 4;
	xpmicon = GetXpmPixmap (&name [startn]);
	if (xpmicon != None) {
	    *widthp   = xpmicon->attributes.width;
	    *heightp  = xpmicon->attributes.height;
	    *depthp   = Scr->d_depth;
	    return (xpmicon->pixmap);
	}
	else return (None);
    }
    else
#endif
    if (strncmp (name, ":xpm:", 5) == 0) {
	int i;
	static struct {
	    char *name;
	    Pixmap (*proc)();
	} pmtab[] = {
	    { TBPM_3DDOT,	Create3DDotPixmap },
	    { TBPM_3DRESIZE,	Create3DResizePixmap },
	    { TBPM_3DMENU,	Create3DMenuPixmap },
	    { TBPM_3DZOOM,	Create3DZoomPixmap },
	    { TBPM_3DBAR,	Create3DBarPixmap },
	};
	
	for (i = 0; i < (sizeof pmtab)/(sizeof pmtab[0]); i++) {
	    if (XmuCompareISOLatin1 (pmtab[i].name, name) == 0) {
	      *depthp   = Scr->d_depth;
	      return (*pmtab[i].proc) (widthp, heightp, cp);
	    }
	}
	fprintf (stderr, "%s:  no such built-in pixmap \"%s\"\n",
		 ProgramName, name);
	return (None);
    }
    else {
	*depthp = 1;
	return (FindBitmap (name, widthp, heightp));
    }
}

Pixmap GetBitmap (name)
    char *name;
{
    return FindBitmap (name, &JunkWidth, &JunkHeight);
}

static int reportxpmerror = 1;

#ifdef XPM
XpmIcon *GetXpmPixmap (name)
char *name;
{
    char fullPath [1024];
    int  status;
    XpmIcon *ret;
    char *bigname;

    if ((! Scr->PixmapDirectory) && (name [0] != '/') && (name [0] != '~')) return (None);

    bigname = ExpandFilename (name);
    if (bigname [0] == '/') {
	sprintf (fullPath, "%s", bigname);
    }
    else
    if (Scr->PixmapDirectory)
	sprintf (fullPath, "%s/%s", Scr->PixmapDirectory, name);
    else {
	if (bigname != name) free (bigname);
	return (None);
    }
    if (bigname != name) free (bigname);
    ret = (XpmIcon*) malloc (sizeof (XpmIcon));
    if (ret == NULL) return (None);

    ret->attributes.valuemask  = 0;
    ret->attributes.valuemask |= XpmSize;
    ret->attributes.valuemask |= XpmReturnPixels;
    status = XpmReadFileToPixmap(dpy, Scr->Root, fullPath,
				 &(ret->pixmap), &(ret->mask), &(ret->attributes));
    switch (status) {
	case XpmSuccess:
	    return (ret);

	case XpmColorError:
	    if (reportxpmerror)
		fprintf (stderr, "Could not parse or alloc requested color : %s\n", fullPath);
	    free (ret);
	    return (None);

	case XpmOpenFailed:
	    if (reportxpmerror)
		fprintf (stderr, "Cannot open XPM file : %s\n", fullPath);
	    free (ret);
	    return (None);

	case XpmFileInvalid:
	    fprintf (stderr, "invalid XPM file : %s\n", fullPath);
	    free (ret);
	    return (None);

	case XpmNoMemory:
	    if (reportxpmerror)
		fprintf (stderr, "Not enough memory for XPM file : %s\n", fullPath);
	    free (ret);
	    return (None);

	case XpmColorFailed:
	    if (reportxpmerror)
		fprintf (stderr, "Color not found in : %s\n", fullPath);
	    free (ret);
	    return (None);

	default :
	    fprintf (stderr, "Unknown error in : %s\n", fullPath);
	    free (ret);
	    return (None);
    }
}

static XpmIcon	*ctwmflag = None;
static GC	welcomeGC;
#endif

MaskScreen (file)
char *file;
{
    unsigned long valuemask;
    XSetWindowAttributes attributes;
    XEvent event;
    Cursor waitcursor;
    int x, y;

    NewFontCursor (&waitcursor, "watch");
    valuemask = (CWBackingStore | CWSaveUnder | CWBackPixel |
		 CWOverrideRedirect | CWEventMask | CWCursor);
    attributes.backing_store	 = NotUseful;
    attributes.save_under	 = False;
    attributes.background_pixel	 = Scr->Black;
    attributes.override_redirect = True;
    attributes.event_mask	 = ExposureMask;
    attributes.cursor		 = waitcursor;
    windowmask = XCreateWindow (dpy, Scr->Root, 0, 0,
			(unsigned int) Scr->MyDisplayWidth,
			(unsigned int) Scr->MyDisplayHeight,
			(unsigned int) 0,
			CopyFromParent, (unsigned int) CopyFromParent,
			(Visual *) CopyFromParent, valuemask,
			&attributes);

    XMapWindow (dpy, windowmask);
    XMaskEvent (dpy, ExposureMask, &event);
#ifdef XPM
    reportxpmerror = 0;
    if (file != NULL) {
	ctwmflag = GetXpmPixmap (file);
    }
    else {
	ctwmflag = GetXpmPixmap ("welcome.xpm");
    }
    reportxpmerror = 1;
    if (ctwmflag == None) return;

    welcomeGC = XCreateGC (dpy, windowmask, 0, NULL);
    x = (Scr->MyDisplayWidth  -  ctwmflag->attributes.width) / 2;
    y = (Scr->MyDisplayHeight - ctwmflag->attributes.height) / 2;
    XCopyArea (dpy, ctwmflag->pixmap, windowmask, welcomeGC, 0, 0,
		ctwmflag->attributes.width, ctwmflag->attributes.height, x, y);
#endif
}


UnmaskScreen () {
#ifdef XPM
    if (ctwmflag != None) {
	XFreeGC (dpy, welcomeGC);
	XFreeColors (dpy, DefaultColormap (dpy, Scr->screen),
			ctwmflag->attributes.pixels,
			ctwmflag->attributes.npixels, 0);
	XFreePixmap (dpy, ctwmflag->pixmap);
	XpmFreeAttributes (&ctwmflag->attributes);
    }
#endif
    XDestroyWindow (dpy, windowmask);
    windowmask = (Window) 0;
}

/* not used, this is a try to hide the ugly pop up of windows when
   ctwm restart. But when the server reparents all the windows in
   the saveset, it always puts them on top of the hierarchy. I don't
   see any way to avoid it.

ExternMaskScreen () {
    Window		*w;
    Atom		_XA_WM_MASKWINDOW;
    unsigned long	nitems, bytesafter;
    Atom		actual_type;
    int			actual_format;
    int			status;
    XEvent		event;

    if (windowmask != (Window) 0) return;
    _XA_WM_MASKWINDOW = XInternAtom (dpy, "WM_MASKWINDOW", True);
    if (_XA_WM_MASKWINDOW != None) {
	if (XGetWindowProperty (dpy, Scr->Root, _XA_WM_MASKWINDOW, 0L, 4, False,
			XA_WINDOW, &actual_type, &actual_format, &nitems,
			&bytesafter, &w) == Success) {
	    if (nitems != 0) {
		windowmask = *w;
		return;
	    }
	}
    }
    status =  system ("/users/lecom/devel/X/ctwm/maskscreen &");
    XMaskEvent (dpy, PropertyChangeMask, &event);
    if (XGetWindowProperty (dpy, Scr->Root, _XA_WM_MASKWINDOW, 0L, 4, False,
			XA_WINDOW, &actual_type, &actual_format, &nitems,
			&bytesafter, &w) == Success) {
	windowmask = *w;
    }
}

ExternUnmaskScreen () {
    Window		*w;
    Atom		_XA_WM_MASKWINDOW;
    unsigned long	nitems, bytesafter;
    Atom		actual_type;
    int			actual_format;
    int			status;

    _XA_WM_MASKWINDOW = XInternAtom (dpy, "WM_MASKWINDOW", True);
    XGetWindowProperty (dpy, Scr->Root, _XA_WM_MASKWINDOW, 0L, 4, True,
			XA_WINDOW, &actual_type, &actual_format, &nitems,
			&bytesafter, &w);
    if (windowmask != (Window) 0) {
	XDestroyWindow (dpy, windowmask);    
	windowmask = (Window) 0;
    }
}
*/

InsertRGBColormap (a, maps, nmaps, replace)
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

RemoveRGBColormap (a)
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

LocateStandardColormaps()
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

GetColor(kind, what, name)
int kind;
Pixel *what;
char *name;
{
    XColor color, junkcolor;
    Status stat = 0;
    Colormap cmap = Scr->TwmRoot.cmaps.cwins[0]->colormap->c;

#ifndef TOM
    if (!Scr->FirstTime)
	return;
#endif

    if (Scr->Monochrome != kind)
	return;

    if (strcmp(name, ":random") == 0) {
	/* generate a nice `random' color */
	name = generateRandomColor();
    }


    if (! XParseColor (dpy, cmap, name, &color)) {
	fprintf (stderr, "%s:  invalid color name \"%s\"\n", ProgramName, name);
	return;
    }
    if (! XAllocColor (dpy, cmap, &color))
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

GetShadeColors (cp)
ColorPair *cp;
{
    XColor	xcol;
    Colormap	cmap = Scr->TwmRoot.cmaps.cwins[0]->colormap->c;
    int		save;
    float	clearfactor;
    float	darkfactor;
    char	clearcol [32], darkcol [32];
    unsigned short nr, ng, nb;

    clearfactor = (float) Scr->ClearShadowContrast / 100.0;
    darkfactor  = (100.0 - (float) Scr->DarkShadowContrast)  / 100.0;
    xcol.pixel = cp->back;
    XQueryColor (dpy, cmap, &xcol);
/*
    nr = (unsigned short) min (65535, xcol.red   * clearfactor);
    ng = (unsigned short) min (65535, xcol.green * clearfactor);
    nb = (unsigned short) min (65535, xcol.blue  * clearfactor);
*/
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

GetFont(font)
MyFont *font;
{
    char *deffontname = "fixed";

    if (font->font != NULL)
	XFreeFont(dpy, font->font);

    if (!font->name || ((font->font = XLoadQueryFont(dpy, font->name)) == NULL))
    {
	if (Scr->DefaultFont.name) {
	    deffontname = Scr->DefaultFont.name;
	}
	if ((font->font = XLoadQueryFont(dpy, deffontname)) == NULL)
	{
	    fprintf (stderr, "%s:  unable to open fonts \"%s\" or \"%s\"\n",
		     ProgramName, font->name, deffontname);
	    exit(1);
	}

    }
    font->height = font->font->ascent + font->font->descent;
    font->y = font->font->ascent;
}


/*
 * SetFocus - separate routine to set focus to make things more understandable
 * and easier to debug
 */
SetFocus (tmp_win, time)
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


#ifdef NOPUTENV
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
    char *s;
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
		environ[idx] = s;
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
    environ[idx] = s;
    environ[idx+1] = 0;

    return 0;
}
#endif /* NOPUTENV */


static Pixmap CreateXLogoPixmap (widthp, heightp)
    unsigned int *widthp, *heightp;
{
    int h = Scr->TBInfo.width - Scr->TBInfo.border * 2;
    if (h < 0) h = 0;

    *widthp = *heightp = (unsigned int) h;
    if (Scr->tbpm.xlogo == None) {
	GC gc, gcBack;

	Scr->tbpm.xlogo = XCreatePixmap (dpy, Scr->Root, h, h, 1);
	gc = XCreateGC (dpy, Scr->tbpm.xlogo, 0L, NULL);
	XSetForeground (dpy, gc, 0);
	XFillRectangle (dpy, Scr->tbpm.xlogo, gc, 0, 0, h, h);
	XSetForeground (dpy, gc, 1);
	gcBack = XCreateGC (dpy, Scr->tbpm.xlogo, 0L, NULL);
	XSetForeground (dpy, gcBack, 0);

	/*
	 * draw the logo large so that it gets as dense as possible; then white
	 * out the edges so that they look crisp
	 */
	XmuDrawLogo (dpy, Scr->tbpm.xlogo, gc, gcBack, -1, -1, h + 2, h + 2);
	XDrawRectangle (dpy, Scr->tbpm.xlogo, gcBack, 0, 0, h - 1, h - 1);

	/*
	 * done drawing
	 */
	XFreeGC (dpy, gc);
	XFreeGC (dpy, gcBack);
    }
    return Scr->tbpm.xlogo;
}


static Pixmap CreateResizePixmap (widthp, heightp)
    unsigned int *widthp, *heightp;
{
    int h = Scr->TBInfo.width - Scr->TBInfo.border * 2;
    if (h < 1) h = 1;

    *widthp = *heightp = (unsigned int) h;
    if (Scr->tbpm.resize == None) {
	XPoint	points[3];
	GC gc;
	int w;
	int lw;

	/*
	 * create the pixmap
	 */
	Scr->tbpm.resize = XCreatePixmap (dpy, Scr->Root, h, h, 1);
	gc = XCreateGC (dpy, Scr->tbpm.resize, 0L, NULL);
	XSetForeground (dpy, gc, 0);
	XFillRectangle (dpy, Scr->tbpm.resize, gc, 0, 0, h, h);
	XSetForeground (dpy, gc, 1);
	lw = h / 16;
	if (lw == 1)
	    lw = 0;
	XSetLineAttributes (dpy, gc, lw, LineSolid, CapButt, JoinMiter);

	/*
	 * draw the resize button,
	 */
	w = (h * 2) / 3;
	points[0].x = w;
	points[0].y = 0;
	points[1].x = w;
	points[1].y = w;
	points[2].x = 0;
	points[2].y = w;
	XDrawLines (dpy, Scr->tbpm.resize, gc, points, 3, CoordModeOrigin);
	w = w / 2;
	points[0].x = w;
	points[0].y = 0;
	points[1].x = w;
	points[1].y = w;
	points[2].x = 0;
	points[2].y = w;
	XDrawLines (dpy, Scr->tbpm.resize, gc, points, 3, CoordModeOrigin);

	/*
	 * done drawing
	 */
	XFreeGC(dpy, gc);
    }
    return Scr->tbpm.resize;
}


static Pixmap CreateDotPixmap (widthp, heightp)
    unsigned int *widthp, *heightp;
{
    int h = Scr->TBInfo.width - Scr->TBInfo.border * 2;

    h = h * 3 / 4;
    if (h < 1) h = 1;
    if (!(h & 1))
	h--;
    *widthp = *heightp = (unsigned int) h;
    if (Scr->tbpm.delete == None) {
	GC  gc;
	Pixmap pix;

	pix = Scr->tbpm.delete = XCreatePixmap (dpy, Scr->Root, h, h, 1);
	gc = XCreateGC (dpy, pix, 0L, NULL);
	XSetLineAttributes (dpy, gc, h, LineSolid, CapRound, JoinRound);
	XSetForeground (dpy, gc, 0L);
	XFillRectangle (dpy, pix, gc, 0, 0, h, h);
	XSetForeground (dpy, gc, 1L);
	XDrawLine (dpy, pix, gc, h/2, h/2, h/2, h/2);
	XFreeGC (dpy, gc);
    }
    return Scr->tbpm.delete;
}

struct Colori {
    Pixel color;
    Pixmap pix;
    struct Colori *next;
};

static Pixmap Create3DDotPixmap (widthp, heightp, cp)
unsigned int *widthp, *heightp;
ColorPair cp;
{
    int		h;
    struct Colori *col;
    static struct Colori *colori = NULL;

    h = Scr->TBInfo.width - Scr->TBInfo.border * 2;
    if (! (h & 1)) h--;
    *widthp = *heightp = (unsigned int) h;

    for (col = colori; col; col = col->next) {
	if (col->color == cp.back) break;
    }
    if (col != NULL) return (col->pix);
    col = (struct Colori*) malloc (sizeof (struct Colori));
    col->color = cp.back;
    col->pix   = XCreatePixmap (dpy, Scr->Root, h, h, Scr->d_depth);

    Draw3DBorder (col->pix, 0, 0, h, h, 2, cp, off, True, False);
    Draw3DBorder (col->pix, (h / 2) - 2, (h / 2) - 2, 5, 5, 2, cp, off, True, False);
    col->next = colori;
    colori = col;

    return (colori->pix);
}

static Pixmap Create3DBarPixmap (widthp, heightp, cp)
unsigned int *widthp, *heightp;
ColorPair cp;
{
    int		h;
    struct Colori *col;
    static struct Colori *colori = NULL;

    h = Scr->TBInfo.width - Scr->TBInfo.border * 2;
    if (!(h & 1)) h--;
    *widthp = *heightp = (unsigned int) h;

    for (col = colori; col; col = col->next) {
	if (col->color == cp.back) break;
    }
    if (col != NULL) return (col->pix);
    col = (struct Colori*) malloc (sizeof (struct Colori));
    col->color = cp.back;
    col->pix   = XCreatePixmap (dpy, Scr->Root, h, h, Scr->d_depth);
    Draw3DBorder (col->pix, 0, 0, h, h, 2, cp, off, True, False);
    Draw3DBorder (col->pix, 4, (h / 2) - 2, h - 8, 5, 2, cp, off, True, False);
    col->next = colori;
    colori = col;

    return (colori->pix);
}

static Pixmap Create3DMenuPixmap (widthp, heightp, cp)
unsigned int *widthp, *heightp;
ColorPair cp;
{
    int		h, i;
    struct Colori *col;
    static struct Colori *colori = NULL;

    h = Scr->TBInfo.width - Scr->TBInfo.border * 2;
    if (!(h & 1)) h--;
    *widthp = *heightp = (unsigned int) h;

    for (col = colori; col; col = col->next) {
	if (col->color == cp.back) break;
    }
    if (col != NULL) return (col->pix);
    col = (struct Colori*) malloc (sizeof (struct Colori));
    col->color = cp.back;
    col->pix   = XCreatePixmap (dpy, Scr->Root, h, h, Scr->d_depth);
    Draw3DBorder (col->pix, 0, 0, h, h, 2, cp, off, True, False);
    for (i = 4; i < h - 7; i += 5) {
	Draw3DBorder (col->pix, 4, i, h - 8, 4, 2, cp, off, True, False);
    }
    col->next = colori;
    colori = col;

    return (colori->pix);
}

Pixmap Create3DMenuIcon (height, widthp, heightp, cp)
unsigned int height, *widthp, *heightp;
ColorPair cp;
{
    unsigned int h, w;
    int		i;
    struct Colori *col;
    static struct Colori *colori = NULL;

    h = height;
    w = h * 7 / 8;
    if (h < 1)
	h = 1;
    if (w < 1)
	w = 1;
    *widthp  = w;
    *heightp = h;

    for (col = colori; col; col = col->next) {
	if (col->color == cp.back) break;
    }
    if (col != NULL) return (col->pix);
    col = (struct Colori*) malloc (sizeof (struct Colori));
    col->color = cp.back;
    col->pix   = XCreatePixmap (dpy, Scr->Root, h, h, Scr->d_depth);
    Draw3DBorder (col->pix, 0, 0, w, h, 1, cp, off, True, False);
    for (i = 3; i < h - 4; i += 4) {
	Draw3DBorder (col->pix, 4, i, w - 8, 2, 1, Scr->MenuC, off, True, False);
    }
    col->next = colori;
    colori = col;

    return (colori->pix);
}

#include "siconify.bm"

Pixmap Create3DIconManagerIcon (cp)
ColorPair cp;
{
    int		i;
    unsigned int w, h;
    struct Colori *col;
    static struct Colori *colori = NULL;

    w = (unsigned int) siconify_width;
    h = (unsigned int) siconify_height;

    for (col = colori; col; col = col->next) {
	if (col->color == cp.back) break;
    }
    if (col != NULL) return (col->pix);
    col = (struct Colori*) malloc (sizeof (struct Colori));
    col->color = cp.back;
    col->pix   = XCreatePixmap (dpy, Scr->Root, w, h, Scr->d_depth);
    Draw3DBorder (col->pix, 0, 0, w, h, 4, cp, off, True, False);
    col->next = colori;
    colori = col;

    return (colori->pix);
}

static Pixmap Create3DResizePixmap (widthp, heightp, cp)
unsigned int *widthp, *heightp;
ColorPair cp;
{
    int		h;
    struct Colori *col;
    static struct Colori *colori = NULL;

    h = Scr->TBInfo.width - Scr->TBInfo.border * 2;
    if (!(h & 1)) h--;
    *widthp = *heightp = (unsigned int) h;

    for (col = colori; col; col = col->next) {
	if (col->color == cp.back) break;
    }
    if (col != NULL) return (col->pix);
    col = (struct Colori*) malloc (sizeof (struct Colori));
    col->color = cp.back;
    col->pix   = XCreatePixmap (dpy, Scr->Root, h, h, Scr->d_depth);
    Draw3DBorder (col->pix, 0, 0, h, h, 2, cp, off, True, False);
    Draw3DBorder (col->pix, 0, h / 4, ((3 * h) / 4) + 1, ((3 * h) / 4) + 1, 2, cp, off, True, False);
    Draw3DBorder (col->pix, 0, h / 2, (h / 2) + 1, (h / 2) + 1, 2, cp, off, True, False);
    col->next = colori;
    colori = col;

    return (colori->pix);
}

static Pixmap Create3DZoomPixmap (widthp, heightp, cp)
unsigned int *widthp, *heightp;
ColorPair cp;
{
    int		h;
    struct Colori *col;
    static struct Colori *colori = NULL;

    h = Scr->TBInfo.width - Scr->TBInfo.border * 2;
    if (!(h & 1)) h--;
    *widthp = *heightp = (unsigned int) h;

    for (col = colori; col; col = col->next) {
	if (col->color == cp.back) break;
    }
    if (col != NULL) return (col->pix);
    col = (struct Colori*) malloc (sizeof (struct Colori));
    col->color = cp.back;
    col->pix   = XCreatePixmap (dpy, Scr->Root, h, h, Scr->d_depth);
    Draw3DBorder (col->pix, 0, 0, h, h, 2, cp, off, True);
    Draw3DBorder (col->pix, h / 4, h / 4, (h / 2) + 2, (h / 2) + 2, 2, cp, off, True, False);
    col->next = colori;
    colori = col;

    return (colori->pix);
}

#define questionmark_width 8
#define questionmark_height 8
static char questionmark_bits[] = {
   0x38, 0x7c, 0x64, 0x30, 0x18, 0x00, 0x18, 0x18};

static Pixmap CreateQuestionPixmap (widthp, heightp)
    unsigned int *widthp, *heightp;
{
    *widthp = questionmark_width;
    *heightp = questionmark_height;
    if (Scr->tbpm.question == None) {
	Scr->tbpm.question = XCreateBitmapFromData (dpy, Scr->Root,
						    questionmark_bits,
						    questionmark_width,
						    questionmark_height);
    }
    /*
     * this must succeed or else we are in deep trouble elsewhere
     */
    return Scr->tbpm.question;
}


static Pixmap CreateMenuPixmap (widthp, heightp)
    int *widthp, *heightp;
{
    CreateMenuIcon (Scr->TBInfo.width - Scr->TBInfo.border * 2,widthp,heightp);
}

Pixmap CreateMenuIcon (height, widthp, heightp)
    int	height;
    int	*widthp, *heightp;
{
    int h, w;
    int ih, iw;
    int	ix, iy;
    int	mh, mw;
    int	tw, th;
    int	lw, lh;
    int	lx, ly;
    int	lines, dly;
    int off;
    int	bw;

    h = height;
    w = h * 7 / 8;
    if (h < 1)
	h = 1;
    if (w < 1)
	w = 1;
    *widthp = w;
    *heightp = h;
    if (Scr->tbpm.menu == None) {
	Pixmap  pix;
	GC	gc;

	pix = Scr->tbpm.menu = XCreatePixmap (dpy, Scr->Root, w, h, 1);
	gc = XCreateGC (dpy, pix, 0L, NULL);
	XSetForeground (dpy, gc, 0L);
	XFillRectangle (dpy, pix, gc, 0, 0, w, h);
	XSetForeground (dpy, gc, 1L);
	ix = 1;
	iy = 1;
	ih = h - iy * 2;
	iw = w - ix * 2;
	off = ih / 8;
	mh = ih - off;
	mw = iw - off;
	bw = mh / 16;
	if (bw == 0 && mw > 2)
	    bw = 1;
	tw = mw - bw * 2;
	th = mh - bw * 2;
	XFillRectangle (dpy, pix, gc, ix, iy, mw, mh);
	XFillRectangle (dpy, pix, gc, ix + iw - mw, iy + ih - mh, mw, mh);
	XSetForeground (dpy, gc, 0L);
	XFillRectangle (dpy, pix, gc, ix+bw, iy+bw, tw, th);
	XSetForeground (dpy, gc, 1L);
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
	{
	    lines--;
	}
	dly = lh / (lines - 1);
	while (lines--)
	{
	    XFillRectangle (dpy, pix, gc, lx, ly, lw, bw);
	    ly += dly;
	}
	XFreeGC (dpy, gc);
    }
    return Scr->tbpm.menu;
}

Draw3DBorder (w, x, y, width, height, bw, cp, state, fill, forcebw)
Window w;
int    x, y, width, height, bw;
ColorPair cp;
int state, fill, forcebw;
{
    int		  i;
    XGCValues	  gcv;
    unsigned long gcm;
    static int firsttime = 1;

    if (Scr->Monochrome != COLOR) {
	if (fill) {
	    gcm = GCFillStyle;
	    gcv.fill_style = FillOpaqueStippled;
	    XChangeGC (dpy, Scr->GreyGC, gcm, &gcv);
	    XFillRectangle (dpy, w, Scr->GreyGC, x, y, width, height);
	}
	gcm  = 0;
	gcm |= GCLineStyle;		
	gcv.line_style = (state == on) ? LineSolid : LineDoubleDash;
	gcm |= GCFillStyle;
	gcv.fill_style = FillSolid;
	XChangeGC (dpy, Scr->GreyGC, gcm, &gcv);
	for (i = 0; i < bw; i++) {
	    XDrawLine (dpy, w, Scr->GreyGC, x,                 y + i,
					    x + width - i - 1, y + i);
	    XDrawLine (dpy, w, Scr->GreyGC, x + i,                  y,
					    x + i, y + height - i - 1);
	}

	gcm  = 0;
	gcm |= GCLineStyle;		
	gcv.line_style = (state == on) ? LineDoubleDash : LineSolid;
	gcm |= GCFillStyle;
	gcv.fill_style = FillSolid;
	XChangeGC (dpy, Scr->GreyGC, gcm, &gcv);
	for (i = 0; i < bw; i++) {
	    XDrawLine (dpy, w, Scr->GreyGC, x + width - i - 1,          y + i,
					    x + width - i - 1, y + height - 1);
	    XDrawLine (dpy, w, Scr->GreyGC, x + i,         y + height - i - 1,
					    x + width - 1, y + height - i - 1);
	}
	return;
    }

    if (fill) {
	FB (cp.back, cp.fore);
	XFillRectangle (dpy, w, Scr->NormalGC, x, y, width, height);
    }
    if (Scr->BeNiceToColormap) {
	int dashoffset = 0;

	gcm  = 0;
	gcm |= GCLineStyle;		
	gcv.line_style = (forcebw) ? LineSolid : LineDoubleDash;
	gcm |= GCBackground;
	gcv.background = cp.back;
	XChangeGC (dpy, Scr->ShadGC, gcm, &gcv);
	    
	if (state == on)
	    XSetForeground (dpy, Scr->ShadGC, Scr->Black);
	else
	    XSetForeground (dpy, Scr->ShadGC, Scr->White);
	for (i = 0; i < bw; i++) {
	    XDrawLine (dpy, w, Scr->ShadGC, x + i,     y + dashoffset,
					    x + i, y + height - i - 1);
	    XDrawLine (dpy, w, Scr->ShadGC, x + dashoffset,    y + i,
					    x + width - i - 1, y + i);
	    dashoffset = 1 - dashoffset;
	}
	if (state == on)
	    XSetForeground (dpy, Scr->ShadGC, Scr->White);
	else
	    XSetForeground (dpy, Scr->ShadGC, Scr->Black);
	for (i = 0; i < bw; i++) {
	    XDrawLine (dpy, w, Scr->ShadGC, x + i,         y + height - i - 1,
					    x + width - 1, y + height - i - 1);
	    XDrawLine (dpy, w, Scr->ShadGC, x + width - i - 1,          y + i,
					    x + width - i - 1, y + height - 1);
	}
	return;
    }

    if (state == on) {
	FB (cp.shadd, cp.shadc);
    }
    else {
	FB (cp.shadc, cp.shadd);
    }
    for (i = 0; i < bw; i++) {
	XDrawLine (dpy, w, Scr->NormalGC, x,                 y + i,
					  x + width - i - 1, y + i);
	XDrawLine (dpy, w, Scr->NormalGC, x + i,                  y,
					  x + i, y + height - i - 1);
    }

    if (state == on) {
	FB (cp.shadc, cp.shadd);
    }
    else {
	FB (cp.shadd, cp.shadc);
    }
    for (i = 0; i < bw; i++) {
	XDrawLine (dpy, w, Scr->NormalGC, x + width - i - 1,          y + i,
					  x + width - i - 1, y + height - 1);
	XDrawLine (dpy, w, Scr->NormalGC, x + i,         y + height - i - 1,
					  x + width - 1, y + height - i - 1);
    }
}

/* Returns a blank cursor */
Cursor NoCursor()
{
        static Cursor blank = (Cursor) NULL;
        Pixmap nopixmap;
        XColor nocolor;
        
        if (!blank) {
                nopixmap = XCreatePixmap(dpy, Scr->Root, 1, 1, 1);
                nocolor.red = nocolor.blue = nocolor.green = 0;
                blank = XCreatePixmapCursor(dpy, nopixmap, nopixmap, &nocolor, &
nocolor, 1, 1);
        }
        return(blank);
}

void
SetupSizeInfo(name, fmt, x, y)
char* name;
char* fmt;
int x, y;
{
    int namelen;
    int height;
    char buf[100];

    if (name) {
	sprintf(buf, "%s: ", name);
	namelen = strlen(buf);
	Scr->SizeStringOffset = (SIZE_HINDENT + XTextWidth (Scr->SizeFont.font, buf, namelen));
    } else {
	Scr->SizeStringOffset = SIZE_HINDENT;
    }
    height = Scr->SizeFont.height + SIZE_VINDENT * 2;
	    
    XResizeWindow (dpy, Scr->SizeWindow, Scr->SizeStringOffset +
		   Scr->SizeStringWidth, height);
    if (Scr->CenteredInfoBox) {
	XMoveWindow(dpy, Scr->SizeWindow,
		    (Scr->MyDisplayWidth - (Scr->SizeStringWidth + Scr->SizeStringOffset))/2,
		    (Scr->MyDisplayHeight - (Scr->SizeFont.height + SIZE_VINDENT*2))/2);
    }
    XMapRaised(dpy, Scr->SizeWindow);
    if (name) {
	FBF(Scr->DefaultC.fore, Scr->DefaultC.back,
	    Scr->SizeFont.font->fid);
	XDrawImageString (dpy, Scr->SizeWindow, Scr->NormalGC,
			  SIZE_HINDENT,
			  SIZE_VINDENT + Scr->SizeFont.font->ascent,
			  buf, namelen);
    }
    DisplaySizeInfo(fmt, x, y);
}

void
DisplaySizeInfo(fmt, x, y)
char* fmt;
int x, y;
{
    char str[100];

    (void) sprintf (str, fmt, x, y);
    XRaiseWindow (dpy, Scr->SizeWindow);
    FBF (Scr->DefaultC.fore, Scr->DefaultC.back, Scr->SizeFont.font->fid);
    XDrawImageString (dpy, Scr->SizeWindow, Scr->NormalGC,
                      Scr->SizeStringOffset,
                      Scr->SizeFont.font->ascent + SIZE_VINDENT,
                      str, strlen(str));
}

void
RemoveSizeInfo()
{
    XUnmapWindow(dpy, Scr->SizeWindow);
}

/* the order in which these are called is significant. */

#define MAXHOSTNAME 255

static char *client[MAXHOSTNAME], server[MAXHOSTNAME], *r;
static Screen *scr;
static Visual *vis;

#define SYM_ALLOC_CHAR(a, b) {		\
	r = (char *)malloc(strlen(a) + strlen(b) + 4);	\
	sprintf(r, "-D%s=%s", a, b);			\
	return r;					\
}
#define SYM_ALLOC_NUM(a, b) {	\
	r = (char *)malloc(strlen(a) + 10 + 4);		\
	sprintf(r, "-D%s=%d", a, b);			\
	return r;					\
}

static char *
get_m4()
{
	return "m4";
}

static char *
get_HOST()
{
	char *colon;

	XmuGetHostname(client, MAXHOSTNAME);
	strcpy(server, XDisplayName(NULL));
	if ((colon = index(server, ':')) != NULL)
		*colon = '\0';
	if (server[0] == '\0')
		strcpy(server, client);

	SYM_ALLOC_CHAR("HOST", server);
}

static char *
get_SERVERHOST()
{
	SYM_ALLOC_CHAR("SERVERHOST", server);
}

static char *
get_CLIENTHOST()
{
	SYM_ALLOC_CHAR("CLIENTHOST", client);
}

static char *
get_VERSION()
{
	SYM_ALLOC_NUM("VERSION", ProtocolVersion(dpy));
}

static char *
get_REVISION()
{
	SYM_ALLOC_NUM("REVISION", ProtocolRevision(dpy));
}

static char *
get_VENDOR()
{
	SYM_ALLOC_CHAR("VENDOR", ServerVendor(dpy));
}

static char *
get_RELEASE()
{
	SYM_ALLOC_NUM("RELEASE", VendorRelease(dpy));
}

static char *
get_WIDTH()
{
	scr = DefaultScreenOfDisplay(dpy);

	SYM_ALLOC_NUM("WIDTH", scr->width);
}

static char *
get_HEIGHT()
{
	SYM_ALLOC_NUM("HEIGHT", scr->height);
}

static char *
get_X_RESOLUTION()
{
	SYM_ALLOC_NUM("X_RESOLUTION",
		      ((scr->width * 100000 / scr->mwidth) + 50) / 100);
}

static char *
get_Y_RESOLUTION()
{
	SYM_ALLOC_NUM("Y_RESOLUTION",
		      ((scr->height * 100000 / scr->mheight) + 50) / 100);
}

static char *
get_PLANES()
{
	SYM_ALLOC_NUM("PLANES", DisplayPlanes(dpy, DefaultScreen(dpy)));
}

static char *
get_BITS_PER_RGB()
{
	vis = DefaultVisualOfScreen(scr);

	SYM_ALLOC_NUM("BITS_PER_RGB", vis->bits_per_rgb);
}

static char *
get_CLASS()
{
	switch (vis->class) {
	case StaticGray:
		SYM_ALLOC_CHAR("CLASS", "StaticGray");
		break;
	case GrayScale:
		SYM_ALLOC_CHAR("CLASS", "GrayScale");
		break;
	case StaticColor:
		SYM_ALLOC_CHAR("CLASS", "StaticColor");
		break;
	case PseudoColor:
		SYM_ALLOC_CHAR("CLASS", "PseudoColor");
		break;
	case TrueColor:
		SYM_ALLOC_CHAR("CLASS", "PseudoColor");
		break;
	case DirectColor:
		SYM_ALLOC_CHAR("CLASS", "DirectColor");
		break;
	default:
		SYM_ALLOC_CHAR("CLASS", "Unknown");
		break;
	}
}

static char *
get_COLOR()
{
	char *r;

	switch (vis->class) {
	case StaticGray:
	case GrayScale:
		r = "-DMONO";
		break;
	case StaticColor:
	case PseudoColor:
	case TrueColor:
	case DirectColor:
		r = "-DCOLOR";
		break;
	}

	return r;
}

static char *(*symbols[])() = {
	get_m4,
	get_HOST,
	get_SERVERHOST,
	get_CLIENTHOST,
	get_VERSION,
	get_REVISION,
	get_VENDOR,
	get_RELEASE,
	get_WIDTH,
	get_HEIGHT,
	get_X_RESOLUTION,
	get_Y_RESOLUTION,
	get_PLANES,
	get_BITS_PER_RGB,
	get_CLASS,
	get_COLOR,
};

/* build a list of `xrdb -symbols' style definitions for
 * passing to m4
 */
char **
buildSymbolList()
{
	char **values;
	int v, z;
	int nsyms = sizeof(symbols) / sizeof(char *(*)());

	if ((values = (char **)malloc((nsyms+1)*sizeof(char *))) == NULL) {
		fprintf(stderr,
			"%s: cannot allocate memory for #define list\n",
			ProgramName);
		exit(1);
	}

	for(v = 0; v < nsyms; v++)
		values[v] = (symbols[v])();

	values[nsyms] = NULL;	/* NULL terminate the list */

	return values;
}


static char *
generateRandomColor()
{
    extern double atof();
    static char ret[8];
    static unsigned int seed = 0;
    static int desiredIntensity = 166;

    int newIntensity = 0;
    int tooDark, red, green, blue;
    int i, c, errflg=0;
	
    if (seed == 0) {
	seed = time(0) + getpid();
	srand(seed);
    }

    /* assign preset values of guns */
    red   = 0;
    green = 0;
    blue  = 0;

    tooDark = 1;

    /* We want a bit of everything, but ensure that it's bright overall */
    while (tooDark && !(red && green && blue)) {
	red   = rand()&255;
	green = rand()&255;
	blue  = rand()&255;

	/* I've seen a better formula for computing intensity 
	 * somewhere, but I can't remember where. 
	 * This is good enough for now. -njw
	 */
	newIntensity = (red + green + blue)/3; 
	tooDark = (newIntensity < desiredIntensity);
    }
    
    sprintf(ret, "#%02x%02x%02x", red, green, blue);
    
    return ret;
}

