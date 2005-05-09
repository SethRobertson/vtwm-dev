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
#include "twm.h"
#include <X11/Xatom.h>
#include "add_window.h"
#include "util.h"
#include "resize.h"
#include "parse.h"

/* djhjr - 4/19/96 */
#include "gram.h"

#include "list.h"
#include "events.h"
#include "menus.h"
#include "screen.h"
#include "iconmgr.h"
#include "desktop.h"

#define gray_width 2
#define gray_height 2
static char gray_bits[] = {
   0x02, 0x01};

/* djhjr - 4/19/96 */
static unsigned char black_bits[] = {
   0xFF, 0xFF};

int AddingX;
int AddingY;
int AddingW;
int AddingH;

static int PlaceX = 50;
static int PlaceY = 50;
static void CreateWindowTitlebarButtons();
void SetHighlightPixmap();

char NoName[] = "Untitled"; /* name if no name is specified */


/************************************************************************
 *
 *  Procedure:
 *	GetGravityOffsets - map gravity to (x,y) offset signs for adding
 *		to x and y when window is mapped to get proper placement.
 *
 ************************************************************************
 */

void GetGravityOffsets (tmp, xp, yp)
    TwmWindow *tmp;			/* window from which to get gravity */
    int *xp, *yp;			/* return values */
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
AddWindow(w, iconm, iconp)
Window w;
int iconm;
IconMgr *iconp;
{
    TwmWindow *tmp_win;			/* new twm window structure */
    int stat;
    XEvent event;
    unsigned long valuemask;		/* mask for create windows */
    XSetWindowAttributes attributes;	/* attributes for create windows */
    XTextProperty text_property;
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytesafter;
    int ask_user;		/* don't know where to put the window */
    int gravx, gravy;			/* gravity signs for positioning */
    int namelen;
    int bw2;

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

	/*
	 * Ask the window manager for a name with "newer" R4 function -
	 * it was 'XFetchName()', which apparently failed more often.
	 * Submitted by Nicholas Jacobs
	 */
	if (XGetWMName(dpy, tmp_win->w, &text_property) != 0)
		tmp_win->name = (char *)text_property.value;
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
	tmp_win->group = tmp_win->w/* NULL */;

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
	(!(short)(int) LookInList(Scr->NoHighlight, tmp_win->full_name,
	    &tmp_win->class));

    tmp_win->stackmode = Scr->StackMode &&
	(!(short)(int) LookInList(Scr->NoStackModeL, tmp_win->full_name,
	    &tmp_win->class));

    tmp_win->titlehighlight = Scr->TitleHighlight &&
	(!(short)(int) LookInList(Scr->NoTitleHighlight, tmp_win->full_name,
	    &tmp_win->class));

    tmp_win->auto_raise =
    	Scr->AutoRaiseDefault ||    /* RAISEDELAY */
       		(short)(int) LookInList(Scr->AutoRaise,
				tmp_win->full_name,
					&tmp_win->class);
    if (tmp_win->auto_raise) Scr->NumAutoRaises++;
    tmp_win->iconify_by_unmapping = Scr->IconifyByUnmapping;
    if (Scr->IconifyByUnmapping)
    {
	tmp_win->iconify_by_unmapping = iconm ? FALSE :
	    !(short)(int) LookInList(Scr->DontIconify, tmp_win->full_name,
		&tmp_win->class);
    }
    tmp_win->iconify_by_unmapping |=
	(short)(int) LookInList(Scr->IconifyByUn, tmp_win->full_name,
	    &tmp_win->class);

    if ( Scr->UseWindowRing
    || LookInList(Scr->WindowRingL, tmp_win->full_name, &tmp_win->class)) {
	if (Scr->Ring) {
	    tmp_win->ring.next = Scr->Ring->ring.next;
	    if (Scr->Ring->ring.next->ring.prev)
	      Scr->Ring->ring.next->ring.prev = tmp_win;
	    Scr->Ring->ring.next = tmp_win;
	    tmp_win->ring.prev = Scr->Ring;
	} else {
	    tmp_win->ring.next = tmp_win->ring.prev = Scr->Ring = tmp_win;
	}
    } else
      tmp_win->ring.next = tmp_win->ring.prev = NULL;
    tmp_win->ring.cursor_valid = False;

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

	/* djhjr - 4/19/96 */
    tmp_win->frame_bw3D = Scr->ThreeDBorderWidth;
    if (LookInList(Scr->NoBorder, tmp_win->full_name, &tmp_win->class)) {
	tmp_win->frame_bw = 0;
	tmp_win->frame_bw3D = 0;
    }
    else
    if (tmp_win->frame_bw3D != 0) {
	tmp_win->frame_bw = 0;
	Scr->ClientBorderWidth = FALSE;
    }
    else

    if (Scr->ClientBorderWidth) {
    	tmp_win->frame_bw = tmp_win->old_bw;
    } else {
    	tmp_win->frame_bw = Scr->BorderWidth;
    }
    bw2 = tmp_win->frame_bw * 2;

    tmp_win->title_height = Scr->TitleHeight + tmp_win->frame_bw;
    if (Scr->NoTitlebar)
        tmp_win->title_height = 0;
    if (LookInList(Scr->MakeTitle, tmp_win->full_name, &tmp_win->class))
        tmp_win->title_height = Scr->TitleHeight + tmp_win->frame_bw;
    if (LookInList(Scr->NoTitle, tmp_win->full_name, &tmp_win->class))
        tmp_win->title_height = 0;

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
    ask_user = TRUE;
    if (tmp_win->transient ||
	(tmp_win->hints.flags & USPosition) ||
        ((tmp_win->hints.flags & PPosition) && Scr->UsePPosition &&
	 (Scr->UsePPosition == PPOS_ON ||
	  tmp_win->attr.x != 0 || tmp_win->attr.y != 0)))
      ask_user = FALSE;

    if (LookInList(Scr->NailedDown, tmp_win->full_name, &tmp_win->class))
	    tmp_win->nailed = TRUE;
    else
	    tmp_win->nailed = FALSE;

    /* 25/09/90 - nailed windows should always be on the real screen,
     * regardless of PPosition or UPosition */
    /* if we are dealing with PPosition, then offset by the current real
     * screen offset on the vd */
    if (tmp_win->nailed ||
	((tmp_win->hints.flags & PPosition) && (ask_user == FALSE))) {
	    tmp_win->attr.x = R_TO_V_X(tmp_win->attr.x);
	    tmp_win->attr.y = R_TO_V_Y(tmp_win->attr.y);
    }

    /*
     * do any prompting for position
     */
    if (HandlingEvents && ask_user) {
      if (Scr->RandomPlacement) {	/* just stick it somewhere */
	if ((PlaceX + tmp_win->attr.width) > Scr->MyDisplayWidth)
	    PlaceX = 50;
	if ((PlaceY + tmp_win->attr.height) > Scr->MyDisplayHeight)
	    PlaceY = 50;

	tmp_win->attr.x = PlaceX;
	tmp_win->attr.y = PlaceY;
	PlaceX += 30;
	PlaceY += 30;
      } else {				/* else prompt */
	if (!(tmp_win->wmhints && tmp_win->wmhints->flags & StateHint &&
	      tmp_win->wmhints->initial_state == IconicState))
	{
	    Bool firsttime = True;

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

/* use initialized size... djhjr - 5/9/96
	    width = (SIZE_HINDENT + XTextWidth (Scr->SizeFont.font,
						tmp_win->name, namelen));
	    height = Scr->SizeFont.height + SIZE_VINDENT * 2;

* djhjr - 4/27/96
	    XResizeWindow (dpy, Scr->SizeWindow, width + SIZE_HINDENT, height);
*
	    XResizeWindow (dpy, Scr->SizeWindow, Scr->SizeStringOffset +
				Scr->SizeStringWidth, height);
*/

	    XMapRaised(dpy, Scr->SizeWindow);
	    InstallRootColormap();

/* DisplayPosition overwrites it anyway... djhjr - 5/9/96
	    FBF(Scr->DefaultC.fore, Scr->DefaultC.back,
		Scr->SizeFont.font->fid);
	    XDrawImageString (dpy, Scr->SizeWindow, Scr->NormalGC,
			      SIZE_HINDENT,
			      SIZE_VINDENT + Scr->SizeFont.font->ascent,
			      tmp_win->name, namelen);
*/

/* djhjr - 4/19/96
	    AddingW = tmp_win->attr.width + bw2;
	    AddingH = tmp_win->attr.height + tmp_win->title_height + bw2;

		MoveOutline(Scr->Root, AddingX, AddingY, AddingW, AddingH,
			    tmp_win->frame_bw, tmp_win->title_height);
*/
	    AddingW = tmp_win->attr.width + bw2 + 2 * tmp_win->frame_bw3D;
	    AddingH = tmp_win->attr.height + tmp_win->title_height +
				bw2 + 2 * tmp_win->frame_bw3D;
		MoveOutline(Scr->Root, AddingX, AddingY, AddingW, AddingH,
			    tmp_win->frame_bw, tmp_win->title_height + tmp_win->frame_bw3D);

/* DisplayPosition() overwrites it anyway... djhjr - 5/9/96
		* djhjr - 4/27/96 *
	    XDrawImageString (dpy, Scr->SizeWindow, Scr->NormalGC, width,
				SIZE_VINDENT + Scr->SizeFont.font->ascent, ": ", 2);
*/

		/* djhjr - 4/27/96 */
	    DisplayPosition (tmp_win, AddingX, AddingY);

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

/* djhjr - 4/19/96
		MoveOutline(Scr->Root, AddingX, AddingY, AddingW, AddingH,
			    tmp_win->frame_bw, tmp_win->title_height);
*/
		MoveOutline(Scr->Root, AddingX, AddingY, AddingW, AddingH,
			    tmp_win->frame_bw, tmp_win->title_height + tmp_win->frame_bw3D);

		/* djhjr - 4/27/96 */
		DisplayPosition (tmp_win, AddingX, AddingY);

	    }

	    if (event.xbutton.button == Button2) {
		int lastx, lasty;

/* AddStartResize() overwrites it anyway... djhjr - 5/9/96
		Scr->SizeStringOffset = width +
		  XTextWidth(Scr->SizeFont.font, ": ", 2);
		XResizeWindow (dpy, Scr->SizeWindow, Scr->SizeStringOffset +
			       Scr->SizeStringWidth, height);
		XDrawImageString (dpy, Scr->SizeWindow, Scr->NormalGC, width,
				  SIZE_VINDENT + Scr->SizeFont.font->ascent,
				  ": ", 2);
*/

#if 0
		if (0/*Scr->AutoRelativeResize*/)
My R5 vtvwm came with this commented out, always 0.
Why?
#endif
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
	    } /* if (event.xbutton.button == Button2) */
	    else if (event.xbutton.button == Button3)
	    {
		int maxw = Scr->MyDisplayWidth - AddingX - bw2;
		int maxh = Scr->MyDisplayHeight - AddingY - bw2;

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
	    }
	    else
	    {
		XMaskEvent(dpy, ButtonReleaseMask, &event);
	    }

	    MoveOutline(Scr->Root, 0, 0, 0, 0, 0, 0);
	    XUnmapWindow(dpy, Scr->SizeWindow);
	    UninstallRootColormap();
	    XUngrabPointer(dpy, CurrentTime);

	    tmp_win->attr.x = AddingX;
	    tmp_win->attr.y = AddingY + tmp_win->title_height;

/* djhjr - 4/19/96
	    tmp_win->attr.width = AddingW - bw2;
	    tmp_win->attr.height = AddingH - tmp_win->title_height - bw2;
*/
	    tmp_win->attr.width = AddingW - bw2 - 2 * tmp_win->frame_bw3D;
	    tmp_win->attr.height = AddingH - tmp_win->title_height -
				bw2 - 2 * tmp_win->frame_bw3D;

	    XUngrabServer(dpy);
	}
      }
    } else {				/* put it where asked, mod title bar */
	    /* interpret the position specified as a virtual one if asked */
#if 0
	    if (Scr->GeometriesAreVirtual)
#endif
/* added 'FixManagedVirtualGeometries' - djhjr - 1/6/98 */
		if (Scr->GeometriesAreVirtual ||
			(!Scr->GeometriesAreVirtual &&
				(tmp_win->nailed ||
				(Scr->FixManagedVirtualGeometries && !tmp_win->transient ||
				(Scr->FixTransientVirtualGeometries && tmp_win->transient))
		)))

		/* if NotVirtualGeometries is set, should also do this if it's
		   a sticky or (it's a transient window and
		   FixTransientVirtualGeometries is set -- this case is a
		   bug workaround; any better ideas???????????) -- DSE */
	    	{
		    tmp_win->attr.x = V_TO_R_X(tmp_win->attr.x);
		    tmp_win->attr.y = V_TO_R_Y(tmp_win->attr.y);
		    }

	/* if the gravity is towards the top, move it by the title height */
	if (gravy < 0) tmp_win->attr.y -= gravy * tmp_win->title_height;
    }


