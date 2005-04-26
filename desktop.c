/*
 * $Id: desktop.c,v 3.0 90/11/20 16:13:09 dme Exp Locker: dme $
 *
 * Copyright (c) 1990 Dave Edmondson.
 * Copyright (c) 1990 Imperial College of Science, Technoology & Medicine
 * All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of Dave Edmondson or Imperial College
 * not be used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission. Dave Edmondson and
 * Imperial College make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 */

#include <stdio.h>
#include "twm.h"
#include "screen.h"
#include "add_window.h"
#include "parse.h"
#include "events.h"
#include "desktop.h"

extern void SetRealScreenInternal();
extern void SetRealScreen();
extern void SnapRealScreen();

static void GetDesktopWindowCoordinates(tmp_win, x, y, w, h)
TwmWindow *tmp_win;
int *x, *y, *w, *h;
{	/* Stig Ostholm <ostholm%ce.chalmers.se@uunet> */
	if (tmp_win->nailed)
	{	if (x)
			*x = tmp_win->frame_x / Scr->VirtualDesktopDScale;
		if (y)
			*y = tmp_win->virtual_frame_y / Scr->VirtualDesktopDScale;
		/* RFB 4/92 no SCALE_D */
		/* *x = SCALE_D(tmp_win->virtual_frame_x); */
		/* *y = SCALE_D(tmp_win->virtual_frame_y); */
	} else {
		if (x)
			*x = tmp_win->virtual_frame_x / Scr->VirtualDesktopDScale;
		if (y)
			*y = tmp_win->virtual_frame_y / Scr->VirtualDesktopDScale;
		/* RFB 4/92 no SCALE_D */
		/* *x = SCALE_D(tmp_win->virtual_frame_x); */
		/* *y = SCALE_D(tmp_win->virtual_frame_y); */
	}

	if (w)
	{	*w = SCALE_D(
			tmp_win->frame_width + Scr->VirtualDesktopDScale / 2
			+ tmp_win->frame_bw + tmp_win->frame_bw )
			- 2;
		if ( *w <= 0 ) *w = 1;  /* 4/92 RFB */
	}

	if (h)
	{	*h = SCALE_D(
			tmp_win->frame_height + Scr->VirtualDesktopDScale / 2
/* #ifdef SHAPE */
				/* + tmp_win->title_height  */
/* #ifdef SHAPE */
			+ tmp_win->frame_bw + tmp_win->frame_bw ) - 2;
/* 4/92 RFB -- subtract borderwidth from windowwidth... */
		if ( *h <= 0 ) *h = 1;  /* 4/92 RFB */
	}
}	/* Stig Ostholm <ostholm%ce.chalmers.se@uunet> */


/*
 * create the virtual desktop display and store the window in the screen structure
 */
