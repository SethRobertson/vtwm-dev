
/*****************************************************************************/
/**               Copyright 1990 by Solbourne Computer Inc.                 **/
/**                          Longmont, Colorado                             **/
/**                                                                         **/
/**                           All Rights Reserved                           **/
/**                                                                         **/
/**    Permission to use, copy, modify, and distribute this software and    **/
/**    its documentation  for  any  purpose  and  without  fee is hereby    **/
/**    granted, provided that the above copyright notice appear  in  all    **/
/**    copies and that both  that  copyright  notice  and  this  permis-    **/
/**    sion  notice appear in supporting  documentation,  and  that  the    **/
/**    name of Solbourne not be used in advertising                         **/
/**    in publicity pertaining to distribution of the  software  without    **/
/**    specific, written prior permission.                                  **/
/**                                                                         **/
/**    SOLBOURNE COMPUTER INC. DISCLAIMS ALL WARRANTIES WITH REGARD         **/
/**    TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES  OF  MERCHANT-    **/
/**    ABILITY  AND  FITNESS,  IN  NO  EVENT SHALL SOLBOURNE                **/
/**    BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL  DAM-           **/
/**    AGES OR  ANY DAMAGES WHATSOEVER  RESULTING FROM LOSS OF USE, DATA    **/
/**    OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER    **/
/**    TORTIOUS ACTION, ARISING OUT OF OR IN  CONNECTION  WITH  THE  USE    **/
/**    OR PERFORMANCE OF THIS SOFTWARE.                                     **/
/*****************************************************************************/

/**********************************************************************
 *
 * $XConsortium: move.c,v 1.140 90/03/23 11:42:33 jim Exp $
 *
 * New window move code to allow interaction with the virtual desktop
 * All of this code came from the Solbourne Window Manager (swm)
 *
 * 23-Aug-90 Tom LaStrange        Initial Version.
 *
 **********************************************************************/

/* This code put into vtwm: 12 Aug 1993 by njw@cs.city.ac.uk and andy@cs.city.ac.uk
 * Modified quite a bit from Tom's original code.  Mainly because I couldn't understand
 * Just what he was trying to do in many places.  I hope this is easier to grok.
 */

#if !defined(lint) && !defined(SABER)
static char RCSinfo[]=
"$XConsortium: move.c,v 1.140 90/03/23 11:42:33 jim Exp $";
#endif

#include <stdio.h>
#include <X11/X.h>
#include <X11/Xatom.h>
#include "twm.h"
#include "screen.h"
#include "desktop.h"
#include "move.h"
#include "events.h"
#include "parse.h"

static int dragX;
static int dragY;
static int origX;
static int origY;
static unsigned int dragWidth;
static unsigned int dragHeight;
static unsigned int dragBW;
static int dragBW2;
static int diffX;
static int diffY;
static int offsetX;
static int offsetY;
static int rootX;
static int rootY;
static Window outlineWindow;
static int scale;
static int titleHeight;
static int doingMove = False;	/* whether the window we're moving is allowed to go in/out panner */

static void reallyStartMove();
static void doMove();
static void getPointer();

/***********************************************************************
 *
 *  Procedure:
 *	DragFrame - move the window frame
 *	
 *  Returned Value:
 *	None
 *
 ***********************************************************************
 */

void
DragFrame(tmp_win, ev, pulldown)
TwmWindow *tmp_win;
XButtonEvent *ev;
int pulldown;
{
    int cancel;
    int x_root, y_root;

    x_root = ev->x_root;
    y_root = ev->y_root;
    StartMove(tmp_win, tmp_win->frame, tmp_win->title_height,
	&x_root, &y_root, &cancel, OUT_PANNER, 1, 0, 0, False, pulldown);

    if (!cancel && WindowMoved) {
	SetupWindow (tmp_win, x_root, y_root,
	     tmp_win->frame_width, tmp_win->frame_height, -1);
	if (!Scr->NoRaiseMove)
	    RaiseFrame(tmp_win,1);
    }
}

/***********************************************************************
 *
 *  Procedure:
 *	DragIcon - move the window icon
 *	
 *  Returned Value:
 *	None
 *
 ***********************************************************************
 */

void
DragIcon(tmp_win, ev, pulldown)
TwmWindow *tmp_win;
XButtonEvent *ev;
int pulldown;
{
    int cancel;
    int x_root, y_root;

    x_root = ev->x_root;
    y_root = ev->y_root;
    StartMove(tmp_win, tmp_win->icon->w, 0, &x_root, &y_root, &cancel,
	OUT_PANNER, 1, 0, 0, False, pulldown);

    if (!cancel && WindowMoved) {
	MoveIcon(tmp_win, x_root, y_root);
	if (!Scr->NoRaiseMove)
	    RaiseIcon(tmp_win);
    }
}