#ifdef DEBUG
	fprintf(stderr, "  position window  %d, %d  %dx%d\n",
	    tmp_win->attr.x,
	    tmp_win->attr.y,
	    tmp_win->attr.width,
	    tmp_win->attr.height);
#endif

    if (!Scr->ClientBorderWidth) {	/* need to adjust for twm borders */

/* djhjr - 4/19/96
	int delta = tmp_win->attr.border_width - tmp_win->frame_bw;
*/
	int delta = tmp_win->attr.border_width - tmp_win->frame_bw - tmp_win->frame_bw3D;

	tmp_win->attr.x += gravx * delta;
	tmp_win->attr.y += gravy * delta;
    }

    tmp_win->title_width = tmp_win->attr.width;

    if (tmp_win->old_bw) XSetWindowBorderWidth (dpy, tmp_win->w, 0);

    tmp_win->name_width = XTextWidth(Scr->TitleBarFont.font, tmp_win->name,
				     namelen);

    if (XGetWindowProperty (dpy, tmp_win->w, XA_WM_ICON_NAME, 0L, 200L, False,
			    XA_STRING, &actual_type, &actual_format, &nitems,
			    &bytesafter,(unsigned char **)&tmp_win->icon_name))
	tmp_win->icon_name = tmp_win->name;

    if (tmp_win->icon_name == NULL)
	tmp_win->icon_name = tmp_win->name;

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

    /* add the window into the twm list */
    tmp_win->next = Scr->TwmRoot.next;
    if (Scr->TwmRoot.next != NULL)
	Scr->TwmRoot.next->prev = tmp_win;
    tmp_win->prev = &Scr->TwmRoot;
    Scr->TwmRoot.next = tmp_win;

    /* get all the colors for the window */