void CreateDesktopDisplay()
{
	int width, height;
	int border;

	if (!Scr->Virtual)
		return;

	width = Scr->VirtualDesktopWidth/Scr->VirtualDesktopDScale;
	height = Scr->VirtualDesktopHeight/Scr->VirtualDesktopDScale;

	/* we have some checking for negative (x,y) to do */
	if (Scr->VirtualDesktopDX < 0) {
		Scr->VirtualDesktopDX = Scr->MyDisplayWidth - width -
			(2 * Scr->BorderWidth) + Scr->VirtualDesktopDX;
	}
	if (Scr->VirtualDesktopDY < 0) {
		Scr->VirtualDesktopDY = Scr->MyDisplayHeight - height -
			(2 * Scr->BorderWidth) + Scr->VirtualDesktopDY;
	}

	Scr->VirtualDesktopDisplayOuter =
		XCreateSimpleWindow(dpy, Scr->Root,
				    Scr->VirtualDesktopDX, Scr->VirtualDesktopDY,
				    width, height,
				    Scr->BorderWidth,
				    Scr->Black, Scr->VirtualDesktopDisplayC.back);

	if ( width > Scr->VirtualDesktopMaxWidth )
		Scr->VirtualDesktopMaxWidth = width;
	if ( height > Scr->VirtualDesktopMaxHeight )
		Scr->VirtualDesktopMaxHeight = height;
/* vtwm 5.2: RFB growable but not unreasonable interior window! */

	Scr->VirtualDesktopDisplay =
		XCreateSimpleWindow(dpy, Scr->VirtualDesktopDisplayOuter,
				    0, 0,
					Scr->VirtualDesktopMaxWidth,
					Scr->VirtualDesktopMaxHeight,
				    0,
				    Scr->VirtualDesktopDisplayBorder,
					Scr->VirtualC.back);/*RFB VCOLOR*/

	XDefineCursor( dpy, Scr->VirtualDesktopDisplay,
		Scr->VirtualCursor );	/*RFBCURSOR*/

	if ( Scr->virtualPm ) /*RFB PIXMAP*/
	{	/* Background pixmap, copied from tvtwm */
	    Pixmap pm = None;
	    GC gc;
	    XGCValues gcv;
	    unsigned long valuemask;

		pm = XCreatePixmap( dpy, Scr->VirtualDesktopDisplay,
				    Scr->virtual_pm_width, Scr->virtual_pm_height,
				    Scr->d_depth);
		gcv.foreground = Scr->VirtualC.fore;
		gcv.background = Scr->VirtualC.back;
		gcv.graphics_exposures = False;
		gc = XCreateGC (dpy, Scr->Root,
				(GCForeground|GCBackground|GCGraphicsExposures),
				&gcv);
		if (gc)
		{
			XCopyPlane (dpy, Scr->virtualPm, pm, gc, 0, 0,
				Scr->virtual_pm_width, Scr->virtual_pm_height,
				0, 0, 1);
		    XFreeGC (dpy, gc);
			XSetWindowBackgroundPixmap( dpy, Scr->VirtualDesktopDisplay,
				pm );
			XClearWindow( dpy, Scr->VirtualDesktopDisplay );
		}
		XFreePixmap (dpy, pm);
	}

	XSetStandardProperties(dpy, Scr->VirtualDesktopDisplayOuter,
			       "Virtual Desktop", "Virtual Desktop",
			       None, NULL, 0, NULL);

/* Stig Ostholm moved a few lines away from here */

	border = 0;
	if ( Scr->UseRealScreenBorder )
		{
		/* border = 2; */
		border = Scr->RealScreenBorderWidth; /* DSE */
		}

	/* create the real screen display */
	Scr->VirtualDesktopDScreen =
		XCreateSimpleWindow(dpy, Scr->VirtualDesktopDisplay,
				    0, 0,
				    SCALE_D(Scr->MyDisplayWidth - 2*border ),
				    SCALE_D(Scr->MyDisplayHeight - 2*border ),
				    border, /* make it distinctive */
/* RFB 4/92: make borderwidth 0 instead of 2 */
/* RFB 5.2: some people need the border... */
				    Scr->VirtualDesktopDisplayBorder,
					Scr->RealScreenC.back ); /* RFB 4/92 */

	if ( Scr->RealScreenPm ) /*RFB PIXMAP*/
	{	/* Background pixmap */
	    Pixmap pm = None;
	    GC gc;
	    XGCValues gcv;
	    unsigned long valuemask;

		pm = XCreatePixmap( dpy, Scr->VirtualDesktopDScreen,
				    Scr->RealScreen_pm_width, Scr->RealScreen_pm_height,
				    Scr->d_depth);
		gcv.foreground = Scr->RealScreenC.fore;
		gcv.background = Scr->RealScreenC.back;
		gcv.graphics_exposures = False;
		gc = XCreateGC (dpy, Scr->Root,
				(GCForeground|GCBackground|GCGraphicsExposures),
				&gcv);
		if (gc)
		{
			XCopyPlane (dpy, Scr->RealScreenPm, pm, gc, 0, 0,
				Scr->RealScreen_pm_width, Scr->RealScreen_pm_height,
				0, 0, 1);
		    XFreeGC (dpy, gc);
			XSetWindowBackgroundPixmap( dpy, Scr->VirtualDesktopDScreen,
				pm );
			XClearWindow( dpy, Scr->VirtualDesktopDScreen );
		}
		XFreePixmap (dpy, pm);
	}

	/* declare our interest */
	XSelectInput(dpy, Scr->VirtualDesktopDisplay, ButtonPressMask | ButtonReleaseMask |
		     KeyPressMask | KeyReleaseMask );



/* Stig Ostholm moved some lines to here: */
	Scr->VirtualDesktopDisplayTwin =
		AddWindow(Scr->VirtualDesktopDisplayOuter, FALSE, NULL);

#ifdef GROSS_HACK
	/* this is a gross hack, but people wanted it */
	Scr->VirtualDesktopDisplayTwin->nailed = TRUE;
#endif /* GROSS_HACK */

	SetMapStateProp(Scr->VirtualDesktopDisplayTwin, NormalState);
/* :ereh ot senil emos devom mlohtsO gitS */

	/* position the representation */
	DisplayScreenOnDesktop();

	/* map them all */
	XMapWindow(dpy, Scr->VirtualDesktopDScreen);
	XMapWindow(dpy, Scr->VirtualDesktopDisplay);
	XMapWindow(dpy, Scr->VirtualDesktopDisplayOuter);

	/* create the autopan windows if we are doing this */
	if (Scr->AutoPanX > 0) {
		short l;

		/* left */
		Scr->VirtualDesktopAutoPan[0] = XCreateWindow(dpy, Scr->Root,
							 0,
							 0,
							 AP_SIZE,
							 Scr->MyDisplayHeight,
							 0,
							 CopyFromParent,
							 InputOnly,
							 CopyFromParent,
							 0, NULL);
		/* right */
		Scr->VirtualDesktopAutoPan[1] = XCreateWindow(dpy, Scr->Root,
							 Scr->MyDisplayWidth - AP_SIZE,
							 0,
							 AP_SIZE,
							 Scr->MyDisplayHeight,
							 0,
							 CopyFromParent,
							 InputOnly,
							 CopyFromParent,
							 0, NULL);
		/* top */
		Scr->VirtualDesktopAutoPan[2] = XCreateWindow(dpy, Scr->Root,
							 0,
							 0,
							 Scr->MyDisplayWidth,
							 AP_SIZE,
							 0,
							 CopyFromParent,
							 InputOnly,
							 CopyFromParent,
							 0, NULL);
		/* bottom */
		Scr->VirtualDesktopAutoPan[3] = XCreateWindow(dpy, Scr->Root,
							 0,
							 Scr->MyDisplayHeight - AP_SIZE,
							 Scr->MyDisplayWidth,
							 AP_SIZE,
							 0,
							 CopyFromParent,
							 InputOnly,
							 CopyFromParent,
							 0, NULL);

		/* set the event masks on the windows */
		for(l = 0; l <= 3; l++) {
			XSetStandardProperties(dpy, Scr->VirtualDesktopAutoPan[l],
					       "Automatic Pan", "Automatic Pan",
					       None, NULL, 0, NULL);

			XSelectInput(dpy, Scr->VirtualDesktopAutoPan[l],
				     EnterWindowMask /* | lowernotify (or something) */ );
			XMapWindow(dpy, Scr->VirtualDesktopAutoPan[l]);
		} /* end for l */
	
	RaiseAutoPan (); /* it may work, why not? -- DSE */
	
	} /* end if Scr->AutoPan */
}

