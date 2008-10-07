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
 * $XConsortium: resize.c,v 1.80 91/05/11 17:35:42 dave Exp $
 *
 * window resizing borrowed from the "wm" window manager
 *
 * 11-Dec-87 Thomas E. LaStrange                File created
 *
 ***********************************************************************/

#include <stdio.h>
#include "twm.h"
#include "parse.h"
#include "util.h"
#include "resize.h"
#include "add_window.h"
#include "screen.h"
#include "desktop.h"
#include "events.h"
#include "menus.h"

#define MINHEIGHT 0     /* had been 32 */
#define MINWIDTH 0      /* had been 60 */

static int dragx;       /* all these variables are used */
static int dragy;       /* in resize operations */
static int dragWidth;
static int dragHeight;

int origx;
int origy;
int origWidth;
int origHeight;

static int clampTop;
static int clampBottom;
static int clampLeft;
static int clampRight;
static int clampDX;
static int clampDY;

static int last_width;
static int last_height;

static int resize_context;

/* set in menus.c:ExecuteFunction(), cleared in *EndResize() - djhjr - 9/5/98 */
int resizing_window = 0;

/* djhjr - 4/6/98 */
void PaintBorderAndTitlebar();

/* djhjr - 4/17/98 */
static void DoVirtualMoveResize();

/* djhjr - 9/10/99 */
void ResizeTwmWindowContents();

/* djhjr - 9/13/02 */
static void SetVirtualDesktopIncrs();
static void EndResizeAdjPointer();

static void do_auto_clamp (tmp_win, evp)
    TwmWindow *tmp_win;
    XEvent *evp;
{
    Window junkRoot;
    int x, y, h, v, junkbw;
    unsigned int junkMask;

    switch (evp->type) {
      case ButtonPress:
	x = evp->xbutton.x_root;
	y = evp->xbutton.y_root;
	break;
      case KeyPress:
	x = evp->xkey.x_root;
	y = evp->xkey.y_root;
	break;
      default:
	if (!XQueryPointer (dpy, Scr->Root, &junkRoot, &junkRoot,
			    &x, &y, &junkbw, &junkbw, &junkMask))
	  return;
    }

    h = ((x - dragx) / (dragWidth < 3 ? 1 : (dragWidth / 3)));
    v = ((y - dragy - tmp_win->title_height) /
	 (dragHeight < 3 ? 1 : (dragHeight / 3)));

    if (h <= 0) {
	clampLeft = 1;
	clampDX = (x - dragx);
    } else if (h >= 2) {
	clampRight = 1;
	clampDX = (x - dragx - dragWidth);
    }

    if (v <= 0) {
	clampTop = 1;
	clampDY = (y - dragy);
    } else if (v >= 2) {
	clampBottom = 1;
	clampDY = (y - dragy - dragHeight);
    }
}


/***********************************************************************
 *
 *  Procedure:
 *      StartResize - begin a window resize operation
 *
 *  Inputs:
 *      ev      - the event structure (button press)
 *      tmp_win - the TwmWindow pointer
 *      fromtitlebar - action invoked from titlebar button
 *
 ***********************************************************************
 */

void
StartResize(evp, tmp_win, fromtitlebar, context)
XEvent *evp;
TwmWindow *tmp_win;
Bool fromtitlebar;
int context;
{
    Window      junkRoot;
    unsigned int junkbw, junkDepth;

    resize_context = context;

    SetVirtualDesktopIncrs(tmp_win);	/* djhjr - 9/13/02 */

    if (context == C_VIRTUAL_WIN)
	    ResizeWindow = tmp_win->VirtualDesktopDisplayWindow.win;
    else
	    ResizeWindow = tmp_win->frame;

/* djhjr - 7/17/98
	* djhjr - 4/15/98 *
	if (!Scr->NoGrabServer)
*/
	{
		/* added test - djhjr - 4/7/98 */
		if (!tmp_win->opaque_resize)
			XGrabServer(dpy);
	}
    if (context == C_VIRTUAL_WIN)
	    XGrabPointer(dpy, Scr->VirtualDesktopDisplay, True,
			 ButtonPressMask | ButtonReleaseMask |
			 ButtonMotionMask | PointerMotionHintMask,
			 GrabModeAsync, GrabModeAsync,
			 Scr->Root, Scr->ResizeCursor, CurrentTime);
    else
	    XGrabPointer(dpy, Scr->Root, True,
			 ButtonPressMask | ButtonReleaseMask |
			 ButtonMotionMask | PointerMotionHintMask,
			 GrabModeAsync, GrabModeAsync,
			 Scr->Root, Scr->ResizeCursor, CurrentTime);

    XGetGeometry(dpy, (Drawable) ResizeWindow, &junkRoot,
        &dragx, &dragy, (unsigned int *)&dragWidth, (unsigned int *)&dragHeight, &junkbw,
                 &junkDepth);

    if (context != C_VIRTUAL_WIN) {
	    dragx += tmp_win->frame_bw;
	    dragy += tmp_win->frame_bw;
    }
    origx = dragx;
    origy = dragy;
    origWidth = dragWidth;
    origHeight = dragHeight;
    clampTop = clampBottom = clampLeft = clampRight = clampDX = clampDY = 0;

    if (Scr->AutoRelativeResize && !fromtitlebar)
      do_auto_clamp (tmp_win, evp);

#ifdef TILED_SCREEN
    if (Scr->use_tiles == TRUE) {
	int k = FindNearestTileToMouse();
	XMoveWindow (dpy, Scr->SizeWindow.win, Lft(Scr->tiles[k]), Bot(Scr->tiles[k]));
    }
#endif

/* use initialized size... djhjr - 5/9/96
	Scr->SizeStringOffset = SIZE_HINDENT;
    XResizeWindow (dpy, Scr->SizeWindow.win,
		   Scr->SizeStringWidth + SIZE_HINDENT * 2,
		   Scr->SizeFont.height + SIZE_VINDENT * 2);
*/

    XMapRaised(dpy, Scr->SizeWindow.win);
    if (!tmp_win->opaque_resize) InstallRootColormap();
    last_width = 0;
    last_height = 0;
    DisplaySize(tmp_win, origWidth, origHeight);

    if (resize_context == C_VIRTUAL_WIN)
	    MoveOutline (Scr->VirtualDesktopDisplay, dragx,
			 dragy, dragWidth,
			 dragHeight,
			 tmp_win->frame_bw, 0);
    else
	/* added this 'if ... else' - djhjr - 4/6/98 */
	if (tmp_win->opaque_resize)
	{
		SetupWindow (tmp_win,
			dragx - tmp_win->frame_bw, dragy - tmp_win->frame_bw,
			dragWidth, dragHeight, -1);
		PaintBorderAndTitlebar(tmp_win);
	}
	else
	    MoveOutline (Scr->Root, dragx - tmp_win->frame_bw,
			 dragy - tmp_win->frame_bw, dragWidth + 2 * tmp_win->frame_bw,
			 dragHeight + 2 * tmp_win->frame_bw,
/* djhjr - 4/24/96
			 tmp_win->frame_bw, tmp_win->title_height);
*/
		tmp_win->frame_bw, tmp_win->title_height + tmp_win->frame_bw3D);
}



/* added the passed 'context' - djhjr - 2/22/99 */
void
MenuStartResize(tmp_win, x, y, w, h, context)
TwmWindow *tmp_win;
int x, y, w, h;
int context;
{
	/* djhjr - 2/22/99 */
	resize_context = context;

    SetVirtualDesktopIncrs(tmp_win);	/* djhjr - 9/13/02 */

/* djhjr - 7/17/98
	* djhjr - 4/15/98 *
	if (!Scr->NoGrabServer)
*/
	{
		/* added test - djhjr - 4/7/98 */
		if (!tmp_win->opaque_resize)
			XGrabServer(dpy);
	}
    XGrabPointer(dpy, Scr->Root, True,
        ButtonPressMask | ButtonMotionMask | PointerMotionMask,
        GrabModeAsync, GrabModeAsync,
        Scr->Root, Scr->ResizeCursor, CurrentTime);
    dragx = x + tmp_win->frame_bw;
    dragy = y + tmp_win->frame_bw;
    origx = dragx;
    origy = dragy;
    dragWidth = origWidth = w; /* - 2 * tmp_win->frame_bw; */
    dragHeight = origHeight = h; /* - 2 * tmp_win->frame_bw; */
    clampTop = clampBottom = clampLeft = clampRight = clampDX = clampDY = 0;
    last_width = 0;
    last_height = 0;

#ifdef TILED_SCREEN
    if (Scr->use_tiles == TRUE) {
	int k = FindNearestTileToMouse();
	XMoveWindow (dpy, Scr->SizeWindow.win, Lft(Scr->tiles[k]), Bot(Scr->tiles[k]));
    }
#endif

/* use initialized size... djhjr - 5/9/96
    Scr->SizeStringOffset = SIZE_HINDENT;
    XResizeWindow (dpy, Scr->SizeWindow.win,
		   Scr->SizeStringWidth + SIZE_HINDENT * 2,
		   Scr->SizeFont.height + SIZE_VINDENT * 2);
*/

    XMapRaised(dpy, Scr->SizeWindow.win);
    if (!tmp_win->opaque_resize) InstallRootColormap();
    DisplaySize(tmp_win, origWidth, origHeight);

	/* added this 'if ... else' - djhjr - 4/6/98 */
	if (tmp_win->opaque_resize)
	{
		SetupWindow (tmp_win,
			dragx - tmp_win->frame_bw, dragy - tmp_win->frame_bw,
			dragWidth, dragHeight, -1);
		PaintBorderAndTitlebar(tmp_win);
	}
	else
    MoveOutline (Scr->Root, dragx - tmp_win->frame_bw,
		 dragy - tmp_win->frame_bw,
		 dragWidth + 2 * tmp_win->frame_bw,
		 dragHeight + 2 * tmp_win->frame_bw,
/* djhjr - 4/23/96
		 tmp_win->frame_bw, tmp_win->title_height);
*/
		 tmp_win->frame_bw, tmp_win->title_height + tmp_win->frame_bw3D);
}

/***********************************************************************
 *
 *  Procedure:
 *      AddStartResize - begin a windorew resize operation from AddWindow
 *
 *  Inputs:
 *      tmp_win - the TwmWindow pointer
 *
 ***********************************************************************
 */

void
AddStartResize(tmp_win, x, y, w, h)
TwmWindow *tmp_win;
int x, y, w, h;
{
	/* djhjr - 2/22/99 */
	resize_context = C_WINDOW;

    SetVirtualDesktopIncrs(tmp_win);	/* djhjr - 9/13/02 */

/* djhjr - 7/17/98
	* djhjr - 4/15/98 *
	if (!Scr->NoGrabServer)
*/
	{
		/* added test - djhjr - 4/7/98 */
		if (!tmp_win->opaque_resize)
			XGrabServer(dpy);
	}

    XGrabPointer(dpy, Scr->Root, True,
        ButtonReleaseMask | ButtonMotionMask | PointerMotionHintMask,
        GrabModeAsync, GrabModeAsync,
        Scr->Root, Scr->ResizeCursor, CurrentTime);

    dragx = x + tmp_win->frame_bw;
    dragy = y + tmp_win->frame_bw;
    origx = dragx;
    origy = dragy;
    dragWidth = origWidth = w - 2 * tmp_win->frame_bw;
    dragHeight = origHeight = h - 2 * tmp_win->frame_bw;
    clampTop = clampBottom = clampLeft = clampRight = clampDX = clampDY = 0;
/*****
    if (Scr->AutoRelativeResize) {
	clampRight = clampBottom = 1;
    }
*****/
    last_width = 0;
    last_height = 0;
    DisplaySize(tmp_win, origWidth, origHeight);
}



