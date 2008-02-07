/*
 * Copyright 1989 Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.  M.I.T. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * M.I.T. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL M.I.T.
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/***********************************************************************
 *
 * $XConsortium: iconmgr.c,v 1.48 91/09/10 15:27:07 dave Exp $
 *
 * Icon Manager routines
 *
 * 09-Mar-89 Tom LaStrange		File Created
 *
 ***********************************************************************/

#include <stdio.h>
#include "twm.h"
#include "util.h"
#include "menus.h"
#include "desktop.h"
#include "parse.h"
#include "screen.h"
#include "resize.h"
#include "add_window.h"
#include <X11/Xos.h>
#include <X11/Xmu/CharSet.h>

#ifdef macII
int strcmp(); /* missing from string.h in AUX 2.0 */
#endif

static int ComputeIconMgrWindowHeight (IconMgr *ip);

/* see AddIconManager() - djhjr - 5/5/98
int iconmgr_textx = siconify_width+11;
*/
int iconmgr_iconx = 0, iconmgr_textx = 0;

WList *Active = NULL;
WList *DownIconManager = NULL;

/* was an external file - djhjr - 10/30/02 */
#define siconify_width 11
#define siconify_height 11
/*
static unsigned char siconify_bits[] = {
   0xff, 0x07, 0x01, 0x04, 0x0d, 0x05, 0x9d, 0x05, 0xb9, 0x04, 0x51, 0x04,
   0xe9, 0x04, 0xcd, 0x05, 0x85, 0x05, 0x01, 0x04, 0xff, 0x07};
*/

int iconifybox_width = siconify_width;
int iconifybox_height = siconify_height;

/* djhjr - 10/30/02 */
void SetIconMgrPixmap(filename)
char *filename;
{
	Scr->iconMgrIconName = filename;
}

/***********************************************************************
 *
 *  Procedure:
 *	CreateIconManagers - creat all the icon manager windows
 *		for this screen.
 *
 *  Returned Value:
 *	none
 *
 *  Inputs:
 *	none
 *
 ***********************************************************************
 */

struct Colori {
    Pixel color;
    Pixmap pix;
    struct Colori *next;
};

#if 0
Pixmap Create3DIconManagerIcon (cp)
ColorPair cp;
{
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
#ifdef ORIGINAL_ICONMGRPIXMAP
    Draw3DBorder (col->pix, 0, 0, w, h, 4, cp, off, True, False);
#else
    Draw3DBorder (col->pix, 0, 0, w, h, 1, cp, off, True, False);
#ifdef DO_DOT
    Draw3DBorder (col->pix, (w / 2) - 1, (h / 2) - 1, 3, 3, 1, cp, off, True, False);
#endif
#endif
    col->next = colori;
    colori = col;

    return (colori->pix);
}
#endif

