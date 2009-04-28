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
#include <string.h>
#include "twm.h"
#include "parse.h"
#include "image_formats.h"
#include "util.h"
#include "resize.h"
#include "add_window.h"
#include "screen.h"
#include "desktop.h"
#include "events.h"
#include "menus.h"
#include "prototypes.h"

#define MINHEIGHT 0		/* had been 32 */
#define MINWIDTH 0		/* had been 60 */

static int dragx;		/* all these variables are used */
static int dragy;		/* in resize operations */
static int dragWidth;
static int dragHeight;

int origx;
int origy;
int origWidth;
int origHeight;
int origMask;

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

static void DoVirtualMoveResize(TwmWindow * tmp_win, int x, int y, int w, int h);
static void SetVirtualDesktopIncrs(TwmWindow * tmp_win);
static void EndResizeAdjPointer(TwmWindow * tmp_win);

static void
do_auto_clamp(TwmWindow * tmp_win, XEvent * evp)
{
  Window junkRoot;
  int x, y, h, v, junkbw;
  unsigned int junkMask;

  switch (evp->type)
  {
  case ButtonPress:
    x = evp->xbutton.x_root;
    y = evp->xbutton.y_root;
    break;
  case KeyPress:
    x = evp->xkey.x_root;
    y = evp->xkey.y_root;
    break;
  default:
    if (!XQueryPointer(dpy, Scr->Root, &junkRoot, &junkRoot, &x, &y, &junkbw, &junkbw, &junkMask))
      return;
  }

  h = ((x - dragx) / (dragWidth < 3 ? 1 : (dragWidth / 3)));
  v = ((y - dragy - tmp_win->title_height) / (dragHeight < 3 ? 1 : (dragHeight / 3)));

  if (h <= 0)
  {
    clampLeft = 1;
    clampDX = (x - dragx);
  }
  else if (h >= 2)
  {
    clampRight = 1;
    clampDX = (x - dragx - dragWidth);
  }

  if (v <= 0)
  {
    clampTop = 1;
    clampDY = (y - dragy);
  }
  else if (v >= 2)
  {
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
StartResize(XEvent * evp, TwmWindow * tmp_win, Bool fromtitlebar, int context)
{
  Window junkRoot;
  unsigned int junkbw, junkDepth;

  resize_context = context;

  SetVirtualDesktopIncrs(tmp_win);

  if (context == C_VIRTUAL_WIN)
    ResizeWindow = tmp_win->VirtualDesktopDisplayWindow.win;
  else
    ResizeWindow = tmp_win->frame;

  if (!tmp_win->opaque_resize)
    XGrabServer(dpy);
  if (context == C_VIRTUAL_WIN)
    XGrabPointer(dpy, Scr->VirtualDesktopDisplay, True,
		 ButtonPressMask | ButtonReleaseMask |
		 ButtonMotionMask | PointerMotionHintMask, GrabModeAsync, GrabModeAsync, Scr->Root, Scr->ResizeCursor, CurrentTime);
  else
    XGrabPointer(dpy, Scr->Root, True,
		 ButtonPressMask | ButtonReleaseMask |
		 ButtonMotionMask | PointerMotionHintMask, GrabModeAsync, GrabModeAsync, Scr->Root, Scr->ResizeCursor, CurrentTime);

  XGetGeometry(dpy, (Drawable) ResizeWindow, &junkRoot,
	       &dragx, &dragy, (unsigned int *)&dragWidth, (unsigned int *)&dragHeight, &junkbw, &junkDepth);

  if (context != C_VIRTUAL_WIN)
  {
    dragx += tmp_win->frame_bw;
    dragy += tmp_win->frame_bw;
  }
  origx = dragx;
  origy = dragy;
  origWidth = dragWidth;
  origHeight = dragHeight;
  clampTop = clampBottom = clampLeft = clampRight = clampDX = clampDY = 0;

  if (Scr->AutoRelativeResize && !fromtitlebar)
    do_auto_clamp(tmp_win, evp);

#ifdef TILED_SCREEN
  if (Scr->use_tiles == TRUE)
  {
    int k = FindNearestTileToMouse();

    XMoveWindow(dpy, Scr->SizeWindow.win, Lft(Scr->tiles[k]), Bot(Scr->tiles[k]));
  }
#endif


  XMapRaised(dpy, Scr->SizeWindow.win);
  if (!tmp_win->opaque_resize)
    InstallRootColormap();
  last_width = 0;
  last_height = 0;
  DisplaySize(tmp_win, origWidth, origHeight);

  if (resize_context == C_VIRTUAL_WIN)
    MoveOutline(Scr->VirtualDesktopDisplay, dragx, dragy, dragWidth, dragHeight, tmp_win->frame_bw, 0);
  else if (tmp_win->opaque_resize)
  {
    SetupWindow(tmp_win, dragx - tmp_win->frame_bw, dragy - tmp_win->frame_bw, dragWidth, dragHeight, -1);
    PaintBorderAndTitlebar(tmp_win);
  }
  else
    MoveOutline(Scr->Root, dragx - tmp_win->frame_bw,
		dragy - tmp_win->frame_bw, dragWidth + 2 * tmp_win->frame_bw,
		dragHeight + 2 * tmp_win->frame_bw, tmp_win->frame_bw, tmp_win->title_height + tmp_win->frame_bw3D);
}


void
MenuStartResize(TwmWindow * tmp_win, int x, int y, int w, int h, int context)
{
  resize_context = context;

  SetVirtualDesktopIncrs(tmp_win);

  if (!tmp_win->opaque_resize)
    XGrabServer(dpy);
  XGrabPointer(dpy, Scr->Root, True,
	       ButtonPressMask | ButtonMotionMask | PointerMotionMask,
	       GrabModeAsync, GrabModeAsync, Scr->Root, Scr->ResizeCursor, CurrentTime);
  dragx = x + tmp_win->frame_bw;
  dragy = y + tmp_win->frame_bw;
  origx = dragx;
  origy = dragy;
  dragWidth = origWidth = w;
  dragHeight = origHeight = h;
  clampTop = clampBottom = clampLeft = clampRight = clampDX = clampDY = 0;
  last_width = 0;
  last_height = 0;

#ifdef TILED_SCREEN
  if (Scr->use_tiles == TRUE)
  {
    int k = FindNearestTileToMouse();

    XMoveWindow(dpy, Scr->SizeWindow.win, Lft(Scr->tiles[k]), Bot(Scr->tiles[k]));
  }
#endif


  XMapRaised(dpy, Scr->SizeWindow.win);
  if (!tmp_win->opaque_resize)
    InstallRootColormap();
  DisplaySize(tmp_win, origWidth, origHeight);

  if (tmp_win->opaque_resize)
  {
    SetupWindow(tmp_win, dragx - tmp_win->frame_bw, dragy - tmp_win->frame_bw, dragWidth, dragHeight, -1);
    PaintBorderAndTitlebar(tmp_win);
  }
  else
    MoveOutline(Scr->Root, dragx - tmp_win->frame_bw,
		dragy - tmp_win->frame_bw,
		dragWidth + 2 * tmp_win->frame_bw,
		dragHeight + 2 * tmp_win->frame_bw, tmp_win->frame_bw, tmp_win->title_height + tmp_win->frame_bw3D);
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
AddStartResize(TwmWindow * tmp_win, int x, int y, int w, int h)
{
  resize_context = C_WINDOW;

  SetVirtualDesktopIncrs(tmp_win);

  if (!tmp_win->opaque_resize)
    XGrabServer(dpy);

  XGrabPointer(dpy, Scr->Root, True,
	       ButtonReleaseMask | ButtonMotionMask | PointerMotionHintMask,
	       GrabModeAsync, GrabModeAsync, Scr->Root, Scr->ResizeCursor, CurrentTime);

  dragx = x + tmp_win->frame_bw;
  dragy = y + tmp_win->frame_bw;
  origx = dragx;
  origy = dragy;
  dragWidth = origWidth = w - 2 * tmp_win->frame_bw;
  dragHeight = origHeight = h - 2 * tmp_win->frame_bw;
  clampTop = clampBottom = clampLeft = clampRight = clampDX = clampDY = 0;
  last_width = 0;
  last_height = 0;
  DisplaySize(tmp_win, origWidth, origHeight);
}



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
DoResize(int x_root, int y_root, TwmWindow * tmp_win)
{
  int action;

  action = 0;

  x_root -= clampDX;
  y_root -= clampDY;

  if (clampTop)
  {
    int delta = y_root - dragy;

    if (dragHeight - delta < MINHEIGHT)
    {
      delta = dragHeight - MINHEIGHT;
      clampTop = 0;
    }
    dragy += delta;
    dragHeight -= delta;
    action = 1;
  }
  else if (y_root <= dragy)
  {
    dragy = y_root;
    dragHeight = origy + origHeight - y_root;
    clampBottom = 0;
    clampTop = 1;
    clampDY = 0;
    action = 1;
  }
  if (clampLeft)
  {
    int delta = x_root - dragx;

    if (dragWidth - delta < MINWIDTH)
    {
      delta = dragWidth - MINWIDTH;
      clampLeft = 0;
    }
    dragx += delta;
    dragWidth -= delta;
    action = 1;
  }
  else if (x_root <= dragx)
  {
    dragx = x_root;
    dragWidth = origx + origWidth - x_root;
    clampRight = 0;
    clampLeft = 1;
    clampDX = 0;
    action = 1;
  }
  if (clampBottom)
  {
    int delta = y_root - dragy - dragHeight;

    if (dragHeight + delta < MINHEIGHT)
    {
      delta = MINHEIGHT - dragHeight;
      clampBottom = 0;
    }
    dragHeight += delta;
    action = 1;
  }
  else if (y_root >= dragy + dragHeight - 1)
  {
    dragy = origy;
    dragHeight = 1 + y_root - dragy;
    clampTop = 0;
    clampBottom = 1;
    clampDY = 0;
    action = 1;
  }
  if (clampRight)
  {
    int delta = x_root - dragx - dragWidth;

    if (dragWidth + delta < MINWIDTH)
    {
      delta = MINWIDTH - dragWidth;
      clampRight = 0;
    }
    dragWidth += delta;
    action = 1;
  }
  else if (x_root >= dragx + dragWidth - 1)
  {
    dragx = origx;
    dragWidth = 1 + x_root - origx;
    clampLeft = 0;
    clampRight = 1;
    clampDX = 0;
    action = 1;
  }

  if (action)
  {
    ConstrainSize(tmp_win, &dragWidth, &dragHeight);
    if (clampLeft)
      dragx = origx + origWidth - dragWidth;
    if (clampTop)
      dragy = origy + origHeight - dragHeight;

    if (tmp_win->opaque_resize)
    {
      SetupWindow(tmp_win, dragx - tmp_win->frame_bw, dragy - tmp_win->frame_bw, dragWidth, dragHeight, -1);

      /* force the redraw of a door - djhjr - 2/28/99 */
      {
	TwmDoor *door;

	if (XFindContext(dpy, tmp_win->w, DoorContext, (caddr_t *) & door) != XCNOENT)
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

	if (ncols == 0)
	  ncols = 1;

	tmp_win->iconmgrp->width = (int)(((dragWidth - 2 * tmp_win->frame_bw3D) * (long)tmp_win->iconmgrp->columns) / ncols);
	PackIconManager(tmp_win->iconmgrp);

	list = tmp_win->iconmgrp->first;
	while (list)
	{
	  RedoListWindow(list->twm);
	  list = list->next;
	}
      }

      PaintBorderAndTitlebar(tmp_win);

      if (!Scr->NoGrabServer && !resizing_window)
      {
	/* these let the application window be drawn - djhjr - 4/14/98 */
	XUngrabServer(dpy);
	XSync(dpy, 0);
	XGrabServer(dpy);
      }
    }
    else
      MoveOutline(Scr->Root,
		  dragx - tmp_win->frame_bw,
		  dragy - tmp_win->frame_bw,
		  dragWidth + 2 * tmp_win->frame_bw,
		  dragHeight + 2 * tmp_win->frame_bw, tmp_win->frame_bw, tmp_win->title_height + tmp_win->frame_bw3D);

    if (Scr->VirtualReceivesMotionEvents)
      DoVirtualMoveResize(tmp_win, dragx, dragy, dragWidth, dragHeight);
  }

  DisplaySize(tmp_win, dragWidth, dragHeight);
}

static void
SetVirtualDesktopIncrs(TwmWindow * tmp_win)
{
  if (strcmp(tmp_win->class.res_class, VTWM_DESKTOP_CLASS) == 0)
  {
    if (Scr->snapRealScreen)
    {
      Scr->VirtualDesktopDisplayTwin->hints.flags |= PResizeInc;
      Scr->VirtualDesktopDisplayTwin->hints.width_inc = SCALE_D(Scr->VirtualDesktopPanDistanceX);
      Scr->VirtualDesktopDisplayTwin->hints.height_inc = SCALE_D(Scr->VirtualDesktopPanDistanceY);
    }
    else
      Scr->VirtualDesktopDisplayTwin->hints.flags &= ~PResizeInc;

    XSetWMNormalHints(dpy, tmp_win->w, &Scr->VirtualDesktopDisplayTwin->hints);
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
DisplaySize(TwmWindow * tmp_win, int width, int height)
{
  char str[100];
  int i, dwidth, dheight;

  if (last_width == width && last_height == height)
    return;

  last_width = width;
  last_height = height;

  if (resize_context == C_VIRTUAL_WIN)
  {
    dheight = SCALE_U(height) - tmp_win->title_height;
    dwidth = SCALE_U(width);
  }
  else
  {
    dheight = height - tmp_win->title_height - 2 * tmp_win->frame_bw3D;
    dwidth = width - 2 * tmp_win->frame_bw3D;

  }

  /*
   * ICCCM says that PMinSize is the default is no PBaseSize is given,
   * and vice-versa.
   * Don't adjust if window is the virtual desktop - djhjr - 9/13/02
   */
  if (tmp_win->hints.flags & (PMinSize | PBaseSize) && tmp_win->hints.flags & PResizeInc)
  {
    if (tmp_win->hints.flags & PBaseSize)
    {
      dwidth -= tmp_win->hints.base_width;
      dheight -= tmp_win->hints.base_height;
    }
    else if (strcmp(tmp_win->class.res_class, VTWM_DESKTOP_CLASS) != 0)
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
 */
  sprintf(str, "%5d x %-5d", dwidth, dheight);
  i = strlen(str);

  XRaiseWindow(dpy, Scr->SizeWindow.win);
  MyFont_DrawImageString(dpy, &Scr->SizeWindow, &Scr->SizeFont,
			 &Scr->DefaultC, (Scr->SizeStringWidth - MyFont_TextWidth(&Scr->SizeFont, str, i)) / 2,
			 /* was 'Scr->use3Dborders' - djhjr - 8/11/98 */
			 Scr->SizeFont.ascent + SIZE_VINDENT + ((Scr->InfoBevelWidth > 0) ? Scr->InfoBevelWidth : 0), str, i);

  /* I know, I know, but the above code overwrites it... djhjr - 5/9/96 */
  if (Scr->InfoBevelWidth > 0)
    Draw3DBorder(Scr->SizeWindow.win, 0, 0,
		 Scr->SizeStringWidth,
		 (unsigned int)(Scr->SizeFont.height + SIZE_VINDENT * 2) +
		 ((Scr->InfoBevelWidth > 0) ? 2 * Scr->InfoBevelWidth : 0), Scr->InfoBevelWidth, Scr->DefaultC, off, False, False);
}

/***********************************************************************
 *
 *  Procedure:
 *      EndResize - finish the resize operation
 *
 ***********************************************************************
 */

void
EndResize(void)
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

  XFindContext(dpy, ResizeWindow, TwmContext, (caddr_t *) & tmp_win);

  if (resize_context == C_VIRTUAL_WIN)
  {
    /* scale up */
    dragWidth = SCALE_U(dragWidth);
    dragHeight = SCALE_U(dragHeight);
    dragx = SCALE_U(dragx);
    dragy = SCALE_U(dragy);
  }

  ConstrainSize(tmp_win, &dragWidth, &dragHeight);

  if (dragWidth != tmp_win->frame_width || dragHeight != tmp_win->frame_height)
    tmp_win->zoomed = ZOOM_NONE;

  SetupWindow(tmp_win, dragx - tmp_win->frame_bw, dragy - tmp_win->frame_bw, dragWidth, dragHeight, -1);

  EndResizeAdjPointer(tmp_win);

  /* added test for opaque resizing - djhjr - 2/28/99, 3/1/99 */
  if (!tmp_win->opaque_resize)
  {
    ResizeTwmWindowContents(tmp_win, dragWidth, dragHeight);
  }

  ResizeWindow = None;

  resizing_window = 0;
}

void
MenuEndResize(TwmWindow * tmp_win, int context)
{
  if (resize_context == C_VIRTUAL_WIN)
    MoveOutline(Scr->VirtualDesktopDisplay, 0, 0, 0, 0, 0, 0);
  else
    MoveOutline(Scr->Root, 0, 0, 0, 0, 0, 0);
  XUnmapWindow(dpy, Scr->SizeWindow.win);

  ConstrainSize(tmp_win, &dragWidth, &dragHeight);

  if (dragWidth != tmp_win->frame_width || dragHeight != tmp_win->frame_height)
    tmp_win->zoomed = ZOOM_NONE;

  SetupWindow(tmp_win, dragx - tmp_win->frame_bw, dragy - tmp_win->frame_bw, dragWidth, dragHeight, -1);

  if (context != C_VIRTUAL_WIN)
    EndResizeAdjPointer(tmp_win);

  if (!tmp_win->opaque_resize)
  {
    ResizeTwmWindowContents(tmp_win, dragWidth, dragHeight);
  }

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
AddEndResize(TwmWindow * tmp_win)
{

#ifdef DEBUG
  fprintf(stderr, "AddEndResize\n");
#endif

  MoveOutline(Scr->Root, 0, 0, 0, 0, 0, 0);
  XUnmapWindow(dpy, Scr->SizeWindow.win);

  ConstrainSize(tmp_win, &dragWidth, &dragHeight);
  AddingX = dragx;
  AddingY = dragy;
  AddingW = dragWidth + (2 * tmp_win->frame_bw);
  AddingH = dragHeight + (2 * tmp_win->frame_bw);

  EndResizeAdjPointer(tmp_win);

  if (dragWidth != tmp_win->frame_width || dragHeight != tmp_win->frame_height)
    tmp_win->zoomed = ZOOM_NONE;

  resizing_window = 0;
}

static void
EndResizeAdjPointer(TwmWindow * tmp_win)
{
  int x, y, bw = tmp_win->frame_bw + tmp_win->frame_bw3D;
  int pointer_x, pointer_y;	/* pointer coordinates relative to root origin */

  /* Declare junk variables for Xlib calls that return more than we need. */
  Window junk_root, junk_child;
  int junk_win_x, junk_win_y;
  unsigned int junk_mask;

  XQueryPointer(dpy, Scr->Root, &junk_root, &junk_child, &pointer_x, &pointer_y, &junk_win_x, &junk_win_y, &junk_mask);
  XTranslateCoordinates(dpy, Scr->Root, tmp_win->frame, pointer_x, pointer_y, &x, &y, &junk_child);

  /* for borderless windows */
  if (bw == 0)
    bw = 4;

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
    if (x >= tmp_win->title_x - ((tmp_win->frame_bw) ? 0 : bw) && x < tmp_win->title_x + tmp_win->title_width + bw)
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

void
ConstrainSize(TwmWindow * tmp_win, int *widthp, int *heightp)
{
#define makemult(a,b) ((b==1) ? (a) : (((int)((a)/(b))) * (b)) )
#define _min(a,b) (((a) < (b)) ? (a) : (b))

  int minWidth, minHeight, maxWidth, maxHeight, xinc, yinc, delta;
  int baseWidth, baseHeight;
  int dwidth = *widthp, dheight = *heightp;


  dwidth -= 2 * tmp_win->frame_bw3D;
  dheight -= (tmp_win->title_height + 2 * tmp_win->frame_bw3D);

  if (tmp_win->hints.flags & PMinSize)
  {
    minWidth = tmp_win->hints.min_width;
    minHeight = tmp_win->hints.min_height;
  }
  else if (tmp_win->hints.flags & PBaseSize)
  {
    minWidth = tmp_win->hints.base_width;
    minHeight = tmp_win->hints.base_height;
  }
  else
    minWidth = minHeight = 1;

  if (resize_context == C_VIRTUAL_WIN)
  {
    minWidth = SCALE_D(minWidth);
    minHeight = SCALE_D(minHeight);
  }

  if (tmp_win->hints.flags & PBaseSize)
  {
    baseWidth = tmp_win->hints.base_width;
    baseHeight = tmp_win->hints.base_height;
  }
  else if (tmp_win->hints.flags & PMinSize)
  {
    baseWidth = tmp_win->hints.min_width;
    baseHeight = tmp_win->hints.min_height;
  }
  else
    baseWidth = baseHeight = 0;

  if (resize_context == C_VIRTUAL_WIN)
  {
    baseWidth = SCALE_D(baseWidth);
    baseHeight = SCALE_D(baseHeight);
  }

  if (tmp_win->hints.flags & PMaxSize)
  {
    maxWidth = _min(Scr->MaxWindowWidth, tmp_win->hints.max_width);
    maxHeight = _min(Scr->MaxWindowHeight, tmp_win->hints.max_height);
  }
  else
  {
    maxWidth = Scr->MaxWindowWidth;
    maxHeight = Scr->MaxWindowHeight;
  }

  if (resize_context == C_VIRTUAL_WIN)
  {
    maxWidth = SCALE_D(maxWidth);
    maxHeight = SCALE_D(maxHeight);
  }

  if (tmp_win->hints.flags & PResizeInc)
  {
    xinc = tmp_win->hints.width_inc;
    yinc = tmp_win->hints.height_inc;
  }
  else
    xinc = yinc = 1;

  if (resize_context == C_VIRTUAL_WIN)
  {
    xinc = SCALE_D(xinc);
    yinc = SCALE_D(yinc);
  }

  /*
   * First, clamp to min and max values
   */
  if (dwidth < minWidth)
    dwidth = minWidth;
  if (dheight < minHeight)
    dheight = minHeight;

  if (dwidth > maxWidth)
    dwidth = maxWidth;
  if (dheight > maxHeight)
    dheight = maxHeight;


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
      delta = makemult(minAspectX * dheight / minAspectY - dwidth, xinc);
      if (dwidth + delta <= maxWidth)
	dwidth += delta;
      else
      {
	delta = makemult(dheight - dwidth * minAspectY / minAspectX, yinc);
	if (dheight - delta >= minHeight)
	  dheight -= delta;
      }
    }

    if (maxAspectX * dheight < maxAspectY * dwidth)
    {
      delta = makemult(dwidth * maxAspectY / maxAspectX - dheight, yinc);
      if (dheight + delta <= maxHeight)
	dheight += delta;
      else
      {
	delta = makemult(dwidth - maxAspectX * dheight / maxAspectY, xinc);
	if (dwidth - delta >= minWidth)
	  dwidth -= delta;
      }
    }
  }


  /*
   * Fourth, account for border width and title height
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

void
SetupWindow(TwmWindow * tmp_win, int x, int y, int w, int h, int bw)
{
  SetupFrame(tmp_win, x, y, w, h, bw, False);
}

void
SetupFrame(TwmWindow * tmp_win, int x, int y, int w, int h, int bw, Bool sendEvent	/* whether or not to force a send */
  )
{
  XWindowChanges frame_wc, xwc;
  unsigned long frame_mask, xwcm;
  int title_width, title_height;
  int reShape;

#ifdef DEBUG
  fprintf(stderr, "SetupWindow: x=%d, y=%d, w=%d, h=%d, bw=%d\n", x, y, w, h, bw);
#endif

  if ((tmp_win->virtual_frame_x + tmp_win->frame_width) < 0)
    x = 16;			/* one "average" cursor width */
  if (x >= Scr->VirtualDesktopWidth)
    x = Scr->VirtualDesktopWidth - 16;
  if ((tmp_win->virtual_frame_y + tmp_win->frame_height) < 0)
    y = 16;			/* one "average" cursor width */
  if (y >= Scr->VirtualDesktopHeight)
    y = Scr->VirtualDesktopHeight - 16;

  if (bw < 0)
    bw = tmp_win->frame_bw;	/* -1 means current frame width */

  if (tmp_win->iconmgr)
  {
    tmp_win->iconmgrp->width = w - (2 * tmp_win->frame_bw3D);
    h = tmp_win->iconmgrp->height + tmp_win->title_height + (2 * tmp_win->frame_bw3D);

  }

  /*
   * According to the July 27, 1988 ICCCM draft, we should send a
   * "synthetic" ConfigureNotify event to the client if the window
   * was moved but not resized.
   */
  if (((x != tmp_win->frame_x || y != tmp_win->frame_y) &&
       (w == tmp_win->frame_width && h == tmp_win->frame_height)) || (bw != tmp_win->frame_bw))
    sendEvent = TRUE;

  xwcm = CWWidth;
  title_width = xwc.width = w - (2 * tmp_win->frame_bw3D);
  title_height = Scr->TitleHeight + bw;

  ComputeWindowTitleOffsets(tmp_win, xwc.width, True);

  reShape = (tmp_win->wShaped ? TRUE : FALSE);
  if (tmp_win->squeeze_info)	/* check for title shaping */
  {
    title_width = tmp_win->rightx + Scr->TBInfo.rightoff;
    if (title_width < xwc.width)
    {
      xwc.width = title_width;
      if (tmp_win->frame_height != h || tmp_win->frame_width != w || tmp_win->frame_bw != bw || title_width != tmp_win->title_width)
	reShape = TRUE;
    }
    else
    {
      if (!tmp_win->wShaped)
	reShape = TRUE;
      title_width = xwc.width;
    }
  }

  tmp_win->title_width = title_width;
  if (tmp_win->title_height)
    tmp_win->title_height = title_height;

  if (tmp_win->title_w.win)
  {
    if (bw != tmp_win->frame_bw)
    {
      xwc.border_width = bw;
      tmp_win->title_x = xwc.x = tmp_win->frame_bw3D - bw;
      tmp_win->title_y = xwc.y = tmp_win->frame_bw3D - bw;

      xwcm |= (CWX | CWY | CWBorderWidth);
    }

    XConfigureWindow(dpy, tmp_win->title_w.win, xwcm, &xwc);
  }

  tmp_win->attr.width = w - (2 * tmp_win->frame_bw3D);
  tmp_win->attr.height = h - tmp_win->title_height - (2 * tmp_win->frame_bw3D);


  /*
   * fix up frame and assign size/location values in tmp_win
   */

  frame_mask = 0;
  if (bw != tmp_win->frame_bw)
  {
    frame_wc.border_width = tmp_win->frame_bw = bw;
    frame_mask |= CWBorderWidth;
  }
  frame_wc.x = tmp_win->frame_x = x;
  frame_wc.y = tmp_win->frame_y = y;
  frame_wc.width = tmp_win->frame_width = w;
  frame_wc.height = tmp_win->frame_height = h;
  frame_mask |= (CWX | CWY | CWWidth | CWHeight);
  XConfigureWindow(dpy, tmp_win->frame, frame_mask, &frame_wc);
  tmp_win->virtual_frame_x = R_TO_V_X(tmp_win->frame_x);
  tmp_win->virtual_frame_y = R_TO_V_Y(tmp_win->frame_y);

  XMoveResizeWindow(dpy, tmp_win->w, tmp_win->frame_bw3D,
		    tmp_win->title_height + tmp_win->frame_bw3D, tmp_win->attr.width, tmp_win->attr.height);

  /*
   * fix up highlight window
   */

  if (tmp_win->title_height && tmp_win->hilite_w)
  {
    xwc.width = ComputeHighlightWindowWidth(tmp_win);

    if (xwc.width <= 0)
    {
      xwc.x = Scr->MyDisplayWidth;	/* move offscreen */
      xwc.width = 1;
    }
    else
    {
      xwc.x = tmp_win->highlightx;
    }

    xwcm = CWX | CWWidth;
    XConfigureWindow(dpy, tmp_win->hilite_w, xwcm, &xwc);
  }

  if (HasShape && reShape)
  {
    SetFrameShape(tmp_win);
  }

  if (sendEvent)
  {
    SendConfigureNotify(tmp_win, x, y);
  }
}

void
PaintBorderAndTitlebar(TwmWindow * tmp_win)
{
  if (tmp_win->highlight)
    SetBorder(tmp_win, True);
  else
  {
    if (Scr->BorderBevelWidth > 0)
      PaintBorders(tmp_win, True);

    if (tmp_win->titlebuttons)
    {
      int i, nb = Scr->TBInfo.nleft + Scr->TBInfo.nright;
      TBWindow *tbw;

      for (i = 0, tbw = tmp_win->titlebuttons; i < nb; i++, tbw++)
	PaintTitleButton(tmp_win, tbw, 0);
    }
  }

  PaintTitle(tmp_win);
  PaintTitleHighlight(tmp_win, on);
}


static void
DoVirtualMoveResize(TwmWindow * tmp_win, int x, int y, int w, int h)
{
  int fw = tmp_win->frame_width, fh = tmp_win->frame_height;

  tmp_win->virtual_frame_x = R_TO_V_X(x - tmp_win->frame_bw);
  tmp_win->virtual_frame_y = R_TO_V_Y(y - tmp_win->frame_bw);
  if (!tmp_win->opaque_resize)
  {
    tmp_win->frame_width = w + 2 * tmp_win->frame_bw;
    tmp_win->frame_height = h + 2 * tmp_win->frame_bw;
  }

  MoveResizeDesktop(tmp_win, TRUE);

  tmp_win->frame_width = fw;
  tmp_win->frame_height = fh;
}


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

static int
FindAreaIntersection(int a[4], int b[4])
{
  int dx, dy;

  dx = Distance1D(Lft(a), Rht(a), Lft(b), Rht(b));
  dy = Distance1D(Bot(a), Top(a), Bot(b), Top(b));
  return Distance2D(dx,dy);
}

#ifdef TILED_SCREEN

int
FindNearestTileToArea(int w[4])
{
  /*
   * Find a screen tile having maximum overlap with 'w'
   * (or, if 'w' is outside of all tiles, a tile with minimum
   * distance to 'w').
   */
  int k, a, m, i;

  m = -xmax(Scr->VirtualDesktopMaxWidth, Scr->VirtualDesktopMaxHeight);	/*some large value */
  i = 0;
  for (k = 0; k < Scr->ntiles; ++k)
  {
    a = FindAreaIntersection (w, Scr->tiles[k]);
    if (m < a)
      m = a, i = k;
  }
  return i;
}

int
FindNearestTileToPoint(int x, int y)
{
  int k, a, m, i, w[4];

  Lft(w) = Rht(w) = x;
  Bot(w) = Top(w) = y;

  i = 0;
  m = -xmax(Scr->MyDisplayWidth, Scr->MyDisplayHeight);
  for (k = 0; k < Scr->ntiles; ++k)
  {
    a = FindAreaIntersection (w, Scr->tiles[k]);
    if (m < a)
      m = a, i = k;
  }
  return i;
}

int
FindNearestTileToClient(TwmWindow * tmp)
{
  int w[4];

  Lft(w) = tmp->frame_x;
  Rht(w) = tmp->frame_x + tmp->frame_width  + 2*tmp->frame_bw - 1;
  Bot(w) = tmp->frame_y;
  Top(w) = tmp->frame_y + tmp->frame_height + 2*tmp->frame_bw - 1;
  return FindNearestTileToArea(w);
}

int
FindNearestTileToMouse(void)
{
  if (False == XQueryPointer(dpy, Scr->Root, &JunkRoot, &JunkChild, &JunkX, &JunkY, &HotX, &HotY, &JunkMask))
    /* emergency: mouse not on current X11-screen */
    return 0;
  else
    return FindNearestTileToPoint(JunkX, JunkY);
}

void
EnsureRectangleOnTile(int tile, int *x0, int *y0, int w, int h)
{
  if ((*x0) + w > Rht(Scr->tiles[tile]) + 1)
    (*x0) = Rht(Scr->tiles[tile]) + 1 - w;
  if ((*x0) < Lft(Scr->tiles[tile]))
    (*x0) = Lft(Scr->tiles[tile]);
  if ((*y0) + h > Top(Scr->tiles[tile]) + 1)
    (*y0) = Top(Scr->tiles[tile]) + 1 - h;
  if ((*y0) < Bot(Scr->tiles[tile]))
    (*y0) = Bot(Scr->tiles[tile]);
}

void
EnsureGeometryVisibility(int tile, int mask, int *x0, int *y0, int w, int h)
{
  int Area[4];

  /*
   * tile < 0 is 'full X11 screen'
   * tile = 0 is 'current panel'
   * tile > 0 is 'panel #'
   */
  --tile;
  if (tile >= 0 && tile < Scr->ntiles)
  {
    Lft(Area) = Lft(Scr->tiles[tile]);
    Bot(Area) = Bot(Scr->tiles[tile]);
    Rht(Area) = Rht(Scr->tiles[tile]);
    Top(Area) = Top(Scr->tiles[tile]);
  }
  else
  {
    int x, y;

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

    TilesFullZoom(Area);
  }

  if (mask & XNegative)
    (*x0) += Rht(Area) + 1 - w;
  else
    (*x0) += Lft(Area);
  if (mask & YNegative)
    (*y0) += Top(Area) + 1 - h;
  else
    (*y0) += Bot(Area);

  /* final sanity check: */
  if ((*x0) + w > Rht(Area) + 1)
    (*x0) = Rht(Area) + 1 - w;
  if ((*x0) < Lft(Area))
    (*x0) = Lft(Area);
  if ((*y0) + h > Top(Area) + 1)
    (*y0) = Top(Area) + 1 - h;
  if ((*y0) < Bot(Area))
    (*y0) = Bot(Area);
}

#define IntersectsV(a,b,t)  (((Bot(a)) <= (t)) && ((b) <= (Top(a))))
#define IntersectsH(a,l,r)  (((Lft(a)) <= (r)) && ((l) <= (Rht(a))))
#define OverlapsV(a,w)	    IntersectsV(a,Bot(w),Top(w))
#define OverlapsH(a,w)	    IntersectsH(a,Lft(w),Rht(w))
#define ContainsV(a,p)	    IntersectsV(a,p,p)
#define ContainsH(a,p)	    IntersectsH(a,p,p)

static int
BoundingBoxLeft(int w[4])
{
  int i, k;

  i = -1;			/* value '-1' is 'not found' */
  /* zoom out: */
  for (k = 0; k < Scr->ntiles; ++k)
    /* consider tiles vertically overlapping the window "height" (along left edge): */
    if (OverlapsV(Scr->tiles[k], w))
      /* consider tiles horizontally intersecting the window left edge: */
      if (ContainsH(Scr->tiles[k], Lft(w)))
      {
	if ((i == -1) || (Lft(Scr->tiles[k]) < Lft(Scr->tiles[i])))
	  i = k;
      }
  if (i == -1)
    /* zoom in (no intersecting tiles found): */
    for (k = 0; k < Scr->ntiles; ++k)
      if (OverlapsV(Scr->tiles[k], w))
	/* consider tiles having left edge right to the left edge of the window: */
	if (Lft(Scr->tiles[k]) > Lft(w))
	{
	  if ((i == -1) || (Lft(Scr->tiles[k]) < Lft(Scr->tiles[i])))
	    i = k;
	}
  return i;
}

static int
BoundingBoxBottom(int w[4])
{
  int i, k;

  i = -1;
  /* zoom out: */
  for (k = 0; k < Scr->ntiles; ++k)
    /* consider tiles horizontally overlapping the window "width" (along bottom edge): */
    if (OverlapsH(Scr->tiles[k], w))
      /* consider tiles vertically interscting the window bottom edge: */
      if (ContainsV(Scr->tiles[k], Bot(w)))
      {
	if ((i == -1) || (Bot(Scr->tiles[k]) < Bot(Scr->tiles[i])))
	  i = k;
      }
  if (i == -1)
    /* zoom in (no intersecting tiles found): */
    for (k = 0; k < Scr->ntiles; ++k)
      if (OverlapsH(Scr->tiles[k], w))
	/* consider tiles having bottom edge ontop of the bottom edge of the window: */
	if (Bot(Scr->tiles[k]) > Bot(w))
	{
	  if ((i == -1) || (Bot(Scr->tiles[k]) < Bot(Scr->tiles[i])))
	    i = k;
	}
  return i;
}

static int
BoundingBoxRight(int w[4])
{
  int i, k;

  i = -1;
  /* zoom out: */
  for (k = 0; k < Scr->ntiles; ++k)
    /* consider tiles vertically overlapping the window "height" (along right edge): */
    if (OverlapsV(Scr->tiles[k], w))
      /* consider tiles horizontally intersecting the window right edge: */
      if (ContainsH(Scr->tiles[k], Rht(w)))
      {
	if ((i == -1) || (Rht(Scr->tiles[k]) > Rht(Scr->tiles[i])))
	  i = k;
      }
  if (i == -1)
    /* zoom in (no intersecting tiles found): */
    for (k = 0; k < Scr->ntiles; ++k)
      if (OverlapsV(Scr->tiles[k], w))
	/* consider tiles having right edge left to the right edge of the window: */
	if (Rht(Scr->tiles[k]) < Rht(w))
	{
	  if ((i == -1) || (Rht(Scr->tiles[k]) > Rht(Scr->tiles[i])))
	    i = k;
	}
  return i;
}

static int
BoundingBoxTop(int w[4])
{
  int i, k;

  i = -1;
  /* zoom out: */
  for (k = 0; k < Scr->ntiles; ++k)
    /* consider tiles horizontally overlapping the window "width" (along top edge): */
    if (OverlapsH(Scr->tiles[k], w))
      /* consider tiles vertically interscting the window top edge: */
      if (ContainsV(Scr->tiles[k], Top(w)))
      {
	if ((i == -1) || (Top(Scr->tiles[k]) > Top(Scr->tiles[i])))
	  i = k;
      }
  if (i == -1)
    /* zoom in (no intersecting tiles found): */
    for (k = 0; k < Scr->ntiles; ++k)
      if (OverlapsH(Scr->tiles[k], w))
	/* consider tiles having top edge below of the top edge of the window: */
	if (Top(Scr->tiles[k]) < Top(w))
	{
	  if ((i == -1) || (Top(Scr->tiles[k]) > Top(Scr->tiles[i])))
	    i = k;
	}
  return i;
}

static int
UncoveredLeft(int skip, int area[4], int thresh)
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
	if (ContainsH(Scr->tiles[k], Lft(area) - thresh))
	{
	  if (Top(Scr->tiles[k]) < Top(area))
	  {
	    Lft(w) = Lft(area);
	    Bot(w) = Top(Scr->tiles[k]) + 1;
	    Rht(w) = Rht(area);
	    Top(w) = Top(area);
	    u = UncoveredLeft(skip, w, thresh);
	    e = 1;
	  }
	  if (Bot(Scr->tiles[k]) > Bot(area))
	  {
	    Lft(w) = Lft(area);
	    Bot(w) = Bot(area);
	    Rht(w) = Rht(area);
	    Top(w) = Bot(Scr->tiles[k]) - 1;
	    if (e == 0)
	      u = UncoveredLeft(skip, w, thresh);
	    else
	      u += UncoveredLeft(skip, w, thresh);
	    e = 1;
	  }
	  if (e == 0)
	  {
	    u = 0;
	    e = 1;
	  }
	}
  return u;
}

static int
ZoomLeft(int bbl, int b, int t, int thresv, int thresh)
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
	if (ContainsH(Scr->tiles[bbl], Lft(Scr->tiles[k])) && (Lft(Scr->tiles[i]) < Lft(Scr->tiles[k])))
	{
	  u = UncoveredLeft(k, Scr->tiles[k], thresh);
	  if (u > thresv)
	    i = k;
	}
  return i;
}

static int
UncoveredBottom(int skip, int area[4], int thresv)
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
	if (ContainsV(Scr->tiles[k], Bot(area) - thresv))
	{
	  if (Rht(Scr->tiles[k]) < Rht(area))
	  {
	    Lft(w) = Rht(Scr->tiles[k]) + 1;
	    Bot(w) = Bot(area);
	    Rht(w) = Rht(area);
	    Top(w) = Top(area);
	    u = UncoveredBottom(skip, w, thresv);
	    e = 1;
	  }
	  if (Lft(Scr->tiles[k]) > Lft(area))
	  {
	    Lft(w) = Lft(area);
	    Bot(w) = Bot(area);
	    Rht(w) = Lft(Scr->tiles[k]) - 1;
	    Top(w) = Top(area);
	    if (e == 0)
	      u = UncoveredBottom(skip, w, thresv);
	    else
	      u += UncoveredBottom(skip, w, thresv);
	    e = 1;
	  }
	  if (e == 0)
	  {
	    u = 0;
	    e = 1;
	  }
	}
  return u;
}

static int
ZoomBottom(int bbb, int l, int r, int thresh, int thresv)
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
	if (ContainsV(Scr->tiles[bbb], Bot(Scr->tiles[k])) && (Bot(Scr->tiles[i]) < Bot(Scr->tiles[k])))
	{
	  u = UncoveredBottom(k, Scr->tiles[k], thresv);
	  if (u > thresh)
	    i = k;
	}
  return i;
}

static int
UncoveredRight(int skip, int area[4], int thresh)
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
	if (ContainsH(Scr->tiles[k], Rht(area) + thresh))
	{
	  if (Top(Scr->tiles[k]) < Top(area))
	  {
	    Lft(w) = Lft(area);
	    Bot(w) = Top(Scr->tiles[k]) + 1;
	    Rht(w) = Rht(area);
	    Top(w) = Top(area);
	    u = UncoveredRight(skip, w, thresh);
	    e = 1;
	  }
	  if (Bot(Scr->tiles[k]) > Bot(area))
	  {
	    Lft(w) = Lft(area);
	    Bot(w) = Bot(area);
	    Rht(w) = Rht(area);
	    Top(w) = Bot(Scr->tiles[k]) - 1;
	    if (e == 0)
	      u = UncoveredRight(skip, w, thresh);
	    else
	      u += UncoveredRight(skip, w, thresh);
	    e = 1;
	  }
	  if (e == 0)
	  {
	    u = 0;
	    e = 1;
	  }
	}
  return u;
}