/* djhjr - 4/25/96
    tmp_win->border = Scr->BorderColor;
*/
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

/* djhjr - 4/25/96
    GetColorFromList(Scr->BorderColorL, tmp_win->full_name, &tmp_win->class,
	&tmp_win->border);
*/
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
    if (!GetColorFromList(Scr->VirtualDesktopColorFL, tmp_win->full_name,
		     &tmp_win->class, &tmp_win->virtual.fore))
	    GetColorFromList(Scr->TitleBackgroundL, tmp_win->full_name,
			     &tmp_win->class, &tmp_win->virtual.fore);
    if (!GetColorFromList(Scr->VirtualDesktopColorBL, tmp_win->full_name,
		     &tmp_win->class, &tmp_win->virtual.back))
	    GetColorFromList(Scr->TitleForegroundL, tmp_win->full_name,
			     &tmp_win->class, &tmp_win->virtual.back);

	/* djhjr - 4/19/96 */
    if (Scr->use3Dtitles  && !Scr->BeNiceToColormap) GetShadeColors (&tmp_win->title);
    if (Scr->ButtonColorIsFrame || (Scr->use3Dborders && !Scr->BeNiceToColormap)) {
	GetShadeColors (&tmp_win->border);
	GetShadeColors (&tmp_win->border_tile);
    }

    /* create windows */

