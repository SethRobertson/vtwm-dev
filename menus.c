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
 * $XConsortium: menus.c,v 1.186 91/07/17 13:58:00 dave Exp $
 *
 * twm menu code
 *
 * 17-Nov-87 Thomas E. LaStrange		File created
 *
 ***********************************************************************/

#include <X11/Xmu/CharSet.h>
#include <X11/Xos.h>
#ifndef NO_REGEX_SUPPORT
#include <sys/types.h>
#include <regex.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#ifdef NEED_PROCESS_H
#include <process.h>
#endif
#include <ctype.h>
#include "twm.h"
#include "gc.h"
#include "menus.h"
#include "resize.h"
#include "events.h"
#include "image_formats.h"
#include "util.h"
#include "parse.h"
#include "gram.h"
#include "screen.h"
#include "doors.h"
#include "desktop.h"
#include "add_window.h"
#ifndef NO_SOUND_SUPPORT
#include "sound.h"
#endif
#include "version.h"
#include "prototypes.h"

extern char *Action;
extern int Context;
extern int ConstrainedMoveTime;
extern TwmWindow *ButtonWindow, *Tmp_win;
extern XEvent Event, ButtonEvent;
extern char *InitFile;

int RootFunction = F_NOFUNCTION;
MenuRoot *ActiveMenu = NULL;	/* the active menu */
MenuItem *ActiveItem = NULL;	/* the active menu item */
int MoveFunction = F_NOFUNCTION;	/* or F_MOVE or F_FORCEMOVE */
int WindowMoved = FALSE;
int menuFromFrameOrWindowOrTitlebar = FALSE;

#ifndef NO_SOUND_SUPPORT
int createSoundFromFunction = FALSE;
int destroySoundFromFunction = FALSE;
#endif

static void BumpWindowColormap(TwmWindow * tmp, int inc);
static void DestroyMenu(MenuRoot * menu);
static void HideIconManager(TwmWindow * tmp_win);
static void MakeMenu(MenuRoot * mr);
static void SendDeleteWindowMessage(TwmWindow * tmp, Time timestamp);
static void SendSaveYourselfMessage(TwmWindow * tmp, Time timestamp);
static void WarpClass(int next, TwmWindow * t, char *class);
static void WarpToScreen(ScreenInfo * scr, int n, int inc);
static void WarpScreenToWindow(TwmWindow * t);
static Cursor NeedToDefer(MenuRoot * root);
static int MatchWinName(char *action, TwmWindow * t);

int ConstMove = FALSE;		/* constrained move variables */

/* for comparison against MoveDelta - djhjr - 9/5/98 */
static int MenuOrigX, MenuOrigY;

/* Globals used to keep track of whether the mouse has moved during
   a resize function. */
int ResizeOrigX;
int ResizeOrigY;

extern int origx, origy, origWidth, origHeight;

int MenuDepth = 0;		/* number of menus up */
static struct
{
  int x;
  int y;
} MenuOrigins[MAXMENUDEPTH];
static Cursor LastCursor;

static char *actionHack = "";

/*
 * context bitmaps for TwmWindows menu, f.showdesktop and f.showiconmgr
 */
static int have_twmwindows = -1;
static int have_showdesktop = -1;
static int have_showlist = -1;

static void WarpAlongRing(XButtonEvent * ev, int forward);
static void Paint3DEntry(MenuRoot * mr, MenuItem * mi, int exposure);
static void Identify(TwmWindow * t);
static void PaintNormalEntry(MenuRoot * mr, MenuItem * mi, int exposure);
static TwmWindow *next_by_class(int next, TwmWindow * t, char *class);
static int warp_if_warpunmapped(TwmWindow * w, int func);
static void setup_restart(Time time);
static int FindMenuOrFuncInBindings(int contexts, MenuRoot * mr, int func);
static int FindMenuOrFuncInWindows(TwmWindow * tmp_win, int contexts, MenuRoot * mr, int func);
static int FindMenuInMenus(MenuRoot * start, MenuRoot * sought);
static int FindFuncInMenus(MenuRoot * mr, int func);
static void HideIconMgr(IconMgr * ip);
static void ShowIconMgr(IconMgr * ip);
static int do_squeezetitle(int context, int func, TwmWindow * tmp_win, SqueezeInfo * squeeze);

#undef MAX
#define MAX(x,y) ((x)>(y)?(x):(y))

#define SHADOWWIDTH 5		/* in pixels */

#ifdef TWM_USE_SPACING
#define EDGE_OFFSET (Scr->MenuFont.ascent/2)
#else
#define EDGE_OFFSET 5
#endif

#define PULLDOWNMENU_OFFSET ((Scr->RightHandSidePulldownMenus)?\
	(JunkWidth - EDGE_OFFSET * 2 - Scr->pullW):\
	(JunkWidth >> 1))


/***********************************************************************
 *
 *  Procedure:
 *	InitMenus - initialize menu roots
 *
 ***********************************************************************
 */

void
InitMenus(void)
{
  int i, j, k;
  FuncKey *key, *tmp;

  for (i = 0; i < NumButtons + 1; i++)
    for (j = 0; j < NUM_CONTEXTS; j++)
      for (k = 0; k < MOD_SIZE; k++)
      {
	Scr->Mouse[MOUSELOC(i, j, k)].func = F_NOFUNCTION;
	Scr->Mouse[MOUSELOC(i, j, k)].item = NULL;
      }

  Scr->DefaultFunction.func = F_NOFUNCTION;
  Scr->WindowFunction.func = F_NOFUNCTION;

  if (FirstScreen)
  {
    for (key = Scr->FuncKeyRoot.next; key != NULL;)
    {
      free(key->name);
      tmp = key;
      key = key->next;
      free((char *)tmp);
    }
    Scr->FuncKeyRoot.next = NULL;
  }
}



/***********************************************************************
 *
 *  Procedure:
 *	AddFuncKey - add a function key to the list
 *
 *  Inputs:
 *	name	- the name of the key
 *	cont	- the context to look for the key press in
 *	mods	- modifier keys that need to be pressed
 *	func	- the function to perform
 *	win_name- the window name (if any)
 *	action	- the action string associated with the function (if any)
 *
 ***********************************************************************
 */

Bool
AddFuncKey(char *name, int cont, int mods, int func, char *win_name, char *action)
{
  FuncKey *tmp;
  KeySym keysym;
  KeyCode keycode;

  /*
   * Don't let a 0 keycode go through, since that means AnyKey to the
   * XGrabKey call in GrabKeys().
   */
  if ((keysym = XStringToKeysym(name)) == NoSymbol || (keycode = XKeysymToKeycode(dpy, keysym)) == 0)
  {
    return False;
  }

  /* see if there already is a key defined for this context */
  for (tmp = Scr->FuncKeyRoot.next; tmp != NULL; tmp = tmp->next)
  {
    if (tmp->keysym == keysym && tmp->cont == cont && tmp->mods == mods)
      break;
  }

  if (tmp == NULL)
  {
    tmp = (FuncKey *) malloc(sizeof(FuncKey));
    tmp->next = Scr->FuncKeyRoot.next;
    Scr->FuncKeyRoot.next = tmp;
  }

  tmp->name = name;
  tmp->keysym = keysym;
  tmp->keycode = keycode;
  tmp->cont = cont;
  tmp->mods = mods;
  tmp->func = func;
  tmp->win_name = win_name;
  tmp->action = action;

  return True;
}



int
CreateTitleButton(char *name, int func, char *action, MenuRoot * menuroot, Bool rightside, Bool append)
{
  TitleButton *tb = (TitleButton *) malloc(sizeof(TitleButton));

  if (!tb)
  {
    fprintf(stderr, "%s:  unable to allocate %lu bytes for title button\n", ProgramName, sizeof(TitleButton));
    return 0;
  }

  tb->next = NULL;
  tb->name = name;		/* note that we are not copying */

  tb->width = 0;		/* see InitTitlebarButtons */
  tb->height = 0;		/* ditto */
  tb->func = func;
  tb->action = action;
  tb->menuroot = menuroot;
  tb->rightside = rightside;
  if (rightside)
  {
    Scr->TBInfo.nright++;
  }
  else
  {
    Scr->TBInfo.nleft++;
  }

  /*
   * Cases for list:
   *
   *     1.  empty list, prepend left       put at head of list
   *     2.  append left, prepend right     put in between left and right
   *     3.  append right                   put at tail of list
   *
   * Do not refer to widths and heights yet since buttons not created
   * (since fonts not loaded and heights not known).
   */
  if ((!Scr->TBInfo.head) || ((!append) && (!rightside)))
  {				/* 1 */
    tb->next = Scr->TBInfo.head;
    Scr->TBInfo.head = tb;
  }
  else if (append && rightside)
  {				/* 3 */
    register TitleButton *t;

    for (t = Scr->TBInfo.head; t->next; t = t->next)
      ;				/* SUPPRESS 530 */
    t->next = tb;
    tb->next = NULL;
  }
  else
  {				/* 2 */
    register TitleButton *t, *prev = NULL;

    for (t = Scr->TBInfo.head; t && !t->rightside; t = t->next)
      prev = t;
    if (prev)
    {
      tb->next = prev->next;
      prev->next = tb;
    }
    else
    {
      tb->next = Scr->TBInfo.head;
      Scr->TBInfo.head = tb;
    }
  }

  return 1;
}


/*
 * InitTitlebarButtons - Do all the necessary stuff to load in a titlebar
 * button.  If we can't find the button, then put in a question; if we can't
 * find the question mark, something is wrong and we are probably going to be
 * in trouble later on.
 */
int
InitTitlebarButtons(void)
{
  Image *image;
  TitleButton *tb;
  int h, height;

  /*
   * initialize dimensions
   */
  Scr->TBInfo.width = (Scr->TitleHeight - 2 * (Scr->FramePadding + Scr->ButtonIndent));

  Scr->TBInfo.pad = Scr->TitlePadding;

  h = Scr->TBInfo.width - 2 * Scr->TBInfo.border;
  if (!(h & 1))
    h--;
  height = h;

  /*
   * add in some useful buttons and bindings so that novices can still
   * use the system. -- modified by DSE
   */

  if (!Scr->NoDefaultTitleButtons)
  {
    /* insert extra buttons */

    if (Scr->TitleBevelWidth > 0)
    {
      if (!CreateTitleButton(TBPM_3DDOT, F_ICONIFY, "", (MenuRoot *) NULL, False, False))
	fprintf(stderr, "%s:  unable to add iconify button\n", ProgramName);
      if (!CreateTitleButton(TBPM_3DRESIZE, F_RESIZE, "", (MenuRoot *) NULL, True, True))
	fprintf(stderr, "%s:  unable to add resize button\n", ProgramName);
    }
    else
    {

      if (!CreateTitleButton(TBPM_ICONIFY, F_ICONIFY, "", (MenuRoot *) NULL, False, False))
	fprintf(stderr, "%s:  unable to add iconify button\n", ProgramName);
      if (!CreateTitleButton(TBPM_RESIZE, F_RESIZE, "", (MenuRoot *) NULL, True, True))
	fprintf(stderr, "%s:  unable to add resize button\n", ProgramName);
    }
  }
  if (!Scr->NoDefaultMouseOrKeyboardBindings)
  {
    AddDefaultBindings();
  }

  ComputeCommonTitleOffsets();

  /*
   * load in images and do appropriate centering
   */
  for (tb = Scr->TBInfo.head; tb; tb = tb->next)
  {

    image = GetImage(tb->name, h, h, Scr->ButtonBevelWidth * 2, (Scr->ButtonColorIsFrame) ? Scr->BorderColorC : Scr->TitleC);

    tb->width = image->width;

    height = tb->height = image->height;

    tb->dstx = (h - tb->width + 1) / 2;
    if (tb->dstx < 0)
    {				/* clip to minimize copying */
      tb->srcx = -(tb->dstx);
      tb->width = h;
      tb->dstx = 0;
    }
    else
    {
      tb->srcx = 0;
    }
    tb->dsty = (h - tb->height + 1) / 2;
    if (tb->dsty < 0)
    {
      tb->srcy = -(tb->dsty);
      tb->height = h;
      tb->dsty = 0;
    }
    else
    {
      tb->srcy = 0;
    }

  }				/* for(...) */

  return (height > h) ? height : h;

/* ...end of moved */
}


void
SetMenuIconPixmap(char *filename)
{
  Scr->menuIconName = filename;
}

void
PaintEntry(MenuRoot * mr, MenuItem * mi, int exposure)
{
  if (Scr->MenuBevelWidth > 0)
    Paint3DEntry(mr, mi, exposure);

  else

    PaintNormalEntry(mr, mi, exposure);
}

void
Paint3DEntry(MenuRoot * mr, MenuItem * mi, int exposure)
{
  int y_offset;
  int text_y;
  GC gc;

  y_offset = (mi->item_num - mr->top) * Scr->EntryHeight + Scr->MenuBevelWidth;

#ifdef TWM_USE_SPACING
  if (mi->func == F_TITLE)
    text_y = y_offset + (((Scr->EntryHeight - Scr->MenuTitleFont.height) / 2) + Scr->MenuTitleFont.y);
  else
#endif
    text_y = y_offset + (((Scr->EntryHeight - Scr->MenuFont.height) / 2) + Scr->MenuFont.y);

  if (mi->func != F_TITLE)
  {
    int x, y;

    if (mi->state)
    {

      Draw3DBorder(mr->w.win, Scr->MenuBevelWidth, y_offset + 1, mr->width - 2 * Scr->MenuBevelWidth, Scr->EntryHeight - 1, 1,
		   mi->highlight, off, True, False);

      MyFont_DrawString(dpy, &mr->w, &Scr->MenuFont, &mi->highlight, mi->x + Scr->MenuBevelWidth, text_y, mi->item, mi->strlen);

#ifdef TWM_USE_XFT
      /*
       * initialise NormalGC with color for XCopyPlane() later
       * ("non-TWM_USE_XFT" does it in MyFont_DrawString() already):
       */
      if (Scr->use_xft > 0)
	FB(Scr, mi->highlight.fore, mi->highlight.back);
#endif
      gc = Scr->NormalGC;
    }
    else
    {
      if (mi->user_colors || !exposure)
      {
	XSetForeground(dpy, Scr->NormalGC, mi->normal.back);

	XFillRectangle(dpy, mr->w.win, Scr->NormalGC, Scr->MenuBevelWidth, y_offset + 1,
		       mr->width - 2 * Scr->MenuBevelWidth, Scr->EntryHeight - 1);
#ifdef TWM_USE_XFT
	if (Scr->use_xft > 0)
	  FB(Scr, mi->normal.fore, mi->normal.back);
#endif
	gc = Scr->NormalGC;
      }
      else
      {
	gc = Scr->MenuGC;
      }

      /* MyFont_DrawString() sets NormalGC: */
      MyFont_DrawString(dpy, &mr->w, &Scr->MenuFont, &mi->normal, mi->x + Scr->MenuBevelWidth, text_y, mi->item, mi->strlen);

      if (mi->separated)
      {
	if (!Scr->BeNiceToColormap)
	{
	  FB(Scr, Scr->MenuC.shadd, Scr->MenuC.shadc);

	  XDrawLine(dpy, mr->w.win, Scr->NormalGC, Scr->MenuBevelWidth + 1, y_offset + Scr->EntryHeight - 1,
		    mr->width - Scr->MenuBevelWidth - 3, y_offset + Scr->EntryHeight - 1);
	}

	FB(Scr, Scr->MenuC.shadc, Scr->MenuC.shadd);

	XDrawLine(dpy, mr->w.win, Scr->NormalGC, Scr->MenuBevelWidth + 2, y_offset + Scr->EntryHeight,
		  mr->width - Scr->MenuBevelWidth - 2, y_offset + Scr->EntryHeight);
      }
    }

    if (mi->func == F_MENU)
    {
      Image *image;
      Pixel back;

      back = Scr->MenuC.back;
      if (mi->state)
	Scr->MenuC.back = mi->highlight.back;
      else
	Scr->MenuC.back = mi->normal.back;

      Scr->pullW = Scr->pullH = Scr->MenuFont.height;
      image = GetImage(Scr->menuIconName, Scr->pullW, Scr->pullH, 0, Scr->MenuC);

      Scr->MenuC.back = back;
      x = mr->width - Scr->pullW - Scr->MenuBevelWidth - EDGE_OFFSET;

      y = y_offset + ((Scr->EntryHeight - Scr->pullH) / 2) + 1;

      XCopyArea(dpy, image->pixmap, mr->w.win, gc, 0, 0, Scr->pullW, Scr->pullH, x, y);
    }
  }
  else
  {

    Draw3DBorder(mr->w.win, Scr->MenuBevelWidth, y_offset, mr->width - 2 * Scr->MenuBevelWidth, Scr->EntryHeight + 1, 1,
		 mi->normal, off, True, False);

    MyFont_DrawString(dpy, &mr->w, &Scr->MenuTitleFont, &mi->normal, mi->x, text_y, mi->item, mi->strlen);
  }
}



void
PaintNormalEntry(MenuRoot * mr, MenuItem * mi, int exposure)
{
  int y_offset;
  int text_y;
  GC gc;

  y_offset = (mi->item_num - mr->top) * Scr->EntryHeight;

#ifdef TWM_USE_SPACING
  if (mi->func == F_TITLE)
    text_y = y_offset + (((Scr->EntryHeight - Scr->MenuTitleFont.height) / 2) + Scr->MenuTitleFont.y);
  else
#endif
    text_y = y_offset + (((Scr->EntryHeight - Scr->MenuFont.height) / 2) + Scr->MenuFont.y);

  if (mi->func != F_TITLE)
  {
    int x, y;

    if (mi->state)
    {
      XSetForeground(dpy, Scr->NormalGC, mi->highlight.back);

      XFillRectangle(dpy, mr->w.win, Scr->NormalGC, 0, y_offset, mr->width, Scr->EntryHeight);

      MyFont_DrawString(dpy, &mr->w, &Scr->MenuFont, &mi->highlight, mi->x, text_y, mi->item, mi->strlen);

#ifdef TWM_USE_XFT
      /*
       * initialise NormalGC with color for XCopyPlane() later
       * ("non-TWM_USE_XFT" does it in MyFont_DrawString() already):
       */
      if (Scr->use_xft > 0)
	FB(Scr, mi->highlight.fore, mi->highlight.back);
#endif
      gc = Scr->NormalGC;
    }
    else
    {
      if (mi->user_colors || !exposure)
      {
	XSetForeground(dpy, Scr->NormalGC, mi->normal.back);

	XFillRectangle(dpy, mr->w.win, Scr->NormalGC, 0, y_offset, mr->width, Scr->EntryHeight);
#ifdef TWM_USE_XFT
	if (Scr->use_xft > 0)
	  FB(Scr, mi->normal.fore, mi->normal.back);
#endif
	gc = Scr->NormalGC;
      }
      else
      {
	gc = Scr->MenuGC;
      }

      /* MyFont_DrawString() sets NormalGC: */
      MyFont_DrawString(dpy, &mr->w, &Scr->MenuFont, &mi->normal, mi->x, text_y, mi->item, mi->strlen);

      if (mi->separated)

	XDrawLine(dpy, mr->w.win, gc, 0, y_offset + Scr->EntryHeight - 1, mr->width, y_offset + Scr->EntryHeight - 1);
    }

    if (mi->func == F_MENU)
    {
      Image *image;
      ColorPair cp;

      cp.fore = cp.back = Scr->MenuC.back;
      if (strncmp(Scr->menuIconName, ":xpm:", 5) != 0)
      {
	cp.fore = Scr->MenuC.fore;
	Scr->MenuC.fore = (mi->state) ? mi->highlight.fore : mi->normal.fore;
	Scr->MenuC.back = (mi->state) ? mi->highlight.back : mi->normal.back;
      }
      else
	Scr->MenuC.back = (mi->state) ? mi->highlight.back : mi->normal.back;

      Scr->pullW = Scr->pullH = Scr->MenuFont.height;
      image = GetImage(Scr->menuIconName, Scr->pullW, Scr->pullH, 0, Scr->MenuC);

      Scr->MenuC.back = cp.back;
      if (strncmp(Scr->menuIconName, ":xpm:", 5) != 0)
	Scr->MenuC.fore = cp.fore;

      x = mr->width - Scr->pullW - EDGE_OFFSET;

      y = y_offset + ((Scr->EntryHeight - Scr->pullH) / 2);

      XCopyArea(dpy, image->pixmap, mr->w.win, gc, 0, 0, Scr->pullW, Scr->pullH, x, y);
    }
  }
  else
  {
    int y;

    XSetForeground(dpy, Scr->NormalGC, mi->normal.back);

    /* fill the rectangle with the title background color */
    XFillRectangle(dpy, mr->w.win, Scr->NormalGC, 0, y_offset, mr->width, Scr->EntryHeight);

    /* Draw separators only if menu has borders: */
    if (Scr->BorderWidth > 0 || Scr->MenuBevelWidth > 0)
    {
      XSetForeground(dpy, Scr->NormalGC, mi->normal.fore);

      /* now draw the dividing lines */
      if (y_offset)
	XDrawLine(dpy, mr->w.win, Scr->NormalGC, 0, y_offset, mr->width, y_offset);

      y = ((mi->item_num + 1) * Scr->EntryHeight) - 1;
      XDrawLine(dpy, mr->w.win, Scr->NormalGC, 0, y, mr->width, y);
    }

    /* finally render the title */
    MyFont_DrawString(dpy, &mr->w, &Scr->MenuTitleFont, &mi->normal, mi->x, text_y, mi->item, mi->strlen);
  }
}