/*
 * Functionally identical with DoResize(), except that this
 * handles a virtual window differently, but it isn't used anyway.
 * djhjr - 10/6/02
 */
#if 0
void
MenuDoResize(x_root, y_root, tmp_win)
int x_root;
int y_root;
TwmWindow *tmp_win;
{
    int action;

    action = 0;

    x_root -= clampDX;
    y_root -= clampDY;

    if (clampTop) {
        int         delta = y_root - dragy;
        if (dragHeight - delta < MINHEIGHT) {
            delta = dragHeight - MINHEIGHT;
            clampTop = 0;
        }
        dragy += delta;
        dragHeight -= delta;
        action = 1;
    }
    else if (y_root <= dragy/* ||
             y_root == findRootInfo(root)->rooty*/) {
        dragy = y_root;
        dragHeight = origy + origHeight -
            y_root;
        clampBottom = 0;
        clampTop = 1;
	clampDY = 0;
        action = 1;
    }
    if (clampLeft) {
        int         delta = x_root - dragx;
        if (dragWidth - delta < MINWIDTH) {
            delta = dragWidth - MINWIDTH;
            clampLeft = 0;
        }
        dragx += delta;
        dragWidth -= delta;
        action = 1;
    }
    else if (x_root <= dragx/* ||
             x_root == findRootInfo(root)->rootx*/) {
        dragx = x_root;
        dragWidth = origx + origWidth -
            x_root;
        clampRight = 0;
        clampLeft = 1;
	clampDX = 0;
        action = 1;
    }
    if (clampBottom) {
        int         delta = y_root - dragy - dragHeight;
        if (dragHeight + delta < MINHEIGHT) {
            delta = MINHEIGHT - dragHeight;
            clampBottom = 0;
        }
        dragHeight += delta;
        action = 1;
    }
    else if (y_root >= dragy + dragHeight - 1/* ||
           y_root == findRootInfo(root)->rooty
           + findRootInfo(root)->rootheight - 1*/) {
        dragy = origy;
        dragHeight = 1 + y_root - dragy;
        clampTop = 0;
        clampBottom = 1;
	clampDY = 0;
        action = 1;
    }
    if (clampRight) {
        int         delta = x_root - dragx - dragWidth;
        if (dragWidth + delta < MINWIDTH) {
            delta = MINWIDTH - dragWidth;
            clampRight = 0;
        }
        dragWidth += delta;
        action = 1;
    }
    else if (x_root >= dragx + dragWidth - 1/* ||
             x_root == findRootInfo(root)->rootx +
             findRootInfo(root)->rootwidth - 1*/) {
        dragx = origx;
        dragWidth = 1 + x_root - origx;
        clampLeft = 0;
        clampRight = 1;
	clampDX = 0;
        action = 1;
    }

    if (action) {
        ConstrainSize (tmp_win, &dragWidth, &dragHeight);
        if (clampLeft)
            dragx = origx + origWidth - dragWidth;
        if (clampTop)
            dragy = origy + origHeight - dragHeight;

	if (resize_context == C_VIRTUAL_WIN)
		MoveOutline(Scr->VirtualDesktopDisplay,
			    dragx,
			    dragy,
			    dragWidth,
			    dragHeight,
			    tmp_win->frame_bw, 0);
	else {
	/* added this 'if ... else' - djhjr - 4/6/98 */
	if (tmp_win->opaque_resize)
	{
		SetupWindow (tmp_win,
			dragx - tmp_win->frame_bw, dragy - tmp_win->frame_bw,
			dragWidth, dragHeight, -1);

		/* force the redraw of a door - djhjr - 2/28/99 */
		{
			TwmDoor *door;

			if (XFindContext(dpy, tmp_win->w, DoorContext, (caddr_t *)&door) != XCNOENT)
				RedoDoorName(tmp_win, door);
		}

		/* force the redraw of the desktop - djhjr - 2/28/99 */
		if (!strcmp(tmp_win->class.res_class, VTWM_DESKTOP_CLASS))
		{
			ResizeDesktopDisplay(dragWidth, dragHeight);

			Draw3DBorder(Scr->VirtualDesktopDisplayOuter, 0, 0,
					Scr->VirtualDesktopMaxWidth + (Scr->VirtualDesktopBevelWidth * 2),
					Scr->VirtualDesktopMaxHeight + (Scr->VirtualDesktopBevelWidth * 2),
					Scr->VirtualDesktopBevelWidth, Scr->VirtualC, off, False, False);
		}

		/* force the redraw of an icon manager - djhjr - 3/1/99 */
		if (tmp_win->iconmgr)
		{
			struct WList *list;
			int ncols = tmp_win->iconmgrp->cur_columns;
			if (ncols == 0) ncols = 1;

/* djhjr - 4/24/96
			tmp_win->iconmgrp->width = (int) ((dragWidth *
*/
			tmp_win->iconmgrp->width = (int) (((dragWidth - 2 * tmp_win->frame_bw3D) *

				   (long) tmp_win->iconmgrp->columns)
				  / ncols);
			PackIconManager(tmp_win->iconmgrp);

			list = tmp_win->iconmgrp->first;
			while (list)
			{
				RedoListWindow(list->twm);
				list = list->next;
			}
		}

		PaintBorderAndTitlebar(tmp_win);

		/* djhjr - 4/15/98 */
		/* added '&& !resizing_window' - djhjr - 11/7/03 */
		if (!Scr->NoGrabServer && !resizing_window)
		{
			/* these let the application window be drawn - djhjr - 4/14/98 */
			XUngrabServer(dpy); XSync(dpy, 0); XGrabServer(dpy);
		}
	}
	else
		MoveOutline(Scr->Root,
			    dragx - tmp_win->frame_bw,
			    dragy - tmp_win->frame_bw,
			    dragWidth + 2 * tmp_win->frame_bw,
			    dragHeight + 2 * tmp_win->frame_bw,
/* djhjr - 4/24/96
			    tmp_win->frame_bw, tmp_win->title_height);
*/
	    tmp_win->frame_bw, tmp_win->title_height + tmp_win->frame_bw3D);

		/* djhjr - 4/17/98 */
		if (Scr->VirtualReceivesMotionEvents)
			DoVirtualMoveResize(tmp_win, dragx, dragy, dragWidth, dragHeight);
	}
    }

    DisplaySize(tmp_win, dragWidth, dragHeight);
}
#endif

/***********************************************************************
 *
 *  Procedure:
 *      DoResize - move the rubberband around.  This is called for
 *                 each motion event when we are resizing
 *
 *  Inputs:
 *      x_root  - the X corrdinate in the root window
 *      y_root  - the Y corrdinate in the root window
 *      tmp_win - the current twm window
 *
 ***********************************************************************
 */

void
DoResize(x_root, y_root, tmp_win)
int x_root;
int y_root;
TwmWindow *tmp_win;
{
    int action;

    action = 0;

    x_root -= clampDX;
    y_root -= clampDY;

    if (clampTop) {
        int         delta = y_root - dragy;
        if (dragHeight - delta < MINHEIGHT) {
            delta = dragHeight - MINHEIGHT;
            clampTop = 0;
        }
        dragy += delta;
        dragHeight -= delta;
        action = 1;
    }
    else if (y_root <= dragy/* ||
             y_root == findRootInfo(root)->rooty*/) {
        dragy = y_root;
        dragHeight = origy + origHeight -
            y_root;
        clampBottom = 0;
        clampTop = 1;
	clampDY = 0;
        action = 1;
    }
    if (clampLeft) {
        int         delta = x_root - dragx;
        if (dragWidth - delta < MINWIDTH) {
            delta = dragWidth - MINWIDTH;
            clampLeft = 0;
        }
        dragx += delta;
        dragWidth -= delta;
        action = 1;
    }
    else if (x_root <= dragx/* ||
             x_root == findRootInfo(root)->rootx*/) {
        dragx = x_root;
        dragWidth = origx + origWidth -
            x_root;
        clampRight = 0;
        clampLeft = 1;
	clampDX = 0;
        action = 1;
    }
    if (clampBottom) {
        int         delta = y_root - dragy - dragHeight;
        if (dragHeight + delta < MINHEIGHT) {
            delta = MINHEIGHT - dragHeight;
            clampBottom = 0;
        }
        dragHeight += delta;
        action = 1;
    }
    else if (y_root >= dragy + dragHeight - 1/* ||
           y_root == findRootInfo(root)->rooty
           + findRootInfo(root)->rootheight - 1*/) {
        dragy = origy;
        dragHeight = 1 + y_root - dragy;
        clampTop = 0;
        clampBottom = 1;
	clampDY = 0;
        action = 1;
    }
    if (clampRight) {
        int         delta = x_root - dragx - dragWidth;
        if (dragWidth + delta < MINWIDTH) {
            delta = MINWIDTH - dragWidth;
            clampRight = 0;
        }
        dragWidth += delta;
        action = 1;
    }
    else if (x_root >= dragx + dragWidth - 1/* ||
             x_root == findRootInfo(root)->rootx +
             findRootInfo(root)->rootwidth - 1*/) {
        dragx = origx;
        dragWidth = 1 + x_root - origx;
        clampLeft = 0;
        clampRight = 1;
	clampDX = 0;
        action = 1;
    }

    if (action) {
        ConstrainSize (tmp_win, &dragWidth, &dragHeight);
        if (clampLeft)
            dragx = origx + origWidth - dragWidth;
        if (clampTop)
            dragy = origy + origHeight - dragHeight;

		/* added this 'if() ... else' - djhjr - 4/6/98 */
		if (tmp_win->opaque_resize)
		{
			SetupWindow(tmp_win,
				dragx - tmp_win->frame_bw, dragy - tmp_win->frame_bw,
				dragWidth, dragHeight, -1);

			/* force the redraw of a door - djhjr - 2/28/99 */
			{
				TwmDoor *door;

				if (XFindContext(dpy, tmp_win->w, DoorContext, (caddr_t *)&door) != XCNOENT)
					RedoDoorName(tmp_win, door);
			}

			/* force the redraw of the desktop - djhjr - 2/28/99 */
			if (!strcmp(tmp_win->class.res_class, VTWM_DESKTOP_CLASS))
			{
				ResizeDesktopDisplay(dragWidth, dragHeight);

				Draw3DBorder(Scr->VirtualDesktopDisplayOuter, 0, 0,
						Scr->VirtualDesktopMaxWidth + (Scr->VirtualDesktopBevelWidth * 2),
						Scr->VirtualDesktopMaxHeight + (Scr->VirtualDesktopBevelWidth * 2),
						Scr->VirtualDesktopBevelWidth, Scr->VirtualC, off, False, False);
			}

			/* force the redraw of an icon manager - djhjr - 3/1/99 */
			if (tmp_win->iconmgr)
			{
				struct WList *list;
				int ncols = tmp_win->iconmgrp->cur_columns;
				if (ncols == 0) ncols = 1;

/* djhjr - 4/24/96
				tmp_win->iconmgrp->width = (int) ((dragWidth *
*/
				tmp_win->iconmgrp->width = (int) (((dragWidth - 2 * tmp_win->frame_bw3D) *

					   (long) tmp_win->iconmgrp->columns)
					  / ncols);
				PackIconManager(tmp_win->iconmgrp);

				list = tmp_win->iconmgrp->first;
				while (list)
				{
					RedoListWindow(list->twm);
					list = list->next;
				}
			}

			PaintBorderAndTitlebar(tmp_win);

			/* djhjr - 4/15/98 */
			/* added '&& !resizing_window' - djhjr - 11/7/03 */
			if (!Scr->NoGrabServer && !resizing_window)
			{
				/* these let the application window be drawn - djhjr - 4/14/98 */
				XUngrabServer(dpy); XSync(dpy, 0); XGrabServer(dpy);
			}
		}
		else
        MoveOutline(Scr->Root,
            dragx - tmp_win->frame_bw,
            dragy - tmp_win->frame_bw,
            dragWidth + 2 * tmp_win->frame_bw,
            dragHeight + 2 * tmp_win->frame_bw,
/* djhjr - 4/24/96
	    tmp_win->frame_bw, tmp_win->title_height);
*/
	    tmp_win->frame_bw, tmp_win->title_height + tmp_win->frame_bw3D);

		/* djhjr - 4/17/98 */
		if (Scr->VirtualReceivesMotionEvents)
			DoVirtualMoveResize(tmp_win, dragx, dragy, dragWidth, dragHeight);
    }

    DisplaySize(tmp_win, dragWidth, dragHeight);
}