/* djhjr - 4/19/96
    tmp_win->frame_x = tmp_win->attr.x + tmp_win->old_bw - tmp_win->frame_bw;
    tmp_win->frame_y = tmp_win->attr.y - tmp_win->title_height +
	tmp_win->old_bw - tmp_win->frame_bw;
    tmp_win->frame_width = tmp_win->attr.width;
    tmp_win->frame_height = tmp_win->attr.height + tmp_win->title_height;
*/
    tmp_win->frame_x = tmp_win->attr.x + tmp_win->old_bw - tmp_win->frame_bw
			- tmp_win->frame_bw3D;
    tmp_win->frame_y = tmp_win->attr.y - tmp_win->title_height +
	tmp_win->old_bw - tmp_win->frame_bw - tmp_win->frame_bw3D;
    tmp_win->frame_width = tmp_win->attr.width + 2 * tmp_win->frame_bw3D;
    tmp_win->frame_height = tmp_win->attr.height + tmp_win->title_height +
				2 * tmp_win->frame_bw3D;
    ConstrainSize (tmp_win, &tmp_win->frame_width, &tmp_win->frame_height);

    tmp_win->virtual_frame_x = R_TO_V_X(tmp_win->frame_x);
    tmp_win->virtual_frame_y = R_TO_V_Y(tmp_win->frame_y);