static int
ZoomRight(int bbr, int b, int t, int thresv, int thresh)
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
	if (ContainsH(Scr->tiles[bbr], Rht(Scr->tiles[k])) && (Rht(Scr->tiles[k]) < Rht(Scr->tiles[i])))
	{
	  u = UncoveredRight(k, Scr->tiles[k], thresh);
	  if (u > thresv)
	    i = k;
	}
  return i;
}

static int
UncoveredTop(int skip, int area[4], int thresv)
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
	if (ContainsV(Scr->tiles[k], Top(area) + thresv))
	{
	  if (Rht(Scr->tiles[k]) < Rht(area))
	  {
	    Lft(w) = Rht(Scr->tiles[k]) + 1;
	    Bot(w) = Bot(area);
	    Rht(w) = Rht(area);
	    Top(w) = Top(area);
	    u = UncoveredTop(skip, w, thresv);
	    e = 1;
	  }
	  if (Lft(Scr->tiles[k]) > Lft(area))
	  {
	    Lft(w) = Lft(area);
	    Bot(w) = Bot(area);
	    Rht(w) = Lft(Scr->tiles[k]) - 1;
	    Top(w) = Top(area);
	    if (e == 0)
	      u = UncoveredTop(skip, w, thresv);
	    else
	      u += UncoveredTop(skip, w, thresv);
	    e = 1;
	  }
	  if (e == 0)
	  {
	    u = 0;
	    e = 1;
	  }
	}
  return u;
}