void CreateIconManagers()
{
	XClassHint *class; /* djhjr - 2/28/99 */
    IconMgr *p;
    int mask;
    char str[100];
    char str1[100];
    Pixel background;
    char *icon_name;

    if (Scr->NoIconManagers)
	return;

/* djhjr - 10/30/02
    if (Scr->siconifyPm == None)
    {
	Scr->siconifyPm->pixmap = XCreatePixmapFromBitmapData(dpy, Scr->Root,
	    (char *)siconify_bits, siconify_width, siconify_height, 1, 0, 1);
    }
*/

    for (p = &Scr->iconmgr; p != NULL; p = p->next)
    {
	mask = XParseGeometry(p->geometry, &JunkX, &JunkY,
			      (unsigned int *) &p->width, (unsigned int *)&p->height);

	/* djhjr - 3/1/99 */
	if (p->width > Scr->MyDisplayWidth) p->width = Scr->MyDisplayWidth;

	if (mask & XNegative)
/* djhjr - 4/19/96
	    JunkX = Scr->MyDisplayWidth - p->width - 
	      (2 * Scr->BorderWidth) + JunkX;
*/
/* djhjr - 8/11/98
	    JunkX += Scr->MyDisplayWidth - p->width - 
	      (2 * (Scr->ThreeDBorderWidth ? Scr->ThreeDBorderWidth : Scr->BorderWidth));
*/
	    JunkX += Scr->MyDisplayWidth - p->width - 
	      (2 * Scr->BorderWidth);

	if (mask & YNegative)
/* djhjr - 4/19/96
	    JunkY = Scr->MyDisplayHeight - p->height -
	      (2 * Scr->BorderWidth) + JunkY;
*/
/* djhjr - 8/11/98
	    JunkY += Scr->MyDisplayHeight - p->height -
	      (2 * (Scr->ThreeDBorderWidth ? Scr->ThreeDBorderWidth : Scr->BorderWidth));
*/
	    JunkY += Scr->MyDisplayHeight - p->height -
	      (2 * Scr->BorderWidth);

	/* djhjr - 9/10/98 */
	if (p->width  < 1) p->width  = 1;
	if (p->height < 1) p->height = 1;

	background = Scr->IconManagerC.back;
	GetColorFromList(Scr->IconManagerBL, p->name, (XClassHint *)NULL,
			 &background);

	p->w = XCreateSimpleWindow(dpy, Scr->Root,
	    JunkX, JunkY, p->width, p->height,
	    
	    0,  /* was '1' - submitted by Rolf Neugebauer */

	    Scr->Black, background);

	sprintf(str, "%s Icon Manager", p->name);
	sprintf(str1, "%s Icons", p->name);
	if (p->icon_name)
	    icon_name = p->icon_name;
	else
	    icon_name = str1;

	/* djhjr - 5/19/98 */
	/* was setting for the TwmWindow after AddWindow() - djhjr - 2/28/99 */
	class = XAllocClassHint();
	class->res_name = strdup(str);
	class->res_class = strdup(VTWM_ICONMGR_CLASS);
	XSetClassHint(dpy, p->w, class);

	XSetStandardProperties(dpy, p->w, str, icon_name, None, NULL, 0, NULL);

	p->twm_win = AddWindow(p->w, TRUE, p);

	SetMapStateProp (p->twm_win, WithdrawnState);
    }
    for (p = &Scr->iconmgr; p != NULL; p = p->next)
    {
	GrabButtons(p->twm_win);
	GrabKeys(p->twm_win);
    }
}

/***********************************************************************
 *
 *  Procedure:
 *	AllocateIconManager - allocate a new icon manager
 *
 *  Inputs:
 *	name	- the name of this icon manager
 *	icon_name - the name of the associated icon
 *	geom	- a geometry string to eventually parse
 *	columns	- the number of columns this icon manager has
 *
 ***********************************************************************
 */

IconMgr *AllocateIconManager(name, icon_name, geom, columns)
    char *name;
    char *geom;
    char *icon_name;
    int columns;
{
    IconMgr *p;

#ifdef DEBUG_ICONMGR
    fprintf(stderr, "AllocateIconManager\n");
    fprintf(stderr, "  name=\"%s\" icon_name=\"%s\", geom=\"%s\", col=%d\n",
	name, icon_name, geom, columns);
#endif

    if (Scr->NoIconManagers)
	return NULL;

    p = (IconMgr *)malloc(sizeof(IconMgr));
    p->name = name;
    p->icon_name = icon_name;
    p->geometry = geom;
    p->columns = columns;
    p->first = NULL;
    p->last = NULL;
    p->active = NULL;
    p->scr = Scr;
    p->count = 0;
    p->x = 0;
    p->y = 0;
    p->width = 150;
    p->height = 10;

    Scr->iconmgr.lasti->next = p;
    p->prev = Scr->iconmgr.lasti;
    Scr->iconmgr.lasti = p;
    p->next = NULL;

    return(p);
}

/***********************************************************************
 *
 *  Procedure:
 *	MoveIconManager - move the pointer around in an icon manager
 *
 *  Inputs:
 *	dir	- one of the following:
 *			F_FORWICONMGR	- forward in the window list
 *			F_BACKICONMGR	- backward in the window list
 *			F_UPICONMGR	- up one row
 *			F_DOWNICONMGR	- down one row
 *			F_LEFTICONMGR	- left one column
 *			F_RIGHTICONMGR	- right one column
 *
 *  Special Considerations:
 *	none
 *
 ***********************************************************************
 */