void
PaintMenu(MenuRoot * mr, XEvent * e)
{
  MenuItem *mi;
  int y_offset;

  if (Scr->MenuBevelWidth > 0)
  {
    Draw3DBorder(mr->w.win, 0, 0, mr->width, mr->height, Scr->MenuBevelWidth, Scr->MenuC, off, False, False);
  }

  for (mi = mr->first; mi != NULL; mi = mi->next)
  {
    if (mi->item_num < mr->top)
      continue;

    y_offset = (mi->item_num - mr->top) * Scr->EntryHeight;

    if (y_offset + Scr->EntryHeight > mr->height)
      break;

    /* some servers want the previous entry redrawn - djhjr - 10/24/00 */
    if (Scr->MenuBevelWidth > 0)
      y_offset += Scr->EntryHeight;

    /*
     * Be smart about handling the expose, redraw only the entries
     * that we need to.
     */
    /* those servers want the next entry redrawn, too - djhjr - 10/24/00 */
    if (e->xexpose.y < (y_offset + Scr->EntryHeight) &&
	(e->xexpose.y + e->xexpose.height) > y_offset - ((mr->shadow) ? Scr->EntryHeight : 0))
    {
#ifdef TWM_USE_XFT
      if (Scr->use_xft > 0)
	PaintEntry(mr, mi, False);	/* enforce clear area first */
      else
#endif
	PaintEntry(mr, mi, True);
    }
  }
  XSync(dpy, 0);
}


static Bool fromMenu;

extern int GlobalFirstTime;

void
UpdateMenu(void)
{
  MenuItem *mi;
  int i, x, y, x_root, y_root, entry;
  int done;
  MenuItem *badItem = NULL;
  static int firstTime = True;

  fromMenu = TRUE;

  while (TRUE)
  {				/* block until there is an event */
    XNextEvent(dpy, &Event);

    if (!DispatchEvent())
      continue;

    if (Event.type == ButtonRelease)
    {
      if (Scr->StayUpMenus)
      {
	if (firstTime == True)
	{			/* it was the first release of the button */
	  firstTime = False;
	}
	else
	{			/* thats the second we need to return now */
	  firstTime = True;
	  menuFromFrameOrWindowOrTitlebar = FALSE;
	  fromMenu = FALSE;
	  return;
	}
      }
      else
      {				/* not stay-up */
	menuFromFrameOrWindowOrTitlebar = FALSE;
	fromMenu = FALSE;
	return;
      }
    }

    if (Cancel)
      return;

    if (Event.type != MotionNotify)
      continue;

    /* if we haven't received the enter notify yet, wait */
    if (!ActiveMenu || !ActiveMenu->entered)
      continue;

    done = FALSE;
    XQueryPointer(dpy, ActiveMenu->w.win, &JunkRoot, &JunkChild, &x_root, &y_root, &x, &y, &JunkMask);

    if (!ActiveItem)
      if (abs(x_root - MenuOrigX) < Scr->MoveDelta && abs(y_root - MenuOrigY) < Scr->MoveDelta)
	continue;


    XFindContext(dpy, ActiveMenu->w.win, ScreenContext, (caddr_t *) & Scr);

    JunkWidth = ActiveMenu->width;
    JunkHeight = ActiveMenu->height;
    if (Scr->MenuBevelWidth > 0)
    {
      x -= Scr->MenuBevelWidth;
      y -= Scr->MenuBevelWidth;

      JunkWidth -= 2 * Scr->MenuBevelWidth;
      JunkHeight -= Scr->MenuBevelWidth;
    }

    if ((x < 0 || y < 0 || x >= JunkWidth || y >= JunkHeight) ||
	(ActiveMenu->too_tall && (y < Scr->MenuScrollBorderWidth || y > JunkHeight - Scr->MenuScrollBorderWidth)))
    {
      if (ActiveItem && ActiveItem->func != F_TITLE)
      {
	ActiveItem->state = 0;
	PaintEntry(ActiveMenu, ActiveItem, False);
      }
      ActiveItem = NULL;

      /* menu scrolling - djhjr - 5/22/00 */
      if (ActiveMenu->too_tall && x >= 0 && x < JunkWidth)
      {
	short j = ActiveMenu->top;

	if (y < Scr->MenuScrollBorderWidth)
	{
	  if (ActiveMenu->top - Scr->MenuScrollJump < 0)
	    continue;
	  else
	    j -= Scr->MenuScrollJump;
	}
	else if (y > JunkHeight - Scr->MenuScrollBorderWidth)
	{
	  int k = JunkHeight / Scr->EntryHeight;

	  if (ActiveMenu->top + k >= ActiveMenu->items)
	    continue;
	  else
	    j += Scr->MenuScrollJump;
	}

	if (ActiveMenu->top != j)
	{
	  ActiveMenu->top = j;
	  XClearArea(dpy, ActiveMenu->w.win, 0, 0, 0, 0, True);
	}
      }

      continue;
    }

    /* look for the entry that the mouse is in */
    entry = (y / Scr->EntryHeight) + ActiveMenu->top;
    for (i = 0, mi = ActiveMenu->first; mi != NULL; i++, mi = mi->next)
    {
      if (i == entry)
	break;
    }

    /* if there is an active item, we might have to turn it off */
    if (ActiveItem)
    {
      /* is the active item the one we are on ? */
      if (ActiveItem->item_num == entry && ActiveItem->state)
	done = TRUE;

      /* if we weren't on the active entry, let's turn the old
       * active one off
       */
      if (!done && ActiveItem->func != F_TITLE)
      {
	ActiveItem->state = 0;
	PaintEntry(ActiveMenu, ActiveItem, False);
      }
    }

    if (ActiveMenu->too_tall && y + Scr->EntryHeight > JunkHeight)
      continue;

    /* if we weren't on the active item, change the active item and turn
     * it on
     */
    if (!done)
    {
      ActiveItem = mi;

      if (ActiveItem && ActiveItem->func != F_TITLE && !ActiveItem->state)
      {
	ActiveItem->state = 1;
	PaintEntry(ActiveMenu, ActiveItem, False);

	if (Scr->StayUpOptionalMenus)
	  GlobalFirstTime = firstTime = False;

      }
    }

    /* now check to see if we were over the arrow of a pull right entry */

    if (ActiveItem && ActiveItem->func == F_MENU && (x > PULLDOWNMENU_OFFSET))
    {
      MenuRoot *save = ActiveMenu;
      int savex = MenuOrigins[MenuDepth - 1].x;
      int savey = MenuOrigins[MenuDepth - 1].y;

      if (MenuDepth < MAXMENUDEPTH)
      {
	PopUpMenu(ActiveItem->sub, (savex + PULLDOWNMENU_OFFSET), (savey + ActiveItem->item_num * Scr->EntryHeight), False);
      }
      else if (!badItem)
      {
	DoAudible();
	badItem = ActiveItem;
      }

      /* if the menu did get popped up, unhighlight the active item */
      if (save != ActiveMenu && ActiveItem->state)
      {
	ActiveItem->state = 0;
	PaintEntry(save, ActiveItem, False);
	ActiveItem = NULL;
      }
    }

    if (badItem != ActiveItem)
      badItem = NULL;
    XFlush(dpy);
  }
}


/***********************************************************************
 *
 *	Procedure:
 *	NewMenuRoot - create a new menu root
 *
 *	Returned Value:
 *	(MenuRoot *)
 *
 *	Inputs:
 *	name	- the name of the menu root
 *
 ***********************************************************************
 */

MenuRoot *
NewMenuRoot(char *name)
{
  MenuRoot *tmp;

#define UNUSED_PIXEL ((unsigned long) (~0))	/* more than 24 bits */

  tmp = (MenuRoot *) malloc(sizeof(MenuRoot));

  tmp->highlight.fore = UNUSED_PIXEL;
  tmp->highlight.back = UNUSED_PIXEL;

  tmp->name = name;
  tmp->prev = NULL;
  tmp->first = NULL;
  tmp->last = NULL;
  tmp->items = 0;
  tmp->width = 0;
  tmp->mapped = NEVER_MAPPED;
  tmp->pull = FALSE;
  tmp->w.win = None;
  tmp->shadow = None;
  tmp->real_menu = FALSE;

  tmp->too_tall = 0;
  tmp->top = 0;

  if (Scr->MenuList == NULL)
  {
    Scr->MenuList = tmp;
    Scr->MenuList->next = NULL;
  }

  if (Scr->LastMenu == NULL)
  {
    Scr->LastMenu = tmp;
    Scr->LastMenu->next = NULL;
  }
  else
  {
    Scr->LastMenu->next = tmp;
    Scr->LastMenu = tmp;
    Scr->LastMenu->next = NULL;
  }

  if (strcmp(name, TWM_WINDOWS) == 0 || strcmp(name, VTWM_WINDOWS) == 0)
    Scr->Windows = tmp;

  return (tmp);
}


/***********************************************************************
 *
 *	Procedure:
 *	AddToMenu - add an item to a root menu
 *
 *	Returned Value:
 *	(MenuItem *)
 *
 *	Inputs:
 *	menu	- pointer to the root menu to add the item
 *	item	- the text to appear in the menu
 *	action	- the string to possibly execute
 *	sub	- the menu root if it is a pull-right entry
 *	func	- the numeric function
 *	fore	- foreground color string
 *	back	- background color string
 *
 ***********************************************************************
 */

MenuItem *
AddToMenu(MenuRoot * menu, char *item, char *action, MenuRoot * sub, int func, char *fore, char *back)
{
  MenuItem *tmp;
  int width;

#ifdef DEBUG_MENUS
  fprintf(stderr, "adding menu item=\"%s\", action=%s, sub=%d, f=%d\n", item, action, sub, func);
#endif

  tmp = (MenuItem *) malloc(sizeof(MenuItem));
  tmp->root = menu;

  if (menu->first == NULL)
  {
    menu->first = tmp;
    tmp->prev = NULL;
  }
  else
  {
    menu->last->next = tmp;
    tmp->prev = menu->last;
  }
  menu->last = tmp;

  tmp->item = item;
  tmp->strlen = strlen(item);
  tmp->action = action;
  tmp->next = NULL;
  tmp->sub = NULL;
  tmp->state = 0;
  tmp->func = func;

  tmp->separated = 0;

  if (!Scr->HaveFonts)
    CreateFonts();

  if (func == F_TITLE)
    width = MyFont_TextWidth(&Scr->MenuTitleFont, item, tmp->strlen);
  else
    width = MyFont_TextWidth(&Scr->MenuFont, item, tmp->strlen);

  if (width <= 0)
    width = 1;
  if (width > menu->width)
    menu->width = width;

  tmp->user_colors = FALSE;
  if (Scr->Monochrome == COLOR && fore != NULL)
  {
    int save;

    save = Scr->FirstTime;
    Scr->FirstTime = TRUE;

    GetColor(COLOR, &tmp->normal.fore, fore);
    GetColor(COLOR, &tmp->normal.back, back);

    if (!Scr->BeNiceToColormap)
      GetShadeColors(&tmp->normal);

    Scr->FirstTime = save;
    tmp->user_colors = TRUE;
  }
  if (sub != NULL)
  {
    tmp->sub = sub;
    menu->pull = TRUE;
  }
  tmp->item_num = menu->items++;

  return (tmp);
}


void
MakeMenus(void)
{
  MenuRoot *mr;

  for (mr = Scr->MenuList; mr != NULL; mr = mr->next)
  {
    if (mr->real_menu == FALSE)
      continue;

    MakeMenu(mr);
  }
}


static void
MakeMenu(MenuRoot * mr)
{
  MenuItem *start, *end, *cur, *tmp;
  XColor f1, f2, f3;
  XColor b1, b2, b3;
  XColor save_fore, save_back;
  int num, i;
  int fred, fgreen, fblue;
  int bred, bgreen, bblue;
  int width;

  int borderwidth;

  unsigned long valuemask;
  XSetWindowAttributes attributes;
  Colormap cmap = Scr->TwmRoot.cmaps.cwins[0]->colormap->c;

#ifdef TWM_USE_SPACING
  Scr->EntryHeight = 120 * Scr->MenuFont.height / 100;	/*baselineskip 1.2 */
#else
  Scr->EntryHeight = MAX(Scr->MenuFont.height, Scr->MenuTitleFont.height) + 4;
#endif

  /* lets first size the window accordingly */
  if (mr->mapped == NEVER_MAPPED)
  {
    if (mr->pull == TRUE)
    {
      mr->width += 16 + 2 * EDGE_OFFSET;
    }

    if (Scr->MenuBevelWidth > 0)
      mr->width += 2 * Scr->MenuBevelWidth;

    width = mr->width + 2 * EDGE_OFFSET;

    for (cur = mr->first; cur != NULL; cur = cur->next)
    {
      if (cur->func != F_TITLE)
	cur->x = EDGE_OFFSET;
      else
      {
	cur->x = width - MyFont_TextWidth(&Scr->MenuTitleFont, cur->item, cur->strlen);
	cur->x /= 2;
      }
    }
    mr->height = mr->items * Scr->EntryHeight;

    if (Scr->MenuBevelWidth > 0)
      mr->height += 2 * Scr->MenuBevelWidth;

    borderwidth = (Scr->MenuBevelWidth > 0) ? 0 : Scr->BorderWidth;

    if (mr->height > Scr->MyDisplayHeight)
    {
      mr->too_tall = 1;
      mr->height = Scr->MyDisplayHeight - borderwidth * 2;
    }

#ifdef TWM_USE_SPACING
    mr->width += 2 * (Scr->MenuBevelWidth + EDGE_OFFSET);
#else
    if (Scr->MenuBevelWidth > 0)
      mr->width += 2 * Scr->MenuBevelWidth + 6;
    else
      mr->width += 10;
#endif

    if (Scr->Shadow)
    {
      /*
       * Make sure that you don't draw into the shadow window or else
       * the background bits there will get saved
       */
      valuemask = (CWBackPixel | CWBorderPixel);
      attributes.background_pixel = Scr->MenuShadowColor;
      attributes.border_pixel = Scr->MenuShadowColor;
      if (Scr->SaveUnder)
      {
	valuemask |= CWSaveUnder;
	attributes.save_under = True;
      }
      mr->shadow = XCreateWindow(dpy, Scr->Root, 0, 0,
				 (unsigned int)mr->width,
				 (unsigned int)mr->height,
				 (unsigned int)0,
				 CopyFromParent, (unsigned int)CopyFromParent, (Visual *) CopyFromParent, valuemask, &attributes);
    }

    valuemask = (CWBackPixel | CWBorderPixel | CWEventMask);
    attributes.background_pixel = Scr->MenuC.back;
    attributes.border_pixel = Scr->MenuC.fore;
    attributes.event_mask = (ExposureMask | EnterWindowMask);
    if (Scr->SaveUnder)
    {
      valuemask |= CWSaveUnder;
      attributes.save_under = True;
    }
    if (Scr->BackingStore)
    {
      valuemask |= CWBackingStore;
      attributes.backing_store = Always;
    }

    mr->w.win = XCreateWindow(dpy, Scr->Root, 0, 0, (unsigned int)mr->width,
			      (unsigned int)mr->height, (unsigned int)borderwidth,
			      CopyFromParent, (unsigned int)CopyFromParent, (Visual *) CopyFromParent, valuemask, &attributes);

#ifdef TWM_USE_XFT
    if (Scr->use_xft > 0)
      mr->w.xft = MyXftDrawCreate(mr->w.win);
#endif
#ifdef TWM_USE_OPACITY
    SetWindowOpacity(mr->w.win, Scr->MenuOpacity);
#endif

    XSaveContext(dpy, mr->w.win, MenuContext, (caddr_t) mr);
    XSaveContext(dpy, mr->w.win, ScreenContext, (caddr_t) Scr);

    mr->mapped = UNMAPPED;
  }

  if (Scr->MenuBevelWidth > 0 && (Scr->Monochrome == COLOR) && (mr->highlight.back == UNUSED_PIXEL))
  {
    XColor xcol;
    char colname[32];
    short save;

    xcol.pixel = Scr->MenuC.back;
    XQueryColor(dpy, cmap, &xcol);
    sprintf(colname, "#%04x%04x%04x", 5 * (xcol.red / 6), 5 * (xcol.green / 6), 5 * (xcol.blue / 6));
    save = Scr->FirstTime;
    Scr->FirstTime = True;
    GetColor(Scr->Monochrome, &mr->highlight.back, colname);
    Scr->FirstTime = save;
  }

  if (Scr->MenuBevelWidth > 0 && (Scr->Monochrome == COLOR) && (mr->highlight.fore == UNUSED_PIXEL))
  {
    XColor xcol;
    char colname[32];
    short save;

    xcol.pixel = Scr->MenuC.fore;
    XQueryColor(dpy, cmap, &xcol);
    sprintf(colname, "#%04x%04x%04x", 5 * (xcol.red / 6), 5 * (xcol.green / 6), 5 * (xcol.blue / 6));
    save = Scr->FirstTime;
    Scr->FirstTime = True;
    GetColor(Scr->Monochrome, &mr->highlight.fore, colname);
    Scr->FirstTime = save;
  }
  if (Scr->MenuBevelWidth > 0 && !Scr->BeNiceToColormap)
    GetShadeColors(&mr->highlight);

  /* get the default colors into the menus */
  for (tmp = mr->first; tmp != NULL; tmp = tmp->next)
  {
    if (!tmp->user_colors)
    {
      if (tmp->func != F_TITLE)
      {
	tmp->normal.fore = Scr->MenuC.fore;
	tmp->normal.back = Scr->MenuC.back;
      }
      else
      {
	tmp->normal.fore = Scr->MenuTitleC.fore;
	tmp->normal.back = Scr->MenuTitleC.back;
      }
    }

    if (mr->highlight.fore != UNUSED_PIXEL)
    {
      tmp->highlight.fore = mr->highlight.fore;
      tmp->highlight.back = mr->highlight.back;
    }
    else
    {
      tmp->highlight.fore = tmp->normal.back;
      tmp->highlight.back = tmp->normal.fore;
    }
    if (Scr->MenuBevelWidth > 0 && !Scr->BeNiceToColormap)
    {
      if (tmp->func != F_TITLE)
	GetShadeColors(&tmp->highlight);
      else
	GetShadeColors(&tmp->normal);
    }

#ifdef TWM_USE_XFT
    if (Scr->use_xft > 0)
    {
      CopyPixelToXftColor(tmp->normal.fore, &tmp->normal.xft);
      CopyPixelToXftColor(tmp->highlight.fore, &tmp->highlight.xft);
    }
#endif

  }				/* end for(...) */

  if (Scr->Monochrome == MONOCHROME || !Scr->InterpolateMenuColors)
    return;

  start = mr->first;
  while (TRUE)
  {
    for (; start != NULL; start = start->next)
    {
      if (start->user_colors)
	break;
    }
    if (start == NULL)
      break;

    for (end = start->next; end != NULL; end = end->next)
    {
      if (end->user_colors)
	break;
    }
    if (end == NULL)
      break;

    /* we have a start and end to interpolate between */
    num = end->item_num - start->item_num;

    f1.pixel = start->normal.fore;
    XQueryColor(dpy, cmap, &f1);
    f2.pixel = end->normal.fore;
    XQueryColor(dpy, cmap, &f2);
    b1.pixel = start->normal.back;
    XQueryColor(dpy, cmap, &b1);
    b2.pixel = end->normal.back;
    XQueryColor(dpy, cmap, &b2);

    fred = ((int)f2.red - (int)f1.red) / num;
    fgreen = ((int)f2.green - (int)f1.green) / num;
    fblue = ((int)f2.blue - (int)f1.blue) / num;

    bred = ((int)b2.red - (int)b1.red) / num;
    bgreen = ((int)b2.green - (int)b1.green) / num;
    bblue = ((int)b2.blue - (int)b1.blue) / num;

    f3 = f1;
    f3.flags = DoRed | DoGreen | DoBlue;

    b3 = b1;
    b3.flags = DoRed | DoGreen | DoBlue;

    start->highlight.back = start->normal.fore;
    start->highlight.fore = start->normal.back;

    num -= 1;
    for (i = 0, cur = start->next; i < num; i++, cur = cur->next)
    {
      f3.red += fred;
      f3.green += fgreen;
      f3.blue += fblue;
      save_fore = f3;

      b3.red += bred;
      b3.green += bgreen;
      b3.blue += bblue;
      save_back = b3;

      if (Scr->DontInterpolateTitles && (cur->func == F_TITLE))
	continue;

      XAllocColor(dpy, cmap, &f3);
      XAllocColor(dpy, cmap, &b3);

      cur->highlight.back = cur->normal.fore = f3.pixel;
      cur->highlight.fore = cur->normal.back = b3.pixel;
      cur->user_colors = True;

      f3 = save_fore;
      b3 = save_back;
#ifdef TWM_USE_XFT
      if (Scr->use_xft > 0)
      {
	CopyPixelToXftColor(cur->normal.fore, &cur->normal.xft);
	CopyPixelToXftColor(cur->highlight.fore, &cur->highlight.xft);
      }
#endif
    }
    start = end;

  }
}



/***********************************************************************
 *
 *	Procedure:
 *	PopUpMenu - pop up a pull down menu
 *
 *	Inputs:
 *	menu	- the root pointer of the menu to pop up
 *	x, y	- location of upper left of menu
 *		center	- whether or not to center horizontally over position
 *
 ***********************************************************************
 */