static int
ZoomTop(int bbt, int l, int r, int thresh, int thresv)
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
	if (ContainsV(Scr->tiles[bbt], Top(Scr->tiles[k])) && (Top(Scr->tiles[k]) < Top(Scr->tiles[i])))
	{
	  u = UncoveredTop(k, Scr->tiles[k], thresv);
	  if (u > thresh)
	    i = k;
	}
  return i;
}

void
TilesFullZoom(int Area[4])
{
  /*
   * First step: find screen tiles (which overlap the 'Area')
   * which define a "bounding box of maximum area".
   * The following 4 functions return indices to these tiles.
   */

  int l, b, r, t, f;

  f = 0;

nxt:;

  l = BoundingBoxLeft(Area);
  b = BoundingBoxBottom(Area);
  r = BoundingBoxRight(Area);
  t = BoundingBoxTop(Area);

  if (l >= 0 && b >= 0 && r >= 0 && t >= 0)
  {
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
    l = ZoomLeft(l, i, j, /*uncovered */ 10, /*slit */ 10);

    i = Lft(Scr->tiles[l]);
    if (i < Lft(Area))
      i += Lft(Area), i /= 2;
    j = Rht(Scr->tiles[r]);
    if (j > Rht(Area))
      j += Rht(Area), j /= 2;
    b = ZoomBottom(b, i, j, 10, 10);

    i = Bot(Scr->tiles[b]);
    if (i < Bot(Area))
      i += Bot(Area), i /= 2;
    j = Top(Scr->tiles[t]);
    if (j > Top(Area))
      j += Top(Area), j /= 2;
    r = ZoomRight(r, i, j, 10, 10);

    i = Lft(Scr->tiles[l]);
    if (i < Lft(Area))
      i += Lft(Area), i /= 2;
    j = Rht(Scr->tiles[r]);
    if (j > Rht(Area))
      j += Rht(Area), j /= 2;
    t = ZoomTop(t, i, j, 10, 10);

    if (l >= 0 && b >= 0 && r >= 0 && t >= 0)
    {
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

  if (f == 0)
  {
    f = 1;
    goto nxt;
  }
}

#endif /*TILED_SCREEN */


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


#define SET_dragx_TO_PHYSICALLY_VISIBLE		\
	dragx = V_TO_R_X(tmp_win->save_frame_x);					\
	if (dragx < basex || dragx + dragWidth + frame_bw_times_2 > basex + basew)	\
	{	\
	    dragx = basex + (tmp_win->save_frame_x % basew);			\
	    if (dragx + dragWidth + frame_bw_times_2 > basex + basew)		\
	    {	\
		/* not fitting 'modulo visible width', try centered: */		\
		dragx = basex + (basew - dragWidth - frame_bw_times_2) / 2;	\
		if (dragx - basex < 0)		\
		/* beyond the edge, ensure origin is visible: */		\
		dragx = basex;			\
	    }	\
	}


#define SET_dragy_TO_PHYSICALLY_VISIBLE		\
	dragy = V_TO_R_Y(tmp_win->save_frame_y);					\
	if (dragy < basey || dragy + dragHeight + frame_bw_times_2 > basey + baseh)	\
	{	\
	    dragy = basey + (tmp_win->save_frame_y % baseh);			\
	    if (dragy + dragHeight + frame_bw_times_2 > basey + baseh)		\
	    {	\
		/* not fitting 'modulo visible height', try centered: */	\
		dragy = basey + (baseh - dragHeight - frame_bw_times_2) / 2;	\
		if (dragy - basey < 0)		\
		/* beyond the edge, ensure origin is visible: */		\
		dragy = basey;			\
	    }	\
	}


void
fullzoom(int tile, TwmWindow * tmp_win, int flag)
{
  int Area[4], basex, basey, basew, baseh, dx, dy, frame_bw_times_2;
  Bool mm = False;


  XGetGeometry (dpy, (Drawable)tmp_win->frame, &JunkRoot,
		&dragx, &dragy, (unsigned int *)&dragWidth, (unsigned int *)&dragHeight,
		&JunkBW, &JunkDepth);

  frame_bw_times_2 = (int)(JunkBW) * 2;


  if (tmp_win->zoomed == flag
	&& flag != F_PANELGEOMETRYZOOM && flag != F_PANELGEOMETRYMOVE)
  {
    unzoom:;

    /** recover location **/

    /* check if zoomed client intersects panel now: */
    Lft(Area) = dragx;
    Bot(Area) = dragy;
    Rht(Area) = Lft(Area) + dragWidth  + 2*tmp_win->frame_bw - 1;
    Top(Area) = Bot(Area) + dragHeight + 2*tmp_win->frame_bw - 1;

    /* f.maximize dragged size beyond panel, undo this first: */
    if (flag == F_MAXIMIZE || flag == F_PANELMAXIMIZE)
    {
      Lft(Area) += ((int)(JunkBW) + tmp_win->frame_bw3D);
      Bot(Area) += ((int)(JunkBW) + tmp_win->frame_bw3D) + tmp_win->title_height; /*Bot() is 'top', here it matters*/
      Rht(Area) -= ((int)(JunkBW) + tmp_win->frame_bw3D);
      Top(Area) -= ((int)(JunkBW) + tmp_win->frame_bw3D);
    }

#if defined TILED_SCREEN  &&  0
    if (Scr->use_tiles == TRUE)
    {
      /* consider current panel while unzooming: */
      int i = FindNearestTileToArea(Area);
      basex = Lft(Scr->tiles[i]);
      basey = Bot(Scr->tiles[i]);
      basew = AreaWidth(Scr->tiles[i]);
      baseh = AreaHeight(Scr->tiles[i]);
      dx = FindAreaIntersection(Area, Scr->tiles[i]);
    }
    else
#endif
    {
      /* consider current virtual desktop while unzooming: */
      int s[4];
      basew = Scr->MyDisplayWidth;
      baseh = Scr->MyDisplayHeight;
      Lft(s) = basex = 0;
      Bot(s) = basey = 0;
      Rht(s) = Lft(s) + basew - 1;
      Top(s) = Bot(s) + baseh - 1;
      dx = FindAreaIntersection(Area, s);
    }

    /* check if client intersected previous panel/virtual desktop prior to zooming: */
    Lft(Area) = tmp_win->save_frame_x;
    Bot(Area) = tmp_win->save_frame_y;
    Rht(Area) = Lft(Area) + tmp_win->save_frame_width  + 2*tmp_win->frame_bw - 1;
    Top(Area) = Bot(Area) + tmp_win->save_frame_height + 2*tmp_win->frame_bw - 1;
    dy = FindAreaIntersection(Area, tmp_win->save_tile);

    /* first check previous panel intersecting previously, then current panel now: */
    if (dy > 0)
    {
      /* pre-zoomed client intersected previous panel: */
      if (dx > 0)
      {
	/* client intersects current VD/panel now: */
	dragx = basex + (Lft(Area) - Lft(tmp_win->save_tile));
	dragy = basey + (Bot(Area) - Bot(tmp_win->save_tile));
      }
      else
      {
	/* client doesn't intersect current panel, recover old panel-relative location: */
	dragx = V_TO_R_X(tmp_win->save_frame_x); /* recover physical coordinates */
	dragy = V_TO_R_Y(tmp_win->save_frame_y); /* recover physical coordinates */
      }
    }
    else
    {
      /* previous panel/VD-relative location not recoverable */
      if (dx > 0)
      {
	if (Scr->UnzoomToScreen == TRUE || tmp_win->nailed)
	{
	  /* client intersects current panel now and is marked 'sticky': */
	  SET_dragx_TO_PHYSICALLY_VISIBLE;
	  SET_dragy_TO_PHYSICALLY_VISIBLE;
	}
	else
	{
	  dragx = V_TO_R_X(tmp_win->save_frame_x); /* recover physical coordinates */
	  dragy = V_TO_R_Y(tmp_win->save_frame_y); /* recover physical coordinates */
	}
      }
      else
      {
	/* don't change location (except slight shift down for f.maximize): */
	if (flag == F_MAXIMIZE || flag == F_PANELMAXIMIZE)
	{
	  dragx += ((int)(JunkBW) + tmp_win->frame_bw3D);
	  dragy += ((int)(JunkBW) + tmp_win->frame_bw3D) + tmp_win->title_height;
	}
      }
    }
    /* location recovered */

    /* recover size: */
    dragWidth = tmp_win->save_frame_width;
    dragHeight = tmp_win->save_frame_height;

    tmp_win->zoomed = ZOOM_NONE;
  }

  else /* zoom */

  {
    int xmask, ymask;

#ifdef TILED_SCREEN
    /*
     * panel-zoom variants (F_PANEL...ZOOM) have their numeric
     * codes greater than F_MAXIMIZE.  See also parse.h.
     */
    if (Scr->use_tiles == TRUE && flag > F_MAXIMIZE && tile >= 0)
    {
      /*
       * On fullzoom() entry:
       * tile < 0 is 'zoom on X11 logical screen' (denoted by "0" in .twmrc)
       * tile = 0 is 'zoom on current panel(s)'   (denoted by "." in .twmrc)
       * tile > 0 is 'zoom on panel #'  (counting in .twmrc starts from "1")
       */
      tile--;
      if (tile >= 0 && tile < Scr->ntiles)
      {
	Lft(Area) = Lft(Scr->tiles[tile]);
	Bot(Area) = Bot(Scr->tiles[tile]);
	Rht(Area) = Rht(Scr->tiles[tile]);
	Top(Area) = Top(Scr->tiles[tile]);
      }
      else
      {
	/*
	 * fallback: compute maximum area across
	 * tiles the window initially intersects
	 */
	Lft(Area) = dragx;
	Rht(Area) = dragx + frame_bw_times_2 + dragWidth  - 1;
	Bot(Area) = dragy;
	Top(Area) = dragy + frame_bw_times_2 + dragHeight - 1;
	TilesFullZoom (Area);

	/* Test if client intersects area or not (then area is X11-screen); bring client to mouse-tile */
	if (!IntersectsH(Area,dragx,dragx+dragWidth-1) || !IntersectsV(Area,dragy,dragy+dragHeight-1))
	  if (True == XQueryPointer(dpy, Scr->Root, &JunkRoot, &JunkChild, &JunkX, &JunkY, &HotX, &HotY, &JunkMask))
	  {
	    int i = FindNearestTileToMouse();
	    Lft(Area) = Lft(Scr->tiles[i]);
	    Bot(Area) = Bot(Scr->tiles[i]);
	    Rht(Area) = Rht(Scr->tiles[i]);
	    Top(Area) = Top(Scr->tiles[i]);
	  }
      }
      basex = Lft(Area); /* Cartesian origin:   */
      basey = Bot(Area); /* (x0,y0) = (Lft,Bot) */
      basew = AreaWidth(Area);
      baseh = AreaHeight(Area);
    }
    else
#endif
    {
      basex = 0;
      basey = 0;
      basew = Scr->MyDisplayWidth;
      baseh = Scr->MyDisplayHeight;
    }


    if (tmp_win->zoomed == ZOOM_NONE)
    {
      tmp_win->save_frame_x = R_TO_V_X(dragx); /* store virtual coordinates */
      tmp_win->save_frame_y = R_TO_V_Y(dragy); /* store virtual coordinates */
      tmp_win->save_frame_width = dragWidth;
      tmp_win->save_frame_height = dragHeight;
#if defined TILED_SCREEN  &&  0
      if (Scr->use_tiles == TRUE)
      {
	/* record current panel geometry for later origin recovery */
        int i = FindNearestTileToClient(tmp_win);
        Lft(tmp_win->save_tile) = R_TO_V_X(Lft(Scr->tiles[i]));
        Bot(tmp_win->save_tile) = R_TO_V_Y(Bot(Scr->tiles[i]));
	Rht(tmp_win->save_tile) = R_TO_V_X(Rht(Scr->tiles[i]));
	Top(tmp_win->save_tile) = R_TO_V_Y(Top(Scr->tiles[i]));
      }
      else
#endif
      {
	/* record current virtual desktop geometry for later origin recovery */
	Lft(tmp_win->save_tile) = R_TO_V_X(0);
	Bot(tmp_win->save_tile) = R_TO_V_Y(0);
	Rht(tmp_win->save_tile) = Lft(tmp_win->save_tile) + Scr->MyDisplayWidth  - 1;
	Top(tmp_win->save_tile) = Bot(tmp_win->save_tile) + Scr->MyDisplayHeight - 1;
      }
    }


    switch (flag)
    {
    case ZOOM_NONE:
      break;

    case F_PANELZOOM:
    case F_ZOOM:
      /* first ensure the client appears on X11 screen/panel (horizontally): */
      SET_dragx_TO_PHYSICALLY_VISIBLE;
      dragy = basey;
      dragWidth = tmp_win->save_frame_width;
      dragHeight = baseh - frame_bw_times_2;
      break;

    case F_PANELHORIZOOM:
    case F_HORIZOOM:
      /* first ensure the client appears on X11 screen/panel (vertically): */
      SET_dragy_TO_PHYSICALLY_VISIBLE;
      dragx = basex;
      dragWidth = basew - frame_bw_times_2;
      dragHeight = tmp_win->save_frame_height;
      break;

    case F_PANELFULLZOOM:
    case F_PANELMAXIMIZE:
    case F_FULLZOOM:
    case F_MAXIMIZE:
      dragx = basex;
      dragy = basey;
      dragWidth = basew;
      dragHeight = baseh;
      if (flag == F_FULLZOOM || flag == F_PANELFULLZOOM)
      {
	dragWidth  -= frame_bw_times_2;
	dragHeight -= frame_bw_times_2;
      }
      else
      {
	dragx -= ((int)(JunkBW) + tmp_win->frame_bw3D);
	dragy -= ((int)(JunkBW) + tmp_win->frame_bw3D) + tmp_win->title_height;
	dragWidth  += 2*tmp_win->frame_bw3D;
	dragHeight += 2*tmp_win->frame_bw3D + tmp_win->title_height;
      }
      break;

    case F_PANELLEFTZOOM:
    case F_LEFTZOOM:
      dragx = basex;
      dragy = basey;
      dragWidth = basew / 2 - frame_bw_times_2;
      dragHeight = baseh - frame_bw_times_2;
      break;

    case F_PANELRIGHTZOOM:
    case F_RIGHTZOOM:
      dragx = basex + basew / 2;
      dragy = basey;
      dragWidth = basew / 2 - frame_bw_times_2;
      dragHeight = baseh - frame_bw_times_2;
      break;

    case F_PANELTOPZOOM:
    case F_TOPZOOM:
      dragx = basex;
      dragy = basey;
      dragWidth = basew - frame_bw_times_2;
      dragHeight = baseh / 2 - frame_bw_times_2;
      break;

    case F_PANELBOTTOMZOOM:
    case F_BOTTOMZOOM:
      dragx = basex;
      dragy = basey + baseh / 2;
      dragWidth = basew - frame_bw_times_2;
      dragHeight = baseh / 2 - frame_bw_times_2;
      break;

    case F_PANELLEFTMOVE:
      SET_dragy_TO_PHYSICALLY_VISIBLE;
      dragx = basex;
      break;

    case F_PANELRIGHTMOVE:
      SET_dragy_TO_PHYSICALLY_VISIBLE;
      dragx = basex + basew - dragWidth - frame_bw_times_2;
      break;

    case F_PANELTOPMOVE:
      SET_dragx_TO_PHYSICALLY_VISIBLE;
      dragy = basey;
      break;

    case F_PANELBOTTOMMOVE:
      SET_dragx_TO_PHYSICALLY_VISIBLE;
      dragy = basey + baseh - dragHeight - frame_bw_times_2;
      break;

    case F_PANELGEOMETRYZOOM:
    case F_PANELGEOMETRYMOVE:

      /*
       * mask and requested geometry are precomputed in fullgeomzoom()
       * as origMask, origx, origy, origWidth, origHeight
       */

      xmask = ((origMask & (XValue|WidthValue))  == (XValue|WidthValue));
      ymask = ((origMask & (YValue|HeightValue)) == (YValue|HeightValue));

      mm = XQueryPointer (dpy, tmp_win->frame, &JunkRoot, &JunkChild,
			    &JunkX, &JunkY, &HotX, &HotY, &JunkMask);

      /* check if mouse on target area */
      if (mm == True
	    && basex <= JunkX && JunkX < basex + basew
	    && basey <= JunkY && JunkY < basey + baseh)
      {
	if ((origx == 0 && origy == 0)
	      || HotX < -(int)(JunkBW) || HotX >= dragWidth  + (int)(JunkBW)
	      || HotY < -(int)(JunkBW) || HotY >= dragHeight + (int)(JunkBW))
	{
	  mm = False;
	}

	/*
	 * special cases on current panel: geometry "0x0-0-0" or "0x0+0+0"
	 * recover size/pos, set 'unzoomed'
	 */
	if (xmask && (origx == 0) && (origWidth == 0)
	      && ymask && (origy == 0) && (origHeight == 0))
	{
	  flag = 1;
	  if (origMask & XNegative) { /* recover horizontal geometry */
	    dragWidth = tmp_win->save_frame_width;
	    dragx = V_TO_R_X(tmp_win->save_frame_x);
	    flag = 0;
	  } else {
	    tmp_win->save_frame_width = dragWidth;
	    tmp_win->save_frame_x = R_TO_V_X(dragx);
	  }
	  if (origMask & YNegative) { /* recover vertical geometry */
	    dragHeight = tmp_win->save_frame_height;
	    dragy = V_TO_R_Y(tmp_win->save_frame_y);
	    flag = 0;
	  } else {
	    tmp_win->save_frame_height = dragHeight;
	    tmp_win->save_frame_y = R_TO_V_Y(dragy);
	  }

	  tmp_win->zoomed = ZOOM_NONE; /* set state to 'unzoomed' */
	  if (flag == 1)
	    return;

	  flag = ZOOM_NONE;
	  break;
	}
      }
      else
	mm = False;


#ifdef TILED_SCREEN

      dx = dragx - basex;
      dy = dragy - basey;

      if (dx < 0 || basew < dx + dragWidth + frame_bw_times_2
	  || dy < 0 || baseh < dy + dragHeight + frame_bw_times_2)
      {
	/*
	 * target area/panel is (partially) outside the source area/panel:
	 * recompute dragx, dragy
	 */
	if (dragWidth + frame_bw_times_2 < basew) {
	  if (dragHeight + frame_bw_times_2 < baseh) {
	    /* window completely fits onto target panel */
	    int k, a;
	    Lft(Area) = dragx;
	    Rht(Area) = dragx + frame_bw_times_2 + dragWidth  - 1;
	    Bot(Area) = dragy;
	    Top(Area) = dragy + frame_bw_times_2 + dragHeight - 1;
	    k = FindNearestTileToArea (Area);
	    a = FindAreaIntersection (Area, Scr->tiles[k]);
	    if (a == (dragWidth+frame_bw_times_2) * (dragHeight+frame_bw_times_2)) {
	      /* completely fitted on source panel, move to target panel, keep relative location */
	      dragx = basex + (dragx - Lft(Scr->tiles[k])) * (basew - frame_bw_times_2 - dragWidth)
				/ (AreaWidth(Scr->tiles[k])  - frame_bw_times_2 - dragWidth);
	      dragy = basey + (dragy - Bot(Scr->tiles[k])) * (baseh - frame_bw_times_2 - dragHeight)
				/ (AreaHeight(Scr->tiles[k]) - frame_bw_times_2 - dragHeight);
	    } else {
	      /* intersected various panels in source area */
#if 1
	      PlaceXY area;
	      area.x = basex;
	      area.y = basey;
	      area.width = basew;
	      area.height = baseh;
	      area.next = NULL;
	      if (FindEmptyArea(Scr->TwmRoot.next, tmp_win, &area, &area) == TRUE)
	      {
		/* found emtpy area large enough to completely fit the window */
		int x, y, b = Scr->TitleHeight + (int)(JunkBW);
		/* slightly off-centered: */
		x = (area.width  - dragWidth)  / 3;
		y = (area.height - dragHeight) / 2;
		/* tight placing: */
		if (y < b && x > b)
		  x = b;
		if (x < b && y > b)
		  y = b;
		/* loosen placing: */
		if (area.width  > 4*b + dragWidth  && x > 2*b)
		  x = 2*b;
		if (area.height > 4*b + dragHeight && y > 2*b)
		  y = 2*b;
		dragx = area.x + x;
		dragy = area.y + y;
	      }
	      else
#endif
	      {
		/* not found empty area, put centered onto target panel */
		dragx = basex + (basew - dragWidth  - frame_bw_times_2) / 2;
		dragy = basey + (baseh - dragHeight - frame_bw_times_2) / 2;
	      }
	    }
	  } else {
	    /* vertically not fitting onto target panel */
	    if ((tmp_win->hints.flags & PWinGravity)
		  && (tmp_win->hints.win_gravity == SouthWestGravity
		      || tmp_win->hints.win_gravity == SouthGravity
		      || tmp_win->hints.win_gravity == SouthEastGravity))
	    {
	      /* align to panel lower edge */
	      dragy = basey + (baseh - dragHeight - frame_bw_times_2);
	      basey = dragy;
	    } else
	      /* align to panel upper edge */
	      dragy = basey;
	    /* put horizontally centered */
	    dragx = basex + (basew - dragWidth  - frame_bw_times_2) / 2;
	  }
	} else {
	  /* horizontally not fitting onto target panel */
	  if ((tmp_win->hints.flags & PWinGravity)
		&& (tmp_win->hints.win_gravity == NorthEastGravity
		    || tmp_win->hints.win_gravity == EastGravity
		    || tmp_win->hints.win_gravity == SouthEastGravity))
	  {
	    /* align to panel right edge */
	    dragx = basex + (basew - dragWidth - frame_bw_times_2);
	    basex = dragx;
	  } else
	    /* align to panel left edge */
	    dragx = basex;

	  /* check fitting vertically */
	  if (dragHeight + frame_bw_times_2 < baseh)
	    /* yes, put vertically centered */
	    dragy = basey + (baseh - dragHeight - frame_bw_times_2) / 2;
	  else {
	    /* no, vertically not fitting onto target panel */
	    if ((tmp_win->hints.flags & PWinGravity)
		  && (tmp_win->hints.win_gravity == SouthWestGravity
		      || tmp_win->hints.win_gravity == SouthGravity
		      || tmp_win->hints.win_gravity == SouthEastGravity))
	    {
	      /* align to panel lower edge */
	      dragy = basey + (baseh - dragHeight - frame_bw_times_2);
	      basey = dragy;
	    } else
	      /* align to panel upper edge */
	      dragy = basey;
	  }
	}

	dx = dragx - basex;
	dy = dragy - basey;
      }

#else /*TILED_SCREEN*/

      SET_dragx_TO_PHYSICALLY_VISIBLE;
      SET_dragy_TO_PHYSICALLY_VISIBLE;
      dx = dragx - basex;
      dy = dragy - basey;

#endif /*TILED_SCREEN*/


      /* now finally treat horizontal geometry ('W' and 'X' of "WxH+X+Y"): */
      if (xmask && (origWidth > 0))
      {
#if 0
	/* stepping/stretching reached panel edge, execute 'unzoom' */
	if ((tmp_win->zoomed != ZOOM_NONE)
	      && origx == 0 && (dx == 0 || dx + dragWidth + frame_bw_times_2 == basew))
	  goto unzoom;
#endif
	if (origMask & XNegative) { /** step/stretch to the left **/
	  if (origx != 0 && -origx*origWidth < dx) { /* "WxH-X+Y" (X != 0) */
	    /* step/stretch inside panel area */
	    if (flag == F_PANELGEOMETRYZOOM)
	      dragWidth -= origx*origWidth;
	    dragx += origx*origWidth;
	    /* keep window right edge fixed */
	    JunkWidth = dragWidth;
	    JunkHeight = dragHeight;
	    ConstrainSize (tmp_win, &JunkWidth, &JunkHeight);
	    if (dragWidth != JunkWidth)
	      dragx += dragWidth - JunkWidth;
	  } else { /* geometry is "WxH-0+Y", or */
	    /* step/stretch to/across panel left edge */
	    if (flag == F_PANELGEOMETRYZOOM)
	      dragWidth += dx;
	    dragx = basex;
	  }
	} else { /** step/stretch to the right **/
	  if (origx != 0 && dx+dragWidth + origx*origWidth + frame_bw_times_2 < basew) { /* "WxH+X+Y" (X != 0) */
	    /* step/stretch inside panel area */
	    if (flag == F_PANELGEOMETRYZOOM)
	      dragWidth += origx*origWidth;
	    else
	      dragx += origx*origWidth;
	  } else { /* geometry is "WxH+0+Y", or */
	    /* step/stretch to/across panel right edge */
	    if (flag == F_PANELGEOMETRYZOOM)
	      dragWidth = basew - dx - frame_bw_times_2;
	    else
	      dragx = basew - dragWidth + basex - frame_bw_times_2;
	    /* fix window right edge at panel edge */
	    JunkWidth = dragWidth;
	    JunkHeight = dragHeight;
	    ConstrainSize (tmp_win, &JunkWidth, &JunkHeight);
	    if (dragWidth != JunkWidth)
	      dragx += dragWidth - JunkWidth;
	  }
	}
      }

      /* treat vertical geometry ('H' and 'Y' of "WxH+X+Y"): */
      if (ymask && (origHeight > 0))
      {
#if 0
	if ((tmp_win->zoomed != ZOOM_NONE)
	      && origy == 0 && (dy == 0 || dy + dragHeight + frame_bw_times_2 == baseh))
	  goto unzoom;
#endif
	if (origMask & YNegative) { /* step/stretch up */
	  if (origy != 0 && -origy*origHeight < dy) {
	    if (flag == F_PANELGEOMETRYZOOM)
	      dragHeight -= origy*origHeight;
	    dragy += origy*origHeight;
	    /* keep window bottom edge fixed */
	    JunkWidth = dragWidth;
	    JunkHeight = dragHeight;
	    ConstrainSize (tmp_win, &JunkWidth, &JunkHeight);
	    if (dragHeight != JunkHeight)
	      dragy += dragHeight - JunkHeight;
	  } else {
	    if (flag == F_PANELGEOMETRYZOOM)
	      dragHeight += dy;
	    dragy = basey;
	  }
	} else { /* step/stretch down */
	  if (origy != 0 && dy+dragHeight + origy*origHeight + frame_bw_times_2 < baseh) {
	    if (flag == F_PANELGEOMETRYZOOM)
	      dragHeight += origy*origHeight;
	    else
	      dragy += origy*origHeight;
	  } else {
	    if (flag == F_PANELGEOMETRYZOOM)
	      dragHeight = baseh - dy - frame_bw_times_2;
	    else
	      dragy = baseh - dragHeight + basey - frame_bw_times_2;
	    /* fix window bottom edge at panel edge */
	    JunkWidth = dragWidth;
	    JunkHeight = dragHeight;
	    ConstrainSize (tmp_win, &JunkWidth, &JunkHeight);
	    if (dragHeight != JunkHeight)
	      dragy += dragHeight - JunkHeight;
	  }
	}
      }

      break;
    }

    tmp_win->zoomed = flag;
  }

  if (!Scr->NoRaiseResize)
  {
    XRaiseWindow(dpy, tmp_win->frame);

    RaiseStickyAbove();
    RaiseAutoPan();
  }

  ConstrainSize(tmp_win, &dragWidth, &dragHeight);
  SetupWindow(tmp_win, dragx, dragy, dragWidth, dragHeight, -1);

  ResizeTwmWindowContents(tmp_win, dragWidth, dragHeight);

#if 1
  if (mm == True && flag == F_PANELGEOMETRYMOVE)
    /* if mouse did/would fall inside the window, move mouse as well */
    XWarpPointer(dpy, None, tmp_win->frame, 0, 0, 0, 0, HotX, HotY);
#endif
}

void
fullgeomzoom(char *geometry_name, TwmWindow *tmp_win, int flag)
{
  int tile;
  char *name;
  char geom[200];

  strncpy (geom, geometry_name, sizeof(geom)-1); /*work on a copy*/
  geom[sizeof(geom)-1] = '\0';
  name = strchr (geom, '@');
  if (name != NULL)
    *name++ = '\0';
  tile = ParsePanelIndex (name);

  /*
   * precompute the mask and requested geometry for fullzoom()
   * as origMask, origx, origy, origWidth, origHeight
   */
  origMask = XParseGeometry (geom, &origx, &origy,
			(unsigned int *)&origWidth, (unsigned int *)&origHeight);

  fullzoom (tile, tmp_win, flag);
}

/*
 * adjust contents of iconmgrs, doors and the desktop - djhjr - 9/10/99
 */
void
ResizeTwmWindowContents(TwmWindow * tmp_win, int width, int height)
{
  TwmDoor *door;
  int ncols;

  if (tmp_win->iconmgr)
  {
    ncols = tmp_win->iconmgrp->cur_columns;
    if (ncols == 0)
      ncols = 1;

    tmp_win->iconmgrp->width = (int)(((width - 2 * tmp_win->frame_bw3D) * (long)tmp_win->iconmgrp->columns) / ncols);
    PackIconManager(tmp_win->iconmgrp);
  }
  else if (tmp_win->w == Scr->VirtualDesktopDisplayOuter)
    ResizeDesktopDisplay(width, height);
  else if (XFindContext(dpy, tmp_win->w, DoorContext, (caddr_t *) & door) != XCNOENT)
    RedoDoorName(tmp_win, door);
}

void
SetFrameShape(TwmWindow * tmp)
{
  /*
   * see if the titlebar needs to move
   */
  if (tmp->title_w.win)
  {
    int oldx = tmp->title_x, oldy = tmp->title_y;

    ComputeTitleLocation(tmp);
    if (oldx != tmp->title_x || oldy != tmp->title_y)
      XMoveWindow(dpy, tmp->title_w.win, tmp->title_x, tmp->title_y);
  }

  /*
   * The frame consists of the shape of the contents window offset by
   * title_height or'ed with the shape of title_w.win (which is always
   * rectangular).
   */
  if (tmp->wShaped)
  {
    /*
     * need to do general case
     */
    XShapeCombineShape(dpy, tmp->frame, ShapeBounding,
		       tmp->frame_bw3D, tmp->title_height + tmp->frame_bw3D, tmp->w, ShapeBounding, ShapeSet);
    if (tmp->title_w.win)
    {
      XShapeCombineShape(dpy, tmp->frame, ShapeBounding,
			 tmp->title_x + tmp->frame_bw, tmp->title_y + tmp->frame_bw, tmp->title_w.win, ShapeBounding, ShapeUnion);
    }
  }
  else
  {
    /*
     * can optimize rectangular contents window
     */
    if (tmp->squeeze_info)
    {
      XRectangle newBounding[3];
      XRectangle newClip[3];
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
      newBounding[0].x = tmp->title_x - tmp->frame_bw3D;
      newBounding[0].y = tmp->title_y - tmp->frame_bw3D;
      newBounding[0].width = tmp->title_width + fbw2 + 2 * tmp->frame_bw3D;
      newBounding[0].height = tmp->title_height + tmp->frame_bw3D;

      if (Scr->BorderBevelWidth > 0 && newBounding[0].width < client_width)
      {
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
      newClip[0].x = tmp->title_x + tmp->frame_bw - tmp->frame_bw3D;
      newClip[0].y = 0;
      newClip[0].width = tmp->title_width + 2 * tmp->frame_bw3D;
      newClip[0].height = Scr->TitleHeight + tmp->frame_bw3D;

      if (count == 3)
      {
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

      XShapeCombineRectangles(dpy, tmp->frame, ShapeBounding, 0, 0, newBounding, count, ShapeSet, order);
      XShapeCombineRectangles(dpy, tmp->frame, ShapeClip, 0, 0, newClip, count, ShapeSet, order);
    }
    else
    {
      (void)XShapeCombineMask(dpy, tmp->frame, ShapeBounding, 0, 0, None, ShapeSet);
      (void)XShapeCombineMask(dpy, tmp->frame, ShapeClip, 0, 0, None, ShapeSet);
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


/*
  Local Variables:
  mode:c
  c-file-style:"GNU"
  c-file-offsets:((substatement-open 0)(brace-list-open 0)(c-hanging-comment-ender-p . nil)(c-hanging-comment-beginner-p . nil)(comment-start . "// ")(comment-end . "")(comment-column . 48))
  End:
*/
/* vim: sw=2
*/
