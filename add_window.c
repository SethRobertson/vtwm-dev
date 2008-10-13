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


/**********************************************************************
 *
 * $XConsortium: add_window.c,v 1.153 91/07/10 13:17:26 dave Exp $
 *
 * Add a new window, put the titlbar and other stuff around
 * the window
 *
 * 31-Mar-88 Tom LaStrange        Initial Version.
 *
 **********************************************************************/

#include <stdio.h>
#include <string.h>
#include "twm.h"
#include <X11/Xatom.h>
#ifndef NO_XPM_SUPPORT
#include <X11/xpm.h>
#endif

#include "add_window.h"
#include "image_formats.h"
#include "util.h"
#include "resize.h"
#include "parse.h"

#include "gram.h"

#include "list.h"
#include "events.h"
#include "menus.h"
#include "screen.h"
#include "iconmgr.h"
#include "desktop.h"
#include "prototypes.h"

/* random placement coordinates */
#define PLACEMENT_START		50
#define PLACEMENT_INCR		30

static void AddMoveAndResize (TwmWindow *tmp_win, int ask_user);
static void CreateWindowTitlebarButtons (TwmWindow *tmp_win);


#define gray_width 2
#define gray_height 2
static char gray_bits[] = {
   0x02, 0x01};

static unsigned char black_bits[] = {
   0xFF, 0xFF};

int AddingX;
int AddingY;
int AddingW;
int AddingH;


char NoName[] = "Untitled"; /* name if no name is specified */

typedef struct _PlaceXY {
    struct _PlaceXY *next;
    int x, y, width, height;
} PlaceXY;


/************************************************************************
 *
 *  Procedure:
 *	GetGravityOffsets - map gravity to (x,y) offset signs for adding
 *		to x and y when window is mapped to get proper placement.
 *
 ************************************************************************
 */

void 
GetGravityOffsets (
    TwmWindow *tmp,			/* window from which to get gravity */
    int *xp,
    int *yp			/* return values */
)
{
    static struct _gravity_offset {
	int x, y;
    } gravity_offsets[11] = {
	{  0,  0 },			/* ForgetGravity */
	{ -1, -1 },			/* NorthWestGravity */
	{  0, -1 },			/* NorthGravity */
	{  1, -1 },			/* NorthEastGravity */
	{ -1,  0 },			/* WestGravity */
	{  0,  0 },			/* CenterGravity */
	{  1,  0 },			/* EastGravity */
	{ -1,  1 },			/* SouthWestGravity */
	{  0,  1 },			/* SouthGravity */
	{  1,  1 },			/* SouthEastGravity */
	{  0,  0 },			/* StaticGravity */
    };
    register int g = ((tmp->hints.flags & PWinGravity)
		      ? tmp->hints.win_gravity : NorthWestGravity);

    if (g < ForgetGravity || g > StaticGravity) {
	*xp = *yp = 0;
    } else {
	*xp = gravity_offsets[g].x;
	*yp = gravity_offsets[g].y;
    }
}


/***********************************************************************
 *
 *  Procedure:
 *	AddWindow - add a new window to the twm list
 *
 *  Returned Value:
 *	(TwmWindow *) - pointer to the TwmWindow structure
 *
 *  Inputs:
 *	w	- the window id of the window to add
 *	iconm	- flag to tell if this is an icon manager window
 *	iconp	- pointer to icon manager struct
 *
 ***********************************************************************
 */