void MoveIconManager(dir)
    int dir;
{
    IconMgr *ip;
    WList *tmp = NULL;
    int cur_row, cur_col, new_row, new_col;
    int row_inc, col_inc;
    int got_it;

    if (!Active) return;

    cur_row = Active->row;
    cur_col = Active->col;
    ip = Active->iconmgr;

    row_inc = 0;
    col_inc = 0;
    got_it = FALSE;

    switch (dir)
    {
	case F_FORWICONMGR:
	    if ((tmp = Active->next) == NULL)
		tmp = ip->first;
	    got_it = TRUE;
	    break;

	case F_BACKICONMGR:
	    if ((tmp = Active->prev) == NULL)
		tmp = ip->last;
	    got_it = TRUE;
	    break;

	case F_UPICONMGR:
	    row_inc = -1;
	    break;

	case F_DOWNICONMGR:
	    row_inc = 1;
	    break;

	case F_LEFTICONMGR:
	    col_inc = -1;
	    break;

	case F_RIGHTICONMGR:
	    col_inc = 1;
	    break;
    }

    /* If got_it is FALSE ast this point then we got a left, right,
     * up, or down, command.  We will enter this loop until we find
     * a window to warp to.
     */
    new_row = cur_row;
    new_col = cur_col;

    while (!got_it)
    {
	new_row += row_inc;
	new_col += col_inc;
	if (new_row < 0)
	    new_row = ip->cur_rows - 1;
	if (new_col < 0)
	    new_col = ip->cur_columns - 1;
	if (new_row >= ip->cur_rows)
	    new_row = 0;
	if (new_col >= ip->cur_columns)
	    new_col = 0;
	    
	/* Now let's go through the list to see if there is an entry with this
	 * new position
	 */
	for (tmp = ip->first; tmp != NULL; tmp = tmp->next)
	{
	    if (tmp->row == new_row && tmp->col == new_col)
	    {
		got_it = TRUE;
		break;
	    }
	}
    }

    if (!got_it)
    {
	fprintf (stderr, 
		 "%s:  unable to find window (%d, %d) in icon manager\n", 
		 ProgramName, new_row, new_col);
	return;
    }

    if (tmp == NULL)
      return;

    /* raise the frame so the icon manager is visible */
    if (ip->twm_win->mapped) {
	XRaiseWindow(dpy, ip->twm_win->frame);

/* djhjr - 5/30/00
	RaiseStickyAbove();
	RaiseAutoPan();

	XWarpPointer(dpy, None, tmp->icon, 0,0,0,0, 5, 5);
*/
	WarpInIconMgr(tmp, ip->twm_win);
    } else {
/* djhjr - 5/30/00
	if (tmp->twm->title_height) {
	    int tbx = Scr->TBInfo.titlex;
	    int x = tmp->twm->highlightx;
	    XWarpPointer (dpy, None, tmp->twm->title_w.win, 0, 0, 0, 0,
			  tbx + (x - tbx) / 2,
			  Scr->TitleHeight / 4);
	} else {
	    XWarpPointer (dpy, None, tmp->twm->w, 0, 0, 0, 0, 5, 5);
	}
*/
	RaiseStickyAbove(); /* DSE */
	RaiseAutoPan();

	WarpToWindow(tmp->twm);
    }
}

/***********************************************************************
 *
 *  Procedure:
 *	JumpIconManager - jump from one icon manager to another,
 *		possibly even on another screen
 *
 *  Inputs:
 *	dir	- one of the following:
 *			F_NEXTICONMGR	- go to the next icon manager 
 *			F_PREVICONMGR	- go to the previous one
 *
 ***********************************************************************
 */