/* djhjr - 9/13/02 */
static void
SetVirtualDesktopIncrs(tmp_win)
TwmWindow *tmp_win;
{
    if (strcmp(tmp_win->class.res_class, VTWM_DESKTOP_CLASS) == 0)
    {
	if (Scr->snapRealScreen)
	{
	    Scr->VirtualDesktopDisplayTwin->hints.flags |= PResizeInc;
	    Scr->VirtualDesktopDisplayTwin->hints.width_inc =
			SCALE_D(Scr->VirtualDesktopPanDistanceX);
	    Scr->VirtualDesktopDisplayTwin->hints.height_inc =
			SCALE_D(Scr->VirtualDesktopPanDistanceY);
	}
	else
	    Scr->VirtualDesktopDisplayTwin->hints.flags &= ~PResizeInc;

	XSetWMNormalHints(dpy, tmp_win->w,
			&Scr->VirtualDesktopDisplayTwin->hints);
    }
}

/***********************************************************************
 *
 *  Procedure:
 *      DisplaySize - display the size in the dimensions window
 *
 *  Inputs:
 *      tmp_win - the current twm window
 *      width   - the width of the rubber band
 *      height  - the height of the rubber band
 *
 ***********************************************************************
 */

void
DisplaySize(tmp_win, width, height)
TwmWindow *tmp_win;
int width;
int height;
{
    char str[100];
    int i, dwidth, dheight;

    if (last_width == width && last_height == height)
        return;

    last_width = width;
    last_height = height;

    if (resize_context == C_VIRTUAL_WIN) {
	    dheight = SCALE_U(height) - tmp_win->title_height;
	    dwidth = SCALE_U(width);
    } else {
/* djhjr - 4/24/96
	    dheight = height - tmp_win->title_height;
	    dwidth = width;
*/
		dheight = height - tmp_win->title_height - 2 * tmp_win->frame_bw3D;
		dwidth = width - 2 * tmp_win->frame_bw3D;

    }

    /*
     * ICCCM says that PMinSize is the default is no PBaseSize is given,
     * and vice-versa.
     * Don't adjust if window is the virtual desktop - djhjr - 9/13/02
     */
    if (tmp_win->hints.flags&(PMinSize|PBaseSize) && tmp_win->hints.flags & PResizeInc)
    {
	if (tmp_win->hints.flags & PBaseSize)
	{
	    dwidth -= tmp_win->hints.base_width;
	    dheight -= tmp_win->hints.base_height;
	} else if (strcmp(tmp_win->class.res_class, VTWM_DESKTOP_CLASS) != 0)
	{
	    dwidth -= tmp_win->hints.min_width;
	    dheight -= tmp_win->hints.min_height;
	}
    }

    if (tmp_win->hints.flags & PResizeInc)
    {
        dwidth /= tmp_win->hints.width_inc;
        dheight /= tmp_win->hints.height_inc;
    }

/*
 * Non-SysV systems - specifically, BSD-derived systems - return a
 * pointer to the string, not its length. Submitted by Goran Larsson
    i = sprintf (str, "%5d x %-5d", dwidth, dheight);
 */
    sprintf (str, "%5d x %-5d", dwidth, dheight);
    i = strlen (str);

    XRaiseWindow(dpy, Scr->SizeWindow.win);
    MyFont_DrawImageString (dpy, &Scr->SizeWindow, &Scr->SizeFont,
			  &Scr->DefaultC,

/* djhjr - 5/9/96
		      Scr->SizeStringOffset,
*/
			  (Scr->SizeStringWidth -
			   MyFont_TextWidth(&Scr->SizeFont,
					str, i)) / 2,

			/* was 'Scr->use3Dborders' - djhjr - 8/11/98 */
			Scr->SizeFont.ascent +
				 SIZE_VINDENT +
				 ((Scr->InfoBevelWidth > 0) ? Scr->InfoBevelWidth : 0),

			str, i);

	/* I know, I know, but the above code overwrites it... djhjr - 5/9/96 */
	/* was 'Scr->use3Dborders' - djhjr - 8/11/98 */
	if (Scr->InfoBevelWidth > 0)
	    Draw3DBorder(Scr->SizeWindow.win, 0, 0,
				Scr->SizeStringWidth,

/* djhjr - 4/29/98
				(unsigned int) (Scr->SizeFont.height + SIZE_VINDENT*2),
				BW, Scr->DefaultC, off, False, False);
*/
				/* was 'Scr->use3Dborders' - djhjr - 8/11/98 */
				(unsigned int) (Scr->SizeFont.height + SIZE_VINDENT*2) +
					((Scr->InfoBevelWidth > 0) ? 2 * Scr->InfoBevelWidth : 0),
				Scr->InfoBevelWidth, Scr->DefaultC, off, False, False);
}

/***********************************************************************
 *
 *  Procedure:
 *      EndResize - finish the resize operation
 *
 ***********************************************************************
 */

void
EndResize()
{
    TwmWindow *tmp_win;

#ifdef DEBUG
    fprintf(stderr, "EndResize\n");
#endif

    if (resize_context == C_VIRTUAL_WIN)
	    MoveOutline(Scr->VirtualDesktopDisplay, 0, 0, 0, 0, 0, 0);
    else
	    MoveOutline(Scr->Root, 0, 0, 0, 0, 0, 0);
    XUnmapWindow(dpy, Scr->SizeWindow.win);

    XFindContext(dpy, ResizeWindow, TwmContext, (caddr_t *)&tmp_win);

    if (resize_context == C_VIRTUAL_WIN) {
	    /* scale up */
	    dragWidth = SCALE_U(dragWidth);
	    dragHeight = SCALE_U(dragHeight);
	    dragx = SCALE_U(dragx);
	    dragy = SCALE_U(dragy);
    }

    ConstrainSize (tmp_win, &dragWidth, &dragHeight);

    if (dragWidth != tmp_win->frame_width ||
        dragHeight != tmp_win->frame_height)
            tmp_win->zoomed = ZOOM_NONE;

    SetupWindow (tmp_win, dragx - tmp_win->frame_bw, dragy - tmp_win->frame_bw,
		 dragWidth, dragHeight, -1);

    EndResizeAdjPointer(tmp_win);	/* djhjr - 9/13/02 */

	/* added test for opaque resizing - djhjr - 2/28/99, 3/1/99 */
	if (!tmp_win->opaque_resize)
	{
		/* was inline code - djhjr - 9/10/99 */
		ResizeTwmWindowContents(tmp_win, dragWidth, dragHeight);
	}

#if 0 /* done in menus.c:ExecuteFunction() - djhjr - 10/6/02 */
    if (!Scr->NoRaiseResize) {
        XRaiseWindow(dpy, tmp_win->frame);
        
	RaiseStickyAbove (); /* DSE */
	RaiseAutoPan();
    }

    UninstallRootColormap();

    /* the resize can have cause the window to move on the screen, hence on the virtual
     * desktop - need to fix the virtual coords */
    tmp_win->virtual_frame_x = R_TO_V_X(dragx);
    tmp_win->virtual_frame_y = R_TO_V_Y(dragy);

    /* UpdateDesktop(tmp_win); Stig */
    MoveResizeDesktop(tmp_win, Scr->NoRaiseResize); /* Stig */
#endif

#if 0 /* done in menus.c:ExecuteFunction() - djhjr - 10/11/01 */
	/* djhjr - 6/4/98 */
	/* don't re-map if the window is the virtual desktop - djhjr - 2/28/99 */
	if (Scr->VirtualReceivesMotionEvents &&
			/* !tmp_win->opaque_resize && */
			tmp_win->w != Scr->VirtualDesktopDisplayOuter)
	{
		XUnmapWindow(dpy, Scr->VirtualDesktopDisplay);
		XMapWindow(dpy, Scr->VirtualDesktopDisplay);
	}
#endif

    ResizeWindow = None;

	/* djhjr - 9/5/98 */
	resizing_window = 0;
}

/* added the passed 'context' - djhjr - 9/30/02 */
void
MenuEndResize(tmp_win, context)
TwmWindow *tmp_win;
int context;
{
    /* added this 'if (...) ... else' - djhjr - 2/22/99 */
    if (resize_context == C_VIRTUAL_WIN)
	    MoveOutline(Scr->VirtualDesktopDisplay, 0, 0, 0, 0, 0, 0);
    else
	    MoveOutline(Scr->Root, 0, 0, 0, 0, 0, 0);
    XUnmapWindow(dpy, Scr->SizeWindow.win);

    ConstrainSize (tmp_win, &dragWidth, &dragHeight);

	/* djhjr - 9/19/96 */
	if (dragWidth != tmp_win->frame_width || dragHeight != tmp_win->frame_height)
		tmp_win->zoomed = ZOOM_NONE;

/* djhjr - 10/6/02
    AddingX = dragx;
    AddingY = dragy;
    AddingW = dragWidth + (2 * tmp_win->frame_bw);
    AddingH = dragHeight + (2 * tmp_win->frame_bw);
    SetupWindow (tmp_win, AddingX, AddingY, AddingW, AddingH, -1);
*/
    SetupWindow (tmp_win, dragx - tmp_win->frame_bw, dragy - tmp_win->frame_bw,
		 dragWidth, dragHeight, -1);

    /* djhjr - 9/13/02 9/30/02 */
    if (context != C_VIRTUAL_WIN)
	EndResizeAdjPointer(tmp_win);

	/* added test for opaque resizing - djhjr - 2/28/99, 3/1/99 */
	if (!tmp_win->opaque_resize)
	{
		/* was inline code - djhjr - 9/10/99 */
		ResizeTwmWindowContents(tmp_win, dragWidth, dragHeight);
	}

#if 0 /* done in menus.c:ExecuteFunction() - djhjr - 10/11/01 */
	/* djhjr - 6/4/98 */
	/* don't re-map if the window is the virtual desktop - djhjr - 2/28/99 */
	if (Scr->VirtualReceivesMotionEvents &&
			/* !tmp_win->opaque_resize && */
			tmp_win->w != Scr->VirtualDesktopDisplayOuter)
	{
		XUnmapWindow(dpy, Scr->VirtualDesktopDisplay);
		XMapWindow(dpy, Scr->VirtualDesktopDisplay);
	}
#endif

	/* djhjr - 9/5/98 */
	resizing_window = 0;
}