TwmWindow *
AddWindow (Window w, int iconm, IconMgr *iconp)
{
    TwmWindow *tmp_win;			/* new twm window structure */
    TwmWindow **ptmp_win;
    unsigned long valuemask;		/* mask for create windows */
    XSetWindowAttributes attributes;	/* attributes for create windows */
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytesafter;
    int ask_user;		/* don't know where to put the window */
    int *ppos_ptr, ppos_on;
    int gravx, gravy;			/* gravity signs for positioning */
    int namelen;
    int bw2;
    char *icon_name;
    char *name;

    Atom motifhints = XInternAtom( dpy, "_MOTIF_WM_HINTS", 0);
    MotifWmHints *mwmhints;

#ifdef DEBUG
    fprintf(stderr, "AddWindow: w = 0x%x\n", w);
#endif

    /* allocate space for the twm window */
    tmp_win = (TwmWindow *)calloc(1, sizeof(TwmWindow));
    if (tmp_win == 0)
    {
	fprintf (stderr, "%s: Unable to allocate memory to manage window ID %lx.\n",
		 ProgramName, w);
	return NULL;
    }
    tmp_win->w = w;
    tmp_win->zoomed = ZOOM_NONE;
    tmp_win->iconmgr = iconm;
    tmp_win->iconmgrp = iconp;
    tmp_win->cmaps.number_cwins = 0;

    XSelectInput(dpy, tmp_win->w, PropertyChangeMask);
    XGetWindowAttributes(dpy, tmp_win->w, &tmp_win->attr);

    if (I18N_FetchName(dpy, tmp_win->w, &name))
    {
	tmp_win->name = strdup(name);
	free(name);
    }
    else
	tmp_win->name = NoName;

    tmp_win->class = NoClass;
    XGetClassHint(dpy, tmp_win->w, &tmp_win->class);
    FetchWmProtocols (tmp_win);
    FetchWmColormapWindows (tmp_win);

    /*
     * do initial clip; should look at window gravity
     */
    if (tmp_win->attr.width > Scr->MaxWindowWidth)
      tmp_win->attr.width = Scr->MaxWindowWidth;
    if (tmp_win->attr.height > Scr->MaxWindowHeight)
      tmp_win->attr.height = Scr->MaxWindowHeight;

    tmp_win->wmhints = XGetWMHints(dpy, tmp_win->w);
    if (tmp_win->wmhints && (tmp_win->wmhints->flags & WindowGroupHint))
      tmp_win->group = tmp_win->wmhints->window_group;
    else
	tmp_win->group = tmp_win->w;

    tmp_win->mwmhints.flags = 0;
    if (motifhints != None)
    {
	if (XGetWindowProperty(dpy, tmp_win->w, motifhints,
			       0L, sizeof(MotifWmHints), False,
			       motifhints, &actual_type, &actual_format,
			       &nitems, &bytesafter,
			       (unsigned char**)&mwmhints) == Success && actual_type != None)
	{
	    if (mwmhints)
	    {
		tmp_win->mwmhints = *mwmhints;
		XFree(mwmhints);
	    }
	}
    }

    /*
     * The July 27, 1988 draft of the ICCCM ignores the size and position
     * fields in the WM_NORMAL_HINTS property.
     */

    tmp_win->transient = Transient(tmp_win->w, &tmp_win->transientfor);

    if (tmp_win->class.res_name == NULL)
	tmp_win->class.res_name = NoName;
    if (tmp_win->class.res_class == NULL)
	tmp_win->class.res_class = NoName;

    tmp_win->full_name = tmp_win->name;
    namelen = strlen (tmp_win->name);

	tmp_win->highlight = Scr->Highlight &&
		(LookInList(Scr->NoHighlight, tmp_win->full_name,
			&tmp_win->class) == (char *)NULL);

	tmp_win->stackmode = Scr->StackMode &&
		(LookInList(Scr->NoStackModeL, tmp_win->full_name,
			&tmp_win->class) == (char *)NULL);

	tmp_win->titlehighlight = Scr->TitleHighlight &&
		(LookInList(Scr->NoTitleHighlight, tmp_win->full_name,
			&tmp_win->class) == (char *)NULL);

	tmp_win->auto_raise = Scr->AutoRaiseDefault ||
		(LookInList(Scr->AutoRaise, tmp_win->full_name,
			&tmp_win->class) != (char *)NULL);
    if (tmp_win->auto_raise) Scr->NumAutoRaises++;

	tmp_win->iconify_by_unmapping = Scr->IconifyByUnmapping;
	if (Scr->IconifyByUnmapping)
	{
	tmp_win->iconify_by_unmapping =
		(LookInList(Scr->DontIconify, tmp_win->full_name,
			&tmp_win->class) == (char *)NULL);
	}
	tmp_win->iconify_by_unmapping |=
		(LookInList(Scr->IconifyByUn, tmp_win->full_name,
			&tmp_win->class) != (char *)NULL);

    if ((Scr->UseWindowRing ||
		LookInList(Scr->WindowRingL, tmp_win->full_name,
			   &tmp_win->class)) &&
		LookInList(Scr->NoWindowRingL, tmp_win->full_name,
			     &tmp_win->class) == (char *)NULL)
    {
	AddWindowToRing(tmp_win);
    }
    else
      tmp_win->ring.next = tmp_win->ring.prev = NULL;

    if (LookInList(Scr->NailedDown, tmp_win->full_name, &tmp_win->class))
	    tmp_win->nailed = TRUE;
    else
	    tmp_win->nailed = FALSE;

    if (LookInList(Scr->DontShowInDisplay, tmp_win->full_name, &tmp_win->class))
	    tmp_win->showindesktopdisplay = FALSE;
    else
	    tmp_win->showindesktopdisplay = TRUE;

    tmp_win->squeeze_info = NULL;
    /*
     * get the squeeze information; note that this does not have to be freed
     * since it is coming from the screen list
     */
    if (HasShape) {
	if (!LookInList (Scr->DontSqueezeTitleL, tmp_win->full_name,
			 &tmp_win->class)) {
	    tmp_win->squeeze_info = (SqueezeInfo *)
	      LookInList (Scr->SqueezeTitleL, tmp_win->full_name,
			  &tmp_win->class);
	    if (!tmp_win->squeeze_info) {
		static SqueezeInfo default_squeeze = { J_LEFT, 0, 0 };
		if (Scr->SqueezeTitle)
		  tmp_win->squeeze_info = &default_squeeze;
	    }
	}
      }

    tmp_win->old_bw = tmp_win->attr.border_width;

    if (Scr->NoBorders || LookInList(Scr->NoBorder, tmp_win->full_name, &tmp_win->class))
    {
		tmp_win->frame_bw = 0;
		tmp_win->frame_bw3D = 0;
    }
    else
	{
		if (Scr->BorderBevelWidth > 0)
		{
			tmp_win->frame_bw3D = Scr->BorderWidth;
			tmp_win->frame_bw = 0;
			Scr->ClientBorderWidth = FALSE;
		}
		else
		{
			tmp_win->frame_bw3D = 0;
			if (Scr->ClientBorderWidth)
				tmp_win->frame_bw = tmp_win->old_bw;
			else
				tmp_win->frame_bw = Scr->BorderWidth;
		}
	}

    if (tmp_win->mwmhints.flags & MWM_HINTS_DECORATIONS)
    {
	if (tmp_win->mwmhints.decorations & MWM_DECOR_ALL)
	    tmp_win->mwmhints.decorations |= (MWM_DECOR_BORDER |
			MWM_DECOR_RESIZEH | MWM_DECOR_TITLE |
			MWM_DECOR_MENU | MWM_DECOR_MINIMIZE |
			MWM_DECOR_MAXIMIZE);

	if (!(tmp_win->mwmhints.decorations & MWM_DECOR_BORDER))
	    tmp_win->frame_bw = tmp_win->frame_bw3D = 0;
    }
    if (tmp_win->mwmhints.flags & MWM_HINTS_FUNCTIONS)
	if (tmp_win->mwmhints.functions & MWM_FUNC_ALL)
	    tmp_win->mwmhints.functions |= (MWM_FUNC_RESIZE |
			MWM_FUNC_MOVE | MWM_FUNC_MINIMIZE |
			MWM_FUNC_MAXIMIZE | MWM_FUNC_CLOSE);

    bw2 = tmp_win->frame_bw * 2;

    tmp_win->title_height = Scr->TitleHeight + tmp_win->frame_bw;

    if (tmp_win->mwmhints.flags & MWM_HINTS_DECORATIONS &&
		!(tmp_win->mwmhints.decorations & MWM_DECOR_TITLE))
	tmp_win->title_height = 0;

    if (Scr->NoTitlebar)
	tmp_win->title_height = 0;
    if (LookInList(Scr->NoTitle, tmp_win->full_name, &tmp_win->class))
	tmp_win->title_height = 0;
    if (LookInList(Scr->MakeTitle, tmp_win->full_name, &tmp_win->class))
	tmp_win->title_height = Scr->TitleHeight + tmp_win->frame_bw;

	if (LookInList(Scr->OpaqueMoveL, tmp_win->full_name, &tmp_win->class))
		tmp_win->opaque_move = TRUE;
	else
	    tmp_win->opaque_move = Scr->OpaqueMove &&
		(LookInList(Scr->NoOpaqueMoveL, tmp_win->full_name,
			&tmp_win->class) == (char *)NULL);

	if (tmp_win->opaque_move) tmp_win->attr.save_under = True;

	if (LookInList(Scr->OpaqueResizeL, tmp_win->full_name, &tmp_win->class))
		tmp_win->opaque_resize = TRUE;
	else
	    tmp_win->opaque_resize = Scr->OpaqueResize &&
		(LookInList(Scr->NoOpaqueResizeL, tmp_win->full_name,
			&tmp_win->class) == (char *)NULL);

	if (tmp_win->opaque_resize) tmp_win->attr.save_under = True;

    /* if it is a transient window, don't put a title on it */
    if (tmp_win->transient && !Scr->DecorateTransients)
	tmp_win->title_height = 0;

    if (LookInList(Scr->StartIconified, tmp_win->full_name, &tmp_win->class))
    {
	if (!tmp_win->wmhints)
	{
	    tmp_win->wmhints = (XWMHints *)malloc(sizeof(XWMHints));
	    tmp_win->wmhints->flags = 0;
	}
	tmp_win->wmhints->initial_state = IconicState;
	tmp_win->wmhints->flags |= StateHint;
    }

    GetWindowSizeHints (tmp_win);
    GetGravityOffsets (tmp_win, &gravx, &gravy);

    /*
     * Don't bother user if:
     *
     *     o  the window is a transient, or
     *
     *     o  a USPosition was requested, or
     *
     *     o  a PPosition was requested and UsePPosition is ON or
     *        NON_ZERO if the window is at other than (0,0)
     */

    if ((ppos_ptr = (int *)LookInList(Scr->UsePPositionL,
				      tmp_win->full_name, &tmp_win->class)))
	ppos_on = *ppos_ptr;
    else
	ppos_on = Scr->UsePPosition;

    if (ppos_on == PPOS_NON_ZERO &&
		(tmp_win->attr.x != 0 || tmp_win->attr.y != 0))
	ppos_on = PPOS_ON;

    ask_user = TRUE;

    if (tmp_win->transient ||
		(tmp_win->hints.flags & USPosition) ||
		((tmp_win->hints.flags & PPosition) && ppos_on == PPOS_ON))
	ask_user = FALSE;

    /* check for applet regions - djhjr - 4/26/99 */
    if (PlaceApplet(tmp_win, tmp_win->attr.x, tmp_win->attr.y,
		    &tmp_win->attr.x, &tmp_win->attr.y))
	ask_user = FALSE;

    /* If we want the window to appear on-screen and it is off, ask the user/use random */
    if (ppos_on == PPOS_ON_SCREEN &&
	(tmp_win->attr.x < Scr->VirtualDesktopX ||
	 tmp_win->attr.x >= Scr->VirtualDesktopX + Scr->MyDisplayWidth ||
	 tmp_win->attr.y < Scr->VirtualDesktopY ||
	 tmp_win->attr.y >= Scr->VirtualDesktopY + Scr->MyDisplayHeight))
	ask_user = TRUE;

    if (LookInList(Scr->NailedDown, tmp_win->full_name, &tmp_win->class))
	    tmp_win->nailed = TRUE;
    else
	    tmp_win->nailed = FALSE;

    /*
     * 25/09/90 - Nailed windows should always be on the real screen,
     * regardless of PPosition or UPosition. If we are dealing with
     * PPosition, then offset by the current real screen offset on the vd.
     */
    if (tmp_win->nailed ||
	((tmp_win->hints.flags & PPosition) && (ask_user == FALSE))) {
	    tmp_win->attr.x = R_TO_V_X(tmp_win->attr.x);
	    tmp_win->attr.y = R_TO_V_Y(tmp_win->attr.y);
    }


    if (!Scr->ClientBorderWidth) {	/* need to adjust for twm borders */

	int delta = -(tmp_win->frame_bw + tmp_win->frame_bw3D);

	tmp_win->attr.x += gravx * delta;
	tmp_win->attr.y += gravy * delta;
    }

    /*
     * For windows with specified non-northwest gravities.
     */
    if (tmp_win->old_bw) {
	if (!Scr->ClientBorderWidth) {
	    JunkX = gravx + 1;
	    JunkY = gravy + 1;
	} else
	    JunkX = JunkY = 1;

	tmp_win->attr.x += JunkX * tmp_win->old_bw;
	tmp_win->attr.y += JunkY * tmp_win->old_bw;
    }

    tmp_win->title_width = tmp_win->attr.width;

    if (tmp_win->old_bw) XSetWindowBorderWidth (dpy, tmp_win->w, 0);

    tmp_win->name_width = MyFont_TextWidth(&Scr->TitleBarFont,
				     tmp_win->name, namelen);

    if (I18N_GetIconName(dpy, tmp_win->w, &icon_name))
    {
	tmp_win->icon_name = strdup(icon_name);
	free(icon_name);
    }
    else
	tmp_win->icon_name = strdup(tmp_win->name);


    tmp_win->iconified = FALSE;
    tmp_win->icon = FALSE;
    tmp_win->icon_on = FALSE;

    XGrabServer(dpy);

    /*
     * Make sure the client window still exists.  We don't want to leave an
     * orphan frame window if it doesn't.  Since we now have the server
     * grabbed, the window can't disappear later without having been
     * reparented, so we'll get a DestroyNotify for it.  We won't have
     * gotten one for anything up to here, however.
     */
    if (XGetGeometry(dpy, tmp_win->w, &JunkRoot, &JunkX, &JunkY,
		     &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth) == 0)
    {
	free((char *)tmp_win);
	XUngrabServer(dpy);
	return(NULL);
    }

    /* _append_ the window into the twm list (simplify 'group leader' search) */
    tmp_win->next = NULL;
    tmp_win->prev = &Scr->TwmRoot;
    ptmp_win = &Scr->TwmRoot.next;
    while (*ptmp_win) {
	tmp_win->prev = (*ptmp_win);
	ptmp_win = &((*ptmp_win)->next);
    }
    (*ptmp_win) = tmp_win;

    /* get all the colors for the window */

	tmp_win->border.back = Scr->BorderColor;

    tmp_win->icon_border = Scr->IconBorderColor;
    tmp_win->border_tile.fore = Scr->BorderTileC.fore;
    tmp_win->border_tile.back = Scr->BorderTileC.back;
    tmp_win->title.fore = Scr->TitleC.fore;
    tmp_win->title.back = Scr->TitleC.back;
    tmp_win->iconc.fore = Scr->IconC.fore;
    tmp_win->iconc.back = Scr->IconC.back;
    tmp_win->virtual.fore = Scr->VirtualDesktopDisplayC.fore;
    tmp_win->virtual.back = Scr->VirtualDesktopDisplayC.back;

    GetColorFromList(Scr->BorderColorL, tmp_win->full_name, &tmp_win->class,
	&tmp_win->border.back);

    GetColorFromList(Scr->IconBorderColorL, tmp_win->full_name, &tmp_win->class,
	&tmp_win->icon_border);
    GetColorFromList(Scr->BorderTileForegroundL, tmp_win->full_name,
	&tmp_win->class, &tmp_win->border_tile.fore);
    GetColorFromList(Scr->BorderTileBackgroundL, tmp_win->full_name,
	&tmp_win->class, &tmp_win->border_tile.back);
    GetColorFromList(Scr->TitleForegroundL, tmp_win->full_name, &tmp_win->class,
	&tmp_win->title.fore);
    GetColorFromList(Scr->TitleBackgroundL, tmp_win->full_name, &tmp_win->class,
	&tmp_win->title.back);
    GetColorFromList(Scr->IconForegroundL, tmp_win->full_name, &tmp_win->class,
	&tmp_win->iconc.fore);
    GetColorFromList(Scr->IconBackgroundL, tmp_win->full_name, &tmp_win->class,
	&tmp_win->iconc.back);

/* fixed transposed fallback fore and back color lists - djhjr - 9/25/01 */
    if (!GetColorFromList(Scr->VirtualDesktopColorFL, tmp_win->full_name,
		     &tmp_win->class, &tmp_win->virtual.fore))
	    GetColorFromList(Scr->TitleForegroundL, tmp_win->full_name,
			     &tmp_win->class, &tmp_win->virtual.fore);
    if (!GetColorFromList(Scr->VirtualDesktopColorBL, tmp_win->full_name,
		     &tmp_win->class, &tmp_win->virtual.back))
	    GetColorFromList(Scr->TitleBackgroundL, tmp_win->full_name,
			     &tmp_win->class, &tmp_win->virtual.back);

	if (!Scr->BeNiceToColormap)
		GetShadeColors (&tmp_win->title);
	if ((Scr->ButtonColorIsFrame || Scr->BorderBevelWidth > 0) && !Scr->BeNiceToColormap)
	{
		GetShadeColors (&tmp_win->border);
		GetShadeColors (&tmp_win->border_tile);
    }

#ifdef TWM_USE_XFT
    if (Scr->use_xft > 0) {
	CopyPixelToXftColor (tmp_win->title.fore, &tmp_win->title.xft);
	CopyPixelToXftColor (tmp_win->iconc.fore, &tmp_win->iconc.xft);
	CopyPixelToXftColor (tmp_win->virtual.fore, &tmp_win->virtual.xft);
    }
#endif

    /* create windows */

    tmp_win->frame_width = tmp_win->attr.width + 2 * tmp_win->frame_bw3D;
    tmp_win->frame_height = tmp_win->attr.height + tmp_win->title_height +
				2 * tmp_win->frame_bw3D;
    valuemask = CWBorderPixel | CWCursor | CWEventMask | CWBackPixel;

    attributes.background_pixel	 = tmp_win->border.back;
    attributes.border_pixel = tmp_win->border.back;
    attributes.cursor = Scr->FrameCursor;
    attributes.event_mask = (SubstructureRedirectMask |
			     ButtonPressMask | ButtonReleaseMask |
			     EnterWindowMask | LeaveWindowMask | ExposureMask);
    if (tmp_win->attr.save_under) {
	attributes.save_under = True;
	valuemask |= CWSaveUnder;
    }

    if (tmp_win->hints.flags & PWinGravity) {
	attributes.win_gravity = tmp_win->hints.win_gravity;
	valuemask |= CWWinGravity;
    }

    tmp_win->frame = XCreateWindow (dpy, Scr->Root, tmp_win->frame_x,
				    tmp_win->frame_y,
				    (unsigned int) tmp_win->frame_width,
				    (unsigned int) tmp_win->frame_height,
				    (unsigned int) tmp_win->frame_bw,
				    Scr->d_depth,
				    (unsigned int) CopyFromParent,
				    Scr->d_visual, valuemask, &attributes);
#ifdef TWM_USE_OPACITY
    PropagateWindowOpacity (tmp_win);
#endif

    if (tmp_win->title_height)
    {
	valuemask = (CWEventMask | CWBorderPixel | CWBackPixel);
	attributes.event_mask = (KeyPressMask | ButtonPressMask |
				 ButtonReleaseMask | ExposureMask);

	attributes.border_pixel = tmp_win->title.back;
	attributes.background_pixel = tmp_win->title.back;

	if (Scr->BackingStore)
	{
		attributes.backing_store = WhenMapped;
		valuemask |= CWBackingStore;
	}

	tmp_win->title_w.win = XCreateWindow (dpy, tmp_win->frame,
					  tmp_win->frame_bw3D - tmp_win->frame_bw,
					  tmp_win->frame_bw3D - tmp_win->frame_bw,

					  (unsigned int) tmp_win->attr.width,
					  (unsigned int) Scr->TitleHeight,
					  (unsigned int) tmp_win->frame_bw,
					  Scr->d_depth,
					  (unsigned int) CopyFromParent,
					  Scr->d_visual, valuemask,
					  &attributes);
#ifdef TWM_USE_XFT
	if (Scr->use_xft > 0)
	    tmp_win->title_w.xft = MyXftDrawCreate (tmp_win->title_w.win);
#endif
    }
    else {
	tmp_win->title_w.win = 0;
	tmp_win->squeeze_info = NULL;
    }

    if (tmp_win->highlight)
    {

	if (Scr->TitleBevelWidth > 0 && (Scr->Monochrome != COLOR))
	    tmp_win->gray = XCreatePixmapFromBitmapData(dpy, Scr->Root,
		(char *)black_bits, gray_width, gray_height,
		tmp_win->border_tile.fore, tmp_win->border_tile.back,
		Scr->d_depth);
	else

	tmp_win->gray = XCreatePixmapFromBitmapData(dpy, Scr->Root,
	    gray_bits, gray_width, gray_height,
	    tmp_win->border_tile.fore, tmp_win->border_tile.back,
	    Scr->d_depth);

	SetBorder (tmp_win, False);
    }
    else
	tmp_win->gray = None;

    if (tmp_win->title_w.win) {
	CreateWindowTitlebarButtons (tmp_win);
	ComputeTitleLocation (tmp_win);
	XMoveWindow (dpy, tmp_win->title_w.win,
		     tmp_win->title_x, tmp_win->title_y);
	XDefineCursor(dpy, tmp_win->title_w.win, Scr->TitleCursor);
    }

    else {
	tmp_win->title_x = tmp_win->frame_bw3D - tmp_win->frame_bw;
	tmp_win->title_y = tmp_win->frame_bw3D - tmp_win->frame_bw;
    }

    valuemask = (CWEventMask | CWDontPropagate);
    attributes.event_mask = (StructureNotifyMask | PropertyChangeMask |
			     ColormapChangeMask | VisibilityChangeMask |
			     EnterWindowMask | LeaveWindowMask);
    attributes.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask;
    XChangeWindowAttributes (dpy, tmp_win->w, valuemask, &attributes);

    if (HasShape)
	XShapeSelectInput (dpy, tmp_win->w, ShapeNotifyMask);

    if (tmp_win->title_w.win) {
	XMapWindow (dpy, tmp_win->title_w.win);
    }

    if (HasShape) {
	int xws, yws, xbs, ybs;
	unsigned wws, hws, wbs, hbs;
	int boundingShaped, clipShaped;

	XShapeSelectInput (dpy, tmp_win->w, ShapeNotifyMask);
	XShapeQueryExtents (dpy, tmp_win->w,
			    &boundingShaped, &xws, &yws, &wws, &hws,
			    &clipShaped, &xbs, &ybs, &wbs, &hbs);
	tmp_win->wShaped = boundingShaped;
    }

    if (!tmp_win->iconmgr)
	XAddToSaveSet(dpy, tmp_win->w);

    XReparentWindow(dpy, tmp_win->w, tmp_win->frame, tmp_win->frame_bw3D,
		tmp_win->title_height + tmp_win->frame_bw3D);

    /*
     * Reparenting generates an UnmapNotify event, followed by a MapNotify.
     * Set the map state to FALSE to prevent a transition back to
     * WithdrawnState in HandleUnmapNotify.  Map state gets set correctly
     * again in HandleMapNotify.
     */
    tmp_win->mapped = FALSE;

	/*
	 * NOW do the move and resize - windows needed to be created
	 * first to accomodate the opaque resources - djhjr - 4/14/98
	 */
	AddMoveAndResize(tmp_win, ask_user);

    SetupFrame (tmp_win, tmp_win->frame_x, tmp_win->frame_y,
		tmp_win->frame_width, tmp_win->frame_height, -1, True);

    /* wait until the window is iconified and the icon window is mapped
     * before creating the icon window
     */
    tmp_win->icon_w.win = None;

    if (!tmp_win->iconmgr)
    {
	GrabButtons(tmp_win);
	GrabKeys(tmp_win);
    }

    (void) AddIconManager(tmp_win);
    UpdateDesktop(tmp_win);

    XSaveContext(dpy, tmp_win->w, TwmContext, (caddr_t) tmp_win);
    XSaveContext(dpy, tmp_win->w, ScreenContext, (caddr_t) Scr);
    XSaveContext(dpy, tmp_win->frame, TwmContext, (caddr_t) tmp_win);
    XSaveContext(dpy, tmp_win->frame, ScreenContext, (caddr_t) Scr);
    if (tmp_win->title_height)
    {
	int i;
	int nb = Scr->TBInfo.nleft + Scr->TBInfo.nright;

	XSaveContext(dpy, tmp_win->title_w.win, TwmContext, (caddr_t) tmp_win);
	XSaveContext(dpy, tmp_win->title_w.win, ScreenContext, (caddr_t) Scr);
	for (i = 0; i < nb; i++) {
	    XSaveContext(dpy, tmp_win->titlebuttons[i].window, TwmContext,
			 (caddr_t) tmp_win);
	    XSaveContext(dpy, tmp_win->titlebuttons[i].window, ScreenContext,
			 (caddr_t) Scr);
	}
	if (tmp_win->hilite_w)
	{
	    XSaveContext(dpy, tmp_win->hilite_w, TwmContext, (caddr_t)tmp_win);
	    XSaveContext(dpy, tmp_win->hilite_w, ScreenContext, (caddr_t)Scr);
	}
    }

    XUngrabServer(dpy);

    /* if we were in the middle of a menu activated function, regrab
     * the pointer
     */
    if (RootFunction != F_NOFUNCTION)
	ReGrab();

    Scr->Newest = tmp_win;

    return (tmp_win);
}