void JumpIconManager(dir)
    register int dir;
{
    IconMgr *ip, *tmp_ip = NULL;
    int got_it = FALSE;
    ScreenInfo *sp;
    int screen;

    if (!Active) return;


#define ITER(i) (dir == F_NEXTICONMGR ? (i)->next : (i)->prev)
#define IPOFSP(sp) (dir == F_NEXTICONMGR ? &(sp->iconmgr) : sp->iconmgr.lasti)
#define TEST(ip) if ((ip)->count != 0 && (ip)->twm_win->mapped) \
		 { got_it = TRUE; break; }

    ip = Active->iconmgr;
    for (tmp_ip = ITER(ip); tmp_ip; tmp_ip = ITER(tmp_ip)) {
	TEST (tmp_ip);
    }

    if (!got_it) {
	int origscreen = ip->scr->screen;
	int inc = (dir == F_NEXTICONMGR ? 1 : -1);

	for (screen = origscreen + inc; ; screen += inc) {
	    if (screen >= NumScreens)
	      screen = 0;
	    else if (screen < 0)
	      screen = NumScreens - 1;

	    sp = ScreenList[screen];
	    if (sp) {
		for (tmp_ip = IPOFSP (sp); tmp_ip; tmp_ip = ITER(tmp_ip)) {
		    TEST (tmp_ip);
		}
	    }
	    if (got_it || screen == origscreen) break;
	}
    }

#undef ITER
#undef IPOFSP
#undef TEST

    if (!got_it) {
	DoAudible(); /* was 'XBell()' - djhjr - 6/22/01 */
	return;
    }

    /* raise the frame so it is visible */
    XRaiseWindow(dpy, tmp_ip->twm_win->frame);
    
/* djhjr - 5/30/00
    RaiseStickyAbove(); * DSE *
    RaiseAutoPan();
*/

    if (tmp_ip->active)
/* djhjr - 5/30/00
	XWarpPointer(dpy, None, tmp_ip->active->icon, 0,0,0,0, 5, 5);
*/
	WarpInIconMgr(tmp_ip->active, tmp_ip->twm_win);
    else
/* djhjr - 5/30/00
	XWarpPointer(dpy, None, tmp_ip->w, 0,0,0,0, 5, 5);
*/
    {
	RaiseStickyAbove(); /* DSE */
	RaiseAutoPan();

	WarpToWindow(tmp_ip->twm_win);
    }
}

/***********************************************************************
 *
 *  Procedure:
 *	AddIconManager - add a window to an icon manager
 *
 *  Inputs:
 *	tmp_win	- the TwmWindow structure
 *
 ***********************************************************************
 */