/***********************************************************************
 *
 *  Procedure:
 *      AddEndResize - finish the resize operation for AddWindow
 *
 ***********************************************************************
 */

void
AddEndResize(tmp_win)
TwmWindow *tmp_win;
{

#ifdef DEBUG
    fprintf(stderr, "AddEndResize\n");
#endif

    /* djhjr - 2/22/99 */
    MoveOutline(Scr->Root, 0, 0, 0, 0, 0, 0);
    XUnmapWindow(dpy, Scr->SizeWindow.win);

    ConstrainSize (tmp_win, &dragWidth, &dragHeight);
    AddingX = dragx;
    AddingY = dragy;
    AddingW = dragWidth + (2 * tmp_win->frame_bw);
    AddingH = dragHeight + (2 * tmp_win->frame_bw);

    EndResizeAdjPointer(tmp_win);	/* djhjr - 9/13/02 */

	/* djhjr - 9/19/96 */
	if (dragWidth != tmp_win->frame_width || dragHeight != tmp_win->frame_height)
		tmp_win->zoomed = ZOOM_NONE;

#if 0 /* done in add_window.c:AddMoveAndResize() - djhjr - 10/11/01 */
	/* djhjr - 6/4/98 */
	if (Scr->VirtualReceivesMotionEvents/* && !tmp_win->opaque_resize*/)
	{
		XUnmapWindow(dpy, Scr->VirtualDesktopDisplay);
		XMapWindow(dpy, Scr->VirtualDesktopDisplay);
	}
#endif

	/* djhjr - 9/5/98 */
	resizing_window = 0;
}

/* djhjr - 9/13/02 */
static void
EndResizeAdjPointer(tmp_win)
TwmWindow *tmp_win;
{
    int x, y, bw = tmp_win->frame_bw + tmp_win->frame_bw3D;
    int pointer_x, pointer_y; /* pointer coordinates relative to root origin */

    /* Declare junk variables for Xlib calls that return more than we need. */
    Window junk_root, junk_child;
    int junk_win_x, junk_win_y;
    unsigned int junk_mask;

    XQueryPointer(dpy, Scr->Root, &junk_root, &junk_child,
		  &pointer_x, &pointer_y, &junk_win_x, &junk_win_y, &junk_mask);
    XTranslateCoordinates(dpy, Scr->Root, tmp_win->frame, pointer_x, pointer_y,
			  &x, &y, &junk_child);

    /* for borderless windows */
    if (bw == 0) bw = 4;

    /* (tmp_win->frame_bw) == no 3D borders */
    
    if (x <= 0)
    {
	if (y < tmp_win->title_height)
	    x = tmp_win->title_x + ((tmp_win->frame_bw) ? (bw / 2) : -(bw / 2));
	else
	    x = ((tmp_win->frame_bw) ? -(bw / 2) : (bw / 2));
    }
    if (x >= tmp_win->frame_width)
    {
	if (y < tmp_win->title_height)
	    x = tmp_win->title_x + tmp_win->title_width + (bw / 2);
	else
	    x = tmp_win->frame_width + ((tmp_win->frame_bw) ? (bw / 2) : -(bw / 2));
    }

    if (y <= tmp_win->title_height)
    {
	if (x >= tmp_win->title_x - ((tmp_win->frame_bw) ? 0 : bw) &&
		x < tmp_win->title_x + tmp_win->title_width + bw)
	{
	    if (y <= 0)
		y = ((tmp_win->frame_bw) ? -(bw / 2) : (bw / 2));
	}
	else
	    y = tmp_win->title_height + ((tmp_win->frame_bw) ? -(bw / 2) : (bw / 2));
    }
    if (y >= tmp_win->frame_height)
	y = tmp_win->frame_height + ((tmp_win->frame_bw) ? (bw / 2) : -(bw / 2));

    XWarpPointer(dpy, None, tmp_win->frame, 0, 0, 0, 0, x, y);
}

/***********************************************************************
 *
 *  Procedure:
 *      ConstrainSize - adjust the given width and height to account for the
 *              constraints imposed by size hints
 *
 *      The general algorithm, especially the aspect ratio stuff, is
 *      borrowed from uwm's CheckConsistency routine.
 *
 ***********************************************************************/

void ConstrainSize (tmp_win, widthp, heightp)
    TwmWindow *tmp_win;
    int *widthp, *heightp;
{
#define makemult(a,b) ((b==1) ? (a) : (((int)((a)/(b))) * (b)) )
#define _min(a,b) (((a) < (b)) ? (a) : (b))

    int minWidth, minHeight, maxWidth, maxHeight, xinc, yinc, delta;
    int baseWidth, baseHeight;
    int dwidth = *widthp, dheight = *heightp;


/* djhjr - 4/24/96
    dheight -= tmp_win->title_height;
*/
    dwidth  -= 2 * tmp_win->frame_bw3D;
    dheight -= (tmp_win->title_height + 2 * tmp_win->frame_bw3D);

    if (tmp_win->hints.flags & PMinSize) {
        minWidth = tmp_win->hints.min_width;
        minHeight = tmp_win->hints.min_height;
    } else if (tmp_win->hints.flags & PBaseSize) {
        minWidth = tmp_win->hints.base_width;
        minHeight = tmp_win->hints.base_height;
    } else
        minWidth = minHeight = 1;

    if (resize_context == C_VIRTUAL_WIN) {
	    minWidth = SCALE_D(minWidth);
	    minHeight = SCALE_D(minHeight);
    }

    if (tmp_win->hints.flags & PBaseSize) {
	baseWidth = tmp_win->hints.base_width;
	baseHeight = tmp_win->hints.base_height;
    } else if (tmp_win->hints.flags & PMinSize) {
	baseWidth = tmp_win->hints.min_width;
	baseHeight = tmp_win->hints.min_height;
    } else
	baseWidth = baseHeight = 0;

    if (resize_context == C_VIRTUAL_WIN) {
	    baseWidth = SCALE_D(baseWidth);
	    baseHeight = SCALE_D(baseHeight);
    }

    if (tmp_win->hints.flags & PMaxSize) {
        maxWidth = _min (Scr->MaxWindowWidth, tmp_win->hints.max_width);
        maxHeight = _min (Scr->MaxWindowHeight, tmp_win->hints.max_height);
    } else {
        maxWidth = Scr->MaxWindowWidth;
	maxHeight = Scr->MaxWindowHeight;
    }

    if (resize_context == C_VIRTUAL_WIN) {
	    maxWidth = SCALE_D(maxWidth);
	    maxHeight = SCALE_D(maxHeight);
    }

    if (tmp_win->hints.flags & PResizeInc) {
        xinc = tmp_win->hints.width_inc;
        yinc = tmp_win->hints.height_inc;
    } else
        xinc = yinc = 1;

    if (resize_context == C_VIRTUAL_WIN) {
	    xinc = SCALE_D(xinc);
	    yinc = SCALE_D(yinc);
    }

    /*
     * First, clamp to min and max values
     */
    if (dwidth < minWidth) dwidth = minWidth;
    if (dheight < minHeight) dheight = minHeight;

    if (dwidth > maxWidth) dwidth = maxWidth;
    if (dheight > maxHeight) dheight = maxHeight;


    /*
     * Second, fit to base + N * inc
     */
    dwidth = ((dwidth - baseWidth) / xinc * xinc) + baseWidth;
    dheight = ((dheight - baseHeight) / yinc * yinc) + baseHeight;


    /*
     * Third, adjust for aspect ratio
     */
#define maxAspectX tmp_win->hints.max_aspect.x
#define maxAspectY tmp_win->hints.max_aspect.y
#define minAspectX tmp_win->hints.min_aspect.x
#define minAspectY tmp_win->hints.min_aspect.y
    /*
     * The math looks like this:
     *
     * minAspectX    dwidth     maxAspectX
     * ---------- <= ------- <= ----------
     * minAspectY    dheight    maxAspectY
     *
     * If that is multiplied out, then the width and height are
     * invalid in the following situations:
     *
     * minAspectX * dheight > minAspectY * dwidth
     * maxAspectX * dheight < maxAspectY * dwidth
     *
     */

    if (tmp_win->hints.flags & PAspect)
    {
        if (minAspectX * dheight > minAspectY * dwidth)
        {
            delta = makemult(minAspectX * dheight / minAspectY - dwidth,
                             xinc);
            if (dwidth + delta <= maxWidth) dwidth += delta;
            else
            {
                delta = makemult(dheight - dwidth*minAspectY/minAspectX,
                                 yinc);
                if (dheight - delta >= minHeight) dheight -= delta;
            }
        }

        if (maxAspectX * dheight < maxAspectY * dwidth)
        {
            delta = makemult(dwidth * maxAspectY / maxAspectX - dheight,
                             yinc);
            if (dheight + delta <= maxHeight) dheight += delta;
            else
            {
                delta = makemult(dwidth - maxAspectX*dheight/maxAspectY,
                                 xinc);
                if (dwidth - delta >= minWidth) dwidth -= delta;
            }
        }
    }


    /*
     * Fourth, account for border width and title height
     */
/* djhjr - 4/26/96
    *widthp = dwidth;
    *heightp = dheight + tmp_win->title_height;
*/
    *widthp = dwidth + 2 * tmp_win->frame_bw3D;
    *heightp = dheight + tmp_win->title_height + 2 * tmp_win->frame_bw3D;

}


/***********************************************************************
 *
 *  Procedure:
 *      SetupWindow - set window sizes, this was called from either
 *              AddWindow, EndResize, or HandleConfigureNotify.
 *
 *  Inputs:
 *      tmp_win - the TwmWindow pointer
 *      x       - the x coordinate of the upper-left outer corner of the frame
 *      y       - the y coordinate of the upper-left outer corner of the frame
 *      w       - the width of the frame window w/o border
 *      h       - the height of the frame window w/o border
 *      bw      - the border width of the frame window or -1 not to change
 *
 *  Special Considerations:
 *      This routine will check to make sure the window is not completely
 *      off the display, if it is, it'll bring some of it back on.
 *
 *      The tmp_win->frame_XXX variables should NOT be updated with the
 *      values of x,y,w,h prior to calling this routine, since the new
 *      values are compared against the old to see whether a synthetic
 *      ConfigureNotify event should be sent.  (It should be sent if the
 *      window was moved but not resized.)
 *
 ***********************************************************************
 */

void SetupWindow (tmp_win, x, y, w, h, bw)
    TwmWindow *tmp_win;
    int x, y, w, h, bw;
{
    SetupFrame (tmp_win, x, y, w, h, bw, False);
}