/*
 * AddPaintRealWindows()
 *
 * a little helper for AddMoveAndResize() - djhjr - 4/15/98
 */
static void 
AddPaintRealWindows (TwmWindow *tmp_win, int x, int y)
{

	/* don't need to send a configure notify event */
	SetupWindow(tmp_win, tmp_win->frame_x, tmp_win->frame_y,
		tmp_win->frame_width, tmp_win->frame_height, -1);

	XMoveWindow(dpy, tmp_win->frame, x, y);

	XMapWindow(dpy, tmp_win->frame);
	XMapSubwindows(dpy, tmp_win->frame);
	XMapSubwindows(dpy, tmp_win->title_w.win);

	PaintBorderAndTitlebar(tmp_win);

	if (!Scr->NoGrabServer)
	{
		/* these allow the application window to be drawn */
		XUngrabServer(dpy); XSync(dpy, 0); XGrabServer(dpy);
	}
}


/**
 * Look for a nonoccupied rectangular area to place a new client.
 * If found return this area in 'pos', otherwise don't change 'pos'.
 * Begin with decomposing screen empty space into (overlapping) 'tiles'.
 *
 *  lst		TWM window list
 *  twm		new client
 *  tiles	tile list to search
 *  pos		return found tile
 */
static int
FindEmptyArea (TwmWindow *lst, TwmWindow *twm, PlaceXY *tiles, PlaceXY *pos)
{
    PlaceXY *t, **pt;

#define OVERLAP(a,f)	((a)->x < (f)->frame_x+(f)->frame_width && (f)->frame_x < (a)->x+(a)->width \
			&& (a)->y < (f)->frame_y+(f)->frame_height && (f)->frame_y < (a)->y+(a)->height)

    /* skip unmapped windows, iconmanagers and vtwm-desktop: */
    while (lst != NULL
	    && (lst->mapped == FALSE
		|| lst->iconmgr == TRUE
		|| strcmp(lst->class.res_class, VTWM_DESKTOP_CLASS) == 0))
	lst = lst->next;

    if (lst == NULL) {
	/* decomposition done, pick and return one 'tile': */
	PlaceXY *m;
	int w, h, d;

	w = twm->attr.width + 2*(twm->frame_bw+twm->frame_bw3D);
	h = twm->attr.height + 2*(twm->frame_bw+twm->frame_bw3D) + twm->title_height;
	d = 2*Scr->TitleHeight;
	m = t = NULL;

	while (tiles != NULL) {
	    if (tiles->width >= w && tiles->height >= h) {
		int x, y;
		x = tiles->x + twm->attr.width/4;
		y = tiles->y + twm->attr.height/4;
		if (tiles->width >= w+d && tiles->height >= h+d) {
		    if ((m == NULL) || (m->y >= y) || (m->x > x))
			m = tiles; /* best choice */
		}
		if ((t == NULL) || (t->y >= y) || (t->x > x))
		    t = tiles; /* second choice */
	    }
	    tiles = tiles->next;
	}

	if (m != NULL)
	    t = m;

	if (t != NULL) {
	    pos->x = t->x;
	    pos->y = t->y;
	    pos->width = t->width;
	    pos->height = t->height;
	    return TRUE;
	}
    } else {
	/* proceed with screen 'tile' decomposition: */
	for (pt = &tiles; (*pt) != NULL; pt = &((*pt)->next))
	    if (OVERLAP((*pt), lst)) {
		PlaceXY _buf[4], *buf = _buf;

		t = (*pt)->next; /*remove current tile from list*/

		if (lst->frame_x + lst->frame_width < (*pt)->x + (*pt)->width) {
		    /* add right subtile: */
		    buf[0].x = lst->frame_x + lst->frame_width;
		    buf[0].y = (*pt)->y;
		    buf[0].width = (*pt)->x + (*pt)->width - (lst->frame_x + lst->frame_width);
		    buf[0].height = (*pt)->height;
		    if (buf[0].width >= twm->attr.width) { /*preselection*/
			buf[0].next = t;
			t = &buf[0];
		    }
		}
		if (lst->frame_y + lst->frame_height < (*pt)->y + (*pt)->height) {
		    /* add bottom subtile: */
		    buf[1].x = (*pt)->x;
		    buf[1].y = lst->frame_y + lst->frame_height;
		    buf[1].width = (*pt)->width;
		    buf[1].height = (*pt)->y + (*pt)->height - (lst->frame_y + lst->frame_height);
		    if (buf[1].height >= twm->attr.height) {
			buf[1].next = t;
			t = &buf[1];
		    }
		}
		if (lst->frame_y > (*pt)->y) {
		    /* add top subtile: */
		    buf[2].x = (*pt)->x;
		    buf[2].y = (*pt)->y;
		    buf[2].width = (*pt)->width;
		    buf[2].height = lst->frame_y - (*pt)->y;
		    if (buf[2].height >= twm->attr.height) {
			buf[2].next = t;
			t = &buf[2];
		    }
		}
		if (lst->frame_x > (*pt)->x) {
		    /* add left subtile: */
		    buf[3].x = (*pt)->x;
		    buf[3].y = (*pt)->y;
		    buf[3].width = lst->frame_x - (*pt)->x;
		    buf[3].height = (*pt)->height;
		    if (buf[3].width >= twm->attr.width) {
			buf[3].next = t;
			t = &buf[3];
		    }
		}

		(*pt) = t;

		return FindEmptyArea (lst, twm, tiles, pos); /*continue subtiling*/
	    }
	return FindEmptyArea (lst->next, twm, tiles, pos); /*start over with next client*/
    }
    return FALSE;
}