WList *AddIconManager(tmp_win)
    TwmWindow *tmp_win;
{
    WList *tmp;
    int h;
    unsigned long valuemask;		/* mask for create windows */
    XSetWindowAttributes attributes;	/* attributes for create windows */
    IconMgr *ip;

    tmp_win->list = NULL;

    /* djhjr - 10/2/01 */
    if (Scr->StrictIconManager)
    {
	if (tmp_win->icon || (!tmp_win->iconified &&
		(tmp_win->wmhints &&
		(tmp_win->wmhints->flags & StateHint) &&
		tmp_win->wmhints->initial_state == IconicState)))
	    ;
	else
	    return NULL;
    }

    if (tmp_win->iconmgr || tmp_win->transient || Scr->NoIconManagers)
	return NULL;

    if (LookInList(Scr->IconMgrNoShow, tmp_win->full_name, &tmp_win->class))
	return NULL;
    if (Scr->IconManagerDontShow &&
	!LookInList(Scr->IconMgrShow, tmp_win->full_name, &tmp_win->class))
	return NULL;
    if ((ip = (IconMgr *)LookInList(Scr->IconMgrs, tmp_win->full_name,
	    &tmp_win->class)) == NULL)
	ip = &Scr->iconmgr;

    tmp = (WList *) malloc(sizeof(WList));
    tmp->iconmgr = ip;
    tmp->next = NULL;
    tmp->active = FALSE;
    tmp->down = FALSE;

    InsertInIconManager(ip, tmp, tmp_win);

    tmp->twm = tmp_win;

/* djhjr - 4/19/96
    tmp->fore = Scr->IconManagerC.fore;
    tmp->back = Scr->IconManagerC.back;
*/
    tmp->cp.fore = Scr->IconManagerC.fore;
    tmp->cp.back = Scr->IconManagerC.back;

    tmp->highlight = Scr->IconManagerHighlight;

/* djhjr - 4/19/96
    GetColorFromList(Scr->IconManagerFL, tmp_win->full_name, &tmp_win->class,
	&tmp->fore);
    GetColorFromList(Scr->IconManagerBL, tmp_win->full_name, &tmp_win->class,
	&tmp->back);
*/
    GetColorFromList(Scr->IconManagerFL, tmp_win->full_name, &tmp_win->class,
	&tmp->cp.fore);
    GetColorFromList(Scr->IconManagerBL, tmp_win->full_name, &tmp_win->class,
	&tmp->cp.back);

    GetColorFromList(Scr->IconManagerHighlightL, tmp_win->full_name,
	&tmp_win->class, &tmp->highlight);

    /* djhjr - 4/19/96 */
	/* was 'Scr->use3Diconmanagers' - djhjr - 8/11/98 */
/* djhjr - 10/30/02
    if (Scr->IconMgrBevelWidth > 0)
*/
    {
	if (!Scr->BeNiceToColormap) GetShadeColors (&tmp->cp);
/* djhjr - 10/30/02
	tmp->iconifypm = Create3DIconManagerIcon (tmp->cp);
*/
	tmp->iconifypm = GetImage(Scr->iconMgrIconName,
				iconifybox_width, iconifybox_height,
				0, tmp->cp);
    }

    h = ComputeIconMgrWindowHeight(ip);

    ip->height = h * ip->count;
    tmp->me = ip->count;
    tmp->x = -1;
    tmp->y = -1;
    
    valuemask = (CWBackPixel | CWBorderPixel | CWEventMask | CWCursor);

/* djhjr - 4/19/96
    attributes.background_pixel = tmp->back;
    attributes.border_pixel = tmp->back;
*/
    attributes.background_pixel = tmp->cp.back;
    attributes.border_pixel = tmp->cp.back;

    attributes.event_mask = (KeyPressMask | ButtonPressMask |
			     ButtonReleaseMask | ExposureMask |
			     EnterWindowMask | LeaveWindowMask);
    attributes.cursor = Scr->IconMgrCursor;

	/* djhjr - 9/17/96 */
	if (Scr->BackingStore)
	{
		attributes.backing_store = WhenMapped;
		valuemask |= CWBackingStore;
	}

    tmp->w.win = XCreateWindow (dpy, ip->w, 0, 0, (unsigned int) 1, 
			    (unsigned int) h, (unsigned int) 0, 
			    CopyFromParent, (unsigned int) CopyFromParent,
			    (Visual *) CopyFromParent, valuemask, &attributes);

#ifdef TWM_USE_XFT
    if (Scr->use_xft > 0) {
	tmp->w.xft = MyXftDrawCreate (dpy, tmp->w.win, Scr->d_visual,
				XDefaultColormap (dpy, Scr->screen));
	CopyPixelToXftColor (XDefaultColormap (dpy, Scr->screen), 
				tmp->cp.fore, &tmp->cp.xft);
    }
#endif

    valuemask = (CWBackPixel | CWBorderPixel | CWEventMask | CWCursor);

/* djhjr - 4/19/96
    attributes.background_pixel = tmp->back;
*/
    attributes.background_pixel = tmp->cp.back;

    attributes.border_pixel = Scr->Black;
    attributes.event_mask = (ButtonReleaseMask| ButtonPressMask |
			     ExposureMask);
    attributes.cursor = Scr->ButtonCursor;

	/* djhjr - 5/5/98 */
	if (!iconmgr_iconx)
	{
		/* was 'Scr->use3Diconmanagers' - djhjr - 8/11/98 */
		if (Scr->IconMgrBevelWidth > 0)
			iconmgr_iconx = Scr->IconMgrBevelWidth + 5;
		else
			iconmgr_iconx = Scr->BorderWidth + 5;
		iconmgr_textx = iconmgr_iconx + siconify_width + 5;
	}

	/* 'iconmgr_iconx' was '5' - djhjr - 5/5/98 */
    tmp->icon = XCreateWindow (dpy, tmp->w.win, iconmgr_iconx, (int) (h - siconify_height)/2,
			       (unsigned int) siconify_width,
			       (unsigned int) siconify_height,
			       (unsigned int) 0, CopyFromParent,
			       (unsigned int) CopyFromParent,
			       (Visual *) CopyFromParent,
			       valuemask, &attributes);

    ip->count += 1;
    PackIconManager(ip);
    XMapWindow(dpy, tmp->w.win);

    XSaveContext(dpy, tmp->w.win, IconManagerContext, (caddr_t) tmp);
    XSaveContext(dpy, tmp->w.win, TwmContext, (caddr_t) tmp_win);
    XSaveContext(dpy, tmp->w.win, ScreenContext, (caddr_t) Scr);
    XSaveContext(dpy, tmp->icon, TwmContext, (caddr_t) tmp_win);
    XSaveContext(dpy, tmp->icon, ScreenContext, (caddr_t) Scr);
    tmp_win->list = tmp;

    if (!ip->twm_win->icon)
    {
	XMapWindow(dpy, ip->w);
	XMapWindow(dpy, ip->twm_win->frame);
    }

	/* djhjr - 9/21/99 */
	else
		XMapWindow(dpy, ip->twm_win->icon_w.win);

    return (tmp);
}