void SetupFrame (tmp_win, x, y, w, h, bw, sendEvent)
    TwmWindow *tmp_win;
    int x, y, w, h, bw;
    Bool sendEvent;			/* whether or not to force a send */
{
    XWindowChanges frame_wc, xwc;
    unsigned long frame_mask, xwcm;
    int title_width, title_height;
    int reShape;

#ifdef DEBUG
    fprintf (stderr, "SetupWindow: x=%d, y=%d, w=%d, h=%d, bw=%d\n",
	     x, y, w, h, bw);
#endif

    if ((tmp_win->virtual_frame_x + tmp_win->frame_width) < 0)
 	    x = 16; /* one "average" cursor width */
    if (x >= Scr->VirtualDesktopWidth)
 	    x = Scr->VirtualDesktopWidth - 16;
    if ((tmp_win->virtual_frame_y + tmp_win->frame_height) < 0)
 	    y = 16; /* one "average" cursor width */
    if (y >= Scr->VirtualDesktopHeight)
 	    y = Scr->VirtualDesktopHeight - 16;

    if (bw < 0)
      bw = tmp_win->frame_bw;		/* -1 means current frame width */

    if (tmp_win->iconmgr) {
/* djhjr - 4/24/96
		tmp_win->iconmgrp->width = w;
        h = tmp_win->iconmgrp->height + tmp_win->title_height;
*/
		tmp_win->iconmgrp->width = w - (2 * tmp_win->frame_bw3D);
        h = tmp_win->iconmgrp->height + tmp_win->title_height + (2 * tmp_win->frame_bw3D);

    }

    /*
     * According to the July 27, 1988 ICCCM draft, we should send a
     * "synthetic" ConfigureNotify event to the client if the window
     * was moved but not resized.
     */
    if (((x != tmp_win->frame_x || y != tmp_win->frame_y) &&
	 (w == tmp_win->frame_width && h == tmp_win->frame_height)) ||
	(bw != tmp_win->frame_bw))
      sendEvent = TRUE;

    xwcm = CWWidth;
/* djhjr 8 4/24/96
    title_width = xwc.width = w;
*/
    title_width  = xwc.width = w - (2 * tmp_win->frame_bw3D);
    title_height = Scr->TitleHeight + bw;

    ComputeWindowTitleOffsets (tmp_win, xwc.width, True);

    reShape = (tmp_win->wShaped ? TRUE : FALSE);
    if (tmp_win->squeeze_info)		/* check for title shaping */
    {
	title_width = tmp_win->rightx + Scr->TBInfo.rightoff;
	if (title_width < xwc.width)
	{
	    xwc.width = title_width;
	    if (tmp_win->frame_height != h ||
	    	tmp_win->frame_width != w ||
		tmp_win->frame_bw != bw ||
	    	title_width != tmp_win->title_width)
	    	reShape = TRUE;
	}
	else
	{
	    if (!tmp_win->wShaped) reShape = TRUE;
	    title_width = xwc.width;
	}
    }

    tmp_win->title_width = title_width;
    if (tmp_win->title_height) tmp_win->title_height = title_height;

    if (tmp_win->title_w.win) {
	if (bw != tmp_win->frame_bw) {
	    xwc.border_width = bw;
/* djhjr - 4/24/96
	    tmp_win->title_x = xwc.x = -bw;
	    tmp_win->title_y = xwc.y = -bw;
*/
	    tmp_win->title_x = xwc.x = tmp_win->frame_bw3D - bw;
	    tmp_win->title_y = xwc.y = tmp_win->frame_bw3D - bw;

	    xwcm |= (CWX | CWY | CWBorderWidth);
	}

	XConfigureWindow(dpy, tmp_win->title_w.win, xwcm, &xwc);
    }

/* djhjr - 4/24/96
    tmp_win->attr.width = w;
    tmp_win->attr.height = h - tmp_win->title_height;
*/
    tmp_win->attr.width  = w - (2 * tmp_win->frame_bw3D);
    tmp_win->attr.height = h - tmp_win->title_height - (2 * tmp_win->frame_bw3D);

/* djhjr - 4/25/96
    XMoveResizeWindow (dpy, tmp_win->w, 0, tmp_win->title_height,
		       w, h - tmp_win->title_height);
*/

    /*
     * fix up frame and assign size/location values in tmp_win
     */

    frame_mask = 0;
    if (bw != tmp_win->frame_bw) {
	frame_wc.border_width = tmp_win->frame_bw = bw;
	frame_mask |= CWBorderWidth;
    }
    frame_wc.x = tmp_win->frame_x = x;
    frame_wc.y = tmp_win->frame_y = y;
    frame_wc.width = tmp_win->frame_width = w;
    frame_wc.height = tmp_win->frame_height = h;
    frame_mask |= (CWX | CWY | CWWidth | CWHeight);
    XConfigureWindow (dpy, tmp_win->frame, frame_mask, &frame_wc);
    tmp_win->virtual_frame_x = R_TO_V_X(tmp_win->frame_x);
    tmp_win->virtual_frame_y = R_TO_V_Y(tmp_win->frame_y);

	/* djhjr - 4/24/96 */
    XMoveResizeWindow (dpy, tmp_win->w, tmp_win->frame_bw3D,
			tmp_win->title_height + tmp_win->frame_bw3D,
			tmp_win->attr.width, tmp_win->attr.height);

    /*
     * fix up highlight window
     */

    if (tmp_win->title_height && tmp_win->hilite_w)
    {
/* djhjr - 4/2/98
	xwc.width = (tmp_win->rightx - tmp_win->highlightx);
	if (Scr->TBInfo.nright > 0) xwc.width -= Scr->TitlePadding;

	* djhjr - 4/24/96 *
	if (Scr->use3Dtitles) xwc.width -= 4;
*/
	xwc.width = ComputeHighlightWindowWidth(tmp_win);

        if (xwc.width <= 0) {
            xwc.x = Scr->MyDisplayWidth;	/* move offscreen */
            xwc.width = 1;
        } else {
            xwc.x = tmp_win->highlightx;
        }

        xwcm = CWX | CWWidth;
        XConfigureWindow(dpy, tmp_win->hilite_w, xwcm, &xwc);
    }

    if (HasShape && reShape) {
	SetFrameShape (tmp_win);
    }

    if (sendEvent)
    {
	    SendConfigureNotify(tmp_win, x, y);
    }
}

/* djhjr - 4/6/98 */
void
PaintBorderAndTitlebar(tmp_win)
TwmWindow *tmp_win;
{
	if (tmp_win->highlight)
		SetBorder(tmp_win, True);
	else
	{
		/* was 'Scr->use3Dborders' - djhjr - 8/11/98 */
		if (Scr->BorderBevelWidth > 0) PaintBorders(tmp_win, True);

		if (tmp_win->titlebuttons)
		{
			int i, nb = Scr->TBInfo.nleft + Scr->TBInfo.nright;
			TBWindow *tbw;

			for (i = 0, tbw = tmp_win->titlebuttons; i < nb; i++, tbw++)
				PaintTitleButton(tmp_win, tbw, 0);
		}
	}

	PaintTitle(tmp_win);
	PaintTitleHighlight(tmp_win, on);	/* djhjr - 10/25/02 */
}


/* djhjr - 4/17/98 */
static void
DoVirtualMoveResize(tmp_win, x, y, w, h)
TwmWindow *tmp_win;
int x, y, w, h;
{
	int fw = tmp_win->frame_width, fh = tmp_win->frame_height;

	tmp_win->virtual_frame_x = R_TO_V_X(x - tmp_win->frame_bw);
	tmp_win->virtual_frame_y = R_TO_V_Y(y - tmp_win->frame_bw);
	if (!tmp_win->opaque_resize)
	{
		tmp_win->frame_width = w + 2 * tmp_win->frame_bw;
		tmp_win->frame_height = h + 2 * tmp_win->frame_bw;
	}

/* djhjr - 5/27/03
	MoveResizeDesktop(tmp_win, Scr->NoRaiseResize);
*/
	MoveResizeDesktop(tmp_win, TRUE);

	tmp_win->frame_width = fw; tmp_win->frame_height = fh;
}


#ifdef TILED_SCREEN

/*
 * Attention: in the following tiled screen Zoom functions 'top-edge'
 * is not towards the upper edge of the monitor but the edge having
 * greater y-coordinate.  I.e. in fact, it is towards the 'lower-edge'.
 * Imagine 'bot' having y0 and 'top' the y1 Cartesian coordinate,
 * and y0 <= y1 imposed.
 *
 * (Thinking of Lft/Bot/Rht/Top-edges as x0/y0/x1/y1-edges instead
 * is correct and probably less confusing.)
 */

int FindNearestTileToArea (int w[4])
{
    /*
     * Find a screen tile having maximum overlap with 'w'
     * (or, if 'w' is outside of all tiles, a tile with minimum
     * distance to 'w').
     */
    int k, dx, dy, a, m, i;

    m = -xmax(Scr->MyDisplayWidth, Scr->MyDisplayHeight); /*some large value*/
    i = 0;
    for (k = 0; k < Scr->ntiles; ++k) {
	dx = Distance1D(Lft(w), Rht(w), Lft(Scr->tiles[k]), Rht(Scr->tiles[k]));
	dy = Distance1D(Bot(w), Top(w), Bot(Scr->tiles[k]), Top(Scr->tiles[k]));
	a  = Distance2D(dx,dy);
	if (m < a)
	    m = a, i = k;
    }
    return i;
}

int FindNearestTileToPoint (int x, int y)
{
    int k, dx, dy, a, m, i;

    i = 0;
    m = -xmax(Scr->MyDisplayWidth, Scr->MyDisplayHeight);
    for (k = 0; k < Scr->ntiles; ++k) {
	dx = Distance1D(x, x, Lft(Scr->tiles[k]), Rht(Scr->tiles[k]));
	dy = Distance1D(y, y, Bot(Scr->tiles[k]), Top(Scr->tiles[k]));
	a  = Distance2D(dx,dy);
	if (m < a)
	    m = a, i = k;
    }
    return i;
}

int FindNearestTileToClient (TwmWindow *tmp)
{
    int k, dx, dy, a, m, i, w[4];

    Lft(w) = tmp->frame_x;
    Rht(w) = tmp->frame_x + tmp->frame_width  - 1;
    Bot(w) = tmp->frame_y;
    Top(w) = tmp->frame_y + tmp->frame_height - 1;
    return FindNearestTileToArea (w);
}

int FindNearestTileToMouse (void)
{
    if (False == XQueryPointer (dpy, Scr->Root, &JunkRoot, &JunkChild,
		    &JunkX, &JunkY, &HotX, &HotY, &JunkMask))
	/* emergency: mouse not on current X11-screen */
	return 0;
    else
	return FindNearestTileToPoint (JunkX, JunkY);
}

void EnsureRectangleOnTile (int tile, int *x0, int *y0, int w, int h)
{
    if ((*x0) + w > Rht(Scr->tiles[tile])+1)
	(*x0) = Rht(Scr->tiles[tile])+1 - w;
    if ((*x0) < Lft(Scr->tiles[tile]))
	(*x0) = Lft(Scr->tiles[tile]);
    if ((*y0) + h > Top(Scr->tiles[tile])+1)
	(*y0) = Top(Scr->tiles[tile])+1 - h;
    if ((*y0) < Bot(Scr->tiles[tile]))
	(*y0) = Bot(Scr->tiles[tile]);
}