/*
 * AddMoveAndResize()
 *
 * was inline in AddWindow() at first call to this function,
 * now handles the opaque move and resize resources - djhjr - 4/14/98
 */
static void 
AddMoveAndResize (TwmWindow *tmp_win, int ask_user)
{
    static int PlaceX = PLACEMENT_START;
    static int PlaceY = PLACEMENT_START;
    XEvent event;
    int stat, gravx, gravy;
    int bw2 = tmp_win->frame_bw * 2;
    int wd = tmp_win->attr.width + bw2;
    int ht = tmp_win->attr.height + tmp_win->title_height + bw2;

    /*
     * do any prompting for position
     */
    if (HandlingEvents && ask_user) {
      if (Scr->RandomPlacement) {	/* just stick it somewhere */
	PlaceXY area;

#ifdef TILED_SCREEN
	if (Scr->use_tiles == TRUE)
	{
	    TwmWindow *tmp;
	    int k;

	    /* look for 'group leader' tile: */
	    for (tmp = Scr->TwmRoot.next; tmp != NULL; tmp = tmp->next)
		if (tmp != tmp_win && tmp->group == tmp_win->group)
		    break;
	    if (tmp != NULL)
		k = FindNearestTileToClient (tmp);
	    else
		k = FindNearestTileToMouse();

	    area.x = Lft(Scr->tiles[k]);
	    area.y = Bot(Scr->tiles[k]);
	    area.width = AreaWidth(Scr->tiles[k]);
	    area.height = AreaHeight(Scr->tiles[k]);
	}
	else
#endif
	{
	    area.x = area.y = 0;
	    area.width = Scr->MyDisplayWidth;
	    area.height = Scr->MyDisplayHeight;
	}
	area.next = NULL;

       if (FindEmptyArea(Scr->TwmRoot.next, tmp_win, &area, &area) == TRUE) {
	    int x, y, b, d;
	    b = Scr->TitleHeight + tmp_win->frame_bw + tmp_win->frame_bw3D;
	    d = tmp_win->title_height;
	    /* slightly off-centered: */
	    x = (area.width - tmp_win->attr.width) / 3;
	    y = (area.height - tmp_win->attr.height + d) / 2;
	    /* tight placing: */
	    if (y < b+d && x > b)
		x = b;
	    if (x < b && y > b+d)
		y = b+d;
	    /* loosen placing: */
	    if (area.width > 4*b + tmp_win->attr.width && x > 2*b)
		x = 2*b;
	    if (area.height > 4*b + tmp_win->attr.height && y > 2*b)
		y = 2*b;
	    tmp_win->attr.x = area.x + x;
	    tmp_win->attr.y = area.y + y;

       } else {

	if (PlaceX + wd > area.width) {
	  if (PLACEMENT_START + wd < area.width)
	    PlaceX = PLACEMENT_START;
	  else {
	    PlaceX = tmp_win->frame_bw;
	    if (wd < area.width)
	      PlaceX += (area.width - wd) / 2;
	  }
	}
	if (PlaceY + ht > area.height) {
	  if (PLACEMENT_START + ht < area.height)
	    PlaceY = PLACEMENT_START;
	  else {
	    PlaceY = tmp_win->title_height + tmp_win->frame_bw;
	    if (ht < area.height)
	      PlaceY += (area.height - ht) / 2;
	  }
	}

	tmp_win->attr.x = area.x + PlaceX;
	tmp_win->attr.y = area.y + PlaceY;
	PlaceX += PLACEMENT_INCR;
	PlaceY += PLACEMENT_INCR;
       }

      } else if (Scr->PointerPlacement) {
	  /* find pointer */
	  if (!XQueryPointer (dpy, Scr->Root, &JunkRoot,
			      &JunkChild, &JunkX, &JunkY,
			      &AddingX, &AddingY, &JunkMask))
	      JunkMask = 0;
	  /* fit window onto screen */
	  if (Scr->WarpSnug) {
	      if (JunkX + wd > Scr->MyDisplayWidth)
		  JunkX -= (JunkX + wd - Scr->MyDisplayWidth);
	      if (JunkY + tmp_win->attr.height > Scr->MyDisplayHeight)
		  JunkY -= (JunkY + tmp_win->attr.height - Scr->MyDisplayHeight);
	  }
	  tmp_win->attr.x = JunkX;
	  tmp_win->attr.y = JunkY;
      } else {				/* else prompt */
	if (!(tmp_win->wmhints && tmp_win->wmhints->flags & StateHint &&
	      tmp_win->wmhints->initial_state == IconicState))
	{
	    Bool firsttime = True;

	    Bool doorismapped = False;
	    TwmDoor *door = NULL;
	    XFindContext(dpy, tmp_win->w, DoorContext, (caddr_t *)&door);

	    /* better wait until all the mouse buttons have been
	     * released.
	     */
	    while (TRUE)
	    {
		XUngrabServer(dpy);
		XSync(dpy, 0);
		XGrabServer(dpy);

		JunkMask = 0;
		if (!XQueryPointer (dpy, Scr->Root, &JunkRoot,
				    &JunkChild, &JunkX, &JunkY,
				    &AddingX, &AddingY, &JunkMask))
		  JunkMask = 0;

		JunkMask &= (Button1Mask | Button2Mask | Button3Mask |
			     Button4Mask | Button5Mask);

		/*
		 * watch out for changing screens
		 */
		if (firsttime) {
		    if (JunkRoot != Scr->Root) {
			register int scrnum;

			for (scrnum = 0; scrnum < NumScreens; scrnum++) {
			    if (JunkRoot == RootWindow (dpy, scrnum)) break;
			}

			if (scrnum != NumScreens) PreviousScreen = scrnum;
		    }
		    firsttime = False;
		}

		/*
		 * wait for buttons to come up; yuck
		 */
		if (JunkMask != 0) continue;

		/*
		 * this will cause a warp to the indicated root
		 */
		stat = XGrabPointer(dpy, Scr->Root, False,
		    ButtonPressMask | ButtonReleaseMask |
		    PointerMotionMask | PointerMotionHintMask,
		    GrabModeAsync, GrabModeAsync,
		    Scr->Root, UpperLeftCursor, CurrentTime);

		if (stat == GrabSuccess)
		    break;
	    }

		if (Scr->NoGrabServer) XUngrabServer(dpy);


#ifdef TILED_SCREEN
	    if (Scr->use_tiles == TRUE) {
		int k = FindNearestTileToMouse();
		XMoveWindow (dpy, Scr->SizeWindow.win, Lft(Scr->tiles[k]), Bot(Scr->tiles[k]));
	    }
#endif
	    XMapRaised(dpy, Scr->SizeWindow.win);
	    InstallRootColormap();

	    AddingW = tmp_win->attr.width + bw2 + 2 * tmp_win->frame_bw3D;
	    AddingH = tmp_win->attr.height + tmp_win->title_height +
				bw2 + 2 * tmp_win->frame_bw3D;

		if (tmp_win->opaque_move)
		{
			AddPaintRealWindows(tmp_win, AddingX, AddingY);

			if (door && !doorismapped)
			{
				XMapWindow(dpy, door->w.win);
				RedoDoorName(tmp_win, door);
				doorismapped = True;
			}
		}
		else
			MoveOutline(Scr->Root, AddingX, AddingY, AddingW, AddingH,
			    tmp_win->frame_bw, tmp_win->title_height + tmp_win->frame_bw3D);


		if (Scr->VirtualReceivesMotionEvents)
		{
			tmp_win->virtual_frame_x = R_TO_V_X(AddingX);
			tmp_win->virtual_frame_y = R_TO_V_Y(AddingY);
			UpdateDesktop(tmp_win);
		}

	    DisplayPosition (AddingX, AddingY);

	    while (TRUE)
		{
		XMaskEvent(dpy, ButtonPressMask | PointerMotionMask, &event);

		if (Event.type == MotionNotify) {
		    /* discard any extra motion events before a release */
		    while(XCheckMaskEvent(dpy,
			ButtonMotionMask | ButtonPressMask, &Event))
			if (Event.type == ButtonPress)
			    break;
		}

		if (event.type == ButtonPress) {
		  AddingX = event.xbutton.x_root;
		  AddingY = event.xbutton.y_root;

		  /* DontMoveOff prohibits user form off-screen placement */
		  if (Scr->DontMoveOff)
		    {
		      int AddingR, AddingB;

		      AddingR = AddingX + AddingW;
		      AddingB = AddingY + AddingH;

		      if (AddingX < 0)
			AddingX = 0;
		      if (AddingR > Scr->MyDisplayWidth)
			AddingX = Scr->MyDisplayWidth - AddingW;

		      if (AddingY < 0)
			AddingY = 0;
		      if (AddingB > Scr->MyDisplayHeight)
			AddingY = Scr->MyDisplayHeight - AddingH;

		    }
		  break;
		}

		if (event.type != MotionNotify) {
		    continue;
	    }

		XQueryPointer(dpy, Scr->Root, &JunkRoot, &JunkChild,
		    &JunkX, &JunkY, &AddingX, &AddingY, &JunkMask);

		if (Scr->DontMoveOff)
		{
		    int AddingR, AddingB;

		    AddingR = AddingX + AddingW;
		    AddingB = AddingY + AddingH;

		    if (AddingX < 0)
			AddingX = 0;
		    if (AddingR > Scr->MyDisplayWidth)
			AddingX = Scr->MyDisplayWidth - AddingW;

		    if (AddingY < 0)
			AddingY = 0;
		    if (AddingB > Scr->MyDisplayHeight)
			AddingY = Scr->MyDisplayHeight - AddingH;
		}

		if (tmp_win->opaque_move)
		{
			XMoveWindow(dpy, tmp_win->frame, AddingX, AddingY);
			PaintBorderAndTitlebar(tmp_win);

			if (!Scr->NoGrabServer)
			{
				/* these allow the application window to be drawn */
				XUngrabServer(dpy); XSync(dpy, 0); XGrabServer(dpy);
			}
		}
		else
			MoveOutline(Scr->Root, AddingX, AddingY, AddingW, AddingH,
			    tmp_win->frame_bw, tmp_win->title_height + tmp_win->frame_bw3D);

		if (Scr->VirtualReceivesMotionEvents)
		{
			tmp_win->virtual_frame_x = R_TO_V_X(AddingX);
			tmp_win->virtual_frame_y = R_TO_V_Y(AddingY);
			MoveResizeDesktop(tmp_win, Scr->NoRaiseResize);
		}

		DisplayPosition (AddingX, AddingY);

	    }

	    if (event.xbutton.button == Button2) {
		int lastx, lasty;

		if (!tmp_win->opaque_move && tmp_win->opaque_resize)
		{
			/* erase the move outline */
		    MoveOutline(Scr->Root, 0, 0, 0, 0, 0, 0);

			AddPaintRealWindows(tmp_win, AddingX, AddingY);

			if (door && !doorismapped)
			{
				XMapWindow(dpy, door->w.win);
				RedoDoorName(tmp_win, door);
				doorismapped = True;
			}
		}

		if (Scr->AutoRelativeResize)
		{
		    int dx = (tmp_win->attr.width / 4);
		    int dy = (tmp_win->attr.height / 4);

#define HALF_AVE_CURSOR_SIZE 8		/* so that it is visible */
		    if (dx < HALF_AVE_CURSOR_SIZE) dx = HALF_AVE_CURSOR_SIZE;
		    if (dy < HALF_AVE_CURSOR_SIZE) dy = HALF_AVE_CURSOR_SIZE;
#undef HALF_AVE_CURSOR_SIZE
		    dx += (tmp_win->frame_bw + 1);
		    dy += (bw2 + tmp_win->title_height + 1);
		    if (AddingX + dx >= Scr->MyDisplayWidth)
		      dx = Scr->MyDisplayWidth - AddingX - 1;
		    if (AddingY + dy >= Scr->MyDisplayHeight)
		      dy = Scr->MyDisplayHeight - AddingY - 1;
		    if (dx > 0 && dy > 0)
		      XWarpPointer (dpy, None, None, 0, 0, 0, 0, dx, dy);
		} else {
		    XWarpPointer (dpy, None, Scr->Root, 0, 0, 0, 0,
				  AddingX + AddingW/2, AddingY + AddingH/2);
		}
		AddStartResize(tmp_win, AddingX, AddingY, AddingW, AddingH);

		lastx = -10000;
		lasty = -10000;
		while (TRUE)
		{
		    XMaskEvent(dpy,
			       ButtonReleaseMask | ButtonMotionMask, &event);

		    if (Event.type == MotionNotify) {
			/* discard any extra motion events before a release */
			while(XCheckMaskEvent(dpy,
			    ButtonMotionMask | ButtonReleaseMask, &Event))
			    if (Event.type == ButtonRelease)
				break;
		    }

		    if (event.type == ButtonRelease)
		    {
			AddEndResize(tmp_win);

			if (tmp_win->opaque_move && !tmp_win->opaque_resize)
				SetupWindow(tmp_win, AddingX, AddingY, AddingW, AddingH, -1);

			if (!tmp_win->opaque_resize && door && doorismapped)
				RedoDoorName(tmp_win, door);

			break;
		    }

		    if (event.type != MotionNotify) {
			continue;
		    }

		    /*
		     * XXX - if we are going to do a loop, we ought to consider
		     * using multiple GXxor lines so that we don't need to
		     * grab the server.
		     */
		    XQueryPointer(dpy, Scr->Root, &JunkRoot, &JunkChild,
			&JunkX, &JunkY, &AddingX, &AddingY, &JunkMask);

		    if (lastx != AddingX || lasty != AddingY)
		    {
			DoResize(AddingX, AddingY, tmp_win);

			lastx = AddingX;
			lasty = AddingY;
		    }

		}
	    }
	    else if (event.xbutton.button == Button3)
	    {
		int maxw, maxh;

#ifdef TILED_SCREEN
		if (Scr->use_tiles == TRUE) {
		    int Area[4];
		    Lft(Area) = AddingX;
		    Rht(Area) = AddingX + AddingW - 1;
		    Bot(Area) = AddingY;
		    Top(Area) = AddingY + AddingH - 1;
		    TilesFullZoom (Area);
		    maxw = Rht(Area)+1 - AddingX - bw2;
		    maxh = Top(Area)+1 - AddingY - bw2;
		}
		else
#endif
		{
		    maxw = Scr->MyDisplayWidth  - AddingX - bw2;
		    maxh = Scr->MyDisplayHeight - AddingY - bw2;
		}

		/*
		 * Make window go to bottom of screen, and clip to right edge.
		 * This is useful when popping up large windows and fixed
		 * column text windows.
		 */
		if (AddingW > maxw) AddingW = maxw;
		AddingH = maxh;

		ConstrainSize (tmp_win, &AddingW, &AddingH);  /* w/o borders */
		AddingW += bw2;
		AddingH += bw2;

		/* don't need to send a configure notify event - djhjr - 4/15/98 */
		SetupWindow(tmp_win, AddingX, AddingY, AddingW, AddingH, -1);

		if (!tmp_win->opaque_resize && door && doorismapped)
			RedoDoorName(tmp_win, door);

	    }
	    else
	    {
		XMaskEvent(dpy, ButtonReleaseMask, &event);
	    }

		/* erase the move outline */
	    MoveOutline(Scr->Root, 0, 0, 0, 0, 0, 0);

	    XUnmapWindow(dpy, Scr->SizeWindow.win);
	    UninstallRootColormap();
	    XUngrabPointer(dpy, CurrentTime);

		tmp_win->attr.x = AddingX + tmp_win->frame_bw + tmp_win->frame_bw3D;
		tmp_win->attr.y = AddingY + tmp_win->title_height +
			tmp_win->frame_bw + tmp_win->frame_bw3D;

	    tmp_win->attr.width = AddingW - bw2 - 2 * tmp_win->frame_bw3D;
	    tmp_win->attr.height = AddingH - tmp_win->title_height -
				bw2 - 2 * tmp_win->frame_bw3D;

	}
      }

		if (Scr->VirtualReceivesMotionEvents && !tmp_win->opaque_move)
		{
			XUnmapWindow(dpy, Scr->VirtualDesktopDisplay);
			XMapWindow(dpy, Scr->VirtualDesktopDisplay);
		}
    } else {				/* put it where asked, mod title bar */
	if (tmp_win->transient) {
	    /* some toolkits put transients beyond screen: */
#ifdef TILED_SCREEN
	    if (Scr->use_tiles == TRUE)
	    {
		TwmWindow *tmp;
		int k = FindNearestTileToMouse();

		/* look for 'parent' tile: */
		for (tmp = Scr->TwmRoot.next; tmp != NULL; tmp = tmp->next)
		    if (tmp != tmp_win && tmp->w == tmp_win->transientfor)
			break;

		if (tmp != NULL) {
		    if ((Distance1D(tmp_win->attr.x, tmp_win->attr.x+tmp_win->attr.width-1,
				Lft(Scr->tiles[k]), Rht(Scr->tiles[k])) < 0)
			|| (Distance1D(tmp_win->attr.y, tmp_win->attr.y+tmp_win->attr.height-1,
				Bot(Scr->tiles[k]), Top(Scr->tiles[k])) < 0))
			k = FindNearestTileToClient (tmp);
		}

		EnsureRectangleOnTile (k, &tmp_win->attr.x, &tmp_win->attr.y,
			tmp_win->attr.width, tmp_win->attr.height);
	    }
	    else
#endif
	    {
		if (tmp_win->attr.x + tmp_win->attr.width > Scr->MyDisplayWidth)
		    tmp_win->attr.x = Scr->MyDisplayWidth - tmp_win->attr.width;
		if (tmp_win->attr.x < 0)
		    tmp_win->attr.x = 0;
		if (tmp_win->attr.y + tmp_win->attr.height > Scr->MyDisplayHeight)
		    tmp_win->attr.y = Scr->MyDisplayHeight - tmp_win->attr.height;
		if (tmp_win->attr.y < 0)
		    tmp_win->attr.y = 0;
	    }
	}

	/* interpret the position specified as a virtual one if asked */

	if (Scr->GeometriesAreVirtual ||
	    (!Scr->GeometriesAreVirtual &&
		(tmp_win->nailed ||
		    (   ( (Scr->FixManagedVirtualGeometries &&
				!tmp_win->transient) ||
			  (Scr->FixTransientVirtualGeometries &&
				tmp_win->transient)
			) && (tmp_win->hints.flags & PPosition)
		    )
		)
	    )
	)
	    /*
	     * If virtual geometries is set, or virtual geometries
	     * isn't set and (nailed or (fix virtual geometries and
	     * preferred position)). This is a bug workaround -- DSE
	     */
	    {
		tmp_win->attr.x = V_TO_R_X(tmp_win->attr.x);
		tmp_win->attr.y = V_TO_R_Y(tmp_win->attr.y);
	    }

	GetGravityOffsets (tmp_win, &gravx, &gravy);

	/* if the gravity is towards the top, move it by the title height */
	if (gravy < 0) tmp_win->attr.y -= gravy * tmp_win->title_height;
    }

	tmp_win->frame_x = tmp_win->attr.x -
			tmp_win->frame_bw - tmp_win->frame_bw3D;
	tmp_win->frame_y = tmp_win->attr.y - tmp_win->title_height -
			tmp_win->frame_bw - tmp_win->frame_bw3D;

	tmp_win->frame_width = tmp_win->attr.width + 2 * tmp_win->frame_bw3D;
	tmp_win->frame_height = tmp_win->attr.height + tmp_win->title_height +
			2 * tmp_win->frame_bw3D;
	ConstrainSize (tmp_win, &tmp_win->frame_width, &tmp_win->frame_height);

	tmp_win->virtual_frame_x = R_TO_V_X(tmp_win->frame_x);
	tmp_win->virtual_frame_y = R_TO_V_Y(tmp_win->frame_y);

#ifdef DEBUG
	fprintf(stderr, "  position window  %d, %d  %dx%d\n",
	    tmp_win->attr.x,
	    tmp_win->attr.y,
	    tmp_win->attr.width,
	    tmp_win->attr.height);
#endif
}