/***********************************************************************
 *
 *  Procedure:
 *	InsertInIconManager - put an allocated entry into an icon 
 *		manager
 *
 *  Inputs:
 *	ip	- the icon manager pointer
 *	tmp	- the entry to insert
 *
 ***********************************************************************
 */

void InsertInIconManager(ip, tmp, tmp_win)
    IconMgr *ip;
    WList *tmp;
    TwmWindow *tmp_win;
{
    WList *tmp1;
    int added;
    int (*compar)() = (Scr->CaseSensitive ? strcmp : XmuCompareISOLatin1);

    added = FALSE;
    if (ip->first == NULL)
    {
	ip->first = tmp;
	tmp->prev = NULL;
	ip->last = tmp;
	added = TRUE;
    }
    else if (Scr->SortIconMgr)
    {
	for (tmp1 = ip->first; tmp1 != NULL; tmp1 = tmp1->next)
	{
	    if ((*compar)(tmp_win->icon_name, tmp1->twm->icon_name) < 0)
	    {
		tmp->next = tmp1;
		tmp->prev = tmp1->prev;
		tmp1->prev = tmp;
		if (tmp->prev == NULL)
		    ip->first = tmp;
		else
		    tmp->prev->next = tmp;
		added = TRUE;
		break;
	    }
	}
    }

    if (!added)
    {
	ip->last->next = tmp;
	tmp->prev = ip->last;
	ip->last = tmp;
    }
}

void RemoveFromIconManager(ip, tmp)
    IconMgr *ip;
    WList *tmp;
{
    if (tmp->prev == NULL)
	ip->first = tmp->next;
    else
	tmp->prev->next = tmp->next;

    if (tmp->next == NULL)
	ip->last = tmp->prev;
    else
	tmp->next->prev = tmp->prev;
}

/***********************************************************************
 *
 *  Procedure:
 *	RemoveIconManager - remove a window from the icon manager
 *
 *  Inputs:
 *	tmp_win	- the TwmWindow structure
 *
 ***********************************************************************
 */

void RemoveIconManager(tmp_win)
    TwmWindow *tmp_win;
{
    IconMgr *ip;
    WList *tmp;

    if (tmp_win->list == NULL)
	return;

    tmp = tmp_win->list;

    /* submitted by Jonathan Paisley - 11/11/02 */
    if (Active == tmp)
	Active = NULL;

	/*
	 * Believe it or not, the kludge in events.c:HandleKeyPress() needs
	 * this, or a window that's been destroyed still registers there,
	 * even though the whole mess gets freed in just a few microseconds!
	 *
	 * djhjr - 6/5/98
	 */
	/*
	 * Somehwere alone the line, whatever it was got fixed, and this is
	 * needed again - djhjr - 5/27/03
	 */
/*#ifdef NEVER*/ /* warps to icon managers uniquely handled in menus.c:WarpToWindow() */
	tmp->active = FALSE;
	tmp->iconmgr->active = NULL;
/*#endif*/

    tmp_win->list = NULL;
    ip = tmp->iconmgr;

    RemoveFromIconManager(ip, tmp);
    
    XDeleteContext(dpy, tmp->icon, TwmContext);
    XDeleteContext(dpy, tmp->icon, ScreenContext);
    XDestroyWindow(dpy, tmp->icon);
#ifdef TWM_USE_XFT
    if (Scr->use_xft > 0)
	MyXftDrawDestroy(tmp->w.xft);
#endif
    XDeleteContext(dpy, tmp->w.win, IconManagerContext);
    XDeleteContext(dpy, tmp->w.win, TwmContext);
    XDeleteContext(dpy, tmp->w.win, ScreenContext);
    XDestroyWindow(dpy, tmp->w.win);
    ip->count -= 1;

#ifdef NEVER /* can't do this, else we lose the button entirely! */
	/* about damn time I did this! - djhjr - 6/5/98 */
	XFreePixmap(dpy, tmp->iconifypm);
#endif

    free((char *) tmp);

    PackIconManager(ip);

    if (ip->count == 0)
    {
	/* djhjr - 9/21/99 */
	if (ip->twm_win->icon)
		XUnmapWindow(dpy, ip->twm_win->icon_w.win);
	else

	XUnmapWindow(dpy, ip->twm_win->frame);
    }

}