void EnsureGeometryVisibility (int mask, int *x0, int *y0, int w, int h)
{
    int x, y, Area[4];

    x = (*x0);
    if (mask & XNegative)
	x += Scr->MyDisplayWidth - w;
    y = (*y0);
    if (mask & YNegative)
	y += Scr->MyDisplayHeight - h;

    Lft(Area) = x;
    Rht(Area) = x + w - 1;
    Bot(Area) = y;
    Top(Area) = y + h - 1;

    TilesFullZoom (Area);

    if (mask & XNegative)
	(*x0) += Rht(Area)+1 - w;
    else
	(*x0) += Lft(Area);
    if (mask & YNegative)
	(*y0) += Top(Area)+1 - h;
    else
	(*y0) += Bot(Area);

    /* final sanity check: */
    if ((*x0) + w > Rht(Area)+1)
	(*x0) = Rht(Area)+1 - w;
    if ((*x0) < Lft(Area))
	(*x0) = Lft(Area);
    if ((*y0) + h > Top(Area)+1)
	(*y0) = Top(Area)+1 - h;
    if ((*y0) < Bot(Area))
	(*y0) = Bot(Area);
}

#define IntersectsV(a,b,t)  (((Bot(a)) <= (t)) && ((b) <= (Top(a))))
#define IntersectsH(a,l,r)  (((Lft(a)) <= (r)) && ((l) <= (Rht(a))))
#define OverlapsV(a,w)	    IntersectsV(a,Bot(w),Top(w))
#define OverlapsH(a,w)	    IntersectsH(a,Lft(w),Rht(w))
#define ContainsV(a,p)	    IntersectsV(a,p,p)
#define ContainsH(a,p)	    IntersectsH(a,p,p)

static int BoundingBoxLeft (int w[4])
{
    int i, k;

    i = -1; /* value '-1' is 'not found' */
    /* zoom out: */
    for (k = 0; k < Scr->ntiles; ++k)
	/* consider tiles vertically overlapping the window "height" (along left edge): */
	if (OverlapsV(Scr->tiles[k], w))
	    /* consider tiles horizontally intersecting the window left edge: */
	    if (ContainsH(Scr->tiles[k], Lft(w))) {
		if ((i == -1) || (Lft(Scr->tiles[k]) < Lft(Scr->tiles[i])))
		    i = k;
	    }
    if (i == -1)
	/* zoom in (no intersecting tiles found): */
	for (k = 0; k < Scr->ntiles; ++k)
	    if (OverlapsV(Scr->tiles[k], w))
		/* consider tiles having left edge right to the left edge of the window: */
		if (Lft(Scr->tiles[k]) > Lft(w)) {
		    if ((i == -1) || (Lft(Scr->tiles[k]) < Lft(Scr->tiles[i])))
			i = k;
		}
    return i;
}

static int BoundingBoxBottom (int w[4])
{
    int i, k;

    i = -1;
    /* zoom out: */
    for (k = 0; k < Scr->ntiles; ++k)
	/* consider tiles horizontally overlapping the window "width" (along bottom edge): */
	if (OverlapsH(Scr->tiles[k], w))
	    /* consider tiles vertically interscting the window bottom edge: */
	    if (ContainsV(Scr->tiles[k], Bot(w))) {
		if ((i == -1) || (Bot(Scr->tiles[k]) < Bot(Scr->tiles[i])))
		    i = k;
	    }
    if (i == -1)
	/* zoom in (no intersecting tiles found): */
	for (k = 0; k < Scr->ntiles; ++k)
	    if (OverlapsH(Scr->tiles[k], w))
		/* consider tiles having bottom edge ontop of the bottom edge of the window: */
		if (Bot(Scr->tiles[k]) > Bot(w)) {
		    if ((i == -1) || (Bot(Scr->tiles[k]) < Bot(Scr->tiles[i])))
			i = k;
		}
    return i;
}

static int BoundingBoxRight (int w[4])
{
    int i, k;

    i = -1;
    /* zoom out: */
    for (k = 0; k < Scr->ntiles; ++k)
	/* consider tiles vertically overlapping the window "height" (along right edge): */
	if (OverlapsV(Scr->tiles[k], w))
	    /* consider tiles horizontally intersecting the window right edge: */
	    if (ContainsH(Scr->tiles[k], Rht(w))) {
		if ((i == -1) || (Rht(Scr->tiles[k]) > Rht(Scr->tiles[i])))
		    i = k;
	    }
    if (i == -1)
	/* zoom in (no intersecting tiles found): */
	for (k = 0; k < Scr->ntiles; ++k)
	    if (OverlapsV(Scr->tiles[k], w))
		/* consider tiles having right edge left to the right edge of the window: */
		if (Rht(Scr->tiles[k]) < Rht(w)) {
		    if ((i == -1) || (Rht(Scr->tiles[k]) > Rht(Scr->tiles[i])))
			i = k;
		}
    return i;
}

static int BoundingBoxTop (int w[4])
{
    int i, k;

    i = -1;
    /* zoom out: */
    for (k = 0; k < Scr->ntiles; ++k)
	/* consider tiles horizontally overlapping the window "width" (along top edge): */
	if (OverlapsH(Scr->tiles[k], w))
	    /* consider tiles vertically interscting the window top edge: */
	    if (ContainsV(Scr->tiles[k], Top(w))) {
		if ((i == -1) || (Top(Scr->tiles[k]) > Top(Scr->tiles[i])))
		    i = k;
	    }
    if (i == -1)
	/* zoom in (no intersecting tiles found): */
	for (k = 0; k < Scr->ntiles; ++k)
	    if (OverlapsH(Scr->tiles[k], w))
		/* consider tiles having top edge below of the top edge of the window: */
		if (Top(Scr->tiles[k]) < Top(w)) {
		    if ((i == -1) || (Top(Scr->tiles[k]) > Top(Scr->tiles[i])))
			i = k;
		}
    return i;
}

static int UncoveredLeft (int skip, int area[4], int thresh)
{
    int u, e, k, w[4];

    u = Top(area) - Bot(area);
    e = 0;
    for (k = 0; k < Scr->ntiles && e == 0; ++k)
	if (k != skip)
	    /* consider tiles vertically intersecting area: */
	    if (IntersectsV(Scr->tiles[k], Bot(area), Top(area)))
		/* consider tiles horizontally intersecting boundingbox and outside window
		 * (but touches the window on its left):
		 */
		if (ContainsH(Scr->tiles[k], Lft(area)-thresh)) {
		    if (Top(Scr->tiles[k]) < Top(area)) {
			Lft(w) = Lft(area);
			Bot(w) = Top(Scr->tiles[k]) + 1;
			Rht(w) = Rht(area);
			Top(w) = Top(area);
			u = UncoveredLeft (skip, w, thresh);
			e = 1;
		    }
		    if (Bot(Scr->tiles[k]) > Bot(area)) {
			Lft(w) = Lft(area);
			Bot(w) = Bot(area);
			Rht(w) = Rht(area);
			Top(w) = Bot(Scr->tiles[k]) - 1;
			if (e == 0)
			    u  = UncoveredLeft (skip, w, thresh);
			else
			    u += UncoveredLeft (skip, w, thresh);
			e = 1;
		    }
		    if (e == 0) {
			u = 0;
			e = 1;
		    }
		}
    return u;
}

static int ZoomLeft (int bbl, int b, int t, int thresv, int thresh)
{
    int i, k, u;

    i = bbl;
    for (k = 0; k < Scr->ntiles; ++k)
	if (k != bbl)
	    /* consider tiles vertically intersecting bbl: */
	    if (IntersectsV(Scr->tiles[k], b, t))
		/* consider tiles having left edge in the bounding region
		 * and to the right of the current best tile 'i':
		 */
		if (ContainsH(Scr->tiles[bbl], Lft(Scr->tiles[k]))
			&& (Lft(Scr->tiles[i]) < Lft(Scr->tiles[k])))
		{
		    u = UncoveredLeft (k, Scr->tiles[k], thresh);
		    if (u > thresv)
			i = k;
		}
    return i;
}

static int UncoveredBottom (int skip, int area[4], int thresv)
{
    int u, e, k, w[4];

    u = Rht(area) - Lft(area);
    e = 0;
    for (k = 0; k < Scr->ntiles && e == 0; ++k)
	if (k != skip)
	    /* consider tiles horizontally intersecting area: */
	    if (IntersectsH(Scr->tiles[k], Lft(area), Rht(area)))
		/* consider tiles vertically intersecting boundingbox and outside window
		 * (but touches the window to its bottom):
		 */
		if (ContainsV(Scr->tiles[k], Bot(area)-thresv)) {
		    if (Rht(Scr->tiles[k]) < Rht(area)) {
			Lft(w) = Rht(Scr->tiles[k]) + 1;
			Bot(w) = Bot(area);
			Rht(w) = Rht(area);
			Top(w) = Top(area);
			u = UncoveredBottom (skip, w, thresv);
			e = 1;
		    }
		    if (Lft(Scr->tiles[k]) > Lft(area)) {
			Lft(w) = Lft(area);
			Bot(w) = Bot(area);
			Rht(w) = Lft(Scr->tiles[k]) - 1;
			Top(w) = Top(area);
			if (e == 0)
			    u  = UncoveredBottom (skip, w, thresv);
			else
			    u += UncoveredBottom (skip, w, thresv);
			e = 1;
		    }
		    if (e == 0) {
			u = 0;
			e = 1;
		    }
		}
    return u;
}

static int ZoomBottom (int bbb, int l, int r, int thresh, int thresv)
{
    int i, k, u;

    i = bbb;
    for (k = 0; k < Scr->ntiles; ++k)
	if (k != bbb)
	    /* consider tiles horizontally intersecting bbb: */
	    if (IntersectsH(Scr->tiles[k], l, r))
		/* consider tiles having bottom edge in the bounding region
		 * and above the current best tile 'i':
		 */
		if (ContainsV(Scr->tiles[bbb], Bot(Scr->tiles[k]))
			&& (Bot(Scr->tiles[i]) < Bot(Scr->tiles[k])))
		{
		    u = UncoveredBottom (k, Scr->tiles[k], thresv);
		    if (u > thresh)
			i = k;
		}
    return i;
}

static int UncoveredRight(int skip, int area[4], int thresh)
{
    int u, e, k, w[4];

    u = Top(area) - Bot(area);
    e = 0;
    for (k = 0; k < Scr->ntiles && e == 0; ++k)
	if (k != skip)
	    /* consider tiles vertically intersecting area: */
	    if (IntersectsV(Scr->tiles[k], Bot(area), Top(area)))
		/* consider tiles horizontally intersecting boundingbox and outside window
		 * (but touches the window on its right):
		 */
		if (ContainsH(Scr->tiles[k], Rht(area)+thresh)) {
		    if (Top(Scr->tiles[k]) < Top(area)) {
			Lft(w) = Lft(area);
			Bot(w) = Top(Scr->tiles[k]) + 1;
			Rht(w) = Rht(area);
			Top(w) = Top(area);
			u = UncoveredRight (skip, w, thresh);
			e = 1;
		    }
		    if (Bot(Scr->tiles[k]) > Bot(area)) {
			Lft(w) = Lft(area);
			Bot(w) = Bot(area);
			Rht(w) = Rht(area);
			Top(w) = Bot(Scr->tiles[k]) - 1;
			if (e == 0)
			    u  = UncoveredRight (skip, w, thresh);
			else
			    u += UncoveredRight (skip, w, thresh);
			e = 1;
		    }
		    if (e == 0) {
			u = 0;
			e = 1;
		    }
		}
    return u;
}