SetVirtualPixmap (filename)
char *filename;
{/*RFB PIXMAP*/
    Pixmap pm = GetBitmap (filename);

    if (pm) {
	if (Scr->virtualPm) {
	    XFreePixmap (dpy, Scr->virtualPm);
	}
	Scr->virtualPm = pm;
	Scr->virtual_pm_width = JunkWidth;
	Scr->virtual_pm_height = JunkHeight;
    }
}
SetRealScreenPixmap (filename)
char *filename;
{/*RFB PIXMAP*/
    Pixmap pm = GetBitmap (filename);

    if (pm) {
	if (Scr->RealScreenPm) {
	    XFreePixmap (dpy, Scr->RealScreenPm);
	}
	Scr->RealScreenPm = pm;
	Scr->RealScreen_pm_width = JunkWidth;
	Scr->RealScreen_pm_height = JunkHeight;
    }
}

/*
 * add this window to the virtual desktop - aka nail it
 */
void UpdateDesktop(tmp_win)
TwmWindow *tmp_win;
{
	int x, y, width, height;
	Window dwindow;

	if (!Scr->Virtual)
		return;

	if (!tmp_win->showindesktopdisplay)
		return;

	if (tmp_win->icon) {
		XUnmapWindow(dpy, tmp_win->VirtualDesktopDisplayWindow);
		return;
	}

	GetDesktopWindowCoordinates(tmp_win, &x, &y, &width, &height);
/* Stig Ostholm <ostholm%ce.chalmers.se@uunet> these 3 lines */
	dwindow = (tmp_win->nailed)
		? Scr->VirtualDesktopDScreen : Scr->VirtualDesktopDisplay;

	/* if it already has a vd display window, just move it to the right place
	   and map it, else actually create the window */
	if (!tmp_win->VirtualDesktopDisplayWindow) {
		Pixel background, border;

#ifdef notdef
		if (!GetColorFromList(Scr->VirtualDesktopColorBL, tmp_win->full_name,
				     &tmp_win->class, &background) &&
		    !GetColorFromList(Scr->TitleBackgroundL, tmp_win->full_name,
				      &tmp_win->class, &background))
			background = Scr->VirtualDesktopDisplayC.back;
#endif /* notdef */
		background = tmp_win->virtual.back;

		/* 7/10/90 - uses border list not foreground */
		if(!GetColorFromList(Scr->VirtualDesktopColorBoL, tmp_win->full_name,
				     &tmp_win->class, &border) &&
		   !GetColorFromList(Scr->TitleForegroundL, tmp_win->full_name,
				     &tmp_win->class, &border))
			border = Scr->VirtualDesktopDisplayBorder;

		/* the position and size don't matter */
		tmp_win->VirtualDesktopDisplayWindow =
			XCreateSimpleWindow(dpy,
				dwindow, x, y, width, height, /* Stig */
				1, border, background);

/*RFBCURSOR*/XDefineCursor( dpy, tmp_win->VirtualDesktopDisplayWindow,
/*RFBCURSOR*/Scr->DesktopCursor );

		/* listen for expose events to redraw the name */
		if (Scr->NamesInVirtualDesktop)
			XSelectInput(dpy, tmp_win->VirtualDesktopDisplayWindow,
				     ExposureMask);

		/* save the twm window on the window */
		XSaveContext(dpy, tmp_win->VirtualDesktopDisplayWindow,
			     VirtualContext, (caddr_t) tmp_win);
		XSaveContext(dpy, tmp_win->VirtualDesktopDisplayWindow,
			     TwmContext, (caddr_t) tmp_win);


#if 0
0		/* Stig Ostholm <ostholm%ce.chalmers.se@uunet> */
0		/* comment out this section */
0	} else
0		/* unmap whilst we reconfigure it */
0		XUnmapWindow(dpy, tmp_win->VirtualDesktopDisplayWindow);
0
0	if (tmp_win->nailed) {
0		x = tmp_win->frame_x / Scr->VirtualDesktopDScale;
0		y = tmp_win->frame_y / Scr->VirtualDesktopDScale;
0/* RFB 4/92 no SCALE_D */
0		/* x = SCALE_D(tmp_win->frame_x); */
0		/* y = SCALE_D(tmp_win->frame_y); */
0
0		/* reparent this window into the little screen representation */
0		XReparentWindow(dpy, tmp_win->VirtualDesktopDisplayWindow,
0				Scr->VirtualDesktopDScreen, x, y);
0	} else {
0		x = tmp_win->virtual_frame_x / Scr->VirtualDesktopDScale;
0		y = tmp_win->virtual_frame_y / Scr->VirtualDesktopDScale;
0/* RFB 4/92 no SCALE_D */
0		/* x = SCALE_D(tmp_win->virtual_frame_x); */
0		/* y = SCALE_D(tmp_win->virtual_frame_y); */
0
0		XReparentWindow(dpy, tmp_win->VirtualDesktopDisplayWindow,
0				Scr->VirtualDesktopDisplay, x, y);
0	}
0
0	/* calculate the sizes and position */
0	width = SCALE_D(
0		tmp_win->frame_width + Scr->VirtualDesktopDScale / 2
0		+ tmp_win->frame_bw + tmp_win->frame_bw )
0		- 2;
0	height = SCALE_D(
0		tmp_win->frame_height + Scr->VirtualDesktopDScale / 2
0/* #ifdef SHAPE */
0			/* + tmp_win->title_height  */
0/* #endif */
0		+ tmp_win->frame_bw + tmp_win->frame_bw ) - 2;
0/* 4/92 RFB -- subtract borderwidth from windowwidth... */
0	if ( width <= 0 ) width = 1;	/* 4/92 RFB */
0	if ( height <= 0 ) height = 1;	/* 4/92 RFB */
0
0#ifdef DEBUG
0	fprintf(stderr, "%d*%d+%d+%d\n", x, y, width, height);
0#endif /* DEBUG */
0
0	/* move and size it */
0	XMoveWindow(dpy, tmp_win->VirtualDesktopDisplayWindow,
0		    x, y);
0	XResizeWindow(dpy, tmp_win->VirtualDesktopDisplayWindow,
0		      width, height);
#else	/* 0, Stig */
	} else {	/* Unmapping is fixed by XReparentWindow */
		XReparentWindow(dpy, tmp_win->VirtualDesktopDisplayWindow,
			dwindow, x, y);
		XResizeWindow(dpy, tmp_win->VirtualDesktopDisplayWindow,
			width, height);
	}