Bool
PopUpMenu(MenuRoot * menu, int x, int y, Bool center)
{
  int WindowNameOffset, WindowNameCount;
  TwmWindow **WindowNames;
  TwmWindow *tmp_win2, *tmp_win3;
  int mask;
  int i;
  int (*compar) (const char *a, const char *b) = (Scr->CaseSensitive ? strcmp : XmuCompareISOLatin1);

  int x_root, y_root;

  if (!menu)
    return False;

#ifndef NO_SOUND_SUPPORT
  if (!PlaySound(F_MENU))
    PlaySound(S_MMAP);
#endif

  menu->top = 0;
  if (menu->w.win)
    XClearArea(dpy, menu->w.win, 0, 0, 0, 0, True);

  InstallRootColormap();

  if (menu == Scr->Windows)
  {
    TwmWindow *tmp_win;

    /* this is the twm windows menu,  let's go ahead and build it */

    DestroyMenu(menu);

    menu->first = NULL;
    menu->last = NULL;
    menu->items = 0;
    menu->width = 0;
    menu->mapped = NEVER_MAPPED;

    AddToMenu(menu, "VTWM Windows", NULLSTR, (MenuRoot *) NULL, F_TITLE, NULLSTR, NULLSTR);

    WindowNameOffset = (char *)Scr->TwmRoot.next->name - (char *)Scr->TwmRoot.next;
    for (tmp_win = Scr->TwmRoot.next, WindowNameCount = 0; tmp_win != NULL; tmp_win = tmp_win->next)
      WindowNameCount++;

    if (WindowNameCount != 0)
    {
      WindowNames = (TwmWindow **) malloc(sizeof(TwmWindow *) * WindowNameCount);
      WindowNames[0] = Scr->TwmRoot.next;
      for (tmp_win = Scr->TwmRoot.next->next, WindowNameCount = 1; tmp_win != NULL; tmp_win = tmp_win->next, WindowNameCount++)
      {
	if (LookInList(Scr->DontShowInTWMWindows, tmp_win->full_name, &tmp_win->class))
	{
	  WindowNameCount--;
	  continue;
	}

	tmp_win2 = tmp_win;
	for (i = 0; i < WindowNameCount; i++)
	{
	  if ((*compar) (tmp_win2->name, WindowNames[i]->name) < 0)
	  {
	    tmp_win3 = tmp_win2;
	    tmp_win2 = WindowNames[i];
	    WindowNames[i] = tmp_win3;
	  }
	}
	WindowNames[WindowNameCount] = tmp_win2;
      }
      for (i = 0; i < WindowNameCount; i++)
      {
	AddToMenu(menu, WindowNames[i]->name, (char *)WindowNames[i], (MenuRoot *) NULL, F_POPUP, NULLSTR, NULLSTR);
	if (!Scr->OldFashionedTwmWindowsMenu && Scr->Monochrome == COLOR)
	{
	  menu->last->user_colors = TRUE;

	  menu->last->normal.fore = Scr->MenuC.fore;

	  menu->last->normal.back = WindowNames[i]->virtual.back;

/**********************************************************/

/*														  */

/*	Okay, okay, it's a bit of a kludge.					  */

/*														  */

/*	On the other hand, it's nice to have the VTWM Windows */

/*	menu come up with "the right colors". And the colors  */

/*	from the panner are not a bad choice...				  */

/*														  */

/**********************************************************/
	}
      }
      free(WindowNames);
    }
    MakeMenu(menu);
  }

  if (menu->w.win == None || menu->items == 0)
    return False;

  /* Prevent recursively bringing up menus. */
  if (menu->mapped == MAPPED)
    return False;

  /*
   * Dynamically set the parent;  this allows pull-ups to also be main
   * menus, or to be brought up from more than one place.
   */
  menu->prev = ActiveMenu;

  mask = ButtonPressMask | ButtonReleaseMask | ButtonMotionMask | PointerMotionHintMask;
  if (Scr->StayUpMenus)
    mask |= PointerMotionMask;

  XGrabPointer(dpy, Scr->Root, True, mask, GrabModeAsync, GrabModeAsync, Scr->Root, Scr->MenuCursor, CurrentTime);

  ActiveMenu = menu;
  menu->mapped = MAPPED;
  menu->entered = FALSE;

  if (center)
  {
    x -= (menu->width / 2);
    y -= (Scr->EntryHeight / 2);	/* sticky menus would be nice here */
  }

  /*
   * clip to screen
   */
  i = (Scr->MenuBevelWidth > 0) ? 0 : 2 * Scr->BorderWidth;
#ifdef TILED_SCREEN
  if (Scr->use_tiles == TRUE)
  {
    int k = FindNearestTileToMouse();

    EnsureRectangleOnTile(k, &x, &y, menu->width + i, menu->height + i);
  }
  else
#endif
  {
    if (x + menu->width + i > Scr->MyDisplayWidth)
      x = Scr->MyDisplayWidth - menu->width - i;
    if (x < 0)
      x = 0;
    if (y + menu->height + i > Scr->MyDisplayHeight)
      y = Scr->MyDisplayHeight - menu->height - i;
    if (y < 0)
      y = 0;
  }

  MenuOrigins[MenuDepth].x = x;
  MenuOrigins[MenuDepth].y = y;
  MenuDepth++;

  XMoveWindow(dpy, menu->w.win, x, y);
  if (Scr->Shadow)
  {
    XMoveWindow(dpy, menu->shadow, x + SHADOWWIDTH, y + SHADOWWIDTH);
  }
  if (Scr->Shadow)
  {
    XRaiseWindow(dpy, menu->shadow);
  }
  XMapRaised(dpy, menu->w.win);
  if (Scr->Shadow)
  {
    XMapWindow(dpy, menu->shadow);
  }
  XSync(dpy, 0);

  XQueryPointer(dpy, menu->w.win, &JunkRoot, &JunkChild, &x_root, &y_root, &JunkX, &JunkY, &JunkMask);
  MenuOrigX = x_root;
  MenuOrigY = y_root;

  return True;
}



/***********************************************************************
 *
 *	Procedure:
 *	PopDownMenu - unhighlight the current menu selection and
 *		take down the menus
 *
 ***********************************************************************
 */

void
PopDownMenu(void)
{
  MenuRoot *tmp;

  if (ActiveMenu == NULL)
    return;

#ifndef NO_SOUND_SUPPORT
  PlaySound(S_MUNMAP);
#endif

  if (ActiveItem)
  {
    ActiveItem->state = 0;
    PaintEntry(ActiveMenu, ActiveItem, False);
  }

  for (tmp = ActiveMenu; tmp != NULL; tmp = tmp->prev)
  {
    if (Scr->Shadow)
    {
      XUnmapWindow(dpy, tmp->shadow);
    }
    XUnmapWindow(dpy, tmp->w.win);
    tmp->mapped = UNMAPPED;
    UninstallRootColormap();
  }

  XFlush(dpy);
  ActiveMenu = NULL;
  ActiveItem = NULL;
  MenuDepth = 0;
  if (Context == C_WINDOW || Context == C_FRAME || Context == C_TITLE)
  {
    menuFromFrameOrWindowOrTitlebar = TRUE;
  }
}



/***********************************************************************
 *
 *	Procedure:
 *	FindMenuRoot - look for a menu root
 *
 *	Returned Value:
 *	(MenuRoot *)  - a pointer to the menu root structure
 *
 *	Inputs:
 *	name	- the name of the menu root
 *
 ***********************************************************************
 */

MenuRoot *
FindMenuRoot(char *name)
{
  MenuRoot *tmp;

  for (tmp = Scr->MenuList; tmp != NULL; tmp = tmp->next)
  {
    if (strcmp(name, tmp->name) == 0)
      return (tmp);
  }
  return NULL;
}



static Bool
belongs_to_twm_window(register TwmWindow * t, register Window w)
{
  if (!t)
    return False;


  if (t && t->titlebuttons)
  {
    register TBWindow *tbw;
    register int nb = Scr->TBInfo.nleft + Scr->TBInfo.nright;

    for (tbw = t->titlebuttons; nb > 0; tbw++, nb--)
    {
      if (tbw->window == w)
	return True;
    }
  }
  return False;
}



/*
 * Hack^H^H^H^HWrapper to moves for non-menu contexts.
 *
 * djhjr - 10/11/01 10/4/02
 */
static void
moveFromCenterWrapper(TwmWindow * tmp_win)
{
  if (!tmp_win->opaque_move)
    XUngrabServer(dpy);

  WarpScreenToWindow(tmp_win);

  /* now here's a nice little kludge... */
  {
    int hilite = tmp_win->highlight;

    tmp_win->highlight = True;
    SetBorder(tmp_win, (hilite) ? True : False);
    tmp_win->highlight = hilite;

    Focus = tmp_win;
  }

  if (!tmp_win->opaque_move)
    XGrabServer(dpy);
}

/*
 * Jason P. Venner jason@tfs.com
 * This function is used by F_WARPTO to match the action name
 * against window names.
 * Re-written to use list.c:MatchName(), allowing VTWM-style wilcards.
 * djhjr - 10/27/02
 */
static int
MatchWinName(char *action, TwmWindow * t)
{
  int matched = 0;

#ifndef NO_REGEX_SUPPORT
  regex_t re;
#else
  char re;
#endif

  if (MatchName(t->full_name, action, &re, LTYPE_ANY_STRING))
    if (MatchName(t->class.res_name, action, &re, LTYPE_ANY_STRING))
      if (MatchName(t->class.res_class, action, &re, LTYPE_ANY_STRING))
	matched = 1;

  return (matched);
}



/***********************************************************************
 *
 *	Procedure:
 *	ExecuteFunction - execute a twm root function
 *
 *	Inputs:
 *	func	- the function to execute
 *	action	- the menu action to execute
 *	w	- the window to execute this function on
 *	tmp_win	- the twm window structure
 *	event	- the event that caused the function
 *	context - the context in which the button was pressed
 *	pulldown- flag indicating execution from pull down menu
 *
 *	Returns:
 *	TRUE if should continue with remaining actions else FALSE to abort
 *
 ***********************************************************************
 */

extern int MovedFromKeyPress;