/***********************************************************************
 *
 *  Procedure:
 *	StartMove - start a move operation on an icon or a frame
 *	
 *  Returned Value:
 *	None
 *
 ***********************************************************************
 */

void
StartMove(tmp_win, window, title_height, x_root, y_root, cancel,
    panner, move_scale, objWidth, objHeight, adding, pulldown)
TwmWindow *tmp_win;
Window window;
int title_height;
int *x_root;			/* at end, the position of the window */
int *y_root;			/* ditto */
int *cancel;			/* how the move routine ended. E.g Button3 was pressed */
int panner;			/* whether it starts/ends in the panner */
int move_scale;			/* pretty useless */
unsigned objWidth;		/* if IN_PANNER */
unsigned objHeight;		/* if IN_PANNER */
int adding;			/* adding a window from add_window() */
int pulldown;			/* moving window from a pulldown menu */
{
    Window junkRoot, junkChild;
    unsigned int junkDepth, numChildren;
    int junkX, junkY;
    int junkxroot, junkyroot;
    unsigned int junkMask;
    int first;
    XSetWindowAttributes attr;

    if (!Scr->NoGrabServer || !tmp_win->OpaqueMove)
	XGrabServer(dpy);

    if (tmp_win->OpaqueMove) {
	attr.save_under = True;
	XChangeWindowAttributes(dpy, window, CWSaveUnder, &attr);
    }

    /* Before we start, make sure the desktop is at top: so we can enter it */
    if ((!Scr->NoRaiseDesktop) && Scr->Virtual) {
	XRaiseWindow(dpy, Scr->VirtualDesktopDisplayTwin->frame);
    }
    if (!adding) {
	XGrabPointer(dpy, Scr->Root, True,
	    PointerMotionMask | EnterWindowMask | LeaveWindowMask | ButtonPressMask | ButtonReleaseMask,
	    GrabModeAsync, GrabModeAsync,
	    Scr->Root, Scr->MoveCursor, CurrentTime);
    }

    rootX = *x_root;
    rootY = *y_root;

    /* how big is this thing we are moving? */
    XGetGeometry(dpy, window, &junkRoot, &junkX, &junkY, &dragWidth, &dragHeight, &dragBW, &junkDepth);
    origX = junkX;
    origY = junkY;

    outlineWindow = (panner==OUT_PANNER) ? Scr->Root : Scr->VirtualDesktopDisplay;
    /* translate its coordinates to root coordinates */
    XTranslateCoordinates(dpy, window, outlineWindow, -dragBW, -dragBW, &dragX, &dragY, &junkChild);

    dragBW2 = 2 * dragBW;
    dragWidth += dragBW2;
    dragHeight += dragBW2;
    diffX = dragX - *x_root;
    diffY = dragY - *y_root;

    if (!tmp_win->nailed && !tmp_win->isicon)
	doingMove = True;
    offsetX = offsetY = 0;
    scale = move_scale;

    if (panner == OUT_PANNER)
    {
	titleHeight = title_height;
	objWidth = dragWidth;
	objHeight = dragHeight;
	outlineWindow = Scr->Root;
    } else {
	titleHeight = 0;
	outlineWindow = Scr->VirtualDesktopDisplay;
    }

    first = True;
    while (True)
    {
	*cancel = False;

	XTranslateCoordinates(dpy, Scr->Root, outlineWindow, 0, 0, &offsetX, &offsetY, &junkChild);

	reallyStartMove(tmp_win, window, x_root, y_root, cancel, outlineWindow, &first, adding, pulldown, panner);

	if (*cancel == IN_PANNER)
	{
	    if (window == tmp_win->frame)
		window = tmp_win->VirtualDesktopDisplayWindow;
	    else
		window = 0; /* XXX */
	    panner = IN_PANNER;
	    dragWidth /= Scr->VirtualDesktopDScale;
	    dragHeight /= Scr->VirtualDesktopDScale;
	    diffX /= Scr->VirtualDesktopDScale;
	    diffY /= Scr->VirtualDesktopDScale;
	    outlineWindow = Scr->VirtualDesktopDisplay;
	    scale = Scr->VirtualDesktopDScale;
	    titleHeight = 0;
	}
	else if (*cancel == OUT_PANNER)
	{
	    if (window == tmp_win->VirtualDesktopDisplayWindow)
		window = tmp_win->frame;
	    else {
		if (tmp_win->icon) {
		    window = tmp_win->icon->w;
		} else {
		    window = tmp_win->frame; /* XXX */
		}
	    }
	    panner = OUT_PANNER;
	    dragWidth = objWidth;
	    dragHeight = objHeight;
	    diffX *= Scr->VirtualDesktopDScale;
	    diffY *= Scr->VirtualDesktopDScale;
	    outlineWindow = Scr->Root;
	    scale = 1;
	    titleHeight = title_height;
	}
	else
	    break;
    }

    doingMove = False;
    if (!adding)
    {
	XUngrabPointer(dpy, CurrentTime);
	XUngrabServer(dpy);
    }
    /* can't cancel an opaque move */
    if (tmp_win->OpaqueMove)
	*cancel = False;

    if (*cancel)
	WindowMoved = False;

    if (tmp_win->OpaqueMove) {
	attr.save_under = False;
	if (window)
	    XChangeWindowAttributes(dpy, window, CWSaveUnder, &attr);
    }
}