#endif
	XMapWindow(dpy, tmp_win->VirtualDesktopDisplayWindow);
}

#if 0	/* Stig */
0/* Stig Ostholm <ostholm%ce.chalmers.se@uunet> */
0/*
0 * remove a window from the desktop display - aka unnail it
0 */
0void RemoveFromDesktop(tmp_win)
0TwmWindow *tmp_win;
0{
0	int x, y;
0
0	if (!Scr->Virtual)
0		return;
0
0	/*
0	if (tmp_win->VirtualDesktopDisplayWindow)
0		XUnmapWindow(dpy, tmp_win->VirtualDesktopDisplayWindow);
0	*/
0	/* reparent its representation out of the real screen window */
0	x = tmp_win->virtual_frame_x /Scr->VirtualDesktopDScale;
0	y = tmp_win->virtual_frame_y / Scr->VirtualDesktopDScale;
0/* RFB 4/92 no SCALE_D */
0	/* x = SCALE_D(tmp_win->virtual_frame_x); */
0	/* y = SCALE_D(tmp_win->virtual_frame_y); */
0
0	XReparentWindow(dpy, tmp_win->VirtualDesktopDisplayWindow,
0			Scr->VirtualDesktopDisplay, x, y);
0}
#endif /* 0, Stig */

/* Stig Ostholm <ostholm%ce.chalmers.se@uunet>
 * Nail/unnail a window on the desktop display.
 */