int
ExecuteFunction(int func, char *action, Window w, TwmWindow * tmp_win, XEvent * eventp, int context, int pulldown)
{
  char tmp[200];
  char *ptr;
  char buff[MAX_FILE_SIZE];
  int count, fd;
  int do_next_action = TRUE;

  actionHack = action;
  RootFunction = F_NOFUNCTION;
  if (Cancel)
    return TRUE;		/* XXX should this be FALSE? */

  switch (func)
  {
  case F_UPICONMGR:
  case F_LEFTICONMGR:
  case F_RIGHTICONMGR:
  case F_DOWNICONMGR:
  case F_FORWICONMGR:
  case F_BACKICONMGR:
  case F_NEXTICONMGR:
  case F_PREVICONMGR:
  case F_NOP:
  case F_TITLE:
  case F_DELTASTOP:
  case F_RAISELOWER:
  case F_WARP:
  case F_WARPCLASSNEXT:
  case F_WARPCLASSPREV:
  case F_WARPTOSCREEN:
  case F_WARPTO:
  case F_WARPRING:
  case F_WARPTOICONMGR:
  case F_COLORMAP:

  case F_SEPARATOR:

  case F_STATICICONPOSITIONS:

  case F_WARPSNUG:
  case F_WARPVISIBLE:

#ifndef NO_SOUND_SUPPORT
  case F_SOUNDS:
#endif

  case F_STRICTICONMGR:

  case F_BINDBUTTONS:
  case F_BINDKEYS:
  case F_UNBINDBUTTONS:
  case F_UNBINDKEYS:
#ifdef TWM_USE_SLOPPYFOCUS
  case F_SLOPPYFOCUS:
#endif

    break;

  default:

    switch (func)
    {
      /* restrict mouse to screen of function execution: */
    case F_FORCEMOVE:
    case F_MOVE:
    case F_RESIZE:
    case F_MOVESCREEN:
      JunkRoot = Scr->Root;
      break;

    default:
      /*
       * evtl. don't warp mouse to another screen
       * (as otherwise imposed by 'confine_to' Scr->Root):
       */
      JunkRoot = None;
    }

    XGrabPointer(dpy, Scr->Root, True,
		 ButtonPressMask | ButtonReleaseMask, GrabModeAsync, GrabModeAsync, JunkRoot, Scr->WaitCursor, CurrentTime);
    break;
  }

#ifndef NO_SOUND_SUPPORT
  switch (func)
  {
  case F_BEEP:
  case F_SQUEEZECENTER:
  case F_SQUEEZELEFT:
  case F_SQUEEZERIGHT:

  case F_MOVESCREEN:

  case F_FORCEMOVE:
  case F_MOVE:
  case F_RESIZE:
  case F_EXEC:
  case F_DELETE:
  case F_DELETEDOOR:
  case F_DESTROY:
  case F_DEICONIFY:
  case F_ICONIFY:
  case F_IDENTIFY:
  case F_VERSION:
  case F_QUIT:
  case F_WARP:
  case F_WARPCLASSNEXT:
  case F_WARPCLASSPREV:
  case F_WARPRING:
  case F_WARPTO:
  case F_WARPTOICONMGR:
  case F_WARPTONEWEST:
  case F_WARPTOSCREEN:
    /* handle uniquely */
    break;
  case F_POPUP:
    /* ignore */
    break;
  case F_LOWER:
  case F_RAISE:
  case F_RAISELOWER:
  case F_NAIL:
  case F_NAMEDOOR:
  case F_BOTTOMZOOM:
  case F_FULLZOOM:
  case F_HORIZOOM:
  case F_LEFTZOOM:
  case F_RIGHTZOOM:
  case F_TOPZOOM:
  case F_ZOOM:
  case F_BACKICONMGR:
  case F_DOWNICONMGR:
  case F_FORWICONMGR:
  case F_LEFTICONMGR:
  case F_RIGHTICONMGR:
  case F_UPICONMGR:
  case F_FOCUS:
  case F_SAVEYOURSELF:
  case F_STICKYABOVE:
  case F_RING:
  case F_WINREFRESH:

  case F_BINDBUTTONS:
  case F_BINDKEYS:
  case F_UNBINDBUTTONS:
  case F_UNBINDKEYS:

    /* ignore if from a root menu */
    if (Context != C_ROOT && Context != C_NO_CONTEXT)
      PlaySound(func);
    break;
  default:
    /* unconditional */
    PlaySound(func);
    break;
  }
#endif

  switch (func)
  {
  case F_NOP:
  case F_TITLE:
    break;

  case F_DELTASTOP:
    if (WindowMoved)
      do_next_action = FALSE;
    break;

  case F_RESTART:

  case F_STARTWM:
    if (func == F_STARTWM)
    {
      /* dynamic allocation of (char **)my_argv - djhjr - 9/26/02 */

      char *p, *delims = " \t";
      char *new_argv = NULL, **my_argv = NULL;
      int i = 0, j = 0;

      p = strtok(action, delims);
      while (p)
      {
	if (j >= i)
	{
	  i += 5;
	  new_argv = (char *)realloc((char *)my_argv, i * sizeof(char *));
	  if (new_argv == NULL)
	  {
	    fprintf(stderr, "%s: unable to allocate %lu bytes for execvp()\n", ProgramName, i * sizeof(char *));
	    break;
	  }
	  else
	    my_argv = (char **)new_argv;
	}

	my_argv[j++] = strdup(p);
	p = strtok(NULL, delims);
      }

      if (new_argv != NULL)
      {
	my_argv[j] = NULL;

	setup_restart(eventp->xbutton.time);

	execvp(*my_argv, my_argv);
	fprintf(stderr, "%s:  unable to start \"%s\"\n", ProgramName, *my_argv);
	new_argv = NULL;
      }

      if (new_argv == NULL)
      {
	i = 0;
	while (i < j)
	  free(my_argv[i++]);
	if (j)
	  free((char *)my_argv);
      }
    }
    else
      RestartVtwm(eventp->xbutton.time);

    break;

  case F_UPICONMGR:
  case F_DOWNICONMGR:
  case F_LEFTICONMGR:
  case F_RIGHTICONMGR:
  case F_FORWICONMGR:
  case F_BACKICONMGR:
    MoveIconManager(func);
    break;

  case F_NEXTICONMGR:
  case F_PREVICONMGR:
    JumpIconManager(func);
    break;

  case F_SHOWLIST:

    if (context == C_ROOT)
    {
      name_list *list;

      ShowIconMgr(&Scr->iconmgr);

      /*
       * New code in list.c necessitates 'next_entry()' and
       * 'contents_of_entry()' - djhjr - 10/20/01
       */
      for (list = Scr->IconMgrs; list != NULL; list = next_entry(list))
	ShowIconMgr((IconMgr *) contents_of_entry(list));
    }
    else
    {
      IconMgr *ip;

      if ((ip = (IconMgr *) LookInList(Scr->IconMgrs, tmp_win->full_name, &tmp_win->class)) == NULL)
	ip = &Scr->iconmgr;

      ShowIconMgr(ip);
    }

    RaiseStickyAbove();
    RaiseAutoPan();

    break;

  case F_HIDELIST:

    if (Scr->NoIconManagers)
      break;

    HideIconManager((context == C_ROOT) ? NULL : tmp_win);

    break;

  case F_SORTICONMGR:

    if (Scr->NoIconManagers || Scr->iconmgr.count == 0)
      break;

    if (DeferExecution(context, func, Scr->SelectCursor))
      return TRUE;

    {
      int save_sort;

      save_sort = Scr->SortIconMgr;
      Scr->SortIconMgr = TRUE;

      if (context == C_ICONMGR)
	SortIconManager((IconMgr *) NULL);
      else if (tmp_win->iconmgr)
	SortIconManager(tmp_win->iconmgrp);
      else
	DoAudible();

      Scr->SortIconMgr = save_sort;
    }
    break;

  case F_IDENTIFY:
    if (DeferExecution(context, func, Scr->SelectCursor))
    {
      return TRUE;
    }

    Identify(tmp_win);
    break;

  case F_VERSION:
    Identify((TwmWindow *) NULL);
    break;

  case F_ZOOMZOOM:
    Zoom(None, NULL, None, NULL);
    break;

  case F_AUTOPAN:
    {				/* toggle autopan */
      static int saved;

      if (Scr->AutoPanX)
      {
	saved = Scr->AutoPanX;
	Scr->AutoPanX = 0;
      }
      else
      {
	Scr->AutoPanX = saved;
	/* if restart with no autopan, we'll set the
	 ** variable but we won't pan
	 */
	RaiseAutoPan();
      }
      break;
    }

  case F_STICKYABOVE:
    if (Scr->StickyAbove)
    {
      LowerSticky();
      Scr->StickyAbove = FALSE;
      /* don't change the order of execution! */
    }
    else
    {
      Scr->StickyAbove = TRUE;
      RaiseStickyAbove();
      RaiseAutoPan();
      /* don't change the order of execution! */
    }
    return TRUE;

  case F_AUTORAISE:
    if (DeferExecution(context, func, Scr->SelectCursor))
      return TRUE;

    tmp_win->auto_raise = !tmp_win->auto_raise;
    if (tmp_win->auto_raise)
      ++(Scr->NumAutoRaises);
    else
      --(Scr->NumAutoRaises);
    break;

  case F_BEEP:

#ifndef NO_SOUND_SUPPORT
    /* sound has priority over bell */
    if (PlaySound(func))
      break;
#endif

    DoAudible();
    break;

  case F_POPUP:
    tmp_win = (TwmWindow *) action;
    if (Scr->WindowFunction.func != F_NOFUNCTION)
    {
      ExecuteFunction(Scr->WindowFunction.func, Scr->WindowFunction.item->action, w, tmp_win, eventp, C_FRAME, FALSE);
    }
    else
    {
      DeIconify(tmp_win);
      XRaiseWindow(dpy, tmp_win->frame);
      XRaiseWindow(dpy, tmp_win->VirtualDesktopDisplayWindow.win);

      RaiseStickyAbove();
      RaiseAutoPan();
    }
    break;

  case F_RESIZE:
    {
      TwmWindow *focused = NULL;
      Bool fromtitlebar = False;
      long releaseEvent;
      long movementMask;
      int resizefromcenter = 0;

#ifndef NO_SOUND_SUPPORT
      int did_playsound = FALSE;
#endif

      if (DeferExecution(context, func, Scr->ResizeCursor))
	return TRUE;

      PopDownMenu();

      if (pulldown)
	XWarpPointer(dpy, None, Scr->Root, 0, 0, 0, 0, eventp->xbutton.x_root, eventp->xbutton.y_root);

      EventHandler[EnterNotify] = HandleUnknown;
      EventHandler[LeaveNotify] = HandleUnknown;

      if (context == C_ICON)	/* can't resize icons */
      {
	DoAudible();
	break;
      }

      /*
       * Resizing from a titlebar menu was handled uniquely long
       * before I got here, and I added virtual windows and icon
       * managers on 9/15/99 and 10/11/01, leveraging that code.
       * It's all been integrated here.
       */
      if (Context & (C_FRAME_BIT | C_WINDOW_BIT | C_TITLE_BIT) && menuFromFrameOrWindowOrTitlebar)
      {
	XGetGeometry(dpy, w, &JunkRoot, &origDragX, &origDragY,
		     (unsigned int *)&DragWidth, (unsigned int *)&DragHeight, &JunkBW, &JunkDepth);

	resizefromcenter = 2;
      }
      else if (Context == C_VIRTUAL_WIN)
      {
	TwmWindow *twin;

	if ((XFindContext(dpy, eventp->xbutton.subwindow, VirtualContext, (caddr_t *) & twin) == XCNOENT))
	{
	  DoAudible();
	  break;
	}

	context = C_WINDOW;
	tmp_win = twin;
	resizefromcenter = 1;
      }
      else if (Context == C_ICONMGR && tmp_win->list)
      {
	if (!warp_if_warpunmapped(tmp_win, F_NOFUNCTION))
	{
	  DoAudible();
	  break;
	}

	resizefromcenter = 1;
      }

      if (resizefromcenter)
      {
	WarpScreenToWindow(tmp_win);

	XWarpPointer(dpy, None, Scr->Root, 0, 0, 0, 0,
		     tmp_win->frame_x + tmp_win->frame_width / 2, tmp_win->frame_y + tmp_win->frame_height / 2);

	focused = Focus;
	Focus = tmp_win;
	SetBorder(Focus, True);

	/* save positions so we can tell if it was moved or not */
	ResizeOrigX = tmp_win->frame_x + tmp_win->frame_width / 2;
	ResizeOrigY = tmp_win->frame_y + tmp_win->frame_height / 2;
      }
      else
      {
	/* save position so we can tell if it was moved or not */
	ResizeOrigX = eventp->xbutton.x_root;
	ResizeOrigY = eventp->xbutton.y_root;
      }

      /* see if this is being done from the titlebar */
      fromtitlebar = belongs_to_twm_window(tmp_win, eventp->xbutton.window);

      if (resizefromcenter == 2)
      {
	MenuStartResize(tmp_win, origDragX, origDragY, DragWidth, DragHeight, Context);

	releaseEvent = ButtonPress;
	movementMask = PointerMotionMask;
      }
      else
      {
	StartResize(eventp, tmp_win, fromtitlebar, context);

	fromtitlebar = False;
	releaseEvent = ButtonRelease;
	movementMask = ButtonMotionMask;
      }

      while (TRUE)
      {
	XMaskEvent(dpy, ButtonPressMask | ButtonReleaseMask |
		   EnterWindowMask | LeaveWindowMask | ExposureMask | VisibilityChangeMask | movementMask, &Event);


	/*
	 * Don't discard exposure events before release
	 * or window borders and/or their titles in the
	 * virtual desktop won't get redrawn - djhjr
	 */

	/* discard any extra motion events before a release */
	if (Event.type == MotionNotify)
	{
	  while (XCheckMaskEvent(dpy, releaseEvent | movementMask, &Event))
	  {
	    if (Event.type == releaseEvent)
	      break;
	  }
	}


	if (Event.type == releaseEvent)
	{
	  if (Cancel)
	  {
	    if (tmp_win->opaque_resize)
	    {
	      ConstrainSize(tmp_win, &origWidth, &origHeight);
	      SetupWindow(tmp_win, origx, origy, origWidth, origHeight, -1);
	      ResizeTwmWindowContents(tmp_win, origWidth, origHeight);
	    }
	    else
	      MoveOutline(Scr->Root, 0, 0, 0, 0, 0, 0);

	    ResizeWindow = None;
	    resizing_window = 0;
	    do_next_action = FALSE;
	  }
	  else
	  {
	    if (resizefromcenter == 2)
	    {
	      MenuEndResize(tmp_win, Context);
	    }
	    else
	      EndResize();

	    DispatchEvent();

	    if (!Scr->NoRaiseResize && !Scr->RaiseOnStart && WindowMoved)
	    {
	      XRaiseWindow(dpy, tmp_win->frame);
	      SetRaiseWindow(tmp_win);
	    }
	  }

	  break;
	}

	if (!DispatchEvent())
	  continue;

	if (Event.type != MotionNotify)
	  continue;

	XQueryPointer(dpy, Scr->Root, &JunkRoot, &JunkChild, &JunkX, &JunkY, &AddingX, &AddingY, &JunkMask);

	if (!resizing_window && (abs(AddingX - ResizeOrigX) < Scr->MoveDelta && abs(AddingY - ResizeOrigY) < Scr->MoveDelta))
	{
	  continue;
	}

	resizing_window = 1;
	WindowMoved = TRUE;

	if ((!Scr->NoRaiseResize && Scr->RaiseOnStart)
	    || (tmp_win->opaque_resize && (HasShape && (tmp_win->wShaped || tmp_win->squeeze_info))))
	{
	  XRaiseWindow(dpy, tmp_win->frame);
	  SetRaiseWindow(tmp_win);
	  if (Scr->Virtual && tmp_win->VirtualDesktopDisplayWindow.win)
	    XRaiseWindow(dpy, tmp_win->VirtualDesktopDisplayWindow.win);
	}

#ifndef NO_SOUND_SUPPORT
	if (did_playsound == FALSE)
	{
	  PlaySound(func);
	  did_playsound = TRUE;
	}
#endif

	DoResize(AddingX, AddingY, tmp_win);
      }

      if (!tmp_win->opaque_resize)
	XUngrabServer(dpy);

      /*
       * All this stuff from resize.c:EndResize() - djhjr - 10/6/02
       */

      if (!tmp_win->opaque_resize)
	UninstallRootColormap();

      /* discard queued enter and leave events - djhjr - 5/27/03 */
      while (XCheckMaskEvent(dpy, EnterWindowMask | LeaveWindowMask, &Event))
	;

      if (!Scr->NoRaiseResize)
      {
	RaiseStickyAbove();
	RaiseAutoPan();
      }

      /* update virtual coords */
      tmp_win->virtual_frame_x = Scr->VirtualDesktopX + tmp_win->frame_x;
      tmp_win->virtual_frame_y = Scr->VirtualDesktopY + tmp_win->frame_y;

      MoveResizeDesktop(tmp_win, Cancel | Scr->NoRaiseResize);

      if (Context == C_VIRTUAL_WIN)
      {
	/*
	 * Mask a bug that calls MoveOutline(zeros) after the
	 * border has been repainted, leaving artifacts. I think
	 * I know what the bug is, but I can't seem to fix it.
	 */
	if (Scr->BorderBevelWidth > 0)
	  PaintBorders(tmp_win, False);

	JunkX = tmp_win->virtual_frame_x + tmp_win->frame_width / 2;
	JunkY = tmp_win->virtual_frame_y + tmp_win->frame_height / 2;
	XWarpPointer(dpy, None, Scr->VirtualDesktopDisplayOuter, 0, 0, 0, 0, SCALE_D(JunkX), SCALE_D(JunkY));

	SetBorder(Focus, False);
	Focus = focused;
      }

      /* don't re-map if the window is the virtual desktop - djhjr - 2/28/99 */
      if (Scr->VirtualReceivesMotionEvents && tmp_win->w != Scr->VirtualDesktopDisplayOuter)
      {
	XUnmapWindow(dpy, Scr->VirtualDesktopDisplay);
	XMapWindow(dpy, Scr->VirtualDesktopDisplay);
      }

      break;
    }

  case F_ZOOM:
  case F_HORIZOOM:
  case F_FULLZOOM:
  case F_LEFTZOOM:
  case F_RIGHTZOOM:
  case F_TOPZOOM:
  case F_BOTTOMZOOM:
    if (DeferExecution(context, func, Scr->SelectCursor))
      return TRUE;

    PopDownMenu();

    fullzoom(tmp_win, func);
    MoveResizeDesktop(tmp_win, Scr->NoRaiseMove);
    break;

  case F_MOVE:
  case F_FORCEMOVE:
    {
      static Time last_time = 0;
      Window rootw;
      Bool fromtitlebar = False;
      int moving_icon = FALSE;
      int constMoveDir, constMoveX = 0, constMoveY = 0;
      int constMoveXL = 0, constMoveXR = 0, constMoveYT = 0, constMoveYB = 0;
      int origX, origY;
      long releaseEvent;
      long movementMask;
      int xl = 0, yt = 0, xr = 0, yb = 0;
      int movefromcenter = 0;

#ifndef NO_SOUND_SUPPORT
      int did_playsound = FALSE;
#endif

      if (DeferExecution(context, func, Scr->MoveCursor))
	return TRUE;

      PopDownMenu();
      rootw = eventp->xbutton.root;

      if (pulldown)
	XWarpPointer(dpy, None, Scr->Root, 0, 0, 0, 0, eventp->xbutton.x_root, eventp->xbutton.y_root);

      EventHandler[EnterNotify] = HandleUnknown;
      EventHandler[LeaveNotify] = HandleUnknown;

      if (!tmp_win->opaque_move)
	XGrabServer(dpy);


      XGrabPointer(dpy, eventp->xbutton.root, True,
		   ButtonPressMask | ButtonReleaseMask |
		   ButtonMotionMask | PointerMotionMask, GrabModeAsync, GrabModeAsync, Scr->Root, Scr->MoveCursor, CurrentTime);

      if (context == C_VIRTUAL_WIN)
      {
	TwmWindow *twin;

	if ((XFindContext(dpy, eventp->xbutton.subwindow, VirtualContext, (caddr_t *) & twin) == XCNOENT))
	{
	  DoAudible();
	  break;
	}

	tmp_win = twin;
	moveFromCenterWrapper(tmp_win);
	w = tmp_win->frame;
	movefromcenter = 1;
      }
      else
       if (context == C_ICONMGR && tmp_win->list)
      {
	if (!warp_if_warpunmapped(tmp_win, F_NOFUNCTION))
	{
	  DoAudible();
	  break;
	}

	moveFromCenterWrapper(tmp_win);
	w = tmp_win->frame;
	movefromcenter = 1;
      }
      else
       if (context == C_ICON && tmp_win->icon_w.win)
      {
	DragX = eventp->xbutton.x;
	DragY = eventp->xbutton.y;

	w = tmp_win->icon_w.win;
	moving_icon = TRUE;
      }
      else if (w != tmp_win->icon_w.win)
      {
	XTranslateCoordinates(dpy, w, tmp_win->frame, eventp->xbutton.x, eventp->xbutton.y, &DragX, &DragY, &JunkChild);

	w = tmp_win->frame;
      }

#ifdef TILED_SCREEN
      if (Scr->use_tiles == TRUE)
      {
	int k = FindNearestTileToMouse();

	XMoveWindow(dpy, Scr->SizeWindow.win, Lft(Scr->tiles[k]), Bot(Scr->tiles[k]));
      }
#endif
      XMapRaised(dpy, Scr->SizeWindow.win);

      DragWindow = None;

      MoveFunction = func;	/* set for DispatchEvent() */

      XGetGeometry(dpy, w, &JunkRoot, &origDragX, &origDragY,
		   (unsigned int *)&DragWidth, (unsigned int *)&DragHeight, &JunkBW, &JunkDepth);

      if (menuFromFrameOrWindowOrTitlebar || movefromcenter || (moving_icon && fromMenu))
      {
	origX = DragX = origDragX + DragWidth / 2;
	origY = DragY = origDragY + DragHeight / 2;
      }
      else
      {
	origX = eventp->xbutton.x_root;
	origY = eventp->xbutton.y_root;
      }

      CurrentDragX = origDragX;
      CurrentDragY = origDragY;

      /*
       * Only do the constrained move if timer is set -
       * need to check it in case of stupid or wicked fast servers.
       */
      if (ConstrainedMoveTime && eventp->xbutton.time - last_time < ConstrainedMoveTime)
      {
	int width, height;

	ConstMove = TRUE;
	constMoveDir = MOVE_NONE;
	constMoveX = eventp->xbutton.x_root - DragX - JunkBW;
	constMoveY = eventp->xbutton.y_root - DragY - JunkBW;
	width = DragWidth + 2 * JunkBW;
	height = DragHeight + 2 * JunkBW;
	constMoveXL = constMoveX + width / 3;
	constMoveXR = constMoveX + 2 * (width / 3);
	constMoveYT = constMoveY + height / 3;
	constMoveYB = constMoveY + 2 * (height / 3);

	XWarpPointer(dpy, None, w, 0, 0, 0, 0, DragWidth / 2, DragHeight / 2);

	XQueryPointer(dpy, w, &JunkRoot, &JunkChild, &JunkX, &JunkY, &DragX, &DragY, &JunkMask);
      }
      last_time = eventp->xbutton.time;

      if (!tmp_win->opaque_move)
      {
	InstallRootColormap();
	{
	  /*
	   * Draw initial outline.  This was previously done the
	   * first time though the outer loop by dropping out of
	   * the XCheckMaskEvent inner loop down to one of the
	   * MoveOutline's below.
	   */
	  MoveOutline(rootw,
		      origDragX - JunkBW, origDragY - JunkBW,
		      DragWidth + 2 * JunkBW, DragHeight + 2 * JunkBW,
		      tmp_win->frame_bw, moving_icon ? 0 : tmp_win->title_height + tmp_win->frame_bw3D);

	}
      }

      /*
       * see if this is being done from the titlebar
       */
      fromtitlebar = belongs_to_twm_window(tmp_win, eventp->xbutton.window);

      if ((menuFromFrameOrWindowOrTitlebar && !fromtitlebar) || movefromcenter || (moving_icon && fromMenu))
      {
	/* warp the pointer to the middle of the window */
	XWarpPointer(dpy, None, Scr->Root, 0, 0, 0, 0, origDragX + DragWidth / 2, origDragY + DragHeight / 2);

	SetBorder(tmp_win, True);

	XFlush(dpy);
      }

      DisplayPosition(CurrentDragX, CurrentDragY);

      if (menuFromFrameOrWindowOrTitlebar)
      {
	releaseEvent = ButtonPress;
	movementMask = PointerMotionMask;
      }
      else
      {
	releaseEvent = ButtonRelease;
	movementMask = ButtonMotionMask;
      }

      while (TRUE)
      {
	/* block until there is an interesting event */
	XMaskEvent(dpy, ButtonPressMask | ButtonReleaseMask |
		   EnterWindowMask | LeaveWindowMask | ExposureMask | VisibilityChangeMask | movementMask, &Event);


	/*
	 * Don't discard exposure events before release
	 * or window borders and/or their titles in the
	 * virtual desktop won't get redrawn - djhjr
	 */

	/* discard any extra motion events before a release */
	if (Event.type == MotionNotify)
	{
	  while (XCheckMaskEvent(dpy, movementMask | releaseEvent, &Event))
	  {
	    if (Event.type == releaseEvent)
	      break;
	  }
	}

	/*
	 * There used to be a couple of routines that handled the
	 * cancel functionality here, each doing a portion of the
	 * job, then returning immediately. They became redundant
	 * to just letting program execution fall through. So now,
	 * the 'if (Event.type == releaseEvent) if (Cancel)' below
	 * does just that, clearing a few flags first.
	 */

	if (fromtitlebar && Event.type == ButtonPress)
	{
	  fromtitlebar = False;
	  CurrentDragX = origX = Event.xbutton.x_root;
	  CurrentDragY = origY = Event.xbutton.y_root;
	  XTranslateCoordinates(dpy, rootw, tmp_win->frame, origX, origY, &DragX, &DragY, &JunkChild);

	  continue;
	}

	if (!DispatchEvent())
	  continue;

	if (Event.type == releaseEvent)
	{
	  MoveOutline(rootw, 0, 0, 0, 0, 0, 0);

	  if (Cancel)
	  {
	    DragWindow = None;
	    ConstMove = WindowMoved = do_next_action = FALSE;
	  }
	  else if (WindowMoved)
	  {
	    if (moving_icon)
	    {
	      tmp_win->icon_moved = TRUE;
	      XMoveWindow(dpy, tmp_win->icon_w.win, CurrentDragX, CurrentDragY);

	      if (!Scr->NoRaiseMove && !Scr->RaiseOnStart)
	      {
		XRaiseWindow(dpy, tmp_win->icon_w.win);
		/* XXXSJR - this used to be SetRaiseWindow(tmp_win->icon_w.win); */
		SetRaiseWindow(tmp_win);
	      }
	    }
	    else
	    {
	      if (movefromcenter)
	      {
		tmp_win->frame_x = Event.xbutton.x_root - DragWidth / 2;
		tmp_win->frame_y = Event.xbutton.y_root - DragHeight / 2;
	      }
	      else
	      {
		tmp_win->frame_x = CurrentDragX;
		tmp_win->frame_y = CurrentDragY;
	      }

	      XMoveWindow(dpy, tmp_win->frame, tmp_win->frame_x, tmp_win->frame_y);
	      SendConfigureNotify(tmp_win, tmp_win->frame_x, tmp_win->frame_y);

	      if (!Scr->NoRaiseMove && !Scr->RaiseOnStart)
	      {
		XRaiseWindow(dpy, tmp_win->frame);
		SetRaiseWindow(tmp_win);
	      }
	    }
	  }

	  break;
	}

	/* something left to do only if the pointer moved */
	if (Event.type != MotionNotify)
	  continue;

	XQueryPointer(dpy, rootw, &(eventp->xmotion.root), &JunkChild,
		      &(eventp->xmotion.x_root), &(eventp->xmotion.y_root), &JunkX, &JunkY, &JunkMask);

	if (DragWindow == None &&
	    abs(eventp->xmotion.x_root - origX) < Scr->MoveDelta && abs(eventp->xmotion.y_root - origY) < Scr->MoveDelta)
	{
	  continue;
	}

	WindowMoved = TRUE;
	DragWindow = w;

	if (!Scr->NoRaiseMove && Scr->RaiseOnStart)
	{
	  if (moving_icon)
	  {
	    XRaiseWindow(dpy, tmp_win->icon_w.win);
	    /* XXXSJR - this used to be SetRaiseWindow(tmp_win->icon_w.win); */
	    SetRaiseWindow(tmp_win);
	  }
	  else
	  {
	    XRaiseWindow(dpy, tmp_win->frame);
	    SetRaiseWindow(tmp_win);
	    if (Scr->Virtual && tmp_win->VirtualDesktopDisplayWindow.win)
	      XRaiseWindow(dpy, tmp_win->VirtualDesktopDisplayWindow.win);
	  }
	}

	if (ConstMove)
	{
	  switch (constMoveDir)
	  {
	  case MOVE_NONE:
	    if (eventp->xmotion.x_root < constMoveXL || eventp->xmotion.x_root > constMoveXR)
	    {
	      constMoveDir = MOVE_HORIZ;
	    }
	    if (eventp->xmotion.y_root < constMoveYT || eventp->xmotion.y_root > constMoveYB)
	    {
	      constMoveDir = MOVE_VERT;
	    }
	    XQueryPointer(dpy, DragWindow, &JunkRoot, &JunkChild, &JunkX, &JunkY, &DragX, &DragY, &JunkMask);
	    break;
	  case MOVE_VERT:
	    constMoveY = eventp->xmotion.y_root - DragY - JunkBW;
	    break;
	  case MOVE_HORIZ:
	    constMoveX = eventp->xmotion.x_root - DragX - JunkBW;
	    break;
	  }

	  xl = constMoveX;
	  yt = constMoveY;
	}
	else if (DragWindow != None)
	{
	  if (!menuFromFrameOrWindowOrTitlebar && !movefromcenter && !(moving_icon && fromMenu))
	  {
	    xl = eventp->xmotion.x_root - DragX - JunkBW;
	    yt = eventp->xmotion.y_root - DragY - JunkBW;
	  }
	  else
	  {
	    xl = eventp->xmotion.x_root - (DragWidth / 2);
	    yt = eventp->xmotion.y_root - (DragHeight / 2);
	  }
	}

	if ((ConstMove && constMoveDir != MOVE_NONE) || DragWindow != None)
	{
	  int width = DragWidth + 2 * JunkBW;
	  int height = DragHeight + 2 * JunkBW;

	  if (Scr->DontMoveOff && MoveFunction != F_FORCEMOVE)
	  {
	    xr = xl + width;
	    yb = yt + height;

	    if (xl < 0)
	      xl = 0;
	    if (xr > Scr->MyDisplayWidth)
	      xl = Scr->MyDisplayWidth - width;

	    if (yt < 0)
	      yt = 0;
	    if (yb > Scr->MyDisplayHeight)
	      yt = Scr->MyDisplayHeight - height;
	  }

	  CurrentDragX = xl;
	  CurrentDragY = yt;

#ifndef NO_SOUND_SUPPORT
	  if ((!ConstMove || constMoveDir != MOVE_NONE) && did_playsound == FALSE)
	  {
	    PlaySound(func);
	    did_playsound = TRUE;
	  }
#endif

	  if (tmp_win->opaque_move)
	    XMoveWindow(dpy, DragWindow, xl, yt);
	  else
	    MoveOutline(eventp->xmotion.root, xl, yt,
			width, height, tmp_win->frame_bw, moving_icon ? 0 : tmp_win->title_height + tmp_win->frame_bw3D);

	  if (Scr->VirtualReceivesMotionEvents)
	  {
	    tmp_win->virtual_frame_x = R_TO_V_X(xl);
	    tmp_win->virtual_frame_y = R_TO_V_Y(yt);
	    MoveResizeDesktop(tmp_win, TRUE);
	  }

	  DisplayPosition(xl, yt);
	}
      }

      if (!tmp_win->opaque_move)
	XUngrabServer(dpy);

      XUnmapWindow(dpy, Scr->SizeWindow.win);

      MovedFromKeyPress = False;

      if (!tmp_win->opaque_move)
	UninstallRootColormap();

      while (XCheckMaskEvent(dpy, EnterWindowMask | LeaveWindowMask, &Event))
	;

      if (!Scr->NoRaiseMove)
      {
	RaiseStickyAbove();
	RaiseAutoPan();
      }

      /* update virtual coords */
      tmp_win->virtual_frame_x = Scr->VirtualDesktopX + tmp_win->frame_x;
      tmp_win->virtual_frame_y = Scr->VirtualDesktopY + tmp_win->frame_y;

      /* UpdateDesktop() hoses the stacking order - djhjr - 10/6/02 */
      MoveResizeDesktop(tmp_win, Cancel | Scr->NoRaiseMove);

      if (Context == C_VIRTUAL_WIN)
      {
	/*
	 * Mask a bug that calls MoveOutline(zeros) after the
	 * border has been repainted, leaving artifacts. I think
	 * I know what the bug is, but I can't seem to fix it.
	 */
	if (Scr->BorderBevelWidth > 0)
	  PaintBorders(tmp_win, False);

	JunkX = tmp_win->virtual_frame_x + tmp_win->frame_width / 2;
	JunkY = tmp_win->virtual_frame_y + tmp_win->frame_height / 2;
	XWarpPointer(dpy, None, Scr->VirtualDesktopDisplayOuter, 0, 0, 0, 0, SCALE_D(JunkX), SCALE_D(JunkY));
      }

      /* don't re-map if the window is the virtual desktop - djhjr - 2/28/99 */
      if (Scr->VirtualReceivesMotionEvents && tmp_win->w != Scr->VirtualDesktopDisplayOuter)
      {
	XUnmapWindow(dpy, Scr->VirtualDesktopDisplay);
	XMapWindow(dpy, Scr->VirtualDesktopDisplay);
      }

      MoveFunction = F_NOFUNCTION;	/* clear for DispatchEvent() */

      /* sanity check (also in events.c:HandleButtonRelease()) - djhjr - 10/6/02 */
      DragWindow = None;
      ConstMove = FALSE;

      break;
    }
  case F_FUNCTION:
    {
      MenuRoot *mroot;
      MenuItem *mitem;
      Cursor cursor;

      if ((mroot = FindMenuRoot(action)) == NULL)
      {
	fprintf(stderr, "%s: couldn't find function \"%s\"\n", ProgramName, action);
	return TRUE;
      }

      if ((cursor = NeedToDefer(mroot)) != None && DeferExecution(context, func, cursor))
	return TRUE;
      else
      {
	for (mitem = mroot->first; mitem != NULL; mitem = mitem->next)
	{
	  if (!ExecuteFunction(mitem->func, mitem->action, w, tmp_win, eventp, context, pulldown))
	    break;
	}
      }
    }
    break;

  case F_DEICONIFY:
  case F_ICONIFY:
    if (DeferExecution(context, func, Scr->SelectCursor))
      return TRUE;

    if (tmp_win->icon || (func == F_DEICONIFY && tmp_win == tmp_win->list->twm))
    {
#ifndef NO_SOUND_SUPPORT
      PlaySound(func);
#endif

      DeIconify(tmp_win);

      /*
       * now HERE's a fine bit of kludge! it's to mask a hole in the
       * code I can't find that messes up when trying to warp to the
       * de-iconified window not in the real screen when WarpWindows
       * isn't used. see also the change in DeIconify().
       */
      if (!Scr->WarpWindows && (Scr->WarpCursor || LookInList(Scr->WarpCursorL, tmp_win->full_name, &tmp_win->class)))
      {
	RaiseStickyAbove();
	RaiseAutoPan();

	WarpToWindow(tmp_win);
      }
    }
    else if (func == F_ICONIFY)
    {
      TwmDoor *d;
      TwmWindow *tmgr = NULL, *twin = NULL;
      MenuRoot *mr;

      if (XFindContext(dpy, tmp_win->w, DoorContext, (caddr_t *) & d) != XCNOENT)
      {
	twin = tmp_win;
	tmp_win = d->twin;
      }

      /*
       * don't iconify if there's no way to get it back - not fool-proof
       */
      if (tmp_win->iconify_by_unmapping)
      {
	/* iconified by unmapping */

	if (tmp_win->list)
	  tmgr = tmp_win->list->iconmgr->twm_win;

	if ((tmgr && !tmgr->mapped && tmgr->iconify_by_unmapping) ||
	    ((Scr->IconManagerDontShow ||
	      LookInList(Scr->IconMgrNoShow, tmp_win->full_name, &tmp_win->class)) &&
	     LookInList(Scr->IconMgrShow, tmp_win->full_name, &tmp_win->class) == (char *)NULL))
	{
	  /* icon manager not mapped or not shown in one */

	  if (mr)
	  {
	    /*
	     * Yes, this is an intentional uninitialized variable dereference, the
	     * sole purpose of which is to draw your attention to the problem
	     * expressed herewithin:
	     * 
	     * Eeri Kask writes:
	     * Actually the whole concept of 'have_twmwindows', 'have_showdesktop',
	     * 'have_showdesktop' being 'global' variables is defect because one can
	     * have different vtwm-configurations for different screens: .vtwmrc.0,
	     * .vtwmrc.1, etc. and then it is not apparent which of these screens
	     * state do these three variables then denote.  (At least they should be
	     * enclosed into 'Scr'.)
	     * 
	     * 
	     * It seems the whole idea of
	     * 
	     * "don't iconify if there's no way to get it back - not fool-proof"
	     * 
	     * snippet is to guard the user from losing windows by iconifying "by
	     * unmapping" and the same time not having e.g.
	     * 
	     * f.menu "TwmWindows"
	     * 
	     * (a) attached to any hotkey/hotbutton (FindMenuOrFuncInBindings()), or
	     * (b) attached to any context (frame, titlebar) or titlebar-buttons
	     * (FindMenuOrFuncInWindows()).
	     * 
	     * 
	     * Further, the 'mr' pointer cannot be made 'static' as well because of the
	     * same reason as the three 'global' context bitmap variables above.
	     * 
	     * The 'mr' pointer is checked in FindMenuOrFuncInWindows() against window
	     * title buttons: i.e. a window is assumed deiconifiable if the
	     * "TwmWindows" menu is attached to some title-bar-button.  In case there
	     * is only one client then it remains to be figured out how to push that
	     * button in emergency in order to deiconify if the window is iconified.
	     */
	    mr = NULL;
	  }

	  if (have_twmwindows == -1)
	  {
	    have_twmwindows = 0;

	    /* better than two calls to FindMenuRoot() */
	    for (mr = Scr->MenuList; mr != NULL; mr = mr->next)
	      if (strcmp(mr->name, TWM_WINDOWS) == 0 || strcmp(mr->name, VTWM_WINDOWS) == 0)
	      {
		have_twmwindows = FindMenuOrFuncInBindings(C_ALL_BITS, mr, F_NOFUNCTION);
		break;
	      }
	  }
	  if (have_showdesktop == -1)
	    have_showdesktop = FindMenuOrFuncInBindings(C_ALL_BITS, NULL, F_SHOWDESKTOP);
	  if (have_showlist == -1)
	    have_showlist = FindMenuOrFuncInBindings(C_ALL_BITS, NULL, F_SHOWLIST);

	  if (!FindMenuOrFuncInWindows(tmp_win, have_twmwindows, mr, F_NOFUNCTION) ||
	      LookInList(Scr->DontShowInTWMWindows, tmp_win->full_name, &tmp_win->class))
	  {
	    /* no TwmWindows menu or not shown in it */

	    if (tmp_win->w == Scr->VirtualDesktopDisplayOuter &&
		FindMenuOrFuncInWindows(tmp_win, have_showdesktop, NULL, F_SHOWDESKTOP))
	      ;
	    else if (tmp_win->iconmgr && FindMenuOrFuncInWindows(tmp_win, have_showlist, NULL, F_SHOWLIST))
	      ;
	    else if (tmgr && FindMenuOrFuncInWindows(tmgr, have_showlist, NULL, F_SHOWLIST))
	      ;
	    else
	    {
	      /* no f.showdesktop or f.showiconmgr */

	      DoAudible();

	      if (twin)
		tmp_win = twin;
	      break;
	    }
	  }
	}
      }

      if (twin)
	tmp_win = twin;

      if (tmp_win->list || !Scr->NoIconifyIconManagers)
      {
#ifndef NO_SOUND_SUPPORT
	PlaySound(func);
#endif

	Iconify(tmp_win, eventp->xbutton.x_root - EDGE_OFFSET, eventp->xbutton.y_root - EDGE_OFFSET);
      }
    }
    break;

  case F_RAISELOWER:
    if (DeferExecution(context, func, Scr->SelectCursor))
      return TRUE;

    if (!WindowMoved)
    {
      XWindowChanges xwc;

      xwc.stack_mode = Opposite;
      if (w != tmp_win->icon_w.win)
	w = tmp_win->frame;
      XConfigureWindow(dpy, w, CWStackMode, &xwc);
      XConfigureWindow(dpy, tmp_win->VirtualDesktopDisplayWindow.win, CWStackMode, &xwc);
      /* ug */
      XLowerWindow(dpy, Scr->VirtualDesktopDScreen);
    }
    break;

  case F_RAISE:
    if (DeferExecution(context, func, Scr->SelectCursor))
      return TRUE;

    /* check to make sure raise is not from the WindowFunction */
    if (w == tmp_win->icon_w.win && Context != C_ROOT)
      XRaiseWindow(dpy, tmp_win->icon_w.win);
    else
    {
      XRaiseWindow(dpy, tmp_win->frame);
      XRaiseWindow(dpy, tmp_win->VirtualDesktopDisplayWindow.win);
    }

    RaiseStickyAbove();
    RaiseAutoPan();

    break;

  case F_LOWER:
    if (DeferExecution(context, func, Scr->SelectCursor))
      return TRUE;

    if (!(Scr->StickyAbove && tmp_win->nailed))
    {
      if (w == tmp_win->icon_w.win)
	XLowerWindow(dpy, tmp_win->icon_w.win);
      else
      {
	XLowerWindow(dpy, tmp_win->frame);
	XLowerWindow(dpy, tmp_win->VirtualDesktopDisplayWindow.win);
	XLowerWindow(dpy, Scr->VirtualDesktopDScreen);
      }
    }

    break;

  case F_FOCUS:
#ifdef TWM_USE_SLOPPYFOCUS
    if (SloppyFocus == TRUE)
    {
      SloppyFocus = FALSE;
      FocusOnRoot();
    }
#endif
    if (DeferExecution(context, func, Scr->SelectCursor))
      return TRUE;

    if (tmp_win->icon == FALSE)
    {
      if (!FocusRoot && Focus == tmp_win)
      {
	FocusOnRoot();
      }
      else
      {
	FocusOnClient(tmp_win);
      }
    }
    break;

  case F_DESTROY:
    if (DeferExecution(context, func, Scr->DestroyCursor))
      return TRUE;

#ifndef NO_SOUND_SUPPORT
    /* flag for the handler */
    if (PlaySound(func))
      destroySoundFromFunction = TRUE;
#endif

    if (tmp_win == Scr->VirtualDesktopDisplayTwin)
    {
      if (have_showdesktop == -1)
	have_showdesktop = FindMenuOrFuncInBindings(C_ALL_BITS, NULL, F_SHOWDESKTOP);
      if (FindMenuOrFuncInWindows(tmp_win, have_showdesktop, NULL, F_SHOWDESKTOP))
	XUnmapWindow(dpy, Scr->VirtualDesktopDisplayTwin->frame);
      else

	DoAudible();
      break;
    }

    {
      TwmDoor *d;

      if (XFindContext(dpy, tmp_win->w, DoorContext, (caddr_t *) & d) != XCNOENT)
      {
	/* for some reason, we don't get the button up event - djhjr - 9/10/99 */
	ButtonPressed = -1;
	door_delete(tmp_win->w, d);

	break;
      }
    }

    if (tmp_win->iconmgr)	/* don't send ourself a message */
    {
      if (have_showlist == -1)
	have_showlist = FindMenuOrFuncInBindings(C_ALL_BITS, NULL, F_SHOWLIST);
      if (FindMenuOrFuncInWindows(tmp_win, have_showlist, NULL, F_SHOWLIST))

	HideIconManager(tmp_win);

      else
	DoAudible();
    }
    else
    {
      AppletDown(tmp_win);

      XKillClient(dpy, tmp_win->w);
    }
    break;

  case F_DELETE:
    if (DeferExecution(context, func, Scr->DestroyCursor))
      return TRUE;

#ifndef NO_SOUND_SUPPORT
    /* flag for the handler */
    if (PlaySound(func))
      destroySoundFromFunction = TRUE;
#endif

    if (tmp_win == Scr->VirtualDesktopDisplayTwin)
    {
      if (have_showdesktop == -1)
	have_showdesktop = FindMenuOrFuncInBindings(C_ALL_BITS, NULL, F_SHOWDESKTOP);
      if (FindMenuOrFuncInWindows(tmp_win, have_showdesktop, NULL, F_SHOWDESKTOP))
	XUnmapWindow(dpy, Scr->VirtualDesktopDisplayTwin->frame);
      else
	DoAudible();

      break;
    }

    {
      TwmDoor *d;

      if (XFindContext(dpy, tmp_win->w, DoorContext, (caddr_t *) & d) != XCNOENT)
      {
	/* for some reason, we don't get the button up event - djhjr - 9/10/99 */
	ButtonPressed = -1;
	door_delete(tmp_win->w, d);

	break;
      }
    }

    if (tmp_win->iconmgr)	/* don't send ourself a message */
    {
      if (have_showlist == -1)
	have_showlist = FindMenuOrFuncInBindings(C_ALL_BITS, NULL, F_SHOWLIST);
      if (FindMenuOrFuncInWindows(tmp_win, have_showlist, NULL, F_SHOWLIST))

	HideIconManager(tmp_win);

      else
	DoAudible();
    }
    else if (tmp_win->protocols & DoesWmDeleteWindow)
    {
      AppletDown(tmp_win);

      SendDeleteWindowMessage(tmp_win, LastTimestamp());
    }
    else
      DoAudible();
    break;

  case F_SAVEYOURSELF:
    if (DeferExecution(context, func, Scr->SelectCursor))
      return TRUE;

    if (tmp_win->protocols & DoesWmSaveYourself)
      SendSaveYourselfMessage(tmp_win, LastTimestamp());
    else
      DoAudible();
    break;

  case F_CIRCLEUP:
    XCirculateSubwindowsUp(dpy, Scr->Root);
    break;

  case F_CIRCLEDOWN:
    XCirculateSubwindowsDown(dpy, Scr->Root);
    break;

  case F_EXEC:
    {
      ScreenInfo *scr;

      if (FocusRoot != TRUE)
	/*
	 * f.focus / f.sloppyfocus:  Execute external program
	 * on the screen where the mouse is and not where
	 * the current X11-event occurred.  (The XGrabPointer()
	 * above should not 'confine_to' the mouse onto that
	 * screen as well.)
	 */
	scr = FindPointerScreenInfo();
      else
	scr = Scr;

      PopDownMenu();
      if (!scr->NoGrabServer)
      {
	XUngrabServer(dpy);
	XSync(dpy, False);
      }

#ifndef NO_SOUND_SUPPORT
      /* flag for the handler */
      if (PlaySound(func))
	createSoundFromFunction = TRUE;
#endif
      Execute(scr, action);
    }
    break;

  case F_UNFOCUS:
#ifdef TWM_USE_SLOPPYFOCUS
    SloppyFocus = FALSE;
#endif
    FocusOnRoot();
    break;

#ifdef TWM_USE_SLOPPYFOCUS
  case F_SLOPPYFOCUS:
    SloppyFocus = TRUE;
    FocusOnRoot();
    break;
#endif

  case F_CUT:
    strcpy(tmp, action);
    strcat(tmp, "\n");
    XStoreBytes(dpy, tmp, strlen(tmp));
    break;

  case F_CUTFILE:
    ptr = XFetchBytes(dpy, &count);
    if (ptr)
    {
      if (sscanf(ptr, "%s", tmp) == 1)
      {
	XFree(ptr);
	ptr = ExpandFilename(tmp);
	if (ptr)
	{
	  fd = open(ptr, 0);
	  if (fd >= 0)
	  {
	    count = read(fd, buff, MAX_FILE_SIZE - 1);
	    if (count > 0)
	      XStoreBytes(dpy, buff, count);
	    close(fd);
	  }
	  else
	  {
	    fprintf(stderr, "%s:  unable to open cut file \"%s\"\n", ProgramName, tmp);
	  }
	  if (ptr != tmp)
	    free(ptr);
	}
      }
      else
      {
	XFree(ptr);
      }
    }
    else
    {
      fprintf(stderr, "%s:  cut buffer is empty\n", ProgramName);
    }
    break;

  case F_WARPTOSCREEN:
    {
      ScreenInfo *scr;

      if (FocusRoot != TRUE)
      {
	/*
	 * f.focus / f.sloppyfocus is active:  KeyPress X11-events
	 * are delivered into the focused window probably on
	 * another screen, then "Scr" is not where the mouse is:
	 */
	scr = FindPointerScreenInfo();
#ifdef TWM_USE_SLOPPYFOCUS
	if (SloppyFocus == TRUE)
	  FocusOnRoot();	/* drop client focus before screen switch */
#endif
      }
      else
	scr = Scr;

      if (strcmp(action, WARPSCREEN_NEXT) == 0)
      {
	WarpToScreen(scr, scr->screen + 1, 1);
      }
      else if (strcmp(action, WARPSCREEN_PREV) == 0)
      {
	WarpToScreen(scr, scr->screen - 1, -1);
      }
      else if (strcmp(action, WARPSCREEN_BACK) == 0)
      {
	WarpToScreen(scr, PreviousScreen, 0);
      }
      else
      {
	WarpToScreen(scr, atoi(action), 0);
      }
    }
    break;

  case F_COLORMAP:
    {
      if (strcmp(action, COLORMAP_NEXT) == 0)
      {
	BumpWindowColormap(tmp_win, 1);
      }
      else if (strcmp(action, COLORMAP_PREV) == 0)
      {
	BumpWindowColormap(tmp_win, -1);
      }
      else
      {
	BumpWindowColormap(tmp_win, 0);
      }
    }
    break;

  case F_WARPCLASSNEXT:
  case F_WARPCLASSPREV:
    WarpClass(func == F_WARPCLASSNEXT, tmp_win, action);
    break;

  case F_WARPTONEWEST:
    if (Scr->Newest && warp_if_warpunmapped(Scr->Newest, F_NOFUNCTION))
    {
      RaiseStickyAbove();
      RaiseAutoPan();

#ifndef NO_SOUND_SUPPORT
      PlaySound(func);
#endif

      WarpToWindow(Scr->Newest);
    }
    else
      DoAudible();
    break;

  case F_WARPTO:
    {
      register TwmWindow *t;
      int did_warpto = FALSE;

      for (t = Scr->TwmRoot.next; t != NULL; t = t->next)
      {
	/*
	 * This used to fall through into F_WARP, but the
	 * warp_if_warpunmapped() meant this loop couldn't
	 * continue to look for a match in the window list.
	 */

	if (MatchWinName(action, t) == 0 && warp_if_warpunmapped(t, func))
	{
	  tmp_win = t;
	  RaiseStickyAbove();
	  RaiseAutoPan();

#ifndef NO_SOUND_SUPPORT
	  PlaySound(func);
#endif
	  did_warpto = TRUE;

	  WarpToWindow(tmp_win);
	  break;
	}
      }

      if (!did_warpto)
	DoAudible();
    }
    break;

  case F_WARP:
    {
      if (tmp_win && warp_if_warpunmapped(tmp_win, F_NOFUNCTION))
      {
	RaiseStickyAbove();
	RaiseAutoPan();

#ifndef NO_SOUND_SUPPORT
	PlaySound(func);
#endif

	WarpToWindow(tmp_win);
      }
      else
      {
	DoAudible();
      }
    }
    break;

  case F_WARPTOICONMGR:
    {
      TwmWindow *t;
      int len;

/*
 * raisewin now points to the window's icon manager entry, and
 * iconwin now points to raisewin's icon manager - djhjr - 5/30/00
 *
 */
      WList *raisewin = NULL;
      TwmWindow *iconwin = None;

      len = strlen(action);
      if (len == 0)
      {
	if (tmp_win && tmp_win->list)
	{
	  raisewin = tmp_win->list;
	}
	else if (Scr->iconmgr.active)
	{
	  raisewin = Scr->iconmgr.active;
	}
      }
      else
      {
	for (t = Scr->TwmRoot.next; t != NULL; t = t->next)
	{
	  if (strncmp(action, t->icon_name, len) == 0)
	  {
	    if (t->list && t->list->iconmgr->twm_win->mapped)
	    {

	      raisewin = t->list;
	      break;
	    }
	  }
	}
      }

      if (!raisewin)
      {
	DoAudible();
	break;
      }

      iconwin = raisewin->iconmgr->twm_win;

      if (iconwin && warp_if_warpunmapped(iconwin, F_NOFUNCTION))
      {
#ifndef NO_SOUND_SUPPORT
	PlaySound(func);
#endif

	WarpInIconMgr(raisewin, iconwin);
      }
      else
      {
	DoAudible();
      }
    }
    break;

  case F_SQUEEZELEFT:
    {
      static SqueezeInfo left_squeeze = { J_LEFT, 0, 0 };

      if (do_squeezetitle(context, func, tmp_win, &left_squeeze))
	return TRUE;		/* deferred */
    }
    break;

  case F_SQUEEZERIGHT:
    {
      static SqueezeInfo right_squeeze = { J_RIGHT, 0, 0 };

      if (do_squeezetitle(context, func, tmp_win, &right_squeeze))
	return TRUE;		/* deferred */
    }
    break;

  case F_SQUEEZECENTER:
    {
      static SqueezeInfo center_squeeze = { J_CENTER, 0, 0 };

      if (do_squeezetitle(context, func, tmp_win, &center_squeeze))
	return TRUE;		/* deferred */
    }
    break;

  case F_RING:
    if (DeferExecution(context, func, Scr->SelectCursor))
      return TRUE;
    if (tmp_win->ring.next || tmp_win->ring.prev)
      RemoveWindowFromRing(tmp_win);
    else
      AddWindowToRing(tmp_win);
    break;

  case F_WARPRING:
    switch (action[0])
    {
    case 'n':
      WarpAlongRing(&eventp->xbutton, True);
      break;
    case 'p':
      WarpAlongRing(&eventp->xbutton, False);
      break;
    default:
      DoAudible();
      break;
    }
    break;

  case F_FILE:
    action = ExpandFilename(action);
    fd = open(action, 0);
    if (fd >= 0)
    {
      count = read(fd, buff, MAX_FILE_SIZE - 1);
      if (count > 0)
	XStoreBytes(dpy, buff, count);

      close(fd);
    }
    else
    {
      fprintf(stderr, "%s:  unable to open file \"%s\"\n", ProgramName, action);
    }
    break;

  case F_REFRESH:
    {
      XSetWindowAttributes attributes;
      unsigned long valuemask;

      valuemask = (CWBackPixel | CWBackingStore | CWSaveUnder);
      attributes.background_pixel = Scr->Black;
      attributes.backing_store = NotUseful;
      attributes.save_under = False;
      w = XCreateWindow(dpy, Scr->Root, 0, 0,
			(unsigned int)Scr->MyDisplayWidth,
			(unsigned int)Scr->MyDisplayHeight,
			(unsigned int)0,
			CopyFromParent, (unsigned int)CopyFromParent, (Visual *) CopyFromParent, valuemask, &attributes);
      XMapWindow(dpy, w);
      XDestroyWindow(dpy, w);
      XFlush(dpy);
    }
    break;

  case F_WINREFRESH:
    if (DeferExecution(context, func, Scr->SelectCursor))
      return TRUE;

    if (context == C_ICON && tmp_win->icon_w.win)
      w = XCreateSimpleWindow(dpy, tmp_win->icon_w.win, 0, 0, 9999, 9999, 0, Scr->Black, Scr->Black);
    else
      w = XCreateSimpleWindow(dpy, tmp_win->frame, 0, 0, 9999, 9999, 0, Scr->Black, Scr->Black);

    XMapWindow(dpy, w);
    XDestroyWindow(dpy, w);
    XFlush(dpy);
    break;

  case F_NAIL:
    if (DeferExecution(context, func, Scr->SelectCursor))
      return TRUE;

    tmp_win->nailed = !tmp_win->nailed;
    /* update the vd display */
    NailDesktop(tmp_win);

#ifdef DEBUG
    fprintf(stdout, "%s:  nail state of %s is now %s\n", ProgramName, tmp_win->name, (tmp_win->nailed ? "nailed" : "free"));
#endif /* DEBUG */

    RaiseStickyAbove();
    RaiseAutoPan();

    break;

    /*
     * move a percentage in a particular direction
     */
  case F_PANDOWN:
    PanRealScreen(0, (atoi(action) * Scr->MyDisplayHeight) / 100, NULL, NULL);
    break;
  case F_PANLEFT:
    PanRealScreen(-((atoi(action) * Scr->MyDisplayWidth) / 100), 0, NULL, NULL);
    break;
  case F_PANRIGHT:
    PanRealScreen((atoi(action) * Scr->MyDisplayWidth) / 100, 0, NULL, NULL);
    break;
  case F_PANUP:
    PanRealScreen(0, -((atoi(action) * Scr->MyDisplayHeight) / 100), NULL, NULL);
    break;

  case F_RESETDESKTOP:
    SetRealScreen(0, 0);
    break;

    {
      TwmWindow *scan;
      int right, left, up, down;
      int inited;

  case F_SNUGDESKTOP:
      right = left = up = down = 0;

      inited = 0;
      for (scan = Scr->TwmRoot.next; scan != NULL; scan = scan->next)
      {
	if (scan->nailed)
	  continue;
	if (scan->frame_x > Scr->MyDisplayWidth || scan->frame_y > Scr->MyDisplayHeight)
	  continue;
	if (scan->frame_x + scan->frame_width < 0 || scan->frame_y + scan->frame_height < 0)
	  continue;
	if (inited == 0 || scan->frame_x < right)
	  right = scan->frame_x;
	if (inited == 0 || scan->frame_y < up)
	  up = scan->frame_y;
	if (inited == 0 || scan->frame_x + scan->frame_width > left)
	  left = scan->frame_x + scan->frame_width;
	if (inited == 0 || scan->frame_y + scan->frame_height > down)
	  down = scan->frame_y + scan->frame_height;
	inited = 1;
      }
      if (inited)
      {
	int dx, dy;

	if (left - right < Scr->MyDisplayWidth && (right < 0 || left > Scr->MyDisplayWidth))
	  dx = right - (Scr->MyDisplayWidth - (left - right)) / 2;
	else
	  dx = 0;
	if (down - up < Scr->MyDisplayHeight && (up < 0 || down > Scr->MyDisplayHeight))
	  dy = up - (Scr->MyDisplayHeight - (down - up)) / 2;
	else
	  dy = 0;
	if (dx != 0 || dy != 0)
	  PanRealScreen(dx, dy, NULL, NULL);

	else
	  DoAudible();
      }
      else
	DoAudible();
      break;

  case F_SNUGWINDOW:
      if (DeferExecution(context, func, Scr->SelectCursor))
	return TRUE;

      inited = 0;
      right = tmp_win->frame_x;
      left = tmp_win->frame_x + tmp_win->frame_width;
      up = tmp_win->frame_y;
      down = tmp_win->frame_y + tmp_win->frame_height;
      inited = 1;
      if (inited)
      {
	int dx, dy;

	dx = 0;
	if (left - right < Scr->MyDisplayWidth)
	{
	  if (right < 0)
	    dx = right;
	  else if (left > Scr->MyDisplayWidth)
	    dx = left - Scr->MyDisplayWidth;
	}

	dy = 0;
	if (down - up < Scr->MyDisplayHeight)
	{
	  if (up < 0)
	    dy = up;
	  else if (down > Scr->MyDisplayHeight)
	    dy = down - Scr->MyDisplayHeight;
	}

	if (dx != 0 || dy != 0)
	  PanRealScreen(dx, dy, NULL, NULL);

	else
	  DoAudible();
      }
      else
	DoAudible();

      break;
    }

  case F_BINDBUTTONS:
    {
      int i, j;

      if (DeferExecution(context, func, Scr->SelectCursor))
	return TRUE;
      for (i = 0; i < NumButtons + 1; i++)
	for (j = 0; j < MOD_SIZE; j++)
	  if (Scr->Mouse[MOUSELOC(i, C_WINDOW, j)].func != F_NOFUNCTION)
	    XGrabButton(dpy, i, j, tmp_win->frame,
			True, ButtonPressMask | ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None, Scr->FrameCursor);
      break;
    }
  case F_BINDKEYS:
    {
      FuncKey *tmp;

      if (DeferExecution(context, func, Scr->SelectCursor))
	return TRUE;
      for (tmp = Scr->FuncKeyRoot.next; tmp != NULL; tmp = tmp->next)
	if (tmp->cont == C_WINDOW)
	  GrabModKeys(tmp_win->w, tmp);
      break;
    }
  case F_UNBINDBUTTONS:
    {
      int i, j;

      if (DeferExecution(context, func, Scr->SelectCursor))
	return TRUE;
      for (i = 0; i < NumButtons + 1; i++)
	for (j = 0; j < MOD_SIZE; j++)
	  if (Scr->Mouse[MOUSELOC(i, C_WINDOW, j)].func != F_NOFUNCTION)
	    XUngrabButton(dpy, i, j, tmp_win->frame);
      break;
    }
  case F_UNBINDKEYS:
    {
      FuncKey *tmp;

      if (DeferExecution(context, func, Scr->SelectCursor))
	return TRUE;
      for (tmp = Scr->FuncKeyRoot.next; tmp != NULL; tmp = tmp->next)
	if (tmp->cont == C_WINDOW)
	  UngrabModKeys(tmp_win->w, tmp);
      break;
    }

  case F_MOVESCREEN:

    /*
     * Breaks badly if not called by the default button press.
     */

    {
      long releaseEvent = ButtonRelease;
      long movementMask = ButtonMotionMask;

#ifndef NO_SOUND_SUPPORT
      int did_playsound = FALSE;
#endif

      StartMoveWindowInDesktop(eventp->xmotion);

      while (TRUE)
      {
	XMaskEvent(dpy, ButtonPressMask | ButtonReleaseMask |
		   EnterWindowMask | LeaveWindowMask | ExposureMask | VisibilityChangeMask | movementMask, &Event);

	/*
	 * Don't discard exposure events before release
	 * or window borders and/or their titles in the
	 * virtual desktop won't get redrawn - djhjr
	 */

	/* discard any extra motion events before a release */
	if (Event.type == MotionNotify)
	{
	  while (XCheckMaskEvent(dpy, releaseEvent | movementMask, &Event))
	  {
	    if (Event.type == releaseEvent)
	      break;
	  }
	}

	if (Event.type == releaseEvent)
	{
	  EndMoveWindowOnDesktop();
	  break;
	}

	if (!DispatchEvent())
	  continue;

	if (Event.type != MotionNotify)
	  continue;

#ifndef NO_SOUND_SUPPORT
	if (did_playsound == FALSE)
	{
	  PlaySound(func);
	  did_playsound = TRUE;
	}
#endif

	DoMoveWindowOnDesktop(Event.xmotion.x, Event.xmotion.y);
      }

      /* discard queued enter and leave events */
      while (XCheckMaskEvent(dpy, EnterWindowMask | LeaveWindowMask, &Event))
	;

      /* will clear the XGrabPointer() in events.c:HandleButtonPress() */
      ButtonPressed = -1;

      break;
    }

  case F_SNAP:
    SnapRealScreen();
    /* and update the data structures */
    SetRealScreen(Scr->VirtualDesktopX, Scr->VirtualDesktopY);
    break;

  case F_SNAPREALSCREEN:
    Scr->snapRealScreen = !Scr->snapRealScreen;
    break;

  case F_STATICICONPOSITIONS:
    Scr->StaticIconPositions = !Scr->StaticIconPositions;
    break;

  case F_STRICTICONMGR:
    {
      TwmWindow *t;

      Scr->StrictIconManager = !Scr->StrictIconManager;
      if (Scr->StrictIconManager)
      {
	for (t = Scr->TwmRoot.next; t != NULL; t = t->next)
	  if (!t->icon)
	    RemoveIconManager(t);
      }
      else
      {
	for (t = Scr->TwmRoot.next; t != NULL; t = t->next)
	  if (!t->list)
	    AddIconManager(t);
      }

      break;
    }

  case F_SETREALSCREEN:
    {
      int newx = Scr->VirtualDesktopX;
      int newy = Scr->VirtualDesktopY;

      /* parse the geometry */
      JunkMask = XParseGeometry(action, &JunkX, &JunkY, &JunkWidth, &JunkHeight);

      if (JunkMask & XValue)
	newx = JunkX;
      if (JunkMask & YValue)
	newy = JunkY;

      if (newx < 0)
	newx = Scr->VirtualDesktopWidth + newx;
      if (newy < 0)
	newy = Scr->VirtualDesktopHeight + newy;

      SetRealScreen(newx, newy);

      break;
    }

  case F_HIDEDESKTOP:
    if (Scr->Virtual)
      XUnmapWindow(dpy, Scr->VirtualDesktopDisplayTwin->frame);
    break;

  case F_SHOWDESKTOP:
    if (Scr->Virtual)
    {
      XMapWindow(dpy, Scr->VirtualDesktopDisplayTwin->frame);

      if (Scr->VirtualDesktopDisplayTwin->icon)
	DeIconify(Scr->VirtualDesktopDisplayTwin);
    }
    break;

  case F_ENTERDOOR:
    {
      TwmDoor *d;

      if (XFindContext(dpy, tmp_win->w, DoorContext, (caddr_t *) & d) != XCNOENT)
	door_enter(tmp_win->w, d);
      break;
    }

  case F_DELETEDOOR:
    {
      TwmDoor *d;

      if (DeferExecution(context, func, Scr->DestroyCursor))
	return TRUE;

#ifndef NO_SOUND_SUPPORT
      /* flag for the handler */
      if (PlaySound(func))
	destroySoundFromFunction = TRUE;
#endif

      if (XFindContext(dpy, tmp_win->w, DoorContext, (caddr_t *) & d) != XCNOENT)
      {
	/* for some reason, we don't get the button up event - djhjr - 5/13/99 */
	ButtonPressed = -1;

	door_delete(tmp_win->w, d);
      }
      break;
    }

  case F_NEWDOOR:
    PopDownMenu();
    door_new();
    break;

  case F_NAMEDOOR:
    {
      TwmDoor *d;

      if (XFindContext(dpy, tmp_win->w, DoorContext, (caddr_t *) & d) != XCNOENT)
	door_paste_name(tmp_win->w, d);
      break;
    }

  case F_QUIT:

#ifndef NO_SOUND_SUPPORT
    if (PlaySound(func))
    {
      /* allow time to emit */
      if (Scr->PauseOnQuit)
	sleep(Scr->PauseOnQuit);
    }
    else
      PlaySoundDone();
#endif

    Done(0);
    break;

  case F_VIRTUALGEOMETRIES:
    Scr->GeometriesAreVirtual = !Scr->GeometriesAreVirtual;
    break;

  case F_WARPVISIBLE:
    Scr->WarpVisible = !Scr->WarpVisible;
    break;

  case F_WARPSNUG:
    Scr->WarpSnug = !Scr->WarpSnug;
    break;

#ifndef NO_SOUND_SUPPORT
  case F_SOUNDS:
    ToggleSounds();
    break;

  case F_PLAYSOUND:
    PlaySoundAdhoc(action);
    break;
#endif
  }

  if (ButtonPressed == -1)
    XUngrabPointer(dpy, CurrentTime);
  return do_next_action;
}