/***********************************************************************
 *
 *  Procedure:
 *	MappedNotOverride - checks to see if we should really
 *		put a twm frame on the window
 *
 *  Returned Value:
 *	TRUE	- go ahead and frame the window
 *	FALSE	- don't frame the window
 *
 *  Inputs:
 *	w	- the window to check
 *
 ***********************************************************************
 */

int 
MappedNotOverride (Window w)
{
    XWindowAttributes wa;

    XGetWindowAttributes(dpy, w, &wa);
    return ((wa.map_state != IsUnmapped) && (wa.override_redirect != True));
}


/***********************************************************************
 *
 *  Procedure:
 *      AddDefaultBindings - attach default bindings so that naive users
 *      don't get messed up if they provide a minimal twmrc.
 */
static void 
do_add_binding (int button, int context, int modifier, int func)
{
    MouseButton *mb = &Scr->Mouse[MOUSELOC(button,context,modifier)];

    if (mb->func) return;		/* already defined */

    mb->func = func;
    mb->item = NULL;
}

void 
AddDefaultBindings (void)
{
    /*
     * The bindings are stored in Scr->Mouse, indexed by
     * Mouse[button_number][C_context][modifier].
     */

#define NoModifierMask 0

    do_add_binding (Button1, C_TITLE, NoModifierMask, F_MOVE);
    do_add_binding (Button1, C_ICON, NoModifierMask, F_ICONIFY);
    do_add_binding (Button1, C_ICONMGR, NoModifierMask, F_ICONIFY);
    do_add_binding (Button1, C_VIRTUAL, NoModifierMask, F_MOVESCREEN);
    do_add_binding (Button1, C_VIRTUAL_WIN, NoModifierMask, F_MOVESCREEN);

    do_add_binding (Button2, C_TITLE, NoModifierMask, F_RAISELOWER);
    do_add_binding (Button2, C_ICON, NoModifierMask, F_ICONIFY);
    do_add_binding (Button2, C_ICONMGR, NoModifierMask, F_ICONIFY);
    do_add_binding (Button2, C_VIRTUAL, NoModifierMask, F_MOVESCREEN);
    do_add_binding (Button2, C_VIRTUAL_WIN, NoModifierMask, F_MOVESCREEN);

#undef NoModifierMask
}