static int ZoomRight (int bbr, int b, int t, int thresv, int thresh)
{
    int i, k, u;

    i = bbr;
    for (k = 0; k < Scr->ntiles; ++k)
	if (k != bbr)
	    /* consider tiles vertically intersecting bbr: */
	    if (IntersectsV(Scr->tiles[k], b, t))
		/* consider tiles having right edge in the bounding region
		 * and to the left of the current best tile 'i':
		 */
		if (ContainsH(Scr->tiles[bbr], Rht(Scr->tiles[k]))
			&& (Rht(Scr->tiles[k]) < Rht(Scr->tiles[i])))
		{
		    u = UncoveredRight (k, Scr->tiles[k], thresh);
		    if (u > thresv)
			i = k;
		}
    return i;
}

static int UncoveredTop (int skip, int area[4], int thresv)
{
    int u, e, k, w[4];

    u = Rht(area) - Lft(area);
    e = 0;
    for (k = 0; k < Scr->ntiles && e == 0; ++k)
	if (k != skip)
	    /* consider tiles horizontally intersecting area: */
	    if (IntersectsH(Scr->tiles[k], Lft(area), Rht(area)))
		/* consider tiles vertically intersecting boundingbox and outside window
		 * (but touches the window on its top):
		 */
		if (ContainsV(Scr->tiles[k], Top(area)+thresv)) {
		    if (Rht(Scr->tiles[k]) < Rht(area)) {
			Lft(w) = Rht(Scr->tiles[k]) + 1;
			Bot(w) = Bot(area);
			Rht(w) = Rht(area);
			Top(w) = Top(area);
			u = UncoveredTop (skip, w, thresv);
			e = 1;
		    }
		    if (Lft(Scr->tiles[k]) > Lft(area)) {
			Lft(w) = Lft(area);
			Bot(w) = Bot(area);
			Rht(w) = Lft(Scr->tiles[k]) - 1;
			Top(w) = Top(area);
			if (e == 0)
			    u  = UncoveredTop (skip, w, thresv);
			else
			    u += UncoveredTop (skip, w, thresv);
			e = 1;
		    }
		    if (e == 0) {
			u = 0;
			e = 1;
		    }
		}
    return u;
}

static int ZoomTop (int bbt, int l, int r, int thresh, int thresv)
{
    int i, k, u;

    i = bbt;
    for (k = 0; k < Scr->ntiles; ++k)
	if (k != bbt)
	    /* consider tiles horizontally intersecting bbt: */
	    if (IntersectsH(Scr->tiles[k], l, r))
		/* consider tiles having top edge in the bounding region
		 * and below the current best tile 'i':
		 */
		if (ContainsV(Scr->tiles[bbt], Top(Scr->tiles[k]))
			&& (Top(Scr->tiles[k]) < Top(Scr->tiles[i])))
		{
		    u = UncoveredTop (k, Scr->tiles[k], thresv);
		    if (u > thresh)
			i = k;
		}
    return i;
}

void TilesFullZoom (int Area[4])
{
    /*
     * First step: find screen tiles (which overlap the 'Area')
     * which define a "bounding box of maximum area".
     * The following 4 functions return indices to these tiles.
     */

    int l, b, r, t, f;

    f = 0;

nxt:;

    l = BoundingBoxLeft (Area);
    b = BoundingBoxBottom (Area);
    r = BoundingBoxRight (Area);
    t = BoundingBoxTop (Area);

    if (l >= 0 && b >=0 && r >= 0 && t >= 0) {
	/*
	 * Second step: take the above bounding box and move its borders
	 * inwards minimising the total area of "dead areas" (i.e. X11-screen
	 * regions not covered by any physical screen tiles) along borders.
	 *    The following zoom functions return indices to screen tiles
	 * defining the reduced area boundaries. (If screen tiles cover a
	 * "compact area" (i.e. which does not have "slits") then the region
	 * found here has no dead areas as well.)
	 *    In general there may be slits between screen tiles and in
	 * following thresh/thresv define tolerances (slit widths, cumulative
	 * uncovered border) which define "coverage defects" not considered
	 * as "dead areas".
	 *    This treatment is considered only along reduced bounding box
	 * boundaries and not in the "inner area". (So a screen tile
	 * arrangement could cover e.g. a circular area and a reduced bounding
	 * box is found without dead areas on borders but the inside of the
	 * arrangement can include any number of topological holes, twists
	 * and knots.)
	 */
	/*
	 * thresh - width of a vertical slit between two horizontally
	 *          adjacent screen tiles not considered as a "dead area".
	 * thresv - height of a horizontal slit between two vertically
	 *          adjacent screen tiles not considered as a "dead area".
	 *
	 * Or respectively how much uncovered border is not considered
	 * "uncovered"/"dead" area.
	 */

	int i, j;

	i = Bot(Scr->tiles[b]);
	if (i < Bot(Area))
	    i += Bot(Area), i /= 2;
	j = Top(Scr->tiles[t]);
	if (j > Top(Area))
	    j += Top(Area), j /= 2;
	l = ZoomLeft (l, i, j, /*uncovered*/10, /*slit*/10);

	i = Lft(Scr->tiles[l]);
	if (i < Lft(Area))
	    i += Lft(Area), i /= 2;
	j = Rht(Scr->tiles[r]);
	if (j > Rht(Area))
	    j += Rht(Area), j /= 2;
	b = ZoomBottom (b, i, j, 10, 10);

	i = Bot(Scr->tiles[b]);
	if (i < Bot(Area))
	    i += Bot(Area), i /= 2;
	j = Top(Scr->tiles[t]);
	if (j > Top(Area))
	    j += Top(Area), j /= 2;
	r = ZoomRight (r, i, j, 10, 10);

	i = Lft(Scr->tiles[l]);
	if (i < Lft(Area))
	    i += Lft(Area), i /= 2;
	j = Rht(Scr->tiles[r]);
	if (j > Rht(Area))
	    j += Rht(Area), j /= 2;
	t = ZoomTop (t, i, j, 10, 10);

	if (l >= 0 && b >=0 && r >= 0 && t >= 0) {
	    Lft(Area) = Lft(Scr->tiles[l]);
	    Bot(Area) = Bot(Scr->tiles[b]);
	    Rht(Area) = Rht(Scr->tiles[r]);
	    Top(Area) = Top(Scr->tiles[t]);
	    return;
	}
    }

    /* fallback: */
    Lft(Area) = Lft(Scr->tiles_bb);
    Bot(Area) = Bot(Scr->tiles_bb);
    Rht(Area) = Rht(Scr->tiles_bb);
    Top(Area) = Top(Scr->tiles_bb);

    if (f == 0) {
	f = 1;
	goto nxt;
    }
}

#endif /*TILED_SCREEN*/


/**********************************************************************
 *  Rutgers mod #1   - rocky.
 *  Procedure:
 *         fullzoom - zooms window to full height of screen or
 *                    to full height and width of screen. (Toggles
 *                    so that it can undo the zoom - even when switching
 *                    between fullzoom and vertical zoom.)
 *
 *  Inputs:
 *         tmp_win - the TwmWindow pointer
 *
 *
 **********************************************************************
 */

void
fullzoom (TwmWindow *tmp_win, int flag)
{
	if (tmp_win->zoomed == flag)
	{
		dragHeight = tmp_win->save_frame_height;
		dragWidth = tmp_win->save_frame_width;
		dragx = tmp_win->save_frame_x;
		dragy = tmp_win->save_frame_y;
		tmp_win->zoomed = ZOOM_NONE;
	}
	else
	{
		int basex, basey;
		int frame_bw_times_2;

#ifdef TILED_SCREEN

		int Area[4];

		if (Scr->use_tiles == TRUE) {
			Lft(Area) = tmp_win->frame_x;
			Rht(Area) = tmp_win->frame_x + tmp_win->frame_width  - 1;
			Bot(Area) = tmp_win->frame_y;
			Top(Area) = tmp_win->frame_y + tmp_win->frame_height - 1;

			TilesFullZoom (Area);

			basex = Lft(Area); /* Cartesian origin:   */
			basey = Bot(Area); /* (x0,y0) = (Lft,Bot) */
		}
		else
#endif
			basex = basey = 0;

		XGetGeometry (dpy, (Drawable) tmp_win->frame, &JunkRoot,
			&dragx, &dragy, (unsigned int *)&dragWidth, (unsigned int *)&dragHeight,
			&JunkBW, &JunkDepth);


		if (tmp_win->zoomed == ZOOM_NONE)
		{
			tmp_win->save_frame_x = dragx;
			tmp_win->save_frame_y = dragy;
			tmp_win->save_frame_width = dragWidth;
			tmp_win->save_frame_height = dragHeight;
		}

		tmp_win->zoomed = flag;

		frame_bw_times_2 = 2 * tmp_win->frame_bw;

		switch (flag)
		{
			case ZOOM_NONE:
				break;
			case F_ZOOM:
				dragx = tmp_win->save_frame_x;
				dragy=basey;
				dragWidth = tmp_win->save_frame_width;
#ifdef TILED_SCREEN
				if (Scr->use_tiles == TRUE)
					dragHeight = AreaHeight(Area) - frame_bw_times_2;
				else
#endif
					dragHeight = Scr->MyDisplayHeight - frame_bw_times_2;
				break;
			case F_HORIZOOM:
				dragx = basex;
				dragy = tmp_win->save_frame_y;
#ifdef TILED_SCREEN
				if (Scr->use_tiles == TRUE)
					dragWidth = AreaWidth(Area) - frame_bw_times_2;
				else
#endif
					dragWidth = Scr->MyDisplayWidth - frame_bw_times_2;
				dragHeight = tmp_win->save_frame_height;
				break;
			case F_FULLZOOM:
				dragx = basex;
				dragy = basey;
#ifdef TILED_SCREEN
				if (Scr->use_tiles == TRUE) {
					dragWidth = AreaWidth(Area) - frame_bw_times_2;
					dragHeight = AreaHeight(Area) - frame_bw_times_2;
				}
				else
#endif
				{
					dragWidth = Scr->MyDisplayWidth - frame_bw_times_2;
					dragHeight = Scr->MyDisplayHeight - frame_bw_times_2;
				}
				break;
			case F_LEFTZOOM:
				dragx = basex;
				dragy = basey;
#ifdef TILED_SCREEN
				if (Scr->use_tiles == TRUE) {
					dragWidth = AreaWidth(Area) / 2 - frame_bw_times_2;
					dragHeight = AreaHeight(Area) - frame_bw_times_2;
				}
				else
#endif
				{
					dragWidth = Scr->MyDisplayWidth / 2 - frame_bw_times_2;
					dragHeight = Scr->MyDisplayHeight - frame_bw_times_2;
				}
				break;
			case F_RIGHTZOOM:
				dragy = basey;
#ifdef TILED_SCREEN
				if (Scr->use_tiles == TRUE) {
					dragx = basex + AreaWidth(Area) / 2;
					dragWidth = AreaWidth(Area) / 2 - frame_bw_times_2;
					dragHeight = AreaHeight(Area) - frame_bw_times_2;
				}
				else
#endif
				{
					dragx = basex + Scr->MyDisplayWidth / 2;
					dragWidth = Scr->MyDisplayWidth / 2 - frame_bw_times_2;
					dragHeight = Scr->MyDisplayHeight - frame_bw_times_2;
				}
				break;
			case F_TOPZOOM:
				dragx = basex;
				dragy = basey;
#ifdef TILED_SCREEN
				if (Scr->use_tiles == TRUE) {
					dragWidth = AreaWidth(Area) - frame_bw_times_2;
					dragHeight = AreaHeight(Area) / 2 - frame_bw_times_2;
				}
				else
#endif
				{
					dragWidth = Scr->MyDisplayWidth - frame_bw_times_2;
					dragHeight = Scr->MyDisplayHeight / 2 - frame_bw_times_2;
				}
				break;
			case F_BOTTOMZOOM:
				dragx = basex;
#ifdef TILED_SCREEN
				if (Scr->use_tiles == TRUE) {
					dragy = basey + AreaHeight(Area) / 2;
					dragWidth = AreaWidth(Area) - frame_bw_times_2;
					dragHeight = AreaHeight(Area) / 2 - frame_bw_times_2;
				}
				else
#endif
				{
					dragy = basey + Scr->MyDisplayHeight / 2;
					dragWidth = Scr->MyDisplayWidth - frame_bw_times_2;
					dragHeight = Scr->MyDisplayHeight / 2 - frame_bw_times_2;
				}
				break;
		}
	}

	if (!Scr->NoRaiseResize) {
		XRaiseWindow(dpy, tmp_win->frame);

		RaiseStickyAbove(); /* DSE */
		RaiseAutoPan();
	}

	ConstrainSize(tmp_win, &dragWidth, &dragHeight);
	SetupWindow (tmp_win, dragx , dragy , dragWidth, dragHeight, -1);

	/* djhjr - 9/10/99 */
	ResizeTwmWindowContents(tmp_win, dragWidth, dragHeight);

	/* 9/21/96 - djhjr */
	if ((Scr->WarpCursor || LookInList(Scr->WarpCursorL, tmp_win->full_name, &tmp_win->class)))
		WarpToWindow (tmp_win);

	XUngrabPointer (dpy, CurrentTime);
	XUngrabServer (dpy);
}