/***********************************************************************
 *
 *  Procedure:
 *	reallyStartMove - do the meat of the move operation
 *	
 *  Returned Value:
 *	None
 *
 ***********************************************************************
 */

static void
reallyStartMove(tmp_win, window, x_root, y_root,
    cancel, outlineWindow, first, adding, pulldown,panner)
TwmWindow *tmp_win;
Window window;
int *x_root;
int *y_root;
int *cancel;
Window outlineWindow;
int *first;
int adding;
int pulldown;
int panner;
{
    int xdest, ydest;
    int done;

    xdest = *x_root;
    ydest = *y_root;
    done = False;

    /* put up the initial outline if adding a window */
    if (*first && adding)
	doMove(tmp_win, window, *x_root, *y_root, &xdest, &ydest, True);
	
    while (True) {
	getPointer(outlineWindow, x_root, y_root, cancel, &done, first, adding, pulldown);
	if (done) {
	    doMove(tmp_win, window, *x_root, *y_root, &xdest, &ydest, panner, False);
	    break;
	}
	if (!*cancel) {
	    doMove(tmp_win, window, *x_root, *y_root, &xdest, &ydest, panner, True);
	    DisplaySizeInfo(MOVE_FMT, xdest, ydest);
	}
    }

    *x_root = xdest;
    *y_root = ydest;
}

static void
doMove(tmp_win, window, x_root, y_root, x_dest, y_dest, panner, paint)
TwmWindow *tmp_win;
Window window;
int x_root;
int y_root;
int *x_dest;
int *y_dest;
int panner;
int paint;
{
    int xl, yt;
    char str[20];
    Window actual, virtual;

    int ax, ay, vx, vy;

    dragX = x_root;
    dragY = y_root;

    xl = dragX + diffX;
    yt = dragY + diffY;

    if (MoveFunction != F_FORCEMOVE && Scr->DontMoveOff && outlineWindow != Scr->VirtualDesktopDisplay) {
	int vdtx = 0, vdty = 0;

	if (xl < vdtx)
	    xl = vdtx;
	else if ((xl + dragWidth) > (Scr->MyDisplayWidth + vdtx))
	    xl = vdtx + Scr->MyDisplayWidth - dragWidth;
	if (yt < vdty)
	    yt = vdty;
	else if ((yt + dragHeight) > (Scr->MyDisplayHeight + vdty))
	    yt = vdty + Scr->MyDisplayHeight - dragHeight;
    }

    /* Work out where the windows should be. */
    if (panner != IN_PANNER) {
	ax = xl;
	ay = yt;
    
	vx = (xl + Scr->VirtualDesktopX) / Scr->VirtualDesktopDScale;
	vy = (yt + Scr->VirtualDesktopY) / Scr->VirtualDesktopDScale;
    } else {
	vx = xl;
	vy = yt;

	ax = (xl * Scr->VirtualDesktopDScale) - Scr->VirtualDesktopX;
	ay = (yt * Scr->VirtualDesktopDScale) - Scr->VirtualDesktopY;
    }

    /* If we are supposed to be providing user feedback then do so. */
    if (paint) {
	if (Scr->Virtual && tmp_win->OpaqueMove && !tmp_win->nailed) {
	    XMoveWindow(dpy, tmp_win->VirtualDesktopDisplayWindow, vx, vy);
	}
	if (tmp_win->OpaqueMove) {
	    if (tmp_win->isicon && tmp_win->icon) {
		XMoveWindow(dpy, tmp_win->icon->w, ax, ay);
	    } else {
		XMoveWindow(dpy, tmp_win->frame, ax, ay);
	    }
	} else {
	    MoveOutline(outlineWindow, xl, yt, dragWidth, dragHeight, 0, titleHeight);
	}
    }
    *x_dest = ax;
    *y_dest = ay;
}