/***********************************************************************
 *
 *  Procedure:
 *	GrabButtons - grab needed buttons for the window
 *
 *  Inputs:
 *	tmp_win - the twm window structure to use
 *
 ***********************************************************************
 */

void 
GrabButtons (TwmWindow *tmp_win)
{
    int i, j;

    for (i = 0; i < NumButtons+1; i++)
    {
	for (j = 0; j < MOD_SIZE; j++)
	{
	    if (Scr->Mouse[MOUSELOC(i,C_WINDOW,j)].func != F_NOFUNCTION)
	    {
		/* twm used to do this grab on the application main window,
		 * tmp_win->w . This was not ICCCM complient and was changed.
		 */
		XGrabButton(dpy, i, j, tmp_win->frame,
			    True, ButtonPressMask | ButtonReleaseMask,
			    GrabModeAsync, GrabModeAsync, None,
			    Scr->FrameCursor);
	    }
	}
    }
}

/***********************************************************************
 *
 *  Procedure:
 *	GrabKeys - grab needed keys for the window
 *
 *  Inputs:
 *	tmp_win - the twm window structure to use
 *
 ***********************************************************************
 */

void 
GrabModKeys (Window w, FuncKey *k)
{
    int i;

    XGrabKey(dpy, k->keycode, k->mods, w, True, GrabModeAsync, GrabModeAsync);

    for (i = 1; i <= Scr->IgnoreModifiers; i++)
	if ((Scr->IgnoreModifiers & i) == i)
	    XGrabKey(dpy, k->keycode, k->mods | i, w, True,
			GrabModeAsync, GrabModeAsync);
}