void NailDesktop(tmp_win)
TwmWindow *tmp_win;
{
	int x, y;

	if (!tmp_win->VirtualDesktopDisplayWindow
	|| !Scr->Virtual
	|| tmp_win->icon)
		return;

	GetDesktopWindowCoordinates(tmp_win, &x, &y, (int *) 0, (int *) 0);
	XReparentWindow(dpy, tmp_win->VirtualDesktopDisplayWindow,
		(tmp_win->nailed)
			? Scr->VirtualDesktopDScreen
			: Scr->VirtualDesktopDisplay,
		x, y);
	XMapWindow(dpy, tmp_win->VirtualDesktopDisplayWindow);
}

/*
 * state during the move
 */
static unsigned int moving_x, moving_y, moving_w, moving_h, moving_bw;
static unsigned int moving_off_x, moving_off_y;
Window moving_window;
TwmWindow *moving_twindow;

/**********************************************************/
/*                                                        */
/*  RFB 7/16/93 -- moved these static variables up to     */
/*  this part of the file so that I could use moving_bw   */
/*  in DisplayScreenOnDesktop().                          */
/*                                                        */
/*  Note that moving_bw is set to 0 if you clicked *in*   */
/*  the window; therefore it can only have a non-zero     */
/*  value if you clicked at a random location in the      */
/*  panner, making the RealScreen jump to the pointer,    */
/*  AND you also had UseRealScreenBorder set.             */
/*                                                        */
/*  This is almost the final step in getting the panner   */
/*  to behave pefectly! It still jitters a bit at the     */
/*  edges...                                              */
/*                                                        */
/**********************************************************/

/*
 * correctly position the real screen representation on the virtual desktop display
 */
void DisplayScreenOnDesktop()
{
	int border;

	if (!Scr->Virtual)
		return;

	border = ( Scr->UseRealScreenBorder ) ? moving_bw : 0;
	moving_bw = 0;

	/* the -3 is to account for the 2 pixel border and the 1 pixel
	 * offset added by SCALE_D.... */
	XMoveWindow(dpy, Scr->VirtualDesktopDScreen,
		Scr->VirtualDesktopX / Scr->VirtualDesktopDScale - border,
		Scr->VirtualDesktopY / Scr->VirtualDesktopDScale - border
		    /* SCALE_D(Scr->VirtualDesktopX), /* - RFB changed 3 to 1 */
		    /* SCALE_D(Scr->VirtualDesktopY) /* - RFB changed 3 to 1 */
			);
/* 4/92 RFB -- simply use SCALE_D; well, no...
** the problem is that SCALE_D adds 1 if the result is 0, but
** just gives the right result otherwise.
*/

	/* Way back, somebody wrote:    */
	/* I've convinced myself that this is not necessary */
	/* XLowerWindow(dpy, Scr->VirtualDesktopDScreen); */
}

void ResizeDesktopDisplay(w, h)
int w, h;
{
	int x, y, np;

	if (!Scr->Virtual)
		return;

	np = 0;
	if ( w > Scr->VirtualDesktopMaxWidth )
	{	Scr->VirtualDesktopMaxWidth = w;
		np = 1;
	}
	if ( h > Scr->VirtualDesktopMaxHeight )
	{	Scr->VirtualDesktopMaxHeight = h;
		np = 1;
	}
	if ( np )
	{	XResizeWindow( dpy, Scr->VirtualDesktopDisplay,
			Scr->VirtualDesktopMaxWidth,
			Scr->VirtualDesktopMaxHeight );
	}

	/* calculate the new vd size */
	Scr->VirtualDesktopWidth = SCALE_U(w);
	Scr->VirtualDesktopHeight = SCALE_U(h);

	x = SCALE_D(Scr->VirtualDesktopX);
	y = SCALE_D(Scr->VirtualDesktopY);

	/* redraw it so that the real screen representation ends up on the display */
	np = SCALE_D(Scr->VirtualDesktopWidth) - SCALE_D(Scr->MyDisplayWidth);
	if (x > np)
		x = np;

	np = SCALE_D(Scr->VirtualDesktopHeight) - SCALE_D(Scr->MyDisplayHeight);
	if (y > np)
		y = np;

#ifdef FUDGING
	/* this is a bit of a fudge factor to account for the borderwidth */
	x -= 2;
	y -= 2;
#endif /* FUDGING */

	SetRealScreen(SCALE_U(x), SCALE_U(y));

	/* done in setrealscreen now */
#ifdef notdef
	/* move the display window */
	XMoveWindow(dpy, Scr->VirtualDesktopDScreen, x - 1, y - 1);
#endif /* notdef */
}


/*
 * F_MOVESCREEN function
 * move a window in the desktop display - possible including the `real' screen
 */