/***********************************************************************
 *
 *  Procedure:
 *	DeferExecution - defer the execution of a function to the
 *	    next button press if the context is C_ROOT
 *
 *  Inputs:
 *	context	- the context in which the mouse button was pressed
 *	func	- the function to defer
 *	cursor	- the cursor to display while waiting
 *
 ***********************************************************************
 */

int
DeferExecution(int context, int func, Cursor cursor)
{
  if (context == C_ROOT)
  {
    LastCursor = cursor;
    XGrabPointer(dpy, Scr->Root, True,
		 ButtonPressMask | ButtonReleaseMask, GrabModeAsync, GrabModeAsync, Scr->Root, cursor, CurrentTime);

    RootFunction = func;
    Action = actionHack;

    return (TRUE);
  }

  return (FALSE);
}



/***********************************************************************
 *
 *  Procedure:
 *	ReGrab - regrab the pointer with the LastCursor;
 *
 ***********************************************************************
 */

void
ReGrab(void)
{
  XGrabPointer(dpy, Scr->Root, True,
	       ButtonPressMask | ButtonReleaseMask, GrabModeAsync, GrabModeAsync, Scr->Root, LastCursor, CurrentTime);
}



/***********************************************************************
 *
 *  Procedure:
 *	NeedToDefer - checks each function in the list to see if it
 *		is one that needs to be defered.
 *
 *  Inputs:
 *	root	- the menu root to check
 *
 ***********************************************************************
 */