void ActiveIconManager(active)
    WList *active;
{
    active->active = TRUE;
    Active = active;
    Active->iconmgr->active = active;

/* djhjr - 4/19/96
    DrawIconManagerBorder(active);
*/
    DrawIconManagerBorder(active, False);
}

void NotActiveIconManager(active)
    WList *active;
{
    active->active = FALSE;

/* djhjr - 4/19/96
    DrawIconManagerBorder(active);
*/
    DrawIconManagerBorder(active, False);
}

/* djhjr - 4/19/96
void DrawIconManagerBorder(tmp)
    WList *tmp;
*/
void DrawIconManagerBorder(tmp, fill)
    WList *tmp;
    int fill;
{
	/* was 'Scr->use3Diconmanagers' - djhjr - 8/11/98 */
    if (Scr->IconMgrBevelWidth > 0) {
	int shadow_width;

/* djhjr - 4/28/98
	shadow_width = 2;
*/
	shadow_width = Scr->IconMgrBevelWidth;

/* djhjr - 1/27/98
	if (tmp->active && Scr->Highlight)
*/
	if (tmp->active && Scr->IconMgrHighlight)
	    Draw3DBorder (tmp->w.win, 0, 0, tmp->width, tmp->height, shadow_width,
				tmp->cp, on, fill, False);
	else
	    Draw3DBorder (tmp->w.win, 0, 0, tmp->width, tmp->height, shadow_width,
				tmp->cp, off, fill, False);
    }
    else {
/*
	XSetForeground(dpy, Scr->NormalGC, tmp->fore);
*/
	XSetForeground(dpy, Scr->NormalGC, tmp->cp.fore);
	    XDrawRectangle(dpy, tmp->w.win, Scr->NormalGC, 2, 2,
		tmp->width-5, tmp->height-5);

/* djhjr - 1/27/98
	if (tmp->active && Scr->Highlight)
*/
	if (tmp->active && Scr->IconMgrHighlight)
	    XSetForeground(dpy, Scr->NormalGC, tmp->highlight);
	else
/*
	    XSetForeground(dpy, Scr->NormalGC, tmp->back);
*/
	    XSetForeground(dpy, Scr->NormalGC, tmp->cp.back);

	XDrawRectangle(dpy, tmp->w.win, Scr->NormalGC, 0, 0,
	    tmp->width-1, tmp->height-1);
	XDrawRectangle(dpy, tmp->w.win, Scr->NormalGC, 1, 1,
	    tmp->width-3, tmp->height-3);
    }
}

/***********************************************************************
 *
 *  Procedure:
 *	SortIconManager - sort the dude
 *
 *  Inputs:
 *	ip	- a pointer to the icon manager struture
 *
 ***********************************************************************
 */

void SortIconManager(ip)
    IconMgr *ip;
{
    WList *tmp1, *tmp2;
    int done;
    int (*compar)() = (Scr->CaseSensitive ? strcmp : XmuCompareISOLatin1);

    if (ip == NULL)
	ip = Active->iconmgr;

    done = FALSE;
    do
    {
	for (tmp1 = ip->first; tmp1 != NULL; tmp1 = tmp1->next)
	{
	    if ((tmp2 = tmp1->next) == NULL)
	    {
		done = TRUE;
		break;
	    }
	    if ((*compar)(tmp1->twm->icon_name, tmp2->twm->icon_name) > 0)
	    {
		/* take it out and put it back in */
		RemoveFromIconManager(ip, tmp2);
		InsertInIconManager(ip, tmp2, tmp2->twm);
		break;
	    }
	}
    }
    while (!done);
    PackIconManager(ip);
}

/***********************************************************************
 *
 *  Procedure:
 *	PackIconManager - pack the icon manager windows following
 *		an addition or deletion
 *
 *  Inputs:
 *	ip	- a pointer to the icon manager struture
 *
 ***********************************************************************
 */