/* djhjr - 4/19/96
    valuemask = CWBackPixmap | CWBorderPixel | CWCursor | CWEventMask;
    attributes.background_pixmap = None;
    attributes.border_pixel = tmp_win->border;
    attributes.cursor = Scr->FrameCursor;
    attributes.event_mask = (SubstructureRedirectMask |
			     ButtonPressMask | ButtonReleaseMask |
			     EnterWindowMask | LeaveWindowMask);
    if (tmp_win->attr.save_under) {
	attributes.save_under = True;
	valuemask |= CWSaveUnder;
    }
*/
/* djhjr - 9/14/96
    valuemask = CWBackPixmap | CWBorderPixel | CWCursor | CWEventMask | CWBackPixel;
    attributes.background_pixmap = None;
*/
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

	/* djhjr - 9/17/96 - slows down iconify/delete/destroy too much...
	if (Scr->BackingStore)
	{
		attributes.backing_store = WhenMapped;
		valuemask |= CWBackingStore;
	}
	*/

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

    if (tmp_win->title_height)
    {
	valuemask = (CWEventMask | CWBorderPixel | CWBackPixel);
	attributes.event_mask = (KeyPressMask | ButtonPressMask |
				 ButtonReleaseMask | ExposureMask);
/* djhjr - 4/19/96
	attributes.border_pixel = tmp_win->border;
*/
	attributes.border_pixel = tmp_win->title.back;
	attributes.background_pixel = tmp_win->title.back;

	/* djhjr - 9/17/96 */
	if (Scr->BackingStore)
	{
		attributes.backing_store = WhenMapped;
		valuemask |= CWBackingStore;
	}

	tmp_win->title_w = XCreateWindow (dpy, tmp_win->frame,
/* djhjr - 4/19/96
					  -tmp_win->frame_bw,
					  -tmp_win->frame_bw,
*/
					  tmp_win->frame_bw3D - tmp_win->frame_bw,
					  tmp_win->frame_bw3D - tmp_win->frame_bw,

					  (unsigned int) tmp_win->attr.width,
					  (unsigned int) Scr->TitleHeight,
					  (unsigned int) tmp_win->frame_bw,
					  Scr->d_depth,
					  (unsigned int) CopyFromParent,
					  Scr->d_visual, valuemask,
					  &attributes);
    }
    else {
	tmp_win->title_w = 0;
	tmp_win->squeeze_info = NULL;
    }

    if (tmp_win->highlight)
    {

	/* djhjr - 4/19/96 */
	if (Scr->use3Dtitles && (Scr->Monochrome != COLOR))
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


    if (tmp_win->title_w) {
	CreateWindowTitlebarButtons (tmp_win);
	ComputeTitleLocation (tmp_win);
	XMoveWindow (dpy, tmp_win->title_w,
		     tmp_win->title_x, tmp_win->title_y);
	XDefineCursor(dpy, tmp_win->title_w, Scr->TitleCursor);
    }

	/* djhjr - 4/19/96 */
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

    if (tmp_win->title_w) {
	XMapWindow (dpy, tmp_win->title_w);
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

/* djhjr - 4/19/96
    XReparentWindow(dpy, tmp_win->w, tmp_win->frame, 0, tmp_win->title_height);
*/
    XReparentWindow(dpy, tmp_win->w, tmp_win->frame, tmp_win->frame_bw3D,
		tmp_win->title_height + tmp_win->frame_bw3D);

    /*
     * Reparenting generates an UnmapNotify event, followed by a MapNotify.
     * Set the map state to FALSE to prevent a transition back to
     * WithdrawnState in HandleUnmapNotify.  Map state gets set correctly
     * again in HandleMapNotify.
     */
    tmp_win->mapped = FALSE;

    SetupFrame (tmp_win, tmp_win->frame_x, tmp_win->frame_y,
		tmp_win->frame_width, tmp_win->frame_height, -1, True);

    /* wait until the window is iconified and the icon window is mapped
     * before creating the icon window
     */
    tmp_win->icon_w = NULL;

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

	XSaveContext(dpy, tmp_win->title_w, TwmContext, (caddr_t) tmp_win);
	XSaveContext(dpy, tmp_win->title_w, ScreenContext, (caddr_t) Scr);
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
    if (RootFunction)
	ReGrab();

	Scr->Newest = tmp_win; /* PF */
	if (tmp_win->transient && Scr->WarpToTransients) /* PF */
		WarpToWindow(tmp_win); /* PF,DSE */

    return (tmp_win);
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
MappedNotOverride(w)
    Window w;
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
static void do_add_binding (button, context, modifier, func)
    int button, context, modifier;
    int func;
{
    MouseButton *mb = &Scr->Mouse[button][context][modifier];

    if (mb->func) return;		/* already defined */

    mb->func = func;
    mb->item = NULL;
}

void AddDefaultBindings ()
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
GrabButtons(tmp_win)
TwmWindow *tmp_win;
{
    int i, j;

    for (i = 0; i < MAX_BUTTONS+1; i++)
    {
	for (j = 0; j < MOD_SIZE; j++)
	{
	    if (Scr->Mouse[i][C_WINDOW][j].func != NULL)
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
GrabKeys(tmp_win)
TwmWindow *tmp_win;
{
    FuncKey *tmp;
    IconMgr *p;

    for (tmp = Scr->FuncKeyRoot.next; tmp != NULL; tmp = tmp->next)
    {
	switch (tmp->cont)
	{
	case C_WINDOW:
	    XGrabKey(dpy, tmp->keycode, tmp->mods, tmp_win->w, True,
		GrabModeAsync, GrabModeAsync);
	    break;

	case C_ICON:
	    if (tmp_win->icon_w)
		XGrabKey(dpy, tmp->keycode, tmp->mods, tmp_win->icon_w, True,
		    GrabModeAsync, GrabModeAsync);

	case C_TITLE:
	    if (tmp_win->title_w)
		XGrabKey(dpy, tmp->keycode, tmp->mods, tmp_win->title_w, True,
		    GrabModeAsync, GrabModeAsync);
	    break;

	case C_NAME:
	    XGrabKey(dpy, tmp->keycode, tmp->mods, tmp_win->w, True,
		GrabModeAsync, GrabModeAsync);
	    if (tmp_win->icon_w)
		XGrabKey(dpy, tmp->keycode, tmp->mods, tmp_win->icon_w, True,
		    GrabModeAsync, GrabModeAsync);
	    if (tmp_win->title_w)
		XGrabKey(dpy, tmp->keycode, tmp->mods, tmp_win->title_w, True,
		    GrabModeAsync, GrabModeAsync);
	    break;
	/*
	case C_ROOT:
	    XGrabKey(dpy, tmp->keycode, tmp->mods, Scr->Root, True,
		GrabModeAsync, GrabModeAsync);
	    break;
	*/
	}
    }
    for (tmp = Scr->FuncKeyRoot.next; tmp != NULL; tmp = tmp->next)
    {
	if (tmp->cont == C_ICONMGR && !Scr->NoIconManagers)
	{
	    for (p = &Scr->iconmgr; p != NULL; p = p->next)
	    {
		XUngrabKey(dpy, tmp->keycode, tmp->mods, p->twm_win->w);
	    }
	}
    }
}

static Window CreateHighlightWindow (tmp_win)
    TwmWindow *tmp_win;
{
    XSetWindowAttributes attributes;	/* attributes for create windows */
    Pixmap pm = None;
    GC gc;
    XGCValues gcv;
    unsigned long valuemask;
    int h = (Scr->TitleHeight - 2 * Scr->FramePadding);
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
    if (!Scr->hilitePm) {

	/* djhjr - 4/20/96 */
	if (Scr->use3Dtitles && (Scr->Monochrome != COLOR))
	    Scr->hilitePm = XCreateBitmapFromData (dpy, tmp_win->title_w, 
					(char *)black_bits, gray_width, gray_height);
	else
	    Scr->hilitePm = XCreateBitmapFromData (dpy, tmp_win->title_w, 
					gray_bits, gray_width, gray_height);

	Scr->hilite_pm_width = gray_width;
	Scr->hilite_pm_height = gray_height;
    }
    if (Scr->hilitePm) {
	pm = XCreatePixmap (dpy, tmp_win->title_w,
			    Scr->hilite_pm_width, Scr->hilite_pm_height,
			    Scr->d_depth);
	gcv.foreground = tmp_win->title.fore;
	gcv.background = tmp_win->title.back;
	gcv.graphics_exposures = False;
	gc = XCreateGC (dpy, pm,
			(GCForeground|GCBackground|GCGraphicsExposures),
			&gcv);
	if (gc) {
	    XCopyPlane (dpy, Scr->hilitePm, pm, gc, 0, 0,
			Scr->hilite_pm_width, Scr->hilite_pm_height,
			0, 0, 1);
	    XFreeGC (dpy, gc);
	} else {
	    XFreePixmap (dpy, pm);
	    pm = None;
	}
    }
    if (pm) {
	valuemask = CWBackPixmap;
	attributes.background_pixmap = pm;
    } else {
	valuemask = CWBackPixel;
	attributes.background_pixel = tmp_win->title.fore;
    }

	/* djhjr - 4/19/96 */
    if (Scr->use3Dtitles)
/* djhjr - 4/25/96
	w = XCreateWindow (dpy, tmp_win->title_w, 0, Scr->FramePadding + 2,
		       (unsigned int) Scr->TBInfo.width, (unsigned int) (h - 4),
*/
	w = XCreateWindow (dpy, tmp_win->title_w, 0, Scr->FramePadding + 3,
		       (unsigned int) Scr->TBInfo.width, (unsigned int) (h - 6),

		       (unsigned int) 0,
		       Scr->d_depth, (unsigned int) CopyFromParent,
		       Scr->d_visual, valuemask, &attributes);
    else

    w = XCreateWindow (dpy, tmp_win->title_w, 0, Scr->FramePadding,
		       (unsigned int) Scr->TBInfo.width, (unsigned int) h,
		       (unsigned int) 0,
		       Scr->d_depth, (unsigned int) CopyFromParent,
		       Scr->d_visual, valuemask, &attributes);
    if (pm) XFreePixmap (dpy, pm);
    return w;
}


void ComputeCommonTitleOffsets ()
{
    int buttonwidth = (Scr->TBInfo.width + Scr->TBInfo.pad);

    Scr->TBInfo.leftx = Scr->TBInfo.rightoff = Scr->FramePadding;
    if (Scr->TBInfo.nleft > 0)
      Scr->TBInfo.leftx += Scr->ButtonIndent;
    Scr->TBInfo.titlex = (Scr->TBInfo.leftx +
			  (Scr->TBInfo.nleft * buttonwidth) - Scr->TBInfo.pad +
			  Scr->TitlePadding);
    if (Scr->TBInfo.nright > 0)
      Scr->TBInfo.rightoff += (Scr->ButtonIndent +
			       ((Scr->TBInfo.nright * buttonwidth) -
				Scr->TBInfo.pad));
    return;
}

void ComputeWindowTitleOffsets (tmp_win, width, squeeze)
    TwmWindow *tmp_win;
	int width;
    Bool squeeze;
{
    tmp_win->highlightx = (Scr->TBInfo.titlex + tmp_win->name_width);

	/* djhjr - 4/19/96
    if (Scr->use3Dtitles) tmp_win->highlightx += 6;
	*/

    if (tmp_win->hilite_w || Scr->TBInfo.nright > 0)
      tmp_win->highlightx += Scr->TitlePadding;

    tmp_win->rightx = width - Scr->TBInfo.rightoff;

	if (squeeze && tmp_win->squeeze_info)
	{
		int rx;

		/* djhjr - 3/13/97 */
		if (Scr->use3Dtitles)
		{
			rx = (tmp_win->highlightx + Scr->TBInfo.width * 2 +
				(Scr->TBInfo.nright > 0 ? Scr->TitlePadding : 0) +
				Scr->FramePadding);
		}
		else
		{
			rx = (tmp_win->highlightx +
				(tmp_win->hilite_w ? Scr->TBInfo.width * 2 : 0) +
				(Scr->TBInfo.nright > 0 ? Scr->TitlePadding : 0) +
				Scr->FramePadding);
		}

		if (rx < tmp_win->rightx) tmp_win->rightx = rx;
    }

    return;
}


/*
 * ComputeTitleLocation - calculate the position of the title window.
 *
 * substantially re-written - djhjr - 1/8/98
 */
void ComputeTitleLocation (tmp)
    TwmWindow *tmp;
{
	tmp->title_x = tmp->frame_bw3D - tmp->frame_bw;
	tmp->title_y = tmp->frame_bw3D - tmp->frame_bw;

	if (tmp->squeeze_info)
	{
		SqueezeInfo *si = tmp->squeeze_info;
		int basex;
		int fw = tmp->frame_bw + tmp->frame_bw3D;

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


static void CreateWindowTitlebarButtons (tmp_win)
    TwmWindow *tmp_win;
{
    unsigned long valuemask;		/* mask for create windows */
    XSetWindowAttributes attributes;	/* attributes for create windows */
    int leftx, rightx, y;
    TitleButton *tb;
    int nb;

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

    leftx = y = Scr->TBInfo.leftx;
    rightx = tmp_win->rightx;

    attributes.background_pixel = tmp_win->title.back;
    attributes.border_pixel = tmp_win->title.fore;
    attributes.event_mask = (ButtonPressMask | ButtonReleaseMask |
			     ExposureMask);
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
	    TBWindow *tbw;
	    int boxwidth = (Scr->TBInfo.width + Scr->TBInfo.pad);
	    unsigned int h = (Scr->TBInfo.width - Scr->TBInfo.border * 2);

	    for (tb = Scr->TBInfo.head, tbw = tmp_win->titlebuttons; tb;
		 tb = tb->next, tbw++) {
		int x;
		if (tb->rightside) {
		    x = rightx;
		    rightx += boxwidth;
		    attributes.win_gravity = NorthEastGravity;
		} else {
		    x = leftx;
		    leftx += boxwidth;
		    attributes.win_gravity = NorthWestGravity;
		}

		tbw->window = XCreateWindow (dpy, tmp_win->title_w, x, y, h, h,
					     (unsigned int) Scr->TBInfo.border,
					     0, (unsigned int) CopyFromParent,
					     (Visual *) CopyFromParent,
					     valuemask, &attributes);

		/* djhjr - 9/14/96 - moved from InitTitlebarButtons()... */
		/* djhjr - 9/21/96 */
		if (!(tb->image = GetImage (tb->name,
			(Scr->ButtonColorIsFrame) ? tmp_win->border : tmp_win->title)))
		    if (!(tb->image = GetImage (TBPM_QUESTION,
				(Scr->ButtonColorIsFrame) ? tmp_win->border : tmp_win->title)))
				fprintf (stderr, "%s:  unable to add titlebar button \"%s\"\n",
					ProgramName, tb->name);

		tb->width  = tb->image->width;
		tb->height = tb->image->height;

		tb->dstx = (h - tb->width + 1) / 2;
		if (tb->dstx < 0) {		/* clip to minimize copying */
	    	tb->srcx = -(tb->dstx);
		    tb->width = h;
		    tb->dstx = 0;
		} else {
		    tb->srcx = 0;
		}
		tb->dsty = (h - tb->height + 1) / 2;
		if (tb->dsty < 0) {
	    	tb->srcy = -(tb->dsty);
		    tb->height = h;
		    tb->dsty = 0;
		} else {
		    tb->srcy = 0;
		}
		/* ...end of moved */

#ifndef NO_XPM_SUPPORT
		/* djhjr - 3/21/98 */
		if (tb->image != NULL)
			if (tb->image->mask != None)
				XShapeCombineMask(dpy, tbw->window, ShapeBounding, 0, 0,
						tb->image->mask, ShapeSet);
#endif

		tbw->info = tb;
	    } /* end for(...) */
	}
    }

    tmp_win->hilite_w = (tmp_win->titlehighlight
			 ? CreateHighlightWindow (tmp_win) : None);

    XMapSubwindows(dpy, tmp_win->title_w);

/* djhjr - 4/25/96
    if (tmp_win->hilite_w)
      XUnmapWindow(dpy, tmp_win->hilite_w);
*/
	PaintTitleHighlight(tmp_win, off);

    return;
}


void SetHighlightPixmap (filename)
    char *filename;
{
    Pixmap pm = GetBitmap (filename);

    if (pm) {
	if (Scr->hilitePm) {
	    XFreePixmap (dpy, Scr->hilitePm);
	}
	Scr->hilitePm = pm;
	Scr->hilite_pm_width = JunkWidth;
	Scr->hilite_pm_height = JunkHeight;
    }
}


void FetchWmProtocols (tmp)
    TwmWindow *tmp;
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
CreateTwmColormap(c)
    Colormap c;
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
CreateColormapWindow(w, creating_parent, property_window)
    Window w;
    Bool creating_parent;
    Bool property_window;
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

void FetchWmColormapWindows (tmp)
    TwmWindow *tmp;
{
    register int i, j;
    Window *cmap_windows = NULL;
    Bool can_free_cmap_windows = False;
    int number_cmap_windows = 0;
    ColormapWindow **cwins = NULL;
    int previously_installed;
    extern void free_cwins();

    number_cmap_windows = 0;

    if (/* SUPPRESS 560 */previously_installed =
       (Scr->cmapInfo.cmaps == &tmp->cmaps && tmp->cmaps.number_cwins)) {
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


void GetWindowSizeHints (tmp)
    TwmWindow *tmp;
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

/* djhjr - 4/19/96
	tmp->hints.win_gravity =
	  gravs[((Scr->MyDisplayHeight - bottom < tmp->title_height) ? 0 : 2) |
		((Scr->MyDisplayWidth - right   < tmp->title_height) ? 0 : 1)];
*/
	tmp->hints.win_gravity = 
	  gravs[((Scr->MyDisplayHeight - bottom <
		tmp->title_height + 2 * tmp->frame_bw3D) ? 0 : 2) |
		((Scr->MyDisplayWidth - right   <
		tmp->title_height + 2 * tmp->frame_bw3D) ? 0 : 1)];

	tmp->hints.flags |= PWinGravity;
    }
}