static Cursor
NeedToDefer(MenuRoot * root)
{
  MenuItem *mitem;

  for (mitem = root->first; mitem != NULL; mitem = mitem->next)
  {
    switch (mitem->func)
    {
    case F_RESIZE:
      return Scr->ResizeCursor;
    case F_MOVE:
    case F_FORCEMOVE:
      return Scr->MoveCursor;
    case F_DELETE:
    case F_DELETEDOOR:
    case F_DESTROY:
      return Scr->DestroyCursor;
    case F_IDENTIFY:
    case F_DEICONIFY:
    case F_ICONIFY:
    case F_RAISELOWER:
    case F_RAISE:
    case F_LOWER:
    case F_FOCUS:
    case F_WINREFRESH:
    case F_ZOOM:
    case F_FULLZOOM:
    case F_HORIZOOM:
    case F_RIGHTZOOM:
    case F_LEFTZOOM:
    case F_TOPZOOM:
    case F_BOTTOMZOOM:
    case F_AUTORAISE:
    case F_NAIL:
    case F_SNUGWINDOW:
      return Scr->SelectCursor;
    }
  }
  return None;
}



void
Execute(ScreenInfo * scr, char *s)
{
  static char buf[256];
  char *ds = DisplayString(dpy);
  char *colon, *dot1;
  char oldDisplay[256];
  char *doisplay;
  int restorevar = 0;

  char *append_this = " &";
  char *es = (char *)malloc(strlen(s) + strlen(append_this) + 1);

  sprintf(es, s);

  if (scr->EnhancedExecResources)
  {
    /* chop all space characters from the end of the string */
    while (isspace(es[strlen(es) - 1]))
    {
      es[strlen(es) - 1] = '\0';
    }
    switch (es[strlen(es) - 1])	/* last character */
    {
    case ';':
      es[strlen(es) - 1] = '\0';	/* remove the semicolon */
      break;
    case '&':			/* already there so do nothing */
      break;
    default:
      strcat(es, append_this);	/* don't block the window manager */
      break;
    }
  }

  oldDisplay[0] = '\0';
  doisplay = getenv("DISPLAY");
  if (doisplay)
    strcpy(oldDisplay, doisplay);

  /*
   * Build a display string using the current screen number, so that
   * X programs which get fired up from a menu come up on the screen
   * that they were invoked from, unless specifically overridden on
   * their command line.
   */
  colon = rindex(ds, ':');
  if (colon)
  {				/* if host[:]:dpy */
    strcpy(buf, "DISPLAY=");
    strcat(buf, ds);
    colon = buf + 8 + (colon - ds);	/* use version in buf */
    dot1 = index(colon, '.');	/* first period after colon */
    if (!dot1)
      dot1 = colon + strlen(colon);	/* if not there, append */
    (void)sprintf(dot1, ".%d", scr->screen);
    putenv(buf);
    restorevar = 1;
  }

  (void)system(es);
  free(es);

  if (restorevar)
  {				/* why bother? */
    (void)sprintf(buf, "DISPLAY=%s", oldDisplay);
    putenv(buf);
  }
}



/***********************************************************************
 *
 *  Procedure:
 *	FocusOnRoot - put input focus on the root window
 *
 ***********************************************************************
 */

void
FocusOnRoot(void)
{
  if (Focus != NULL)
  {
    SetBorder(Focus, False);
    PaintTitleHighlight(Focus, off);
  }
  InstallWindowColormaps(0, &Scr->TwmRoot);

  SetFocus((TwmWindow *) NULL, LastTimestamp());
  Focus = NULL;
  FocusRoot = TRUE;
}

void
FocusOnClient(TwmWindow * tmp_win)
{
  /* assign focus if 'Passive'/'Locally Active' ICCCM model: */
  if (!tmp_win->wmhints || tmp_win->wmhints->input)
  {
    if (Focus != NULL)
    {
      SetBorder(Focus, False);
      PaintTitleHighlight(Focus, off);
    }
    InstallWindowColormaps(0, tmp_win);
    SetBorder(tmp_win, True);
    PaintTitleHighlight(tmp_win, on);

    SetFocus(tmp_win, LastTimestamp());
    Focus = tmp_win;
    FocusRoot = FALSE;
  }
  else
    FocusOnRoot();
}