void PackIconManager(ip)
    IconMgr *ip;
{
    int newwidth, i, row, col, maxcol,  colinc, rowinc, wheight, wwidth;
    int new_x, new_y;
    int savewidth;
    WList *tmp;

    wheight = ComputeIconMgrWindowHeight(ip);

    wwidth = ip->width / ip->columns;

    rowinc = wheight;
    colinc = wwidth;

    row = 0;
    col = ip->columns;
    maxcol = 0;
    for (i = 0, tmp = ip->first; tmp != NULL; i++, tmp = tmp->next)
    {
	tmp->me = i;
	if (++col >= ip->columns)
	{
	    col = 0;
	    row += 1;
	}
	if (col > maxcol)
	    maxcol = col;

	new_x = col * colinc;
	new_y = (row-1) * rowinc;

	/* if the position or size has not changed, don't touch it */
	if (tmp->x != new_x || tmp->y != new_y ||
	    tmp->width != wwidth || tmp->height != wheight)
	{
	    XMoveResizeWindow(dpy, tmp->w.win, new_x, new_y, wwidth, wheight);

	    tmp->row = row-1;
	    tmp->col = col;
	    tmp->x = new_x;
	    tmp->y = new_y;
	    tmp->width = wwidth;
	    tmp->height = wheight;
	}
    }
    maxcol += 1;

    ip->cur_rows = row;
    ip->cur_columns = maxcol;
    ip->height = row * rowinc;
    if (ip->height == 0)
    	ip->height = rowinc;
    newwidth = maxcol * colinc;
    if (newwidth == 0)
	newwidth = colinc;

    XResizeWindow(dpy, ip->w, newwidth, ip->height);

    savewidth = ip->width;
    if (ip->twm_win)
    {

      /* limit the min and max sizes of an icon manager - djhjr - 3/1/99 */
      ip->twm_win->hints.flags |= (PMinSize | PMaxSize);
      ip->twm_win->hints.min_width = maxcol * (2 * iconmgr_iconx + siconify_width);
      ip->twm_win->hints.min_height = ip->height;
      ip->twm_win->hints.max_width = Scr->MyDisplayWidth;
      ip->twm_win->hints.max_height = ip->height;

/* djhjr - 4/19/96     
      SetupWindow (ip->twm_win,
		   ip->twm_win->frame_x, ip->twm_win->frame_y,
		   newwidth, ip->height + ip->twm_win->title_height, -1);
*/
      SetupWindow (ip->twm_win,
            ip->twm_win->frame_x, ip->twm_win->frame_y,
            newwidth + 2 * ip->twm_win->frame_bw3D,
            ip->height + ip->twm_win->title_height + 2 * ip->twm_win->frame_bw3D, -1);
    }

    ip->width = savewidth;
}

/*
 * ComputeIconMgrWindowHeight()
 * scale the icon manager window height to the font used
 */
static int ComputeIconMgrWindowHeight (IconMgr *ip)
{
    int h;

    if (Scr->IconMgrBevelWidth > 0)
    {
#ifdef TWM_USE_SPACING
	if (ip->columns > 1) /* 100*pow(sqrt(1.2), i) for i = 2, 3, 4 */
	    h = 144*Scr->IconManagerFont.height/100; /* i = 4 multicolumn*/
	else
	    h = 120*Scr->IconManagerFont.height/100; /* i = 2 unicolumn*/
	h += 2 * Scr->IconMgrBevelWidth;
#else
	h = Scr->IconManagerFont.height + 2 * Scr->IconMgrBevelWidth + 4;
#endif
	if (h < (siconify_height + 2 * Scr->IconMgrBevelWidth + 4))
		h = siconify_height + 2 * Scr->IconMgrBevelWidth + 4;
    }
    else
    {
#ifdef TWM_USE_SPACING
	if (ip->columns > 1)
	    h = 144*Scr->IconManagerFont.height/100;
	else
	    h = 120*Scr->IconManagerFont.height/100;
	h += 4; /* highlighted border */
#else
	h = Scr->IconManagerFont.height + 10;
#endif
	if (h < (siconify_height + 4))
	    h = siconify_height + 4;
	}

    /* make height be odd so buttons look nice and centered */
    if (!(h & 1)) h++;

    return (h);
}