/*
 * adjust contents of iconmgrs, doors and the desktop - djhjr - 9/10/99
 */
void ResizeTwmWindowContents(tmp_win, width, height)
    TwmWindow *tmp_win;
	int width, height;
{
	TwmDoor *door;
	int ncols;

	if (tmp_win->iconmgr)
	{
		ncols = tmp_win->iconmgrp->cur_columns;
		if (ncols == 0) ncols = 1;

/* djhjr - 4/24/96
		tmp_win->iconmgrp->width = (int) ((width *
*/
		tmp_win->iconmgrp->width =
				(int)(((width - 2 * tmp_win->frame_bw3D) *

				(long) tmp_win->iconmgrp->columns) / ncols);
		PackIconManager(tmp_win->iconmgrp);
	}
	else if (tmp_win->w == Scr->VirtualDesktopDisplayOuter)
		ResizeDesktopDisplay(width, height);
	else if (XFindContext(dpy, tmp_win->w, DoorContext, (caddr_t *)&door) != XCNOENT)
		RedoDoorName(tmp_win, door);
}

void SetFrameShape (tmp)
    TwmWindow *tmp;
{
    /*
     * see if the titlebar needs to move
     */
    if (tmp->title_w.win) {
	int oldx = tmp->title_x, oldy = tmp->title_y;
	ComputeTitleLocation (tmp);
	if (oldx != tmp->title_x || oldy != tmp->title_y)
	  XMoveWindow (dpy, tmp->title_w.win, tmp->title_x, tmp->title_y);
    }

    /*
     * The frame consists of the shape of the contents window offset by
     * title_height or'ed with the shape of title_w.win (which is always
     * rectangular).
     */
    if (tmp->wShaped) {
	/*
	 * need to do general case
	 */
	XShapeCombineShape (dpy, tmp->frame, ShapeBounding,
/* djhjr - 4/24/96
			    0, tmp->title_height, tmp->w,
*/
			    tmp->frame_bw3D, tmp->title_height + tmp->frame_bw3D, tmp->w,

			    ShapeBounding, ShapeSet);
	if (tmp->title_w.win) {
	    XShapeCombineShape (dpy, tmp->frame, ShapeBounding,
				tmp->title_x + tmp->frame_bw,
				tmp->title_y + tmp->frame_bw,
				tmp->title_w.win, ShapeBounding,
				ShapeUnion);
	}
    } else {
	/*
	 * can optimize rectangular contents window
	 */
	if (tmp->squeeze_info) {
	    XRectangle  newBounding[3];
	    XRectangle  newClip[3];
	    int count = 3, order = YXSorted, fbw2 = 2 * tmp->frame_bw;
	    int client_width = tmp->attr.width + fbw2 + 2 * tmp->frame_bw3D;

	    /*
	     * Build the border clipping rectangles; one around title, one
	     * around window.  The title_[xy] field already have had frame_bw
	     * subtracted off them so that they line up properly in the frame.
	     *
	     * The frame_width and frame_height do *not* include borders.
	     */
	    /* border */
/* djhjr - 4/24/96
	    newBounding[0].x = tmp->title_x;
	    newBounding[0].y = tmp->title_y;
	    newBounding[0].width = tmp->title_width + fbw2;
	    newBounding[0].height = tmp->title_height;
	    newBounding[1].x = -tmp->frame_bw;
	    newBounding[1].y = Scr->TitleHeight;
	    newBounding[1].width = tmp->attr.width + fbw2;
	    newBounding[1].height = tmp->attr.height + fbw2;
*/
	    newBounding[0].x = tmp->title_x - tmp->frame_bw3D;
	    newBounding[0].y = tmp->title_y - tmp->frame_bw3D;
	    newBounding[0].width = tmp->title_width + fbw2 + 2 * tmp->frame_bw3D;
	    newBounding[0].height = tmp->title_height + tmp->frame_bw3D;

	    /* was 'Scr->use3Dborders' - djhjr - 8/11/98 */
	    if (Scr->BorderBevelWidth > 0 &&
			newBounding[0].width < client_width)
	    {
		/* re-ordered arrays for XYSorted - djhjr - 11/5/03 */
		newBounding[1].x = -tmp->frame_bw3D;
		newBounding[1].y = tmp->title_height;
		newBounding[1].width = tmp->attr.width + 3 * tmp->frame_bw3D;
		newBounding[1].height = tmp->frame_bw3D;
		newBounding[2].x = -tmp->frame_bw;
		newBounding[2].y = Scr->TitleHeight + tmp->frame_bw3D;
		newBounding[2].width = client_width;
		newBounding[2].height = tmp->attr.height + fbw2 + tmp->frame_bw3D;
	    }
	    else
	    {
		newBounding[1].x = -tmp->frame_bw;
		newBounding[1].y = Scr->TitleHeight + tmp->frame_bw3D;
		newBounding[1].width = client_width;
		newBounding[1].height = tmp->attr.height + fbw2 + tmp->frame_bw3D;
		count = 2;
		order = YXBanded;
	    }

	    /* insides */
/* djhjr - 4/24/96
	    newClip[0].x = tmp->title_x + tmp->frame_bw;
	    newClip[0].y = 0;
	    newClip[0].width = tmp->title_width;
	    newClip[0].height = Scr->TitleHeight;
	    newClip[1].x = 0;
	    newClip[1].y = tmp->title_height;
	    newClip[1].width = tmp->attr.width;
	    newClip[1].height = tmp->attr.height;
*/
	    newClip[0].x = tmp->title_x + tmp->frame_bw - tmp->frame_bw3D;
	    newClip[0].y = 0;
	    newClip[0].width = tmp->title_width + 2 * tmp->frame_bw3D;
	    newClip[0].height = Scr->TitleHeight + tmp->frame_bw3D;

	    if (count == 3)
	    {
		/* re-ordered arrays for XYSorted - djhjr - 11/5/03 */
		newClip[1].x = newBounding[1].x;
		newClip[1].y = newBounding[1].y;
		newClip[1].width = newBounding[1].width;
		newClip[1].height = newBounding[1].height;
		newClip[2].x = 0;
		newClip[2].y = tmp->title_height + tmp->frame_bw3D;
		newClip[2].width = client_width;
		newClip[2].height = tmp->attr.height + tmp->frame_bw3D;
	    }
	    else
	    {
		newClip[1].x = 0;
		newClip[1].y = tmp->title_height + tmp->frame_bw3D;
		newClip[1].width = client_width;
		newClip[1].height = tmp->attr.height + tmp->frame_bw3D;
	    }

	    XShapeCombineRectangles (dpy, tmp->frame, ShapeBounding, 0, 0,
				     newBounding, count, ShapeSet, order);
	    XShapeCombineRectangles (dpy, tmp->frame, ShapeClip, 0, 0,
				     newClip, count, ShapeSet, order);
	} else {
	    (void) XShapeCombineMask (dpy, tmp->frame, ShapeBounding, 0, 0,
 				      None, ShapeSet);
	    (void) XShapeCombineMask (dpy, tmp->frame, ShapeClip, 0, 0,
				      None, ShapeSet);
	}
    }
}

/*
 * Squeezed Title:
 *
 *                         tmp->title_x
 *                   0     |
 *  tmp->title_y   ........+--------------+.........  -+,- tmp->frame_bw
 *             0   : ......| +----------+ |....... :  -++
 *                 : :     | |          | |      : :   ||-Scr->TitleHeight
 *                 : :     | |          | |      : :   ||
 *                 +-------+ +----------+ +--------+  -+|-tmp->title_height
 *                 | +---------------------------+ |  --+
 *                 | |                           | |
 *                 | |                           | |
 *                 | |                           | |
 *                 | |                           | |
 *                 | |                           | |
 *                 | +---------------------------+ |
 *                 +-------------------------------+
 *
 *
 * Unsqueezed Title:
 *
 *                 tmp->title_x
 *                 | 0
 *  tmp->title_y   +-------------------------------+  -+,tmp->frame_bw
 *             0   | +---------------------------+ |  -+'
 *                 | |                           | |   |-Scr->TitleHeight
 *                 | |                           | |   |
 *                 + +---------------------------+ +  -+
 *                 |-+---------------------------+-|
 *                 | |                           | |
 *                 | |                           | |
 *                 | |                           | |
 *                 | |                           | |
 *                 | |                           | |
 *                 | +---------------------------+ |
 *                 +-------------------------------+
 *
 *
 *
 * Dimensions and Positions:
 *
 *     frame orgin                 (0, 0)
 *     frame upper left border     (-tmp->frame_bw, -tmp->frame_bw)
 *     frame size w/o border       tmp->frame_width , tmp->frame_height
 *     frame/title border width    tmp->frame_bw
 *     extra title height w/o bdr  tmp->title_height = TitleHeight + frame_bw
 *     title window height         Scr->TitleHeight
 *     title origin w/o border     (tmp->title_x, tmp->title_y)
 *     client origin               (0, Scr->TitleHeight + tmp->frame_bw)
 *     client size                 tmp->attr.width , tmp->attr.height
 *
 * When shaping, need to remember that the width and height of rectangles
 * are really deltax and deltay to lower right handle corner, so they need
 * to have -1 subtracted from would normally be the actual extents.
 */