void
DeIconify(TwmWindow * tmp_win)
{
  TwmWindow *t;

  /*
   * De-iconify the main window first
   */

  if (Scr->DoZoom && Scr->ZoomCount > 0)
  {
    IconMgr *ipf = NULL;
    Window wt = None, wf = None;

    if (tmp_win->icon)
    {
      if (tmp_win->icon_on)
      {
	wf = tmp_win->icon_w.win;
	wt = tmp_win->frame;
      }
      else if (tmp_win->list)
      {
	wf = tmp_win->list->w.win;
	wt = tmp_win->frame;
	ipf = tmp_win->list->iconmgr;
      }
      else if (tmp_win->group != None)
      {
	for (t = Scr->TwmRoot.next; t != NULL; t = t->next)
	  if (tmp_win->group == t->w)
	  {
	    if (t->icon_on)
	      wf = t->icon_w.win;
	    else if (t->list)
	    {
	      wf = t->list->w.win;
	      ipf = t->list->iconmgr;
	    }

	    wt = tmp_win->frame;
	    break;
	  }
      }
    }

    if (Scr->ZoomZoom || (wf != None && wt != None))
      Zoom(wf, ipf, wt, NULL);
  }

  XMapWindow(dpy, tmp_win->w);
  tmp_win->mapped = TRUE;

  if (Scr->NoRaiseDeicon)
    XMapWindow(dpy, tmp_win->frame);
  else
  {
    XMapRaised(dpy, tmp_win->frame);
    XRaiseWindow(dpy, tmp_win->VirtualDesktopDisplayWindow.win);
  }
  SetMapStateProp(tmp_win, NormalState);

  if (tmp_win->icon_w.win)
  {
    XUnmapWindow(dpy, tmp_win->icon_w.win);
    IconDown(tmp_win);
  }

  tmp_win->icon = FALSE;
  tmp_win->icon_on = FALSE;

  /* Force the windows onto this virtual screen */
  if (Scr->DeIconifyToScreen)
  {
    int noraisemove = Scr->NoRaiseMove;

    Scr->NoRaiseMove = 1;
    if (tmp_win->frame_x < 0 || tmp_win->frame_x >= Scr->MyDisplayWidth)
    {
      tmp_win->frame_x = tmp_win->virtual_frame_x % Scr->MyDisplayWidth;
      if (tmp_win->frame_x + tmp_win->frame_width > Scr->MyDisplayWidth)
	tmp_win->frame_x = Scr->MyDisplayWidth - tmp_win->frame_width;
      if (tmp_win->frame_x < 0)
	tmp_win->frame_x = 0;
    }
    if (tmp_win->frame_y < 0 || tmp_win->frame_y >= Scr->MyDisplayHeight)
    {
      tmp_win->frame_y = tmp_win->virtual_frame_y % Scr->MyDisplayHeight;
      if (tmp_win->frame_y + tmp_win->frame_height > Scr->MyDisplayHeight)
	tmp_win->frame_y = Scr->MyDisplayHeight - tmp_win->frame_height;
      if (tmp_win->frame_y < 0)
	tmp_win->frame_y = 0;
    }
    VirtualMoveWindow(tmp_win, tmp_win->frame_x + Scr->VirtualDesktopX, tmp_win->frame_y + Scr->VirtualDesktopY);
    Scr->NoRaiseMove = noraisemove;
  }

  if (tmp_win->list)
    XUnmapWindow(dpy, tmp_win->list->icon);

  UpdateDesktop(tmp_win);

  /*
   * Now de-iconify transients
   */

  for (t = Scr->TwmRoot.next; t != NULL; t = t->next)
  {
    if (t->transient && t->transientfor == tmp_win->w)
    {
      if (Scr->DontDeiconifyTransients && t->icon_w.win && t->icon == TRUE && t->icon_on == FALSE)
      {
	IconUp(t);
	XMapRaised(dpy, t->icon_w.win);
	t->icon_on = TRUE;
      }
      else
      {
	if (t->icon_on)
	  Zoom(t->icon_w.win, NULL, t->frame, NULL);
	else
	  Zoom(tmp_win->icon_w.win, NULL, t->frame, NULL);

	XMapWindow(dpy, t->w);
	t->mapped = TRUE;

	if (Scr->NoRaiseDeicon)
	  XMapWindow(dpy, t->frame);
	else
	{
	  XMapRaised(dpy, t->frame);
	  XRaiseWindow(dpy, t->VirtualDesktopDisplayWindow.win);
	}
	SetMapStateProp(t, NormalState);

	if (t->icon_w.win)
	{
	  XUnmapWindow(dpy, t->icon_w.win);
	  IconDown(t);
	}

	t->icon = FALSE;
	t->icon_on = FALSE;

	if (t->list)
	  XUnmapWindow(dpy, t->list->icon);

	UpdateDesktop(t);
      }
    }
  }

  RaiseStickyAbove();
  RaiseAutoPan();

#ifdef TWM_USE_SLOPPYFOCUS
  if (SloppyFocus == TRUE || FocusRoot == TRUE)
#else
  if (FocusRoot == TRUE)	/* only warp if f.focus is not active */
#endif
    if (((Scr->WarpCursor ||
	  LookInList(Scr->WarpCursorL, tmp_win->full_name, &tmp_win->class)) && tmp_win->icon) && Scr->WarpWindows)
      WarpToWindow(tmp_win);

  XSync(dpy, 0);
}



void
Iconify(TwmWindow * tmp_win, int def_x, int def_y)
{
  TwmWindow *t;
  int iconify;
  XWindowAttributes winattrs;
  unsigned long eventMask;
  short fake_icon;

  iconify = ((!tmp_win->iconify_by_unmapping) || tmp_win->transient);
  if (iconify)
  {
    if (tmp_win->icon_w.win == None)
      CreateIconWindow(tmp_win, def_x, def_y);
    else
      IconUp(tmp_win);

    XMapRaised(dpy, tmp_win->icon_w.win);

    RaiseStickyAbove();
    RaiseAutoPan();
  }

  XGetWindowAttributes(dpy, tmp_win->w, &winattrs);
  eventMask = winattrs.your_event_mask;

  /*
   * Iconify transients first
   */

  for (t = Scr->TwmRoot.next; t != NULL; t = t->next)
  {
    if (t->transient && t->transientfor == tmp_win->w)
    {

      /*
       * Prevent the receipt of an UnmapNotify, since that would
       * cause a transition to the Withdrawn state.
       */
      t->mapped = FALSE;
      XSelectInput(dpy, t->w, eventMask & ~StructureNotifyMask);
      XUnmapWindow(dpy, t->w);
      XSelectInput(dpy, t->w, eventMask);
      XUnmapWindow(dpy, t->frame);

      if (iconify)
      {
	if (t->icon_on)
	  Zoom(t->icon_w.win, NULL, tmp_win->icon_w.win, NULL);
	else
	  Zoom(t->frame, NULL, tmp_win->icon_w.win, NULL);
      }

      if (t->icon_w.win)
	XUnmapWindow(dpy, t->icon_w.win);

      SetMapStateProp(t, IconicState);

      if (t == Focus)
	FocusOnRoot();

      /*
       * let current status ride, but "fake out" UpdateDesktop()
       * (see also DeIconify()) - djhjr - 6/22/99
       */
      fake_icon = t->icon;

      t->icon = TRUE;
      t->icon_on = FALSE;

      if (Scr->StrictIconManager)
	if (!t->list)
	  AddIconManager(t);

      if (t->list)
	XMapWindow(dpy, t->list->icon);

      UpdateDesktop(t);

      /* restore icon status - djhjr - 6/22/99 */
      t->icon = fake_icon;
    }
  }

  /*
   * Now iconify the main window
   */

  /*
   * Prevent the receipt of an UnmapNotify, since that would
   * cause a transition to the Withdrawn state.
   */
  tmp_win->mapped = FALSE;
  XSelectInput(dpy, tmp_win->w, eventMask & ~StructureNotifyMask);
  XUnmapWindow(dpy, tmp_win->w);
  XSelectInput(dpy, tmp_win->w, eventMask);
  XUnmapWindow(dpy, tmp_win->frame);

  SetMapStateProp(tmp_win, IconicState);

  if (tmp_win == Focus)
    FocusOnRoot();

  tmp_win->icon = TRUE;
  if (iconify)
    tmp_win->icon_on = TRUE;
  else
    tmp_win->icon_on = FALSE;

  if (Scr->StrictIconManager)
    if (!tmp_win->list)
      AddIconManager(tmp_win);

  if (iconify)
    Zoom(tmp_win->frame, NULL, tmp_win->icon_w.win, NULL);
  else if (tmp_win->list)
    Zoom(tmp_win->frame, NULL, tmp_win->list->w.win, tmp_win->list->iconmgr);

  if (tmp_win->list)
    XMapWindow(dpy, tmp_win->list->icon);

  UpdateDesktop(tmp_win);
  XSync(dpy, 0);
}



static void
Identify(TwmWindow * t)
{
  int i, n, twidth, width, height;
  int x, y;
  unsigned int wwidth, wheight, bw, depth;
  Window junk;
  int px, py, dummy;
  unsigned udummy;

#ifndef NO_SOUND_SUPPORT
  PlaySound(F_IDENTIFY);
#endif

  n = 0;
  (void)sprintf(Info[n++], "%s", Version);
  Info[n++][0] = '\0';

  if (t)
  {
    XGetGeometry(dpy, t->w, &JunkRoot, &JunkX, &JunkY, &wwidth, &wheight, &bw, &depth);
    (void)XTranslateCoordinates(dpy, t->w, Scr->Root, 0, 0, &x, &y, &junk);

    (void)sprintf(Info[n++], "Name:  \"%s\"", t->full_name);
    (void)sprintf(Info[n++], "Class.res_name:  \"%s\"", t->class.res_name);
    (void)sprintf(Info[n++], "Class.res_class:  \"%s\"", t->class.res_class);
    (void)sprintf(Info[n++], "Icon name:  \"%s\"", t->icon_name);
    Info[n++][0] = '\0';
    (void)sprintf(Info[n++], "Geometry/root:  %dx%d+%d+%d", wwidth, wheight, x, y);
    (void)sprintf(Info[n++], "Border width:  %d", bw);
    (void)sprintf(Info[n++], "Depth:  %d", depth);

    Info[n++][0] = '\0';
  }

#ifndef NO_BUILD_INFO
  else
  {
    char is_m4, is_xpm;
    char is_rplay;
    char is_regex;


    /*
     * Was a 'do ... while()' that accessed unallocated memory.
     */
    i = 0;
    while (lastmake[i][0] != '\0')
      (void)sprintf(Info[n++], "%s", lastmake[i++]);

#ifdef NO_M4_SUPPORT
    is_m4 = '-';
#else
    is_m4 = '+';
#endif
#ifdef NO_XPM_SUPPORT
    is_xpm = '-';
#else
    is_xpm = '+';
#endif
#ifdef NO_SOUND_SUPPORT
    is_rplay = '-';
#else
    is_rplay = '+';
#endif
#ifdef NO_REGEX_SUPPORT
    is_regex = '-';
#else
    is_regex = '+';
#endif
    (void)sprintf(Info[n++], "Options:  %cm4 %cregex %crplay %cxpm", is_m4, is_regex, is_rplay, is_xpm);

    Info[n++][0] = '\0';
  }
#endif

  (void)sprintf(Info[n++], "Click to dismiss...");

  /* figure out the width and height of the info window */

#ifdef TWM_USE_SPACING
  height = n * (120 * Scr->InfoFont.height / 100);	/*baselineskip 1.2 */
  height += Scr->InfoFont.height - Scr->InfoFont.y;
#else
  i = (Scr->InfoBevelWidth > 0) ? Scr->InfoBevelWidth + 8 : 10;
  height = (n * (Scr->InfoFont.height + 2)) + i;	/* some padding */
#endif

  width = 1;
  for (i = 0; i < n; i++)
  {
    twidth = MyFont_TextWidth(&Scr->InfoFont, Info[i], strlen(Info[i]));
    if (twidth > width)
      width = twidth;
  }
  if (InfoLines)
    XUnmapWindow(dpy, Scr->InfoWindow.win);

#ifdef TWM_USE_SPACING
  i = Scr->InfoFont.height;
#else
  i = (Scr->InfoBevelWidth > 0) ? Scr->InfoBevelWidth + 18 : 20;
#endif
  width += i;			/* some padding */

  if (XQueryPointer(dpy, Scr->Root, &JunkRoot, &JunkChild, &px, &py, &dummy, &dummy, &udummy))
  {
    px -= (width / 2);
    py -= (height / 3);

    if (Scr->InfoBevelWidth > 0)
      dummy = 2 * Scr->InfoBevelWidth;
    else
      dummy = 2 * Scr->BorderWidth;

#ifdef TILED_SCREEN
    if (Scr->use_tiles == TRUE)
    {
      i = FindNearestTileToMouse();
      EnsureRectangleOnTile(i, &px, &py, width + dummy, height + dummy);
    }
    else
#endif
    {
      if (px + width + dummy > Scr->MyDisplayWidth)
	px = Scr->MyDisplayWidth - width - dummy;
      if (px < 0)
	px = 0;
      if (py + height + dummy > Scr->MyDisplayHeight)
	py = Scr->MyDisplayHeight - height - dummy;
      if (py < 0)
	py = 0;
    }
  }
  else
  {
#ifdef TILED_SCREEN
    if (Scr->use_tiles == TRUE)
    {
      /* emergency: mouse not on current X11-screen */
      px = Lft(Scr->tiles[0]);
      py = Bot(Scr->tiles[0]);
    }
    else
#endif
      px = py = 0;
  }

  XMoveResizeWindow(dpy, Scr->InfoWindow.win, px, py, width, height);


  XMapRaised(dpy, Scr->InfoWindow.win);
  InfoLines = n;
}



void
SetMapStateProp(TwmWindow * tmp_win, int state)
{
  unsigned long data[2];	/* "suggested" by ICCCM version 1 */

  data[0] = (unsigned long)state;
  data[1] = (unsigned long)(tmp_win->iconify_by_unmapping ? None : tmp_win->icon_w.win);

  XChangeProperty(dpy, tmp_win->w, _XA_WM_STATE, _XA_WM_STATE, 32, PropModeReplace, (unsigned char *)data, 2);
}



Bool
GetWMState(Window w, int *statep, Window * iwp)
{
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytesafter;
  unsigned long *datap = NULL;
  Bool retval = False;

  /* used to test for '!datap' - djhjr - 1/10/98 */
  if (XGetWindowProperty(dpy, w, _XA_WM_STATE, 0L, 2L, False, _XA_WM_STATE,
			 &actual_type, &actual_format, &nitems, &bytesafter,
			 (unsigned char **)&datap) != Success || actual_type == None)
    return False;

  if (nitems <= 2)
  {				/* "suggested" by ICCCM version 1 */
    *statep = (int)datap[0];
    *iwp = (Window) datap[1];
    retval = True;
  }

  XFree((char *)datap);
  return retval;
}



/*
 * BumpWindowColormap - rotate our internal copy of WM_COLORMAP_WINDOWS
 */

static void
BumpWindowColormap(TwmWindow * tmp, int inc)
{
  int i, j, previously_installed;
  ColormapWindow **cwins;

  if (!tmp)
    return;

  if (inc && tmp->cmaps.number_cwins > 0)
  {
    cwins = (ColormapWindow **) malloc(sizeof(ColormapWindow *) * tmp->cmaps.number_cwins);
    if (cwins)
    {
      if ((previously_installed = (Scr->cmapInfo.cmaps == &tmp->cmaps) && tmp->cmaps.number_cwins))
      {
	for (i = tmp->cmaps.number_cwins; i-- > 0;)
	  tmp->cmaps.cwins[i]->colormap->state = 0;
      }

      for (i = 0; i < tmp->cmaps.number_cwins; i++)
      {
	j = i - inc;
	if (j >= tmp->cmaps.number_cwins)
	  j -= tmp->cmaps.number_cwins;
	else if (j < 0)
	  j += tmp->cmaps.number_cwins;
	cwins[j] = tmp->cmaps.cwins[i];
      }

      free((char *)tmp->cmaps.cwins);

      tmp->cmaps.cwins = cwins;

      if (tmp->cmaps.number_cwins > 1)
	memset(tmp->cmaps.scoreboard, 0, ColormapsScoreboardLength(&tmp->cmaps));

      if (previously_installed)
	InstallWindowColormaps(PropertyNotify, (TwmWindow *) NULL);
    }
  }
  else
    FetchWmColormapWindows(tmp);
}



static void
HideIconManager(TwmWindow * tmp_win)
{
  if (tmp_win == NULL)
  {
    name_list *list;

    HideIconMgr(&Scr->iconmgr);

    for (list = Scr->IconMgrs; list != NULL; list = next_entry(list))
      HideIconMgr((IconMgr *) contents_of_entry(list));
  }
  else
  {
    IconMgr *ip;

    if ((ip = (IconMgr *) LookInList(Scr->IconMgrs, tmp_win->full_name, &tmp_win->class)) == NULL)
      ip = &Scr->iconmgr;

    HideIconMgr(ip);
  }
}

void
HideIconMgr(IconMgr * ip)
{
  if (ip->count == 0)
    return;

  SetMapStateProp(ip->twm_win, WithdrawnState);
  XUnmapWindow(dpy, ip->twm_win->frame);
  if (ip->twm_win->icon_w.win)
    XUnmapWindow(dpy, ip->twm_win->icon_w.win);
  ip->twm_win->mapped = FALSE;
  ip->twm_win->icon = TRUE;
}

void
ShowIconMgr(IconMgr * ip)
{
  if (Scr->NoIconManagers || ip->count == 0)
    return;

  DeIconify(ip->twm_win);
  XRaiseWindow(dpy, ip->twm_win->frame);
  XRaiseWindow(dpy, ip->twm_win->VirtualDesktopDisplayWindow.win);
}


void
SetBorder(TwmWindow * tmp, Bool onoroff)
{
  if (tmp->highlight)
  {
    if (Scr->BorderBevelWidth > 0)
      PaintBorders(tmp, onoroff);
    else
    {
      if (onoroff)
      {
	XSetWindowBorder(dpy, tmp->frame, tmp->border.back);

	if (tmp->title_w.win)
	  XSetWindowBorder(dpy, tmp->title_w.win, tmp->border.back);
      }
      else
      {
	XSetWindowBorderPixmap(dpy, tmp->frame, tmp->gray);

	if (tmp->title_w.win)
	  XSetWindowBorderPixmap(dpy, tmp->title_w.win, tmp->gray);
      }
    }

    if (tmp->titlebuttons)
    {
      int i, nb = Scr->TBInfo.nleft + Scr->TBInfo.nright;
      TBWindow *tbw;

      for (i = 0, tbw = tmp->titlebuttons; i < nb; i++, tbw++)
	PaintTitleButton(tmp, tbw, (onoroff) ? 2 : 1);
    }
  }
}



static void
DestroyMenu(MenuRoot * menu)
{
  MenuItem *item;

  if (menu->w.win)
  {
#ifdef TWM_USE_XFT
    if (Scr->use_xft > 0)
      MyXftDrawDestroy(menu->w.xft);
#endif
    XDeleteContext(dpy, menu->w.win, MenuContext);
    XDeleteContext(dpy, menu->w.win, ScreenContext);
    if (Scr->Shadow)
      XDestroyWindow(dpy, menu->shadow);
    XDestroyWindow(dpy, menu->w.win);
  }

  for (item = menu->first; item;)
  {
    MenuItem *tmp = item;

    item = item->next;
    free((char *)tmp);
  }
}



/*
 * warping routines
 */

/* for moves and resizes from center - djhjr - 10/4/02 */
static void
WarpScreenToWindow(TwmWindow * t)
{
  int warpwin = Scr->WarpWindows;
  int warpsnug = Scr->WarpSnug;

  Scr->WarpWindows = Scr->WarpSnug = FALSE;
  WarpToWindow(t);
  Scr->WarpWindows = warpwin;
  Scr->WarpSnug = warpsnug;

  /*
   * This is an attempt to have windows redraw themselves, but
   * it doesn't always work (non-raising windows in particular).
   */
  XSync(dpy, 0);
}

void
WarpWindowOrScreen(TwmWindow * t)
{

  /*
   * we are either moving the window onto the screen, or the screen to the
   * window, the distances remain the same
   */

  if ((t->frame_x < Scr->MyDisplayWidth)
      && (t->frame_y < Scr->MyDisplayHeight) && (t->frame_x + t->frame_width >= 0) && (t->frame_y + t->frame_height >= 0))
  {

    /*
     *      window is visible; you can simply
     *      snug it if WarpSnug or WarpWindows is set -- DSE
     */

    if (Scr->WarpSnug || Scr->WarpWindows)
    {
      int right, left, up, down, dx, dy;

      /*
       * Adjustment for border widths submitted by Steve Ratcliffe
       * Note: Do not include the 3D border width!
       */
      right = t->frame_x;
      left = t->frame_x + t->frame_width + 2 * t->frame_bw;
      up = t->frame_y;
      down = t->frame_y + t->frame_height + 2 * t->frame_bw;

      dx = 0;
      if (left - right < Scr->MyDisplayWidth)
      {
	if (right < 0)
	  dx = right;
	else if (left > Scr->MyDisplayWidth)
	  dx = left - Scr->MyDisplayWidth;
      }

      dy = 0;
      if (down - up < Scr->MyDisplayHeight)
      {
	if (up < 0)
	  dy = up;
	else if (down > Scr->MyDisplayHeight)
	  dy = down - Scr->MyDisplayHeight;
      }

      if (dx != 0 || dy != 0)
      {
	if (Scr->WarpSnug || Scr->WarpWindows)
	{
	  /* move the window */
	  VirtualMoveWindow(t, t->virtual_frame_x - dx, t->virtual_frame_y - dy);
	}
	else
	{
	  /* move the screen */
	  PanRealScreen(dx, dy, NULL, NULL);
	}
      }
    }
  }
  else
  {

    /*
     *      Window is invisible; we need to move it or the screen.
     */

    int xdiff, ydiff;

    xdiff = ((Scr->MyDisplayWidth - t->frame_width) / 2) - t->frame_x;
    ydiff = ((Scr->MyDisplayHeight - t->frame_height) / 2) - t->frame_y;

    if (Scr->WarpSnug || Scr->WarpWindows)
    {
      /* move the window */
      VirtualMoveWindow(t, t->virtual_frame_x + xdiff, t->virtual_frame_y + ydiff);
    }
    else
    {
      /* move the screen */
      PanRealScreen(-xdiff, -ydiff, NULL, NULL);
    }
  }

  if (t->auto_raise || !Scr->NoRaiseWarp)
    AutoRaiseWindow(t);
}

/* for icon manager management - djhjr - 5/30/00 */
void
WarpInIconMgr(WList * w, TwmWindow * t)
{
  int x, y, pan_margin = 0;
  int bw = t->frame_bw3D + t->frame_bw;

  RaiseStickyAbove();
  RaiseAutoPan();

  WarpWindowOrScreen(t);

  x = w->x + bw + EDGE_OFFSET + 5;
  x += (Scr->IconMgrBevelWidth > 0) ? Scr->IconMgrBevelWidth : bw;
  y = w->y + bw + w->height / 2;
  y += (Scr->IconMgrBevelWidth > 0) ? Scr->IconMgrBevelWidth : bw;
  y += w->iconmgr->twm_win->title_height;

  /*
   * adjust the pointer for partially visible windows and the
   * AutoPan border width
   */

  if (Scr->AutoPanX)
    pan_margin = Scr->AutoPanBorderWidth;

  if (x + t->frame_x >= Scr->MyDisplayWidth - pan_margin)
    x = Scr->MyDisplayWidth - t->frame_x - pan_margin - 2;
  if (x + t->frame_x <= pan_margin)
    x = -t->frame_x + pan_margin + 2;
  if (y + t->frame_y >= Scr->MyDisplayHeight - pan_margin)
    y = Scr->MyDisplayHeight - t->frame_y - pan_margin - 2;
  if (y + t->frame_y <= pan_margin)
    y = -t->frame_y + pan_margin + 2;

  XWarpPointer(dpy, None, t->frame, 0, 0, 0, 0, x, y);
}