static void
getPointer(window, x_root, y_root, cancel, done, first, adding, pulldown)
int *x_root;
int *y_root;
int *cancel;
int *done;
int *first;
int adding;
int pulldown;
{
    Window junkChild;
    int doingFine;
    XEvent event,bakevent;

    int xdest, ydest;
    unsigned mask;
    static int buttons;

    if (*first) {
	*first = False;
	buttons = 0;
    }

    doingFine = True;
    while (doingFine) {
	XMaskEvent(dpy, ButtonPressMask|ButtonReleaseMask|PointerMotionMask|
			EnterWindowMask|LeaveWindowMask|ExposureMask, &event);
	switch (event.type) {
	    case ButtonPress:
		if (pulldown) {
		    if (buttons++)
			pulldown = False;
		}
		if (!pulldown) {
		    *cancel = event.xbutton.button;
		    doingFine = False;
		    MoveOutline(outlineWindow, 0,0,0,0,0,0);
		    if (adding) {
			*done = True;
		    }
		    else {
			XGrabPointer(dpy, Scr->Root, True,
			    ButtonPressMask | ButtonReleaseMask,
			    GrabModeAsync, GrabModeAsync,
			    Scr->Root, Scr->WaitCursor, CurrentTime);
		    }
		}
		break;
	    case ButtonRelease:
		MoveOutline(outlineWindow, 0,0,0,0,0,0);

                /* clear the mask bit for the button just released */
                mask = (Button1Mask|Button2Mask|Button3Mask|Button4Mask|Button5Mask);
                switch (event.xbutton.button)
                {
                    case Button1: mask &= ~Button1Mask; break;
                    case Button2: mask &= ~Button2Mask; break;
                    case Button3: mask &= ~Button3Mask; break;
                    case Button4: mask &= ~Button4Mask; break;
                    case Button5: mask &= ~Button5Mask; break;
                }

                /* if all buttons have been released */
                if ((event.xbutton.state & mask) == 0)
                {
		    ButtonPressed = -1;
                    *done = True;
		    doingFine = False;

		    /* RJC: added code to do a final move here */
		    if (!WindowMoved &&
			(abs(event.xbutton.x_root - rootX) >= Scr->MoveDelta ||
			 abs(event.xbutton.y_root - rootY) >= Scr->MoveDelta))
			{
			    WindowMoved = True;
			}
		    if (WindowMoved) {
		        *x_root = event.xbutton.x_root + offsetX;
			*y_root = event.xbutton.y_root + offsetY;
		    }
		}
		break;
	    case EnterNotify:
                if (doingMove && event.xcrossing.window == Scr->VirtualDesktopDisplayOuter && 
		    event.xcrossing.detail != NotifyInferior)
                {
                    MoveOutline(outlineWindow, 0,0,0,0,0,0);
                    *cancel = IN_PANNER;
                    *done = True;
		    doingFine = False;
                }
		break;
	    case LeaveNotify:
                if (doingMove && event.xcrossing.window == Scr->VirtualDesktopDisplayOuter && 
		    event.xcrossing.detail != NotifyInferior && event.xcrossing.mode == NotifyNormal)
                {
                    MoveOutline(outlineWindow, 0,0,0,0,0,0);
                    *cancel = OUT_PANNER;
                    *done = True;
		    doingFine = False;
                }
		break;
	    case MotionNotify:
 		while(XCheckTypedEvent(dpy,MotionNotify,&event));
		if (!WindowMoved &&
		    (abs(event.xmotion.x_root - rootX) >= Scr->MoveDelta ||
		     abs(event.xmotion.y_root - rootY) >= Scr->MoveDelta))
		{
		    WindowMoved = True;
		}
		if (WindowMoved) {
		    *x_root = event.xmotion.x_root + offsetX;
		    *y_root = event.xmotion.y_root + offsetY;
		    doingFine = False;
		}
		break;

	    /* handle other generic events here */
	    case Expose:
		bakevent = Event;
		Event=event;
		DispatchEvent();
		Event=bakevent;
		break;
	}
    }
}