void StartMoveWindowInDesktop(ev)
XMotionEvent ev;
{
	Window nowindow;
	int xoff, yoff;
	unsigned int d;
#ifdef notdef
	TwmWindow *Tmp_win;
#endif /* notdef */

	if (!Scr->Virtual)
		return;

	moving_window = ev.subwindow;

	if (!moving_window)
		moving_window = Scr->VirtualDesktopDScreen;

	XGrabPointer(dpy, Scr->VirtualDesktopDisplayOuter, True,
		     ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
		     GrabModeAsync, GrabModeAsync,
		     Scr->VirtualDesktopDisplay, Scr->NoCursor, CurrentTime);

	moving_x = ev.x;
	moving_y = ev.y;

	/* find the window by looking at the context on the little window */
	if ((moving_window != Scr->VirtualDesktopDScreen) &&
	    (XFindContext(dpy, moving_window, VirtualContext,
			  (caddr_t *) &moving_twindow) == XCNOENT)) {
		/* i don't think that this should _ever_ happen */
		moving_window = NULL;
		moving_twindow = NULL;
		return;
	}

	XGetGeometry(dpy, moving_window, &nowindow, &xoff, &yoff,
		     &moving_w, &moving_h, &moving_bw, &d);

	moving_off_x = moving_off_y = 0;
	if ( xoff <= moving_x && moving_x <= ( xoff + moving_w )
	&&   yoff <= moving_y && moving_y <= ( yoff + moving_h ))
	{	/* The pointer is IN the window.
		** don't start by moving the window so its upper-left is at
		** the cursor! RFB
		*/
		moving_off_x = xoff - moving_x;
		moving_off_y = yoff - moving_y;
		moving_bw = 0;
	}
	/* get things going */
	DoMoveWindowOnDesktop(ev.x, ev.y);
}

void DoMoveWindowOnDesktop(x, y)
int x, y;
{
	if (!Scr->Virtual)
		return;

	x += moving_off_x;
	y += moving_off_y;
	/* check that we are legit */
	if (x < 0)
		x = 0;
	else {
		int np = ( Scr->VirtualDesktopWidth /
			Scr->VirtualDesktopDScale ) - moving_w;
/* RFB 4/92 no SCALE_D */
		if (x > np)
			x = np;
	}
	if (y < 0)
		y = 0;
	else {
		int np = ( Scr->VirtualDesktopHeight /
			Scr->VirtualDesktopDScale ) - moving_h;
/* RFB 4/92 no SCALE_D */
		if (y > np)
			y = np;
	}

	moving_x = x;
	moving_y = y;

	/* move the display window */
	XMoveWindow(dpy, moving_window, x - moving_bw, y - moving_bw);

	/* move the real window */
	/* this is very difficult on anything not very powerful */
	/* XMoveWindow(dpy, moving_twindow->frame, SCALE_U(x), SCALE_U(y)); */
}

void EndMoveWindowOnDesktop()
{
	if (!Scr->Virtual)
		return;

	if (moving_window == Scr->VirtualDesktopDScreen) {
		SetRealScreen(SCALE_U(moving_x),/* - moving_bw,*/
			SCALE_U(moving_y) /*- moving_bw*/ );
	} else {
		/* move the window in virtual space */
		moving_twindow->virtual_frame_x = SCALE_U(moving_x);
		moving_twindow->virtual_frame_y = SCALE_U(moving_y);

		/* move it in real space */
		moving_twindow->frame_x = V_TO_R_X(moving_twindow->virtual_frame_x);
		moving_twindow->frame_y = V_TO_R_Y(moving_twindow->virtual_frame_y);

		XMoveWindow(dpy, moving_twindow->frame,
			    moving_twindow->frame_x, moving_twindow->frame_y);
		/* notify the window */
		SendConfigureNotify(moving_twindow,
			    moving_twindow->frame_x, moving_twindow->frame_y);

		/* raise the window ? */
		if(!Scr->NoRaiseMove) {
			XRaiseWindow(dpy, moving_twindow->frame);
			XRaiseWindow(dpy, moving_twindow->VirtualDesktopDisplayWindow);

			RaiseAutoPan();
		}

		moving_window = None;

		return;
	}
	moving_window = None;
	moving_twindow = NULL;
}

/* Stig Ostholm <ostholm%ce.chalmers.se@uunet>
 * move and resize a window on the desktop.
 */
void MoveResizeDesktop(tmp_win, noraise)
TwmWindow *tmp_win;
int noraise;
{
	int x, y, w, h;

	if (!tmp_win->VirtualDesktopDisplayWindow
	|| !Scr->Virtual
	|| tmp_win->icon)
		return;

	GetDesktopWindowCoordinates(tmp_win, &x, &y, &w, &h);
	/* Resize the desktop representation window */
	XMoveResizeWindow(dpy, tmp_win->VirtualDesktopDisplayWindow,
		x, y, w, h);
	if (!noraise)
		XRaiseWindow(dpy, tmp_win->VirtualDesktopDisplayWindow);
}