static void
WarpClass(int next, TwmWindow * t, char *class)
{
  TwmWindow *tt;
  XClassHint ch;
  int i;

  /*
   * if an empty class string
   *     if a TwmWindow
   *         class = the TwmWindow's class
   *     else if a window with focus
   *         if it's classed
   *             class = the focused window's class
   *     else
   *         return
   * if still an empty class string
   *     class = "VTWM"
   */
  if (!strlen(class))
  {
    if (t)
      class = t->class.res_class;
    else if (Focus)
    {
      i = XGetClassHint(dpy, Focus->w, &ch);
      if (i && !strncmp(class, ch.res_class, strlen(class)))
	class = ch.res_class;
    }
    else
    {
      DoAudible();
      return;
    }
  }
  if (!strlen(class) || !strncmp(class, "VTWM", 4))
    class = "VTWM";

  while (1)
  {

    tt = NULL;
    do
    {
      if ((tt = next_by_class(next, t, class)))
      {
	/* multiple icon managers: gotta test for those without entries */
	if (tt->iconmgr && tt->iconmgrp->count == 0)
	{
	  t = tt;
	  tt = NULL;
	}
      }
      else if (t)
	tt = t;
      else
	break;
    } while (!tt);

    if (tt && warp_if_warpunmapped(tt, (next) ? F_WARPCLASSNEXT : F_WARPCLASSPREV))
    {
      RaiseStickyAbove();
      RaiseAutoPan();

#ifndef NO_SOUND_SUPPORT
      PlaySound((next) ? F_WARPCLASSNEXT : F_WARPCLASSPREV);
#endif

      WarpToWindow(tt);

      break;
    }
    t = tt;
  }				/* while (1) */
}



void
AddWindowToRing(TwmWindow * tmp_win)
{
  if (Scr->Ring)
  {
    /* link window in after Scr->Ring */
    tmp_win->ring.prev = Scr->Ring;
    tmp_win->ring.next = Scr->Ring->ring.next;

    /* Scr->Ring's next's prev points to this */
    Scr->Ring->ring.next->ring.prev = tmp_win;

    /* Scr->Ring's next points to this */
    Scr->Ring->ring.next = tmp_win;
  }
  else
    tmp_win->ring.next = tmp_win->ring.prev = Scr->Ring = tmp_win;
}

void
RemoveWindowFromRing(TwmWindow * tmp_win)
{
  /* unlink window */
  if (tmp_win->ring.prev)
    tmp_win->ring.prev->ring.next = tmp_win->ring.next;
  if (tmp_win->ring.next)
    tmp_win->ring.next->ring.prev = tmp_win->ring.prev;

  /*  if window was only thing in ring, null out ring */
  if (Scr->Ring == tmp_win)
    Scr->Ring = (tmp_win->ring.next != tmp_win) ? tmp_win->ring.next : (TwmWindow *) NULL;

  /* if window was ring leader, set to next (or null) */
  if (!Scr->Ring || Scr->RingLeader == tmp_win)
    Scr->RingLeader = Scr->Ring;

  tmp_win->ring.next = tmp_win->ring.prev = NULL;
}

void
WarpAlongRing(XButtonEvent * ev, Bool forward)
{
  TwmWindow *r, *head;

  /*
   * Re-vamped much of this to properly handle icon managers
   */

  if (!(head = (Scr->RingLeader) ? Scr->RingLeader : Scr->Ring))
  {
    DoAudible();
    return;
  }

  if (forward)
    r = head->ring.next;
  else
    r = head->ring.prev;

  while (r && r != head)
  {
    if (r->mapped || warp_if_warpunmapped(r, F_WARPRING))
      break;

    r = (forward) ? r->ring.next : r->ring.prev;
  }

  if (r && r->mapped)
  {

    RaiseStickyAbove();
    RaiseAutoPan();

#ifndef NO_SOUND_SUPPORT
    PlaySound(F_WARPRING);
#endif

    WarpToWindow(r);

  }
  else
    DoAudible();
}



static void
WarpToScreen(ScreenInfo * scr, int n, int inc)
{
  Window dumwin;
  int x, y, dumint;
  unsigned int dummask;
  ScreenInfo *newscr = NULL;

  while (!newscr)
  {
    /* wrap around */
    if (n < 0)
      n = NumScreens - 1;
    else if (n >= NumScreens)
      n = 0;

    newscr = ScreenList[n];
    if (!newscr)
    {				/* make sure screen is managed */
      if (inc)
      {				/* walk around the list */
	n += inc;
	continue;
      }
      fprintf(stderr, "%s:  unable to warp to unmanaged screen %d\n", ProgramName, n);
      DoAudible();
      return;
    }
  }

  if (scr->screen == n)
    return;			/* already on that screen */

  PreviousScreen = scr->screen;
  XQueryPointer(dpy, scr->Root, &dumwin, &dumwin, &x, &y, &dumint, &dumint, &dummask);

#ifndef NO_SOUND_SUPPORT
  PlaySound(F_WARPTOSCREEN);
#endif

  XWarpPointer(dpy, None, newscr->Root, 0, 0, 0, 0, x, y);
}



void
WarpToWindow(TwmWindow * t)
{
  int x, y;
  int pan_margin = 0;
  int bw = t->frame_bw3D + t->frame_bw;
  Window w = t->frame;

  WarpWindowOrScreen(t);

  if (t->iconmgr && t->iconmgrp->count > 0)
  {
    w = t->iconmgrp->twm_win->frame;

    x = t->iconmgrp->x + bw + EDGE_OFFSET + 5;
    x += (Scr->IconMgrBevelWidth > 0) ? Scr->IconMgrBevelWidth : bw;
    y = t->iconmgrp->y + bw + t->iconmgrp->first->height / 2;
    y += (Scr->IconMgrBevelWidth > 0) ? Scr->IconMgrBevelWidth : bw;
    y += t->iconmgrp->twm_win->title_height;
  }
  else if (!t->title_w.win)
  {
    if (Scr->WarpCentered & WARPC_UNTITLED)
    {
      x = t->frame_width / 2;
      y = t->frame_height / 2;
    }
    else
    {
      x = t->frame_width / 2;
      y = (t->wShaped) ? bw : bw / 2;
    }
  }
  else
  {
    if (Scr->WarpCentered & WARPC_TITLED)
    {
      x = t->frame_width / 2;
      y = t->frame_height / 2;
    }
    else
    {
      /*
       * Added 't->title_x + ' to handle titlebars that
       * aren't flush left.
       */
      x = t->title_x + t->title_width / 2 + bw;
      y = t->title_height / 2 + bw;
    }
  }

  if (!Scr->BorderBevelWidth > 0)
    y -= t->frame_bw;

  /*
   * adjust the pointer for partially visible windows and the
   * AutoPan border width - djhjr - 5/30/00
   */

  if (Scr->AutoPanX)
    pan_margin = Scr->AutoPanBorderWidth;

  if (x + t->frame_x >= Scr->MyDisplayWidth - pan_margin)
    x = Scr->MyDisplayWidth - t->frame_x - pan_margin - 2;
  if (x + t->frame_x <= pan_margin)
  {
    if (t->title_w.win)
      x = t->title_width - (t->frame_x + t->title_width) + pan_margin + 2;
    else
      x = -t->frame_x + pan_margin + 2;
  }

  if (t->title_w.win && !(Scr->WarpCentered & WARPC_TITLED) && (x < t->title_x || x > t->title_x + t->title_width))
  {
    y = t->title_height + bw / 2;
  }
  if (y + t->frame_y >= Scr->MyDisplayHeight - pan_margin)
  {
    y = Scr->MyDisplayHeight - t->frame_y - pan_margin - 2;

    /* move centered warp to titlebar - djhjr - 10/16/02 */
    if (y < t->title_y + t->title_height)
      x = t->title_x + t->title_width / 2 + bw;
  }
  if (y + t->frame_y <= pan_margin)
    y = -t->frame_y + pan_margin + 2;

  XWarpPointer(dpy, None, w, 0, 0, 0, 0, x, y);

  if (t->ring.next)
    Scr->RingLeader = t;
}



/*
 * substantially re-written and added receiving and using 'next'
 */
static TwmWindow *
next_by_class(int next, TwmWindow * t, char *class)
{
  static TwmWindow *tp = NULL;
  TwmWindow *tt, *tl;
  int i, len = strlen(class);
  XClassHint ch;

#ifdef DEBUG_WARPCLASS
  fprintf(stderr, "class=\"%s\", next=%d, %s t, ", class, next, (t) ? "have" : "no");
#endif

  /* forward or backward from current */
  tl = (next) ? ((tp) ? tp->next : Scr->TwmRoot.next) : ((tp) ? tp->prev : Scr->TwmRoot.prev);
  for (tt = (next) ? ((t) ? t->next : tl) : ((t) ? t->prev : tl); tt != NULL; tt = (next) ? tt->next : tt->prev)
    if (Scr->WarpUnmapped || tt->mapped)
    {
      i = XGetClassHint(dpy, tt->w, &ch);
      if (i && !strncmp(class, ch.res_class, len))
      {
#ifdef DEBUG_WARPCLASS
	fprintf(stderr, "matched \"%s\" \"%s\"\n", tt->class.res_class, tt->class.res_name);
#endif
	tp = tt;
	return tp;
      }
      else
      {
#ifdef DEBUG_WARPCLASS
	fprintf(stderr, "(1) skipping \"%s\"\n", (i) ? tt->class.res_class : "NO RES_CLASS!");
#endif
	if (i == 0)
	  break;
      }
    }

  /* no match, wrap and retry */
  tp = tl = NULL;
  for (tt = Scr->TwmRoot.next; tt != NULL; tt = tt->next)
    if (Scr->WarpUnmapped || tt->mapped)
    {
      i = XGetClassHint(dpy, tt->w, &ch);
      if (i && !strncmp(class, ch.res_class, len))
      {
	if (next)
	{
#ifdef DEBUG_WARPCLASS
	  fprintf(stderr, "next wrapped to \"%s\ \"%s\"\n", tt->class.res_class, tt->class.res_name);
#endif
	  tp = tt;
	  return tp;
	}
	else
	  tl = tt;
      }
#ifdef DEBUG_WARPCLASS
      else
	fprintf(stderr, "(2) skipping \"%s\"\n", (i) ? tt->class.res_class : "NO RES_CLASS!");
#endif
    }

#ifdef DEBUG_WARPCLASS
  i = 0;
  if (tl)
    i = XGetClassHint(dpy, tl->w, &ch);
  fprintf(stderr, "prev wrapped to \"%s\ \"%s\"\n", (i) ? ch.res_class : "NO RES_CLASS!", (i) ? ch.res_name : "NO RES_CLASS!");
#endif
  tp = tl;
  return tp;
}

static int
warp_if_warpunmapped(TwmWindow * w, int func)
{
  if (w && (w->iconmgr && w->iconmgrp->count == 0))
    return (0);

  if (Scr->WarpUnmapped || w->mapped)
  {
    if (func != F_NOFUNCTION && Scr->WarpVisible)
    {
      int pan_margin = 0;

      if (Scr->AutoPanX)
	pan_margin = Scr->AutoPanBorderWidth;

      if (w->frame_x >= Scr->MyDisplayWidth - pan_margin ||
	  w->frame_y >= Scr->MyDisplayHeight - pan_margin ||
	  w->frame_x + w->frame_width <= pan_margin || w->frame_y + w->frame_height <= pan_margin)
	return 0;
    }

    if (!w->mapped)
      DeIconify(w);
    if (!Scr->NoRaiseWarp)
      XRaiseWindow(dpy, w->frame);
    XRaiseWindow(dpy, w->VirtualDesktopDisplayWindow.win);

    return (1);
  }

  return (0);
}

static int
do_squeezetitle(int context, int func, TwmWindow * tmp_win, SqueezeInfo * squeeze)
{
  if (DeferExecution(context, func, Scr->SelectCursor))
    return TRUE;

  /* honor "Don't Squeeze" resources - djhjr - 9/17/02 */
  if (Scr->SqueezeTitle && !LookInList(Scr->DontSqueezeTitleL, tmp_win->full_name, &tmp_win->class))
  {
    if (tmp_win->title_height)	/* Not for untitled windows! */
    {
      PopDownMenu();

#ifndef NO_SOUND_SUPPORT
      PlaySound(func);
#endif

      tmp_win->squeeze_info = squeeze;
      SetFrameShape(tmp_win);

      /* Can't go in SetFrameShape()... - djhjr - 4/1/00 */
      if (Scr->WarpCursor || LookInList(Scr->WarpCursorL, tmp_win->full_name, &tmp_win->class))
	WarpToWindow(tmp_win);
    }
  }
  else
    DoAudible();

  return FALSE;
}

/*
 * Two functions to handle a restart from a SIGUSR1 signal
 * (see also twm.c:Done() and twm.c:QueueRestartVtwm())
 *
 * adapted from TVTWM-pl11 - djhjr - 7/31/98
 */

static void
setup_restart(Time time)
{
#ifndef NO_SOUND_SUPPORT
  CloseSound();
#endif

  SetRealScreen(0, 0);
  XSync(dpy, 0);
  Reborder(time);
  XSync(dpy, 0);

  XCloseDisplay(dpy);

  delete_pidfile();
}

void
RestartVtwm(Time time)
{
  setup_restart(time);

  execvp(*Argv, Argv);
  fprintf(stderr, "%s:  unable to restart \"%s\"\n", ProgramName, *Argv);
}


/*
 * ICCCM Client Messages - Section 4.2.8 of the ICCCM dictates that all
 * client messages will have the following form:
 *
 *     event type	ClientMessage
 *     message type	_XA_WM_PROTOCOLS
 *     window		tmp->w
 *     format		32
 *     data[0]		message atom
 *     data[1]		time stamp
 */

static void
send_clientmessage(Window w, Atom a, Time timestamp)
{
  XClientMessageEvent ev;

  ev.type = ClientMessage;
  ev.window = w;
  ev.message_type = _XA_WM_PROTOCOLS;
  ev.format = 32;
  ev.data.l[0] = a;
  ev.data.l[1] = timestamp;
  XSendEvent(dpy, w, False, 0L, (XEvent *) & ev);
}

static void
SendDeleteWindowMessage(TwmWindow * tmp, Time timestamp)
{
  send_clientmessage(tmp->w, _XA_WM_DELETE_WINDOW, timestamp);
}

static void
SendSaveYourselfMessage(TwmWindow * tmp, Time timestamp)
{
  send_clientmessage(tmp->w, _XA_WM_SAVE_YOURSELF, timestamp);
}

void
SendTakeFocusMessage(TwmWindow * tmp, Time timestamp)
{
  send_clientmessage(tmp->w, _XA_WM_TAKE_FOCUS, timestamp);
}


void
DisplayPosition(int x, int y)
{
  char str[100];
  int i;

  sprintf(str, "%+6d %-+6d", x, y);
  i = strlen(str);

  XRaiseWindow(dpy, Scr->SizeWindow.win);
  MyFont_DrawImageString(dpy, &Scr->SizeWindow, &Scr->SizeFont,
			 &Scr->DefaultC,
			 (Scr->SizeStringWidth -
			  MyFont_TextWidth(&Scr->SizeFont,
					   str, i)) / 2,
			 Scr->SizeFont.ascent + SIZE_VINDENT + ((Scr->InfoBevelWidth > 0) ? Scr->InfoBevelWidth : 0), str, i);

  /* I know, I know, but the above code overwrites it... djhjr - 5/9/96 */
  if (Scr->InfoBevelWidth > 0)
    Draw3DBorder(Scr->SizeWindow.win, 0, 0,
		 Scr->SizeStringWidth,
		 (unsigned int)(Scr->SizeFont.height + SIZE_VINDENT * 2) +
		 ((Scr->InfoBevelWidth > 0) ? 2 * Scr->InfoBevelWidth : 0), Scr->InfoBevelWidth, Scr->DefaultC, off, False, False);
}

int
FindMenuOrFuncInBindings(int contexts, MenuRoot * mr, int func)
{
  MenuRoot *start;
  FuncKey *key;
  int found = 0;		/* context bitmap for menu or function */
  int i, j, k, l, fallback = 0;

  if (mr)
  {
    if (Scr->DefaultFunction.func == F_MENU)
      if (FindMenuInMenus(Scr->DefaultFunction.menu, mr))
	fallback = 1;
  }
  else
  {
    if (Scr->DefaultFunction.func == func)
      fallback = 1;
    else if (Scr->DefaultFunction.func == F_MENU)
      if (FindFuncInMenus(Scr->DefaultFunction.menu, func))
	fallback = 1;
  }

  for (j = 0; j < NUM_CONTEXTS; j++)
  {
    if ((contexts & (1 << j)) == 0)
      continue;

    if (fallback)
    {
      found |= (1 << j);
      continue;
    }

    for (i = 0; i < NumButtons + 1; i++)
    {
      l = 0;

      for (k = 0; k < MOD_SIZE; k++)
      {
	if (mr)
	{
	  if (Scr->Mouse[MOUSELOC(i, j, k)].func == F_MENU)
	    l = FindMenuInMenus(Scr->Mouse[MOUSELOC(i, j, k)].menu, mr);
	}
	else
	{
	  if (Scr->Mouse[MOUSELOC(i, j, k)].func == func)
	    l = 1;
	  else if (Scr->Mouse[MOUSELOC(i, j, k)].func == F_MENU)
	    l = FindFuncInMenus(Scr->Mouse[MOUSELOC(i, j, k)].menu, func);
	}

	if (l)
	{
	  found |= (1 << j);
	  i = NumButtons + 1;
	  break;
	}
      }
    }

    l = 0;
    for (key = Scr->FuncKeyRoot.next; key != NULL; key = key->next)
      if (key->cont & (1 << j))
      {
	if (mr)
	{
	  if (key->func == F_MENU)
	    for (start = Scr->MenuList; start != NULL; start = start->next)
	      if (strcmp(start->name, key->action) == 0)
	      {
		l = FindMenuInMenus(start, mr);
		break;
	      }
	}
	else
	{
	  if (key->func == func)
	    l = 1;
	  else if (key->func == F_MENU)
	    for (start = Scr->MenuList; start != NULL; start = start->next)
	      if (strcmp(start->name, key->action) == 0)
	      {
		l = FindFuncInMenus(start, func);
		break;
	      }
	}

	if (l)
	{
	  found |= (1 << j);
	  break;
	}
      }
  }

  return found;
}

int
FindMenuOrFuncInWindows(TwmWindow * tmp_win, int contexts, MenuRoot * mr, int func)
{
  TwmWindow *twin;
  TwmDoor *d;
  TBWindow *tbw;
  int i, nb;

  if (contexts & C_ROOT_BIT)
    return 1;

  for (twin = Scr->TwmRoot.next; twin != NULL; twin = twin->next)
    if (twin != tmp_win)
    {
      /*
       * if this window is an icon manager,
       * skip the windows that aren't in it
       */
      if (tmp_win->iconmgr && twin->list && tmp_win != twin->list->iconmgr->twm_win)
	continue;

      if (twin->mapped)
      {
	for (i = 1; i < C_ALL_BITS; i = (1 << i))
	{
	  if ((contexts & i) == 0)
	    continue;

	  switch (i)
	  {
	  case C_WINDOW_BIT:
	  case C_FRAME_BIT:
	    break;
	  case C_TITLE_BIT:
	    if (!twin->title_height)
	      continue;
	    break;
	  case C_VIRTUAL_BIT:
	    if (twin->w != Scr->VirtualDesktopDisplayOuter)
	      continue;
	    break;
	  case C_DOOR_BIT:
	    if (XFindContext(dpy, twin->w, DoorContext, (caddr_t *) & d) == XCNOENT)
	      continue;
	    break;
	  default:
	    continue;
	    break;
	  }

	  return 1;
	}

	if (twin->titlebuttons)
	{
	  nb = Scr->TBInfo.nleft + Scr->TBInfo.nright;

	  for (tbw = twin->titlebuttons; nb > 0; tbw++, nb--)
	    if (mr)
	    {
	      if (tbw->info->menuroot)
		if (FindMenuInMenus(tbw->info->menuroot, mr))
		  return 1;
	    }
	    else
	    {
	      if (tbw->info->func == func)
		return 1;
	      else if (tbw->info->menuroot)
		if (FindFuncInMenus(tbw->info->menuroot, func))
		  return 1;
	    }
	}
      }
      else if (!twin->iconify_by_unmapping)
      {
	/* not mapped and shows an icon */

	if (contexts & C_ICON_BIT)
	  return 1;
      }
    }

  return 0;
}

int
FindMenuInMenus(MenuRoot * start, MenuRoot * sought)
{
  MenuItem *mi;

  if (!start)
    return 0;
  if (start == sought)
    return 1;

  for (mi = start->first; mi != NULL; mi = mi->next)
    if (mi->sub)
      if (FindMenuInMenus(mi->sub, sought))
	return 1;

  return 0;
}

int
FindFuncInMenus(MenuRoot * mr, int func)
{
  MenuItem *mi;

  for (mi = mr->first; mi != NULL; mi = mi->next)
    if (mi->func == func)
      return 1;
    else if (mi->sub)
      if (FindFuncInMenus(mi->sub, func))
	return 1;

  return 0;
}

void
DoAudible(void)
{
#ifndef NO_SOUND_SUPPORT
  if (PlaySound(S_BELL))
    return;
#endif

  XBell(dpy, 0);
}