void 
UngrabModKeys (Window w, FuncKey *k)
{
    int i;

    XUngrabKey(dpy, k->keycode, k->mods, w);

    for (i = 1; i <= Scr->IgnoreModifiers; i++)
	if ((Scr->IgnoreModifiers & i) == i)
	    XUngrabKey(dpy, k->keycode, k->mods | i, w);
}

void 
GrabKeys (TwmWindow *tmp_win)
{
    FuncKey *tmp;
    IconMgr *p;

    for (tmp = Scr->FuncKeyRoot.next; tmp != NULL; tmp = tmp->next)
    {
	switch (tmp->cont)
	{
	case C_WINDOW:
	    GrabModKeys(tmp_win->w, tmp);
	    break;

	case C_ICON:
	    if (tmp_win->icon_w.win)
		GrabModKeys(tmp_win->icon_w.win, tmp);

	case C_TITLE:
	    if (tmp_win->title_w.win)
		GrabModKeys(tmp_win->title_w.win, tmp);
	    break;

	case C_NAME:
	    GrabModKeys(tmp_win->w, tmp);
	    if (tmp_win->icon_w.win)
		GrabModKeys(tmp_win->icon_w.win, tmp);
	    if (tmp_win->title_w.win)
		GrabModKeys(tmp_win->title_w.win, tmp);
	    break;

	}
    }
    for (tmp = Scr->FuncKeyRoot.next; tmp != NULL; tmp = tmp->next)
    {
	if (tmp->cont == C_ICONMGR && !Scr->NoIconManagers)
	{
	    for (p = &Scr->iconmgr; p != NULL; p = p->next)
	    {
		UngrabModKeys(p->twm_win->w, tmp);
	    }
	}
    }
}

static Window 
CreateHighlightWindow (TwmWindow *tmp_win)
{
    XSetWindowAttributes attributes;	/* attributes for create windows */
    Pixmap pm = None;
    GC gc;
    XGCValues gcv;
    unsigned long valuemask;
	unsigned int pm_numcolors;
    int h = (Scr->TitleHeight - 2 * Scr->FramePadding) - 2;
    Window w;


    /*
     * If a special highlight pixmap was given, use that.  Otherwise,
     * use a nice, even gray pattern.  The old horizontal lines look really
     * awful on interlaced monitors (as well as resembling other looks a
     * little bit too closely), but can be used by putting
     *
     *                 Pixmaps { TitleHighlight "hline2" }
     *
     * (or whatever the horizontal line bitmap is named) in the startup
     * file.  If all else fails, use the foreground color to look like a
     * solid line.
     */
	if (!Scr->hilitePm)
	{
		Scr->hilitePm = (Image *)malloc(sizeof(Image));
		if (Scr->hilitePm)
		{
			Scr->hilitePm->width = gray_width;
			Scr->hilitePm->height = gray_height;
			Scr->hilitePm->mask = None;

			if (Scr->TitleBevelWidth > 0 && (Scr->Monochrome != COLOR))
				Scr->hilitePm->pixmap = XCreateBitmapFromData (dpy,
						tmp_win->title_w.win, (char *)black_bits,
						Scr->hilitePm->width, Scr->hilitePm->height);
			else
				Scr->hilitePm->pixmap = XCreateBitmapFromData (dpy,
						tmp_win->title_w.win, gray_bits,
						Scr->hilitePm->width, Scr->hilitePm->height);
		}
	}

	pm_numcolors  = 0;

#ifndef NO_XPM_SUPPORT
		pm_numcolors = SetPixmapsBackground(Scr->hilitePm, Scr->Root,
				tmp_win->title.back);
#endif

	/*
	 * Modified to handle non-transparent pixmaps - Jason Gloudon
	 */
	if (Scr->hilitePm->pixmap)
	{
		if (pm_numcolors > 2) /* not a bitmap */
		{
			valuemask = CWBackPixmap;
			attributes.background_pixmap = Scr->hilitePm->pixmap;
		}
		else
		{
			pm = XCreatePixmap (dpy, tmp_win->title_w.win,
					Scr->hilitePm->width, Scr->hilitePm->height,
					Scr->d_depth);

			gcv.foreground = tmp_win->title.fore;
			gcv.background = tmp_win->title.back;
			gcv.graphics_exposures = False;

			gc = XCreateGC (dpy, pm,
					(GCForeground | GCBackground | GCGraphicsExposures),
					&gcv);

			if (gc)
			{
				/* the copy plane works on color ! */
				XCopyPlane (dpy, Scr->hilitePm->pixmap, pm, gc, 0, 0,
						Scr->hilitePm->width, Scr->hilitePm->height,
						0, 0, 1);

				XFreeGC(dpy, gc);

				valuemask = CWBackPixmap;
				attributes.background_pixmap = pm;
			}
			else
			{
				valuemask = CWBackPixel;
				attributes.background_pixel = tmp_win->title.fore;
			}
		}
	}
	else
	{
		valuemask = CWBackPixel;
		attributes.background_pixel = tmp_win->title.fore;
	}

	w = XCreateWindow (dpy, tmp_win->title_w.win,
		       tmp_win->highlightx, Scr->FramePadding + 1,
		       ComputeHighlightWindowWidth(tmp_win), (unsigned int) h,
		       (unsigned int) 0,
		       Scr->d_depth, (unsigned int) CopyFromParent,
		       Scr->d_visual, valuemask, &attributes);

    if (pm) XFreePixmap (dpy, pm);

    return w;
}


void 
ComputeCommonTitleOffsets (void)
{
    int en = MyFont_TextWidth(&Scr->TitleBarFont, "n", 1);

    int buttonwidth = (Scr->TBInfo.width + Scr->TBInfo.pad);

    Scr->TBInfo.leftx = Scr->TBInfo.rightoff = Scr->FramePadding;

    if (Scr->TBInfo.nleft > 0)
      Scr->TBInfo.leftx += Scr->ButtonIndent;

    /* 'en' was 'Scr->TitlePadding' - djhjr - 10/18/02 */
    Scr->TBInfo.titlex = (Scr->TBInfo.leftx +
			 (Scr->TBInfo.nleft * buttonwidth) - Scr->TBInfo.pad +
			 en);

    if (Scr->TBInfo.nright > 0)
      Scr->TBInfo.rightoff += (Scr->ButtonIndent +
			      ((Scr->TBInfo.nright * buttonwidth) -
			      Scr->TBInfo.pad));
    return;
}

void 
ComputeWindowTitleOffsets (TwmWindow *tmp_win, int width, Bool squeeze)
{
    int en = MyFont_TextWidth(&Scr->TitleBarFont, "n", 1);

    tmp_win->highlightx = Scr->TBInfo.titlex + tmp_win->name_width + en;


    tmp_win->rightx = width - Scr->TBInfo.rightoff;

	if (squeeze && tmp_win->squeeze_info)
	{
		int rx = tmp_win->highlightx +
			 ((tmp_win->titlehighlight) ? Scr->TitleHeight * 2 : 0);

		if (rx < tmp_win->rightx) tmp_win->rightx = rx;
    }

    return;
}


/*
 * ComputeTitleLocation - calculate the position of the title window.
 */
void 
ComputeTitleLocation (TwmWindow *tmp)
{
	tmp->title_x = tmp->frame_bw3D - tmp->frame_bw;
	tmp->title_y = tmp->frame_bw3D - tmp->frame_bw;

	if (tmp->squeeze_info)
	{
		SqueezeInfo *si = tmp->squeeze_info;
		int fw = tmp->frame_bw + tmp->frame_bw3D;
		int basex = fw;

		if (si->denom != 0 && si->num != 0)
			basex = ((tmp->frame_width - tmp->title_width) / si->denom) * si->num + fw;
		else
			if (si->denom == 0 && si->num != 0)
				basex = si->num + fw;
			else
				switch (si->justify)
				{
					case J_LEFT:
						basex = tmp->title_x + fw;
						break;
					case J_CENTER:
						basex = tmp->frame_width / 2 - (tmp->title_width / 2 - fw);
						break;
					case J_RIGHT:
						basex = tmp->frame_width - tmp->title_width;
						break;
				}

		if (basex > tmp->frame_width - tmp->title_width)
			basex = tmp->frame_width - tmp->title_width;
		if (basex < 0)
			basex = tmp->title_x + fw;

		tmp->title_x = basex - fw;
	}
}


static void 
CreateWindowTitlebarButtons (TwmWindow *tmp_win)
{
    unsigned long valuemask;		/* mask for create windows */
    XSetWindowAttributes attributes;	/* attributes for create windows */
    TitleButton *tb;
    TBWindow *tbw;
    int boxwidth = Scr->TBInfo.width + Scr->TBInfo.pad;
    unsigned int h = Scr->TBInfo.width - Scr->TBInfo.border * 2;
    int x, y = Scr->FramePadding + Scr->ButtonIndent;
    int nb, leftx, rightx;

    if (tmp_win->title_height == 0)
    {
	tmp_win->hilite_w = 0;
	return;
    }

    /*
     * create the title bar windows; let the event handler deal with painting
     * so that we don't have to spend two pixmaps (or deal with hashing)
     */
    ComputeWindowTitleOffsets (tmp_win, tmp_win->attr.width, False);

    leftx = Scr->TBInfo.leftx;
    rightx = tmp_win->rightx;

    attributes.background_pixel = tmp_win->title.back;
    attributes.border_pixel = tmp_win->title.fore;
    attributes.event_mask = (ButtonPressMask | ButtonReleaseMask | ExposureMask);
    attributes.cursor = Scr->ButtonCursor;

    valuemask = (CWWinGravity | CWBackPixel | CWBorderPixel | CWEventMask | CWCursor);

    tmp_win->titlebuttons = NULL;
    nb = Scr->TBInfo.nleft + Scr->TBInfo.nright;
    if (nb > 0) {
	tmp_win->titlebuttons = (TBWindow *) malloc (nb * sizeof(TBWindow));
	if (!tmp_win->titlebuttons) {
	    fprintf (stderr, "%s:  unable to allocate %d titlebuttons\n",
		     ProgramName, nb);
	} else {
	    for (tb = Scr->TBInfo.head, tbw = tmp_win->titlebuttons; tb;
		 tb = tb->next, tbw++) {
		if (tb->rightside) {
		    x = rightx;
		    rightx += boxwidth;
		    attributes.win_gravity = NorthEastGravity;
		} else {
		    x = leftx;
		    leftx += boxwidth;
		    attributes.win_gravity = NorthWestGravity;
		}

		tbw->window = XCreateWindow (dpy, tmp_win->title_w.win, x, y, h, h,
					     (unsigned int) Scr->TBInfo.border,
					     0, (unsigned int) CopyFromParent,
					     (Visual *) CopyFromParent,
					     valuemask, &attributes);

		tbw->info = tb;
	    } /* end for(...) */
	}
    }

	tmp_win->hilite_w =
		(tmp_win->titlehighlight && !Scr->hiliteName) ?
		CreateHighlightWindow(tmp_win) : None;

    return;
}