void SetVirtualDesktop(geom, scale)
char *geom;
int scale;
{

	if (Scr->Virtual) {
		twmrc_error_prefix();
		fprintf(stderr, "VirtualDesktop already defined -- ignored.\n");
		return;
	}

	if (scale < 0) {
		twmrc_error_prefix();
		fprintf(stderr,
			"VirtualDesktop scale must be positive, not %d\n", scale);
		return;
	}
	Scr->VirtualDesktopDScale = scale;

	JunkMask = XParseGeometry (geom, &JunkX, &JunkY, &JunkWidth, &JunkHeight);

	if ((JunkMask & (WidthValue | HeightValue)) !=
	    (WidthValue | HeightValue)) {
	    twmrc_error_prefix();
	    fprintf (stderr, "bad VirtualDesktop \"%s\"\n", geom);
	    return;
	}
	if (JunkWidth <= 0 || JunkHeight <= 0) {
	    twmrc_error_prefix();
	    fprintf (stderr, "VirtualDesktop \"%s\" must be positive\n", geom);
	    return;
	}

	/*
	 * More flexible way of selecting size of virtual desktop (ala tvtwm)
	 * M.J.E. Mol marcel@duteca.et.tudelft.nl
         */
	if (JunkWidth > Scr->MyDisplayWidth)
		/* specified as total pixels */
		JunkWidth /= Scr->VirtualDesktopDScale;
	else if (JunkWidth*Scr->VirtualDesktopDScale < Scr->MyDisplayWidth) {
        	/* specified as number of physical screens */
		JunkWidth *= Scr->MyDisplayWidth;
		JunkWidth /= Scr->VirtualDesktopDScale;
	}
        /* else specified as size of panner window */

	if (JunkHeight > Scr->MyDisplayHeight)
		/* specified as total pixels */
		JunkHeight /= Scr->VirtualDesktopDScale;
	else if (JunkHeight*Scr->VirtualDesktopDScale < Scr->MyDisplayHeight) {
        	/* specified as number of physical screens */
		JunkHeight *= Scr->MyDisplayHeight;
		JunkHeight /= Scr->VirtualDesktopDScale;
	}
        /* else specified as size of panner window */

	/* tar@math.ksu.edu: fix handling of -0 X and Y geometry */
	if (JunkMask & XValue) {
		if (JunkMask & XNegative) {
			Scr->VirtualDesktopDX = Scr->MyDisplayWidth
			    - JunkWidth - (2 * Scr->BorderWidth) + JunkX;
		}
		else	Scr->VirtualDesktopDX = JunkX;
	}
	if (JunkMask & YValue) {
		if (JunkMask & YNegative) {
			Scr->VirtualDesktopDY = Scr->MyDisplayHeight
			    - JunkHeight - (2 * Scr->BorderWidth) + JunkY;
		}
		else	Scr->VirtualDesktopDY = JunkY;
	}

	JunkWidth *= Scr->VirtualDesktopDScale;
	JunkHeight *= Scr->VirtualDesktopDScale;

	/* check that the vd is at least as big as the screen */
/* handled above, M.J.E. Mol
**	if ((JunkWidth < Scr->MyDisplayWidth)
**	    || (JunkHeight < Scr->MyDisplayHeight))
**	{
**		twmrc_error_prefix();
**		fprintf(stderr,
**			"VirtualDesktop must be larger than screen (%dx%d)\n",
**			Scr->MyDisplayWidth, Scr->MyDisplayHeight);
**		return;
**	}
*/
	Scr->VirtualDesktopWidth = JunkWidth;
	Scr->VirtualDesktopHeight = JunkHeight;

	/* all of the values looked reasonable */
	Scr->Virtual = TRUE;
}

void VirtualMoveWindow(t, x, y)
TwmWindow *t;
int x, y;
{
	if (!Scr->Virtual)
		return;

	/* move  window in virtual space */
	t->virtual_frame_x = x;
	t->virtual_frame_y = y;

	/* move it in real space */
	t->frame_x = V_TO_R_X(x);
	t->frame_y = V_TO_R_Y(y);

	XMoveWindow(dpy, t->frame,
		    t->frame_x, t->frame_y);

	/* update the display */
	/* UpdateDesktop(t); Stig */
	MoveResizeDesktop(t, FALSE); /* Stig */

	if (!Scr->NoRaiseMove) {
		XRaiseWindow(dpy, t->frame);
		/* XRaiseWindow(dpy, t->VirtualDesktopDisplayWindow); Stig */

		RaiseAutoPan();
	}
}

/*
 * F_SNAP function
 * for Kevin Twidle <kpt@doc.ic.ac.uk>
 * this snaps the real screen to a grid defined by the pandistance values.
 */
void SnapRealScreen()
{
	int newx, newy;
	int mod, div;

	mod = Scr->VirtualDesktopX % Scr->VirtualDesktopPanDistanceX;
	div = Scr->VirtualDesktopX / Scr->VirtualDesktopPanDistanceX;

	if (mod > (Scr->VirtualDesktopPanDistanceX / 2))
		newx = Scr->VirtualDesktopPanDistanceX * (div + 1);
	else
		newx = Scr->VirtualDesktopPanDistanceX * div;

	mod = Scr->VirtualDesktopY % Scr->VirtualDesktopPanDistanceY;
	div = Scr->VirtualDesktopY / Scr->VirtualDesktopPanDistanceY;

	if (mod > (Scr->VirtualDesktopPanDistanceY / 2))
		newy = Scr->VirtualDesktopPanDistanceY * (div + 1);
	else
		newy = Scr->VirtualDesktopPanDistanceY * div;

	SetRealScreenInternal(newx, newy, FALSE, NULL, NULL); /* DSE */
}


void SetRealScreen(x, y)
int x, y;
{
	if (Scr->SnapRealScreen)
		SetRealScreenInternal(x, y, TRUE, NULL, NULL); /* DSE */
	else
		SetRealScreenInternal(x, y, FALSE, NULL, NULL); /* DSE */
}

/*
 * handles the possibility of snapping
 */
void SetRealScreenInternal(x, y, dosnap, dx, dy)
int x, y;
int *dx, *dy; /* a pointer to an integer that contains the value
                 (AutoPanBorderWidth + AutoPanExtraWarp) is passed in to
                 both dx and dy when autopanning, or NULL is passed.  On
                 return, the value is modified to store how much the pointer
                 should actually be warped, in case
                 AutoPanWarpWithRespectToRealScreen is nonzero. -- DSE */
short dosnap;
{
	int xdiff, ydiff;
	TwmWindow *Tmp_win;

	/* check bounds */
	if (x < 0) x = 0;
	if (y < 0) y = 0;
	if (x > (Scr->VirtualDesktopWidth - Scr->MyDisplayWidth))
		x = Scr->VirtualDesktopWidth - Scr->MyDisplayWidth;
	if (y > (Scr->VirtualDesktopHeight - Scr->MyDisplayHeight))
		y = Scr->VirtualDesktopHeight - Scr->MyDisplayHeight;

	/* if ( Scr->UseRealScreenBorder ) */
	/* {	x -= 2; */
		/* y -= 2; */
	/* } */

	/* how big a move is this ? */
	xdiff = Scr->VirtualDesktopX - x;
	ydiff = Scr->VirtualDesktopY - y;
	
	/* DSE */
	{
	int x_warp = ((xdiff<0) ? -1 : 1) * 
	    ( (50 + abs(xdiff) * Scr->AutoPanWarpWithRespectToRealScreen) / 100 );
	int y_warp = ((ydiff<0) ? -1 : 1) *
	    ( (50 + abs(ydiff) * Scr->AutoPanWarpWithRespectToRealScreen) / 100 );
	
	if (dx)
		if ( abs(x_warp) > abs(*dx) ) /* gonna warp the pointer enough */
			*dx = x_warp;
	if (dy)
		if ( abs(y_warp) > abs(*dy) ) /* gonna warp the pointer enough */
			*dy = y_warp;
	}

	/* move all of the windows by walking the twm list */
	for (Tmp_win = Scr->TwmRoot.next; Tmp_win != NULL; Tmp_win = Tmp_win->next)
		if (Tmp_win->nailed || (Scr->DeIconifyToScreen && Tmp_win->icon)) {
			/* the window is nailed or...
			 * the window is currently an icon, we are trying to
			 * keep things on the screen, so move it around the
			 * virtual desktop so that it stays on the real
			 * screen */
			Tmp_win->virtual_frame_x -= xdiff;
			Tmp_win->virtual_frame_y -= ydiff;
		} else {
			/* move the window */
			Tmp_win->frame_x += xdiff;
			Tmp_win->frame_y += ydiff;

			if (!dosnap) {
				XMoveWindow(dpy, Tmp_win->frame,
					    Tmp_win->frame_x, Tmp_win->frame_y);
				SendConfigureNotify(Tmp_win,
					    Tmp_win->frame_x, Tmp_win->frame_y);
			}
		}

	Scr->VirtualDesktopX = x;
	Scr->VirtualDesktopY = y;

	if (dosnap)
		SnapRealScreen();
	else
		DisplayScreenOnDesktop();
}

/*
 * pan the real screen on the virtual desktop by (xoff, yoff)
 */
void PanRealScreen(xoff, yoff, dx, dy)
int xoff, yoff;
int *dx, *dy; /* DSE */
{
	/* panning the screen can never mean that you need to snap */
	SetRealScreenInternal(Scr->VirtualDesktopX + xoff, Scr->VirtualDesktopY + yoff,
			      FALSE, dx, dy);
			             /* DSE */
}

/*
 * raise the auto-pan windows if needed
 */
void RaiseAutoPan()
{
	int i;

	if (Scr->AutoPanX > 0)
		for (i = 0; i <= 3; i++)
			XRaiseWindow(dpy, Scr->VirtualDesktopAutoPan[i]);
}