void 
SetHighlightPixmap (char *filename)
{
	if (filename[0] == ':')
		Scr->hiliteName = filename;
	else

	if (!Scr->hilitePm) Scr->hilitePm = SetPixmapsPixmap(filename);
}


void 
FetchWmProtocols (TwmWindow *tmp)
{
    unsigned long flags = 0L;
    Atom *protocols = NULL;
    int n;

    if (XGetWMProtocols (dpy, tmp->w, &protocols, &n)) {
	register int i;
	register Atom *ap;

	for (i = 0, ap = protocols; i < n; i++, ap++) {
	    if (*ap == _XA_WM_TAKE_FOCUS) flags |= DoesWmTakeFocus;
	    if (*ap == _XA_WM_SAVE_YOURSELF) flags |= DoesWmSaveYourself;
	    if (*ap == _XA_WM_DELETE_WINDOW) flags |= DoesWmDeleteWindow;
	}
	if (protocols) XFree ((char *) protocols);
    }
    tmp->protocols = flags;
}

TwmColormap *
CreateTwmColormap (Colormap c)
{
    TwmColormap *cmap;
    cmap = (TwmColormap *) malloc(sizeof(TwmColormap));
    if (!cmap ||
	XSaveContext(dpy, c, ColormapContext, (caddr_t) cmap)) {
	if (cmap) free((char *) cmap);
	return (NULL);
    }
    cmap->c = c;
    cmap->state = 0;
    cmap->install_req = 0;
    cmap->w = None;
    cmap->refcnt = 1;
    return (cmap);
}

ColormapWindow *
CreateColormapWindow (Window w, Bool creating_parent, Bool property_window)
{
    ColormapWindow *cwin;
    TwmColormap *cmap;
    XWindowAttributes attributes;

    cwin = (ColormapWindow *) malloc(sizeof(ColormapWindow));
    if (cwin) {
	if (!XGetWindowAttributes(dpy, w, &attributes) ||
	    XSaveContext(dpy, w, ColormapContext, (caddr_t) cwin)) {
	    free((char *) cwin);
	    return (NULL);
	}

	if (XFindContext(dpy, attributes.colormap,  ColormapContext,
		(caddr_t *)&cwin->colormap) == XCNOENT) {
	    cwin->colormap = cmap = CreateTwmColormap(attributes.colormap);
	    if (!cmap) {
		XDeleteContext(dpy, w, ColormapContext);
		free((char *) cwin);
		return (NULL);
	    }
	} else {
	    cwin->colormap->refcnt++;
	}

	cwin->w = w;
	/*
	 * Assume that windows in colormap list are
	 * obscured if we are creating the parent window.
	 * Otherwise, we assume they are unobscured.
	 */
	cwin->visibility = creating_parent ?
	    VisibilityPartiallyObscured : VisibilityUnobscured;
	cwin->refcnt = 1;

	/*
	 * If this is a ColormapWindow property window and we
	 * are not monitoring ColormapNotify or VisibilityNotify
	 * events, we need to.
	 */
	if (property_window &&
	    (attributes.your_event_mask &
		(ColormapChangeMask|VisibilityChangeMask)) !=
		    (ColormapChangeMask|VisibilityChangeMask)) {
	    XSelectInput(dpy, w, attributes.your_event_mask |
		(ColormapChangeMask|VisibilityChangeMask));
	}
    }

    return (cwin);
}

void 
FetchWmColormapWindows (TwmWindow *tmp)
{
    register int i, j;
    Window *cmap_windows = NULL;
    Bool can_free_cmap_windows = False;
    int number_cmap_windows = 0;
    ColormapWindow **cwins = NULL;
    int previously_installed;

    number_cmap_windows = 0;

    if ((previously_installed =
       (Scr->cmapInfo.cmaps == &tmp->cmaps) && tmp->cmaps.number_cwins)) {
	cwins = tmp->cmaps.cwins;
	for (i = 0; i < tmp->cmaps.number_cwins; i++)
	    cwins[i]->colormap->state = 0;
    }

    if (XGetWMColormapWindows (dpy, tmp->w, &cmap_windows,
			       &number_cmap_windows) &&
	number_cmap_windows > 0) {

	can_free_cmap_windows = False;
	/*
	 * check if the top level is in the list, add to front if not
	 */
	for (i = 0; i < number_cmap_windows; i++) {
	    if (cmap_windows[i] == tmp->w) break;
	}
	if (i == number_cmap_windows) {	 /* not in list */
	    Window *new_cmap_windows =
	      (Window *) malloc (sizeof(Window) * (number_cmap_windows + 1));

	    if (!new_cmap_windows) {
		fprintf (stderr,
			 "%s:  unable to allocate %d element colormap window array\n",
			ProgramName, number_cmap_windows+1);
		goto done;
	    }
	    new_cmap_windows[0] = tmp->w;  /* add to front */
	    for (i = 0; i < number_cmap_windows; i++) {	 /* append rest */
		new_cmap_windows[i+1] = cmap_windows[i];
	    }
	    XFree ((char *) cmap_windows);
	    can_free_cmap_windows = True;  /* do not use XFree any more */
	    cmap_windows = new_cmap_windows;
	    number_cmap_windows++;
	}

	cwins = (ColormapWindow **) malloc(sizeof(ColormapWindow *) *
		number_cmap_windows);
	if (cwins) {
	    for (i = 0; i < number_cmap_windows; i++) {

		/*
		 * Copy any existing entries into new list.
		 */
		for (j = 0; j < tmp->cmaps.number_cwins; j++) {
		    if (tmp->cmaps.cwins[j]->w == cmap_windows[i]) {
			cwins[i] = tmp->cmaps.cwins[j];
			cwins[i]->refcnt++;
			break;
		    }
		}

		/*
		 * If the colormap window is not being pointed by
		 * some other applications colormap window list,
		 * create a new entry.
		 */
		if (j == tmp->cmaps.number_cwins) {
		    if (XFindContext(dpy, cmap_windows[i], ColormapContext,
				     (caddr_t *)&cwins[i]) == XCNOENT) {
			if ((cwins[i] = CreateColormapWindow(cmap_windows[i],
				    (Bool) tmp->cmaps.number_cwins == 0,
				    True)) == NULL) {
			    int k;
			    for (k = i + 1; k < number_cmap_windows; k++)
				cmap_windows[k-1] = cmap_windows[k];
			    i--;
			    number_cmap_windows--;
			}
		    } else
			cwins[i]->refcnt++;
		}
	    }
	}
    }

    /* No else here, in case we bailed out of clause above.
     */
    if (number_cmap_windows == 0) {

	number_cmap_windows = 1;

	cwins = (ColormapWindow **) malloc(sizeof(ColormapWindow *));
	if (XFindContext(dpy, tmp->w, ColormapContext, (caddr_t *)&cwins[0]) ==
		XCNOENT)
	    cwins[0] = CreateColormapWindow(tmp->w,
			    (Bool) tmp->cmaps.number_cwins == 0, False);
	else
	    cwins[0]->refcnt++;
    }

    if (tmp->cmaps.number_cwins)
	free_cwins(tmp);

    tmp->cmaps.cwins = cwins;
    tmp->cmaps.number_cwins = number_cmap_windows;
    if (number_cmap_windows > 1)
	tmp->cmaps.scoreboard =
	  (char *) calloc(1, ColormapsScoreboardLength(&tmp->cmaps));

    if (previously_installed)
	InstallWindowColormaps(PropertyNotify, (TwmWindow *) NULL);

  done:
    if (cmap_windows) {
	if (can_free_cmap_windows)
	  free ((char *) cmap_windows);
	else
	  XFree ((char *) cmap_windows);
    }

    return;
}


void 
GetWindowSizeHints (TwmWindow *tmp)
{
    long supplied = 0;

    if (!XGetWMNormalHints (dpy, tmp->w, &tmp->hints, &supplied))
      tmp->hints.flags = 0;

    if (tmp->hints.flags & PResizeInc) {
	if (tmp->hints.width_inc == 0) tmp->hints.width_inc = 1;
	if (tmp->hints.height_inc == 0) tmp->hints.height_inc = 1;
    }

    if (!(supplied & PWinGravity) && (tmp->hints.flags & USPosition)) {
	static int gravs[] = { SouthEastGravity, SouthWestGravity,
			       NorthEastGravity, NorthWestGravity };
	int right =  tmp->attr.x + tmp->attr.width + 2 * tmp->old_bw;
	int bottom = tmp->attr.y + tmp->attr.height + 2 * tmp->old_bw;

	tmp->hints.win_gravity =
	  gravs[((Scr->MyDisplayHeight - bottom <
		tmp->title_height + 2 * tmp->frame_bw3D) ? 0 : 2) |
		((Scr->MyDisplayWidth - right   <
		tmp->title_height + 2 * tmp->frame_bw3D) ? 0 : 1)];

	tmp->hints.flags |= PWinGravity;
    }
}
