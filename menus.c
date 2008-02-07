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
/* djhjr - 10/27/02 */
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
#include <ctype.h> /* DSE */
#include "twm.h"
#include "gc.h"
#include "menus.h"
#include "resize.h"
#include "events.h"
#include "util.h"
#include "parse.h"
#include "gram.h"
#include "screen.h"
#include "doors.h"
#include "desktop.h"
#include "add_window.h"
/* djhjr - 6/22/01 */
#ifndef NO_SOUND_SUPPORT
#include "sound.h"
#endif
#include "version.h"

extern void IconUp(), IconDown(), CreateIconWindow();

/* djhjr - 4/26/99 */
extern void AppletDown();

/* djhjr - 12/2/01 */
extern void delete_pidfile();

/* djhjr - 10/27/02 */
extern int MatchName();

extern void ResizeTwmWindowContents();
extern void SetRaiseWindow();

extern char *Action;
extern int Context;
extern int ConstrainedMoveTime;
extern TwmWindow *ButtonWindow, *Tmp_win;
extern XEvent Event, ButtonEvent;
extern char *InitFile;

int RootFunction = F_NOFUNCTION;
MenuRoot *ActiveMenu = NULL;		/* the active menu */
MenuItem *ActiveItem = NULL;		/* the active menu item */
int MoveFunction = F_NOFUNCTION;	/* or F_MOVE or F_FORCEMOVE */
int WindowMoved = FALSE;
int menuFromFrameOrWindowOrTitlebar = FALSE;
/* djhjr - 6/22/01 */
#ifndef NO_SOUND_SUPPORT
int createSoundFromFunction = FALSE;
int destroySoundFromFunction = FALSE;
#endif

void BumpWindowColormap();
void DestroyMenu();
void HideIconManager();
void MakeMenu();
void SendDeleteWindowMessage();
void SendSaveYourselfMessage();
void WarpClass();
void WarpToScreen();
void WarpScreenToWindow();
Cursor NeedToDefer(); /* was an 'int' - Submitted by Michel Eyckmans */

int ConstMove = FALSE;		/* constrained move variables */

/* for comparison against MoveDelta - djhjr - 9/5/98 */
static int MenuOrigX, MenuOrigY;

/* Globals used to keep track of whether the mouse has moved during
   a resize function. */
int ResizeOrigX;
int ResizeOrigY;

extern int origx, origy, origWidth, origHeight;

int MenuDepth = 0;		/* number of menus up */
static struct {
	int x;
	int y;
} MenuOrigins[MAXMENUDEPTH];
static Cursor LastCursor;

static char *actionHack = ""; /* Submitted by Michel Eyckmans */

/*
 * context bitmaps for TwmWindows menu, f.showdesktop and f.showiconmgr
 * djhjr - 9/10/99
 */
static int have_twmwindows = -1;
static int have_showdesktop = -1;
static int have_showlist = -1;

void WarpAlongRing();

/* djhjr - 4/18/96 */
void Paint3DEntry();

static void Identify();
void PaintNormalEntry();

/* djhjr - 5/13/98 */
static TwmWindow *next_by_class();
static int warp_if_warpunmapped();

/* djhjr - 7/31/98 */
static void setup_restart();
void RestartVtwm();

/* djhjr - 9/21/99 */
int FindMenuOrFuncInBindings();
int FindMenuOrFuncInWindows();
int FindMenuInMenus();
int FindFuncInMenus();

/* djhjr - 9/21/99 */
void HideIconMgr();
void ShowIconMgr();

/* djhjr - 9/17/02 */
static int do_squeezetitle();

/* djhjr */
#undef MAX
/* DSE */
#define MAX(x,y) ((x)>(y)?(x):(y))

#define SHADOWWIDTH 5			/* in pixels */

#ifdef TWM_USE_SPACING
#define EDGE_OFFSET (Scr->MenuFont.ascent/2)
#else
#define EDGE_OFFSET 5 /* DSE */
#endif

/* djhjr - 5/5/98
#define PULLDOWNMENU_OFFSET ((Scr->RightHandSidePulldownMenus)?\
	(ActiveMenu->width - EDGE_OFFSET * 2 - Scr->pullW):\
	(ActiveMenu->width >> 1)) * DSE *
*/
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
InitMenus()
{
    int i, j, k;
    FuncKey *key, *tmp;

    for (i = 0; i < NumButtons+1; i++)
	for (j = 0; j < NUM_CONTEXTS; j++)
	    for (k = 0; k < MOD_SIZE; k++)
	    {
		Scr->Mouse[MOUSELOC(i,j,k)].func = F_NOFUNCTION;
		Scr->Mouse[MOUSELOC(i,j,k)].item = NULL;
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
	    free((char *) tmp);
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

Bool AddFuncKey (name, cont, mods, func, win_name, action)
    char *name;
    int cont, mods, func;
    char *win_name;
    char *action;
{
    FuncKey *tmp;
    KeySym keysym;
    KeyCode keycode;

    /*
     * Don't let a 0 keycode go through, since that means AnyKey to the
     * XGrabKey call in GrabKeys().
     */
    if ((keysym = XStringToKeysym(name)) == NoSymbol ||
	(keycode = XKeysymToKeycode(dpy, keysym)) == 0)
    {
	return False;
    }

    /* see if there already is a key defined for this context */
    for (tmp = Scr->FuncKeyRoot.next; tmp != NULL; tmp = tmp->next)
    {
	if (tmp->keysym == keysym &&
	    tmp->cont == cont &&
	    tmp->mods == mods)
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



int CreateTitleButton (name, func, action, menuroot, rightside, append)
    char *name;
    int func;
    char *action;
    MenuRoot *menuroot;
    Bool rightside;
    Bool append;
{
    TitleButton *tb = (TitleButton *) malloc (sizeof(TitleButton));

    if (!tb) {
	fprintf (stderr,
		 "%s:  unable to allocate %d bytes for title button\n",
		 ProgramName, sizeof(TitleButton));
	return 0;
    }

    tb->next = NULL;
    tb->name = name;			/* note that we are not copying */

/* djhjr - 10/30/02
    * djhjr - 4/19/96 *
    tb->image = NULL;
*/

/*    tb->bitmap = None;*/			/* WARNING, values not set yet */
    tb->width = 0;			/* see InitTitlebarButtons */
    tb->height = 0;			/* ditto */
    tb->func = func;
    tb->action = action;
    tb->menuroot = menuroot;
    tb->rightside = rightside;
    if (rightside) {
	Scr->TBInfo.nright++;
    } else {
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
    if ((!Scr->TBInfo.head) || ((!append) && (!rightside))) {	/* 1 */
		tb->next = Scr->TBInfo.head;
		Scr->TBInfo.head = tb;
    } else if (append && rightside) {	/* 3 */
		register TitleButton *t;

		for (t = Scr->TBInfo.head; t->next; t = t->next)
			; /* SUPPRESS 530 */
		t->next = tb;
		tb->next = NULL;
   	} else {				/* 2 */
		register TitleButton *t, *prev = NULL;

		for (t = Scr->TBInfo.head; t && !t->rightside; t = t->next)
    		prev = t;
		if (prev) {
    		tb->next = prev->next;
    		prev->next = tb;
		} else {
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
/* was of type 'void', now returns button height - djhjr - 12/10/98 */
int InitTitlebarButtons ()
{
    Image *image;
    TitleButton *tb;
    int h, height;

    /*
     * initialize dimensions
     */
    Scr->TBInfo.width = (Scr->TitleHeight -
			 2 * (Scr->FramePadding + Scr->ButtonIndent));

/* djhjr - 10/18/02
	* djhjr - 4/19/96 *
	* was 'Scr->use3Dtitles' - djhjr - 8/11/98 *
    if (Scr->TitleBevelWidth > 0) 
	Scr->TBInfo.pad = ((Scr->TitlePadding > 1)
		       ? ((Scr->TitlePadding + 1) / 2) : 0);
    else

    Scr->TBInfo.pad = ((Scr->TitlePadding > 1)
		       ? ((Scr->TitlePadding + 1) / 2) : 1);
*/
    Scr->TBInfo.pad = Scr->TitlePadding;

    h = Scr->TBInfo.width - 2 * Scr->TBInfo.border;
    /* djhjr - 10/30/02 */
    if (!(h & 1)) h--;
    height = h;

    /*
     * add in some useful buttons and bindings so that novices can still
     * use the system. -- modified by DSE 
     */

    if (!Scr->NoDefaultTitleButtons) /* DSE */
    	{
		/* insert extra buttons */

	/* djhjr - 4/19/96 */
	/* was 'Scr->use3Dtitles' - djhjr - 8/11/98 */
	if (Scr->TitleBevelWidth > 0) {
	    if (!CreateTitleButton (TBPM_3DDOT, F_ICONIFY, "", (MenuRoot *) NULL,
				False, False))
	        fprintf (stderr, "%s:  unable to add iconify button\n", ProgramName);
	    if (!CreateTitleButton (TBPM_3DRESIZE, F_RESIZE, "", (MenuRoot *) NULL,
				True, True))
	        fprintf (stderr, "%s:  unable to add resize button\n", ProgramName);
	}
	else {

		if (!CreateTitleButton (TBPM_ICONIFY, F_ICONIFY, "", (MenuRoot *) NULL,
				False, False))
			fprintf(stderr,"%s:  unable to add iconify button\n",ProgramName);
		if (!CreateTitleButton (TBPM_RESIZE, F_RESIZE, "", (MenuRoot *) NULL,
				True, True))
			fprintf(stderr,"%s:  unable to add resize button\n",ProgramName);
	}
	}
	if (!Scr->NoDefaultMouseOrKeyboardBindings) /* DSE */
		{
		AddDefaultBindings ();
		}

    ComputeCommonTitleOffsets ();

/* djhjr - 6/15/98 - moved it back to here... */
/* djhjr - 9/14/96 - moved to CreateWindowTitlebarButtons()... */
	/*
	 * load in images and do appropriate centering
	 */
    for (tb = Scr->TBInfo.head; tb; tb = tb->next) {

/* djhjr - 4/19/96
	tb->bitmap = FindBitmap (tb->name, &tb->width, &tb->height);
	if (!tb->bitmap) {
	    tb->bitmap = FindBitmap (TBPM_QUESTION, &tb->width, &tb->height);
	    if (!tb->bitmap) {		* cannot happen (see util.c) *
		fprintf (stderr,
			 "%s:  unable to add titlebar button \"%s\"\n",
			 ProgramName, tb->name);
	    }
	}
*/
/* djhjr - 9/21/96
	tb->image = GetImage (tb->name, Scr->TitleC);
	if (!tb->image) {
	    tb->image = GetImage (TBPM_QUESTION, Scr->TitleC);
	    if (!tb->image) {		* cannot happen (see util.c) *
		fprintf (stderr, "%s:  unable to add titlebar button \"%s\"\n",
			 ProgramName, tb->name);
	    }
	}
*/
	/* added width and height - 10/30/02 */
	image = GetImage (tb->name, h, h, Scr->ButtonBevelWidth * 2,
		(Scr->ButtonColorIsFrame) ? Scr->BorderColorC : Scr->TitleC);

	tb->width  = image->width;

	/* added 'height = ' - djhjr - 12/10/98 */
	height = tb->height = image->height;

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

    } /* for(...) */

    /* djhjr - 12/10/98 */
    return (height > h) ? height : h;
/* ...end of moved */
}



/* djhjr - 10/30/02 */
void SetMenuIconPixmap(filename)
    char *filename;
{
	Scr->menuIconName = filename;
}

void PaintEntry(mr, mi, exposure)
MenuRoot *mr;
MenuItem *mi;
int exposure;
{
	/* was 'Scr->use3Dmenus' - djhjr - 8/11/98 */
    if (Scr->MenuBevelWidth > 0)
	Paint3DEntry (mr, mi, exposure);

	/* djhjr - 4/22/96 */
	else

    PaintNormalEntry (mr, mi, exposure);
}

void Paint3DEntry(mr, mi, exposure)
MenuRoot *mr;
MenuItem *mi;
int exposure;
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

/* djhjr - 9/25/96
			Draw3DBorder (mr->w.win, 2, y_offset, mr->width - 4, Scr->EntryHeight, 1, 
				mi->highlight, off, True, False);
*/
/* djhjr - 4/29/98
			Draw3DBorder (mr->w.win, 2, y_offset + 1, mr->width - 4, Scr->EntryHeight - 1, 1,
				mi->highlight, off, True, False);
*/
			Draw3DBorder (mr->w.win, Scr->MenuBevelWidth, y_offset + 1, mr->width - 2 * Scr->MenuBevelWidth, Scr->EntryHeight - 1, 1,
				mi->highlight, off, True, False);

			MyFont_DrawString (dpy, &mr->w, &Scr->MenuFont,
					&mi->highlight, mi->x + Scr->MenuBevelWidth, text_y, mi->item, mi->strlen);

#ifdef TWM_USE_XFT
			/*
			 * initialise NormalGC with color for XCopyPlane() later
			 * ("non-TWM_USE_XFT" does it in MyFont_DrawString() already):
			 */
			if (Scr->use_xft > 0)
			    FB(mi->highlight.fore, mi->highlight.back);
#endif
			gc = Scr->NormalGC;
		}
		else
		{
			if (mi->user_colors || !exposure)
			{
				XSetForeground (dpy, Scr->NormalGC, mi->normal.back);

/* djhjr - 9/25/96
				XFillRectangle (dpy, mr->w.win, Scr->NormalGC, 2, y_offset,
					mr->width - 4, Scr->EntryHeight);
*/
/* djhjr - 4/29/98
				XFillRectangle (dpy, mr->w.win, Scr->NormalGC, 2, y_offset + 1,
					mr->width - 4, Scr->EntryHeight - 1);
*/
				XFillRectangle (dpy, mr->w.win, Scr->NormalGC, Scr->MenuBevelWidth, y_offset + 1,
					mr->width - 2 * Scr->MenuBevelWidth, Scr->EntryHeight - 1);
#ifdef TWM_USE_XFT
				if (Scr->use_xft > 0)
				    FB(mi->normal.fore, mi->normal.back);
#endif
				gc = Scr->NormalGC;
		    }
			else
			{
				gc = Scr->MenuGC;
			}

			/* MyFont_DrawString() sets NormalGC: */
			MyFont_DrawString (dpy, &mr->w, &Scr->MenuFont,
					&mi->normal, mi->x + Scr->MenuBevelWidth, text_y, mi->item, mi->strlen);

			if (mi->separated)
			{
				/* this 'if (...)' - djhjr - 1/19/98 */
				if (!Scr->BeNiceToColormap)
				{
					FB (Scr->MenuC.shadd, Scr->MenuC.shadc);

/* djhjr - 9/25/96
					XDrawLine (dpy, mr->w.win, Scr->NormalGC, 1, y_offset + Scr->MenuFont.y + 5,
						mr->width - 2, y_offset + Scr->MenuFont.y + 5);
*/
/* djhjr - 4/29/98
					XDrawLine (dpy, mr->w.win, Scr->NormalGC, 1, y_offset + Scr->EntryHeight - 1,
						mr->width - 2, y_offset + Scr->EntryHeight - 1);
*/
					XDrawLine (dpy, mr->w.win, Scr->NormalGC, Scr->MenuBevelWidth + 1, y_offset + Scr->EntryHeight - 1,
						mr->width - Scr->MenuBevelWidth - 3, y_offset + Scr->EntryHeight - 1);
				}

				FB (Scr->MenuC.shadc, Scr->MenuC.shadd);

/* djhjr - 9/25/96
				XDrawLine (dpy, mr->w.win, Scr->NormalGC, 2, y_offset + Scr->MenuFont.y + 6,
					mr->width - 3, y_offset + Scr->MenuFont.y + 6);
*/
/* djhjr - 4/29/98
				XDrawLine (dpy, mr->w.win, Scr->NormalGC, 2, y_offset + Scr->EntryHeight,
					mr->width - 3, y_offset + Scr->EntryHeight);
*/
				XDrawLine (dpy, mr->w.win, Scr->NormalGC, Scr->MenuBevelWidth + 2, y_offset + Scr->EntryHeight,
					mr->width - Scr->MenuBevelWidth - 2, y_offset + Scr->EntryHeight);
			}
		}

		if (mi->func == F_MENU)
		{
/* djhjr - 10/30/02
			* create the pull right pixmap if needed *
			if (Scr->pullPm == None)
			{
				Scr->pullPm = Create3DMenuIcon (Scr->MenuFont.height, &Scr->pullW,
					&Scr->pullH, Scr->MenuC);
*/
				Image *image;
				Pixel back;

				back = Scr->MenuC.back;
				if (mi->state)
					Scr->MenuC.back = mi->highlight.back;
				else
					Scr->MenuC.back = mi->normal.back;

				Scr->pullW = Scr->pullH = Scr->MenuFont.height;
				image = GetImage(Scr->menuIconName,
						 Scr->pullW, Scr->pullH,
						 0, Scr->MenuC);

				Scr->MenuC.back = back;
/* djhjr - 10/30/02
			}
*/

/* djhjr - 4/29/98
			x = mr->width - Scr->pullW - 5;
*/
			x = mr->width - Scr->pullW - Scr->MenuBevelWidth - EDGE_OFFSET;

/* djhjr - 9/25/96
			y = y_offset + ((Scr->MenuFont.height - Scr->pullH) / 2) + 2;
*/
			y = y_offset + ((Scr->EntryHeight - Scr->pullH) / 2) + 1;

			XCopyArea (dpy, image->pixmap, mr->w.win, gc, 0, 0, Scr->pullW, Scr->pullH, x, y);
		}
	}
	else
	{

/* djhjr - 4/29/96
		Draw3DBorder (mr->w.win, 2, y_offset, mr->width - 4, Scr->EntryHeight, 1, 
			mi->normal, off, True, False);
*/
/* djhjr - 4/29/98
		Draw3DBorder (mr->w.win, 2, y_offset, mr->width - 4, Scr->EntryHeight + 1, 1, 
			mi->normal, off, True, False);
*/
		Draw3DBorder (mr->w.win, Scr->MenuBevelWidth, y_offset, mr->width - 2 * Scr->MenuBevelWidth, Scr->EntryHeight + 1, 1, 
			mi->normal, off, True, False);

		MyFont_DrawString (dpy, &mr->w, &Scr->MenuTitleFont,
				&mi->normal, mi->x, text_y, mi->item, mi->strlen);
	}
}
    


void PaintNormalEntry(mr, mi, exposure)
MenuRoot *mr;
MenuItem *mi;
int exposure;
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

			XFillRectangle(dpy, mr->w.win, Scr->NormalGC, 0, y_offset,
				mr->width, Scr->EntryHeight);

			MyFont_DrawString(dpy, &mr->w, &Scr->MenuFont,
				&mi->highlight, mi->x,
				text_y, mi->item, mi->strlen);

#ifdef TWM_USE_XFT
			/*
			 * initialise NormalGC with color for XCopyPlane() later
			 * ("non-TWM_USE_XFT" does it in MyFont_DrawString() already):
			 */
			if (Scr->use_xft > 0)
			    FB(mi->highlight.fore, mi->highlight.back);
#endif
			gc = Scr->NormalGC;
		}
		else
		{
			if (mi->user_colors || !exposure)
			{
				XSetForeground(dpy, Scr->NormalGC, mi->normal.back);

				XFillRectangle(dpy, mr->w.win, Scr->NormalGC, 0, y_offset,
					mr->width, Scr->EntryHeight);
#ifdef TWM_USE_XFT
				if (Scr->use_xft > 0)
				    FB(mi->normal.fore, mi->normal.back);
#endif
				gc = Scr->NormalGC;
			}
			else
			{
				gc = Scr->MenuGC;
			}

			/* MyFont_DrawString() sets NormalGC: */
			MyFont_DrawString(dpy, &mr->w, &Scr->MenuFont,
					&mi->normal, mi->x, text_y, mi->item, mi->strlen);

			if (mi->separated)

/* djhjr - 9/26/96
				XDrawLine (dpy, mr->w.win, gc, 0, y_offset + Scr->MenuFont.y + 5,
					mr->width, y_offset + Scr->MenuFont.y + 5);
*/
				XDrawLine (dpy, mr->w.win, gc, 0, y_offset + Scr->EntryHeight - 1,
					mr->width, y_offset + Scr->EntryHeight - 1);
		}

		if (mi->func == F_MENU)
		{
/* djhjr - 10/30/02
			* create the pull right pixmap if needed *
			if (Scr->pullPm == None)
			{
				Scr->pullPm = CreateMenuIcon (Scr->MenuFont.height,
					&Scr->pullW, &Scr->pullH);
*/
				Image *image;
				ColorPair cp;

				cp.back = Scr->MenuC.back;
				if (strncmp(Scr->menuIconName, ":xpm:", 5) != 0)
				{
					cp.fore = Scr->MenuC.fore;
					Scr->MenuC.fore = (mi->state) ? mi->highlight.fore : mi->normal.fore;
					Scr->MenuC.back = (mi->state) ? mi->highlight.back : mi->normal.back;
				}
				else
					Scr->MenuC.back = (mi->state) ? mi->highlight.back : mi->normal.back;

				Scr->pullW = Scr->pullH = Scr->MenuFont.height;
				image = GetImage(Scr->menuIconName,
						 Scr->pullW, Scr->pullH,
						 0, Scr->MenuC);

				Scr->MenuC.back = cp.back;
				if (strncmp(Scr->menuIconName, ":xpm:", 5) != 0)
					Scr->MenuC.fore = cp.fore;
/* djhjr - 10/30/02
			}
*/

			x = mr->width - Scr->pullW - EDGE_OFFSET;

/* djhjr - 9/26/96
			y = y_offset + ((Scr->MenuFont.height - Scr->pullH) / 2);
*/
			y = y_offset + ((Scr->EntryHeight - Scr->pullH) / 2);

/* djhjr - 10/30/02
			XCopyPlane(dpy, Scr->pullPm->pixmap, mr->w.win, gc, 0, 0,
				Scr->pullW, Scr->pullH, x, y, 1);
*/
			XCopyArea (dpy, image->pixmap, mr->w.win, gc, 0, 0,
				Scr->pullW, Scr->pullH, x, y);
		}
	}
	else
	{
		int y;

		XSetForeground(dpy, Scr->NormalGC, mi->normal.back);

		/* fill the rectangle with the title background color */
		XFillRectangle(dpy, mr->w.win, Scr->NormalGC, 0, y_offset,
			mr->width, Scr->EntryHeight);

		XSetForeground(dpy, Scr->NormalGC, mi->normal.fore);

		/* now draw the dividing lines */
		if (y_offset)
			XDrawLine (dpy, mr->w.win, Scr->NormalGC, 0, y_offset,
				mr->width, y_offset);

		y = ((mi->item_num+1) * Scr->EntryHeight)-1;
		XDrawLine(dpy, mr->w.win, Scr->NormalGC, 0, y, mr->width, y);

		/* finally render the title */
		MyFont_DrawString(dpy, &mr->w, &Scr->MenuTitleFont,
			&mi->normal, mi->x, text_y, mi->item, mi->strlen);
	}
}

void PaintMenu(mr, e)
MenuRoot *mr;
XEvent *e;
{
    MenuItem *mi;
	/* djhjr - 5/22/00 */
	int y_offset;

	/* djhjr - 4/22/96 */
	/* was 'Scr->use3Dmenus' - djhjr - 8/11/98 */
    if (Scr->MenuBevelWidth > 0) {
/* djhjr - 4/29/98
	Draw3DBorder (mr->w.win, 0, 0, mr->width, mr->height, 2, Scr->MenuC, off, False, False);
*/
	Draw3DBorder (mr->w.win, 0, 0, mr->width, mr->height, Scr->MenuBevelWidth, Scr->MenuC, off, False, False);
    }

    for (mi = mr->first; mi != NULL; mi = mi->next)
    {
	/* djhjr - 5/22/00 */
	if (mi->item_num < mr->top) continue;

/* djhjr - 5/22/00
	int y_offset = mi->item_num * Scr->EntryHeight;
*/
	y_offset = (mi->item_num - mr->top) * Scr->EntryHeight;

	/* djhjr - 5/22/00 */
	if (y_offset + Scr->EntryHeight > mr->height) break;

	/* some servers want the previous entry redrawn - djhjr - 10/24/00 */
	if (Scr->MenuBevelWidth > 0) y_offset += Scr->EntryHeight;

	/*
	 * Be smart about handling the expose, redraw only the entries
	 * that we need to.
	 */
	/* those servers want the next entry redrawn, too - djhjr - 10/24/00 */
	if (e->xexpose.y < (y_offset + Scr->EntryHeight) &&
	    (e->xexpose.y + e->xexpose.height) > y_offset - ((mr->shadow) ? Scr->EntryHeight : 0))
	{
	    PaintEntry(mr, mi, True);
	}
    }
    XSync(dpy, 0);
}



static Bool fromMenu;

extern int GlobalFirstTime; /* for StayUpMenus -- PF */

void UpdateMenu()
{
	MenuItem *mi;
    int i, x, y, x_root, y_root, entry;
	int done;
	MenuItem *badItem = NULL;
	static int firstTime = True;

	fromMenu = TRUE;

	while (TRUE)
	{	/* block until there is an event */
#ifdef NEVER /* see the '#else' - Steve Ratcliffe */
#if 0
		if (!menuFromFrameOrWindowOrTitlebar && ! Scr->StayUpMenus) {
			XMaskEvent(dpy,
				ButtonPressMask | ButtonReleaseMask |
				EnterWindowMask | ExposureMask |
				VisibilityChangeMask | LeaveWindowMask |
				ButtonMotionMask, &Event);
		}
		if (Event.type == MotionNotify) {
			/* discard any extra motion events before a release */
			while(XCheckMaskEvent(dpy,
					ButtonMotionMask | ButtonReleaseMask, &Event))
				if (Event.type == ButtonRelease)
					break;
		}
#else
		while (XCheckMaskEvent(dpy, ButtonPressMask | ButtonReleaseMask |
				EnterWindowMask | ExposureMask, &Event))
		{	/* taken from tvtwm */
#endif /* 0 */
#else
		/* Submitted by Steve Ratcliffe */
		XNextEvent(dpy, &Event);
#endif /* NEVER */

		if (!DispatchEvent ()) continue;

		if (Event.type == ButtonRelease )
		{	if (Scr->StayUpMenus)
			{
				if (firstTime == True)
				{	/* it was the first release of the button */
					firstTime = False;
				}
				else
				{	/* thats the second we need to return now */
					firstTime = True;
					menuFromFrameOrWindowOrTitlebar = FALSE;
					fromMenu = FALSE;
					return;
				}
			}
			else
			{	/* not stay-up */
				menuFromFrameOrWindowOrTitlebar = FALSE;
				fromMenu = FALSE;
				return;
			}
		}

		if (Cancel) return;

#ifdef NEVER /* see the above - Steve Ratcliffe */
		}
#endif

		/* re-instated - Steve Ratcliffe */
		if (Event.type != MotionNotify)
			continue;

		/* if we haven't received the enter notify yet, wait */
		if (!ActiveMenu || !ActiveMenu->entered)
			continue;

		done = FALSE;
		XQueryPointer( dpy, ActiveMenu->w.win, &JunkRoot, &JunkChild,
				&x_root, &y_root, &x, &y, &JunkMask);

		/* djhjr - 9/5/98 */
		if (!ActiveItem)
			if (abs(x_root - MenuOrigX) < Scr->MoveDelta &&
					abs(y_root - MenuOrigY) < Scr->MoveDelta)
				continue;

#if 0
		/* if we haven't recieved the enter notify yet, wait */
		if (ActiveMenu && !ActiveMenu->entered)
			continue;
#endif

		XFindContext(dpy, ActiveMenu->w.win, ScreenContext, (caddr_t *)&Scr);

		JunkWidth = ActiveMenu->width;
		JunkHeight = ActiveMenu->height;
		/* was 'Scr->use3Dmenus' - djhjr - 8/11/98 */
		if (Scr->MenuBevelWidth > 0)
		{
			x -= Scr->MenuBevelWidth;
			y -= Scr->MenuBevelWidth;

			JunkWidth -= 2 * Scr->MenuBevelWidth;
			JunkHeight -= Scr->MenuBevelWidth;
		}

/* djhjr - 5/22/00
		if (x < 0 || y < 0 || x >= JunkWidth || y >= JunkHeight)
*/
		if ((x < 0 || y < 0 || x >= JunkWidth || y >= JunkHeight) ||
				(ActiveMenu->too_tall && (y < Scr->MenuScrollBorderWidth ||
						y > JunkHeight - Scr->MenuScrollBorderWidth)))
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
/* djhjr - 5/22/00
		entry = y / Scr->EntryHeight;
*/
		entry = (y / Scr->EntryHeight) + ActiveMenu->top;
		for (i = 0, mi = ActiveMenu->first; mi != NULL; i++, mi=mi->next)
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

		/* djhjr - 5/22/00 */
		if (ActiveMenu->too_tall && y + Scr->EntryHeight > JunkHeight)
			continue;

		/* if we weren't on the active item, change the active item and turn
		 * it on
		 */
		if (!done)
		{
			ActiveItem = mi;

/* djhjr - 5/20/98
			if (ActiveItem->func != F_TITLE && !ActiveItem->state)
*/
			if (ActiveItem && ActiveItem->func != F_TITLE && !ActiveItem->state)
			{
				ActiveItem->state = 1;
				PaintEntry(ActiveMenu, ActiveItem, False);

				if (Scr->StayUpOptionalMenus)            /* PF */
					GlobalFirstTime = firstTime = False; /* PF */
		
			}
		}

		/* now check to see if we were over the arrow of a pull right entry */

/* djhjr - 5/20/98
		if (ActiveItem->func == F_MENU &&
*/
		if (ActiveItem && ActiveItem->func == F_MENU && 

/*			((ActiveMenu->width - x) < (ActiveMenu->width >> 1))) */
			( x > PULLDOWNMENU_OFFSET )) /* DSE */
		{
			MenuRoot *save = ActiveMenu;
			int savex = MenuOrigins[MenuDepth - 1].x;
			int savey = MenuOrigins[MenuDepth - 1].y;

			if (MenuDepth < MAXMENUDEPTH) {
				PopUpMenu (ActiveItem->sub,
				   (savex + PULLDOWNMENU_OFFSET), /* DSE */
				   (savey + ActiveItem->item_num * Scr->EntryHeight)
				   /*(savey + ActiveItem->item_num * Scr->EntryHeight +
					(Scr->EntryHeight >> 1))*/, False);
			} else if (!badItem) {
				DoAudible(); /* was 'XBell()' - djhjr - 6/22/01 */
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

		if (badItem != ActiveItem) badItem = NULL;
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
NewMenuRoot(name)
	char *name;
{
	MenuRoot *tmp;

#define UNUSED_PIXEL ((unsigned long) (~0))	/* more than 24 bits */

	tmp = (MenuRoot *) malloc(sizeof(MenuRoot));

/* djhjr - 5/22/96
	tmp->hi_fore = UNUSED_PIXEL;
	tmp->hi_back = UNUSED_PIXEL;
*/
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

	/* djhjr - 5/22/00 */
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

/* djhjr - 5/4/98
	if (strcmp(name, TWM_WINDOWS) == 0)
*/
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
AddToMenu(menu, item, action, sub, func, fore, back)
	MenuRoot *menu;
	char *item, *action;
	MenuRoot *sub;
	int func;
	char *fore, *back;
{
	MenuItem *tmp;
	int width;
	MyFont *font; /* DSE */

#ifdef DEBUG_MENUS
	fprintf(stderr, "adding menu item=\"%s\", action=%s, sub=%d, f=%d\n",
	item, action, sub, func);
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

	/* djhjr - 4/22/96 */
	tmp->separated = 0;

    if ( func == F_TITLE && (Scr->MenuTitleFont.name != NULL) ) /* DSE */
		font= &(Scr->MenuTitleFont);
    else
		font= &(Scr->MenuFont);

	if (!Scr->HaveFonts) CreateFonts();
	width = MyFont_TextWidth(font,
			item, tmp->strlen);
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

/* djhjr - 4/22/96
	GetColor(COLOR, &tmp->fore, fore);
	GetColor(COLOR, &tmp->back, back);
*/
	GetColor(COLOR, &tmp->normal.fore, fore);
	GetColor(COLOR, &tmp->normal.back, back);

	/* djhjr - 4/22/96 */
	/* was 'Scr->use3Dmenus' - djhjr - 8/11/98 */
	/* rem'd 'Scr->MenuBevelWidth' djhjr - 10/30/02 */
	if (/*Scr->MenuBevelWidth > 0 && */!Scr->BeNiceToColormap) GetShadeColors (&tmp->normal);

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



void MakeMenus()
{
	MenuRoot *mr;

	for (mr = Scr->MenuList; mr != NULL; mr = mr->next)
	{
	if (mr->real_menu == FALSE)
		continue;

	MakeMenu(mr);
	}
}



void MakeMenu(mr)
MenuRoot *mr;
{
	MenuItem *start, *end, *cur, *tmp;
	XColor f1, f2, f3;
	XColor b1, b2, b3;
	XColor save_fore, save_back;
	int num, i;
	int fred, fgreen, fblue;
	int bred, bgreen, bblue;
	int width;

	/* djhjr - 4/22/96 */
	int borderwidth;

	unsigned long valuemask;
	XSetWindowAttributes attributes;
	Colormap cmap = Scr->TwmRoot.cmaps.cwins[0]->colormap->c;
	MyFont *titleFont;

#ifdef TWM_USE_SPACING
	Scr->EntryHeight = 120*Scr->MenuFont.height/100; /*baselineskip 1.2*/
	titleFont = &(Scr->MenuTitleFont);
#else
	if ( Scr->MenuTitleFont.name != NULL ) /* DSE */
		{
		Scr->EntryHeight = MAX(Scr->MenuFont.height,
		                       Scr->MenuTitleFont.height) + 4;
		titleFont = &(Scr->MenuTitleFont);
		}
	else
		{
		Scr->EntryHeight = Scr->MenuFont.height + 4;
		titleFont= &(Scr->MenuFont);
		}
#endif

	/* lets first size the window accordingly */
	if (mr->mapped == NEVER_MAPPED)
	{
	if (mr->pull == TRUE)
	{
		mr->width += 16 + 2 * EDGE_OFFSET; /* DSE */
	}

/* djhjr - 4/29/98
	* djhjr - 9/18/96 *
	if (Scr->use3Dmenus) mr->width += 4;
*/
	/* was 'Scr->use3Dmenus' - djhjr - 8/11/98 */
	if (Scr->MenuBevelWidth > 0) mr->width += 2 * Scr->MenuBevelWidth;

	width = mr->width + 2 * EDGE_OFFSET; /* DSE */

	for (cur = mr->first; cur != NULL; cur = cur->next)
	{
		if (cur->func != F_TITLE)
		cur->x = EDGE_OFFSET; /* DSE */
		else
		{
		cur->x = width -
			MyFont_TextWidth(titleFont,
				cur->item, cur->strlen);
		cur->x /= 2;
		}
	}
	mr->height = mr->items * Scr->EntryHeight;

/* djhjr - 4/29/98
	* djhjr - 4/22/96 *
	if (Scr->use3Dmenus) mr->height += 4;
*/
	/* was 'Scr->use3Dmenus' - djhjr - 8/11/98 */
	if (Scr->MenuBevelWidth > 0) mr->height += 2 * Scr->MenuBevelWidth;

	/* djhjr - 4/22/96 */
	/* was 'Scr->use3Dmenus' - djhjr - 8/11/98 */
	borderwidth = (Scr->MenuBevelWidth > 0) ? 0 : 1;

	/* djhjr - 5/22/00 */
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
		if (Scr->SaveUnder) {
		valuemask |= CWSaveUnder;
		attributes.save_under = True;
		}
		mr->shadow = XCreateWindow (dpy, Scr->Root, 0, 0,
					(unsigned int) mr->width,
					(unsigned int) mr->height,
					(unsigned int)0,
					CopyFromParent,
					(unsigned int) CopyFromParent,
					(Visual *) CopyFromParent,
					valuemask, &attributes);
	}

	valuemask = (CWBackPixel | CWBorderPixel | CWEventMask);
	attributes.background_pixel = Scr->MenuC.back;
	attributes.border_pixel = Scr->MenuC.fore;
	attributes.event_mask = (ExposureMask | EnterWindowMask);
	if (Scr->SaveUnder) {
		valuemask |= CWSaveUnder;
		attributes.save_under = True;
	}
	if (Scr->BackingStore) {
		valuemask |= CWBackingStore;
		attributes.backing_store = Always;
	}

	mr->w.win = XCreateWindow (dpy, Scr->Root, 0, 0, (unsigned int) mr->width,

/* djhjr - 4/22/96
				   (unsigned int) mr->height, (unsigned int) 1,
*/
				   (unsigned int) mr->height, (unsigned int) borderwidth,

				   CopyFromParent, (unsigned int) CopyFromParent,
				   (Visual *) CopyFromParent,
				   valuemask, &attributes);

#ifdef TWM_USE_XFT
	if (Scr->use_xft > 0)
	    mr->w.xft = MyXftDrawCreate (dpy, mr->w.win, Scr->d_visual,
				XDefaultColormap (dpy, Scr->screen));
#endif
#ifdef TWM_USE_OPACITY
	SetWindowOpacity (mr->w.win, Scr->MenuOpacity);
#endif

	XSaveContext(dpy, mr->w.win, MenuContext, (caddr_t)mr);
	XSaveContext(dpy, mr->w.win, ScreenContext, (caddr_t)Scr);

	mr->mapped = UNMAPPED;
	}

	/* was 'Scr->use3Dmenus' - djhjr - 8/11/98 */
    if (Scr->MenuBevelWidth > 0 && (Scr->Monochrome == COLOR) &&  (mr->highlight.back == UNUSED_PIXEL)) {
	XColor xcol;
	char colname [32];
	short save;

	xcol.pixel = Scr->MenuC.back;
	XQueryColor (dpy, cmap, &xcol);
	sprintf (colname, "#%04x%04x%04x", 
		5 * (xcol.red / 6), 5 * (xcol.green / 6), 5 * (xcol.blue / 6));
	save = Scr->FirstTime;
	Scr->FirstTime = True;
	GetColor (Scr->Monochrome, &mr->highlight.back, colname);
	Scr->FirstTime = save;
    }

	/* djhjr - 4/22/96 */
	/* was 'Scr->use3Dmenus' - djhjr - 8/11/98 */
    if (Scr->MenuBevelWidth > 0 && (Scr->Monochrome == COLOR) && (mr->highlight.fore == UNUSED_PIXEL)) {
	XColor xcol;
	char colname [32];
	short save;
	xcol.pixel = Scr->MenuC.fore;
	XQueryColor (dpy, cmap, &xcol);
	sprintf (colname, "#%04x%04x%04x",
		5 * (xcol.red / 6), 5 * (xcol.green / 6), 5 * (xcol.blue / 6));
	save = Scr->FirstTime;
	Scr->FirstTime = True;
	GetColor (Scr->Monochrome, &mr->highlight.fore, colname);
	Scr->FirstTime = save;
    }
	/* was 'Scr->use3Dmenus' - djhjr - 8/11/98 */
    if (Scr->MenuBevelWidth > 0 && !Scr->BeNiceToColormap) GetShadeColors (&mr->highlight);

	/* get the default colors into the menus */
	for (tmp = mr->first; tmp != NULL; tmp = tmp->next)
	{
/* djhjr - 4/22/96
	if (!tmp->user_colors) {
		if (tmp->func != F_TITLE) {
		tmp->fore = Scr->MenuC.fore;
		tmp->back = Scr->MenuC.back;
		} else {
		tmp->fore = Scr->MenuTitleC.fore;
		tmp->back = Scr->MenuTitleC.back;
		}
	}

	if (mr->hi_fore != UNUSED_PIXEL)
	{
		tmp->hi_fore = mr->hi_fore;
		tmp->hi_back = mr->hi_back;
	}
	else
	{
		tmp->hi_fore = tmp->back;
		tmp->hi_back = tmp->fore;
	}
*/
	if (!tmp->user_colors) {
	    if (tmp->func != F_TITLE) {
		tmp->normal.fore = Scr->MenuC.fore;
		tmp->normal.back = Scr->MenuC.back;
	    } else {
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
	/* was 'Scr->use3Dmenus' - djhjr - 8/11/98 */
	if (Scr->MenuBevelWidth > 0 && !Scr->BeNiceToColormap) {
	    if (tmp->func != F_TITLE)
		GetShadeColors (&tmp->highlight);
	    else
		GetShadeColors (&tmp->normal);
	}

#ifdef TWM_USE_XFT
	if (Scr->use_xft > 0) {
	    CopyPixelToXftColor (XDefaultColormap (dpy, Scr->screen),
			    tmp->normal.fore, &tmp->normal.xft);
	    CopyPixelToXftColor (XDefaultColormap (dpy, Scr->screen),
			    tmp->highlight.fore, &tmp->highlight.xft);
	}
#endif

	} /* end for(...) */

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

/* djhjr - 4/22/96
	f1.pixel = start->fore;
	XQueryColor(dpy, cmap, &f1);
	f2.pixel = end->fore;
	XQueryColor(dpy, cmap, &f2);

	b1.pixel = start->back;
	XQueryColor(dpy, cmap, &b1);
	b2.pixel = end->back;
	XQueryColor(dpy, cmap, &b2);
*/
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

	/* djhjr - 4/23/96 */
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
			continue; /* DSE -- from tvtwm */
		
		XAllocColor(dpy, cmap, &f3);
		XAllocColor(dpy, cmap, &b3);

/* djhjr - 4/22/96
		cur->hi_back = cur->fore = f3.pixel;
		cur->hi_fore = cur->back = b3.pixel;
*/
		cur->highlight.back = cur->normal.fore = f3.pixel;
		cur->highlight.fore = cur->normal.back = b3.pixel;
		cur->user_colors = True;

		f3 = save_fore;
		b3 = save_back;
#ifdef TWM_USE_XFT
		if (Scr->use_xft > 0) {
		    CopyPixelToXftColor (XDefaultColormap (dpy, Scr->screen),
				cur->normal.fore, &cur->normal.xft);
		    CopyPixelToXftColor (XDefaultColormap (dpy, Scr->screen),
				cur->highlight.fore, &cur->highlight.xft);
		}
#endif
	}
	start = end;

	/* djhjr - 4/22/96
	start->highlight.back = start->normal.fore;
	start->highlight.fore = start->normal.back;
	*/
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

Bool PopUpMenu (menu, x, y, center)
	MenuRoot *menu;
	int x, y;
	Bool center;
{
	int WindowNameOffset, WindowNameCount;
	TwmWindow **WindowNames;
	TwmWindow *tmp_win2,*tmp_win3;
	int mask;
	int i;
	int (*compar)() =
	  (Scr->CaseSensitive ? strcmp : XmuCompareISOLatin1);

	/* djhjr - 9/5/98 */
	int x_root, y_root;

	if (!menu) return False;

/* djhjr - 6/22/01 */
#ifndef NO_SOUND_SUPPORT
	if (!PlaySound(F_MENU)) PlaySound(S_MMAP);
#endif

	/* djhjr - 5/22/00 */
	menu->top = 0;
	if (menu->w.win) XClearArea(dpy, menu->w.win, 0, 0, 0, 0, True);

	InstallRootColormap();

	if (menu == Scr->Windows)
	{
	TwmWindow *tmp_win;

	/* this is the twm windows menu,  let's go ahead and build it */

	DestroyMenu (menu);

	menu->first = NULL;
	menu->last = NULL;
	menu->items = 0;
	menu->width = 0;
	menu->mapped = NEVER_MAPPED;

/* djhjr - 5/4/98
  	AddToMenu(menu, "TWM Windows", NULLSTR, (MenuRoot *)NULL, F_TITLE,NULLSTR,NULLSTR);
*/
  	AddToMenu(menu, "VTWM Windows", NULLSTR, (MenuRoot *)NULL, F_TITLE,NULLSTR,NULLSTR);

		WindowNameOffset=(char *)Scr->TwmRoot.next->name -
							   (char *)Scr->TwmRoot.next;
		for(tmp_win = Scr->TwmRoot.next , WindowNameCount=0;
			tmp_win != NULL;
			tmp_win = tmp_win->next)
		  WindowNameCount++;
		  
	    if (WindowNameCount != 0)	/* Submitted by Jennifer Elaan */
	    {
		WindowNames =
		  (TwmWindow **)malloc(sizeof(TwmWindow *)*WindowNameCount);
		WindowNames[0] = Scr->TwmRoot.next;
		for(tmp_win = Scr->TwmRoot.next->next , WindowNameCount=1;
			tmp_win != NULL;
			tmp_win = tmp_win->next,WindowNameCount++)
		{
			/* Submitted by Erik Agsjo <erik.agsjo@aktiedirekt.com> */
			if (LookInList(Scr->DontShowInTWMWindows, tmp_win->full_name, &tmp_win->class))
			{
				WindowNameCount--;
				continue;
			}

			tmp_win2 = tmp_win;
			for (i=0;i<WindowNameCount;i++)
			{
				if ((*compar)(tmp_win2->name,WindowNames[i]->name) < 0)
				{
					tmp_win3 = tmp_win2;
					tmp_win2 = WindowNames[i];
					WindowNames[i] = tmp_win3;
				}
			}
			WindowNames[WindowNameCount] = tmp_win2;
		}
		for (i=0; i<WindowNameCount; i++)
		{
			AddToMenu(menu, WindowNames[i]->name, (char *)WindowNames[i],
					  (MenuRoot *)NULL, F_POPUP,NULLSTR,NULLSTR);
			if (!Scr->OldFashionedTwmWindowsMenu
			&& Scr->Monochrome == COLOR)/*RFBCOLOR*/
			{/*RFBCOLOR*/
				menu->last->user_colors = TRUE;/*RFBCOLOR*/

/* djhjr - 4/22/96
				menu->last->fore =
					WindowNames[i]->virtual.fore;*RFBCOLOR*
*/
/* djhjr - 5/4/98
				menu->last->normal.fore =
					WindowNames[i]->virtual.fore;*RFBCOLOR*
*/
				menu->last->normal.fore = Scr->MenuC.fore;

/* djhjr - 4/22/96
				menu->last->back =
					WindowNames[i]->virtual.back;*RFBCOLOR*
*/
				menu->last->normal.back =
					WindowNames[i]->virtual.back;

/**********************************************************/
/*														  */
/*	Okay, okay, it's a bit of a kludge.					  */
/*														  */
/*	On the other hand, it's nice to have the VTWM Windows */
/*	menu come up with "the right colors". And the colors  */
/*	from the panner are not a bad choice...				  */
/*														  */
/**********************************************************/
			}/*RFBCOLOR*/
		}
		free(WindowNames);
	    }
	MakeMenu(menu);
	}

	if (menu->w.win == None || menu->items == 0) return False;

	/* Prevent recursively bringing up menus. */
	if (menu->mapped == MAPPED) return False;

	/*
	 * Dynamically set the parent;	this allows pull-ups to also be main
	 * menus, or to be brought up from more than one place.
	 */
	menu->prev = ActiveMenu;

	/*
	 * Submitted by Steve Ratcliffe
	 */
	mask = ButtonPressMask | ButtonReleaseMask |
	ButtonMotionMask | PointerMotionHintMask;
	if (Scr->StayUpMenus)
		mask |= PointerMotionMask;

	XGrabPointer(dpy, Scr->Root, True, mask,
		GrabModeAsync, GrabModeAsync,
		Scr->Root, Scr->MenuCursor, CurrentTime);

	ActiveMenu = menu;
	menu->mapped = MAPPED;
	menu->entered = FALSE;

	if (center) {
		x -= (menu->width / 2);
		y -= (Scr->EntryHeight / 2);	/* sticky menus would be nice here */
	}

	/*
	 * clip to screen
	 */
	/* next line and " - i" to "x = " and "y = " - djhjr - 5/22/00 */
	i = (Scr->MenuBevelWidth > 0) ? 0 : 2;
	if (x + menu->width > Scr->MyDisplayWidth) {
		x = Scr->MyDisplayWidth - menu->width - i;
	}
	if (x < 0) x = 0;
	if (y + menu->height > Scr->MyDisplayHeight) {
		y = Scr->MyDisplayHeight - menu->height - i;
	}
	if (y < 0) y = 0;

	MenuOrigins[MenuDepth].x = x;
	MenuOrigins[MenuDepth].y = y;
	MenuDepth++;

	XMoveWindow(dpy, menu->w.win, x, y);
	if (Scr->Shadow) {
	XMoveWindow (dpy, menu->shadow, x + SHADOWWIDTH, y + SHADOWWIDTH);
	}
	if (Scr->Shadow) {
	XRaiseWindow (dpy, menu->shadow);
	}
	XMapRaised(dpy, menu->w.win);
	if (Scr->Shadow) {
	XMapWindow (dpy, menu->shadow);
	}
	XSync(dpy, 0);

	/* djhjr - 9/5/98 */
	XQueryPointer(dpy, menu->w.win, &JunkRoot, &JunkChild,
			&x_root, &y_root, &JunkX, &JunkY, &JunkMask);
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

void PopDownMenu()
{
	MenuRoot *tmp;

	if (ActiveMenu == NULL)
	return;

/* djhjr - 6/22/01 */
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
	if (Scr->Shadow) {
		XUnmapWindow (dpy, tmp->shadow);
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
	{  menuFromFrameOrWindowOrTitlebar = TRUE;
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
FindMenuRoot(name)
	char *name;
{
	MenuRoot *tmp;

	for (tmp = Scr->MenuList; tmp != NULL; tmp = tmp->next)
	{
	if (strcmp(name, tmp->name) == 0)
		return (tmp);
	}
	return NULL;
}



static Bool belongs_to_twm_window (t, w)
	register TwmWindow *t;
	register Window w;
{
	if (!t) return False;

#if 0
StayUpMenus
	if (w == t->frame || w == t->title_w.win || w == t->hilite_w ||
	w == t->icon_w.win || w == t->icon_bm_w) return True;
#endif

	if (t && t->titlebuttons) {
	register TBWindow *tbw;
	register int nb = Scr->TBInfo.nleft + Scr->TBInfo.nright;
	for (tbw = t->titlebuttons; nb > 0; tbw++, nb--) {
		if (tbw->window == w) return True;
	}
	}
	return False;
}



/*
 * Hack^H^H^H^HWrapper to moves for non-menu contexts.
 *
 * djhjr - 10/11/01 10/4/02
 */
static void moveFromCenterWrapper(tmp_win)
TwmWindow *tmp_win;
{
	if (!tmp_win->opaque_move) XUngrabServer(dpy);

	WarpScreenToWindow(tmp_win);

	/* now here's a nice little kludge... */
	{
		int hilite = tmp_win->highlight;

		tmp_win->highlight = True;
		SetBorder(tmp_win, (hilite) ? True : False);
		tmp_win->highlight = hilite;

		Scr->Focus = tmp_win;
	}

	if (!tmp_win->opaque_move) XGrabServer(dpy);
}

/*
 * Jason P. Venner jason@tfs.com
 * This function is used by F_WARPTO to match the action name
 * against window names.
 * Re-written to use list.c:MatchName(), allowing VTWM-style wilcards.
 * djhjr - 10/27/02
 */
int MatchWinName(action, t)
char		*action;
TwmWindow	*t;
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
ExecuteFunction(func, action, w, tmp_win, eventp, context, pulldown)
	int func;
	char *action;
	Window w;
	TwmWindow *tmp_win;
	XEvent *eventp;
	int context;
	int pulldown;
{
	char tmp[200];
	char *ptr;
	char buff[MAX_FILE_SIZE];
	int count, fd;
	int do_next_action = TRUE;

	actionHack = action; /* Submitted by Michel Eyckmans */
	RootFunction = F_NOFUNCTION;
	if (Cancel)
	return TRUE;			/* XXX should this be FALSE? */

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
	case F_WARP:          /* PF */
	case F_WARPCLASSNEXT: /* PF */
	case F_WARPCLASSPREV: /* PF */
	case F_WARPTOSCREEN:
	case F_WARPTO:
	case F_WARPRING:
	case F_WARPTOICONMGR:
	case F_COLORMAP:

	/* djhjr - 4/30/96 */
	case F_SEPARATOR:

	/* djhjr - 12/14/98 */
	case F_STATICICONPOSITIONS:

	/* djhjr - 5/30/00 */
	case F_WARPSNUG:
	case F_WARPVISIBLE:

/* djhjr - 6/22/01 */
#ifndef NO_SOUND_SUPPORT
	case F_SOUNDS:
#endif

	/* djhjr - 10/2/01 */
	case F_STRICTICONMGR:

	/* djhjr - 9/9/02 */
	case F_BINDBUTTONS:
	case F_BINDKEYS:
	case F_UNBINDBUTTONS:
	case F_UNBINDKEYS:

	break;
	default:
		XGrabPointer(dpy, Scr->Root, True,
			ButtonPressMask | ButtonReleaseMask,
			GrabModeAsync, GrabModeAsync,
			Scr->Root, Scr->WaitCursor, CurrentTime);
	break;
	}

/* djhjr - 6/22/01 */
#ifndef NO_SOUND_SUPPORT
	switch (func)
	{
		case F_BEEP:
		case F_SQUEEZECENTER:
		case F_SQUEEZELEFT:
		case F_SQUEEZERIGHT:

		/* djhjr - 11/4/03 */
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

		/* djhjr - 9/9/02 */
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
	if (WindowMoved) do_next_action = FALSE;
	break;

	case F_RESTART:

	/* added this 'case' and 'if () ... else ' - djhjr - 7/15/98 */
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
				new_argv = (char *)realloc((char *)my_argv,
							   i * sizeof(char *));
				if (new_argv == NULL)
				{
					fprintf(stderr,
						"%s: unable to allocate %d bytes for execvp()\n",
						ProgramName, i * sizeof(char *));
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

			/* djhjr - 7/31/98 */
			setup_restart(eventp->xbutton.time);

			execvp(*my_argv, my_argv);
			fprintf(stderr, "%s:  unable to start \"%s\"\n",
				ProgramName, *my_argv);
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
		/* djhjr - 7/31/98 */
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

	/* added this 'if (...) else ...' - djhjr - 9/21/99 */
	if (context == C_ROOT)
	{
		name_list *list;

		ShowIconMgr(&Scr->iconmgr);

		/*
		 * New code in list.c necessitates 'next_entry()' and
		 * 'contents_of_entry()' - djhjr - 10/20/01
		 */
		for (list = Scr->IconMgrs; list != NULL; list = next_entry(list))
			ShowIconMgr((IconMgr *)contents_of_entry(list));
	}
	else
	{
		IconMgr *ip;

		if ((ip = (IconMgr *)LookInList(Scr->IconMgrs, tmp_win->full_name,
				&tmp_win->class)) == NULL)
			ip = &Scr->iconmgr;

		ShowIconMgr(ip);
	}

	RaiseStickyAbove(); /* DSE */
	RaiseAutoPan();
	
	break;

    case F_HIDELIST:

	if (Scr->NoIconManagers)
	    break;

	/* added argument - djhjr - 9/21/99 */
	HideIconManager((context == C_ROOT) ? NULL : tmp_win);

	break;

    case F_SORTICONMGR:

	/* djhjr - 6/10/98 */
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
		DoAudible(); /* was 'XBell()' - djhjr - 6/22/01 */

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
	Identify ((TwmWindow *) NULL);
	break;

	case F_ZOOMZOOM: /* RFB silly */
		/* added args to iconmgrs - djhjr - 10/11/01 */
		Zoom( None, NULL, None, NULL );
		break;

	case F_AUTOPAN:/*RFB F_AUTOPAN*/
	{ /* toggle autopan *//*RFB F_AUTOPAN*/
		static int saved;/*RFB F_AUTOPAN*/

		if ( Scr->AutoPanX )
		{	saved = Scr->AutoPanX;/*RFB F_AUTOPAN*/
			Scr->AutoPanX = 0;/*RFB F_AUTOPAN*/
		} else { /*RFB F_AUTOPAN*/
			Scr->AutoPanX = saved;/*RFB F_AUTOPAN*/
			/* if restart with no autopan, we'll set the
			** variable but we won't pan
			*/
			RaiseAutoPan(); /* DSE */
		}/*RFB F_AUTOPAN*/
		break;/*RFB F_AUTOPAN*/
	}/*RFB F_AUTOPAN*/
	
	case F_STICKYABOVE: /* DSE */
		if (Scr->StickyAbove) {
			LowerSticky(); Scr->StickyAbove = FALSE;
			/* don't change the order of execution! */
		} else {
			Scr->StickyAbove = TRUE; RaiseStickyAbove(); RaiseAutoPan();
			/* don't change the order of execution! */
		}
		return TRUE;
		/* break; *//* NOT REACHABLE */

    case F_AUTORAISE:
	if (DeferExecution(context, func, Scr->SelectCursor))
	    return TRUE;

	tmp_win->auto_raise = !tmp_win->auto_raise;
	if (tmp_win->auto_raise) ++(Scr->NumAutoRaises);
	else --(Scr->NumAutoRaises);
	break;

    case F_BEEP:

/* djhjr - 6/22/01 */
#ifndef NO_SOUND_SUPPORT
	/* sound has priority over bell */
	if (PlaySound(func)) break;
#endif

	DoAudible(); /* was 'XBell()' - djhjr - 6/22/01 */
	break;

    case F_POPUP:
	tmp_win = (TwmWindow *)action;
	if (Scr->WindowFunction.func != F_NOFUNCTION)
	{
	   ExecuteFunction(Scr->WindowFunction.func,
			   Scr->WindowFunction.item->action,
			   w, tmp_win, eventp, C_FRAME, FALSE);
	}
	else
	{
	    DeIconify(tmp_win);
	    XRaiseWindow (dpy, tmp_win->frame);
	    XRaiseWindow (dpy, tmp_win->VirtualDesktopDisplayWindow.win);
	    
	    RaiseStickyAbove();
	    RaiseAutoPan();
	}
	break;

    case F_RESIZE:
	{
	    TwmWindow *focused = NULL;		/* djhjr - 5/27/03 */
	    Bool fromtitlebar = False;
	    long releaseEvent;
	    long movementMask;
	    int resizefromcenter = 0;		/* djhjr - 10/2/02 */
/* djhjr - 10/6/02 */
#ifndef NO_SOUND_SUPPORT
	    int did_playsound = FALSE;
#endif

	    if (DeferExecution(context, func, Scr->ResizeCursor))
		return TRUE;

	    PopDownMenu();

	    if (pulldown)
		XWarpPointer(dpy, None, Scr->Root, 0, 0, 0, 0,
			     eventp->xbutton.x_root,
			     eventp->xbutton.y_root);

	    EventHandler[EnterNotify] = HandleUnknown;
	    EventHandler[LeaveNotify] = HandleUnknown;

/* allow the resizing of doors - djhjr - 2/22/99
	    if ((w != tmp_win->icon_w.win) && (context != C_DOOR))
*/
	    if (context == C_ICON) /* can't resize icons */
	    {
		DoAudible(); /* was 'XBell()' - djhjr - 6/22/01 */
		break;
	    }

	    /*
	     * Resizing from a titlebar menu was handled uniquely long
	     * before I got here, and I added virtual windows and icon
	     * managers on 9/15/99 and 10/11/01, leveraging that code.
	     * It's all been integrated here.
	     * djhjr - 10/3/02
	     */
	    if (Context & (C_FRAME_BIT | C_WINDOW_BIT | C_TITLE_BIT)
			&& menuFromFrameOrWindowOrTitlebar)
	    {
		XGetGeometry(dpy, w, &JunkRoot, &origDragX, &origDragY,
			     (unsigned int *)&DragWidth,
			     (unsigned int *)&DragHeight,
			     &JunkBW, &JunkDepth);

		resizefromcenter = 2;
	    }
	    else if (Context == C_VIRTUAL_WIN)
	    {
		TwmWindow *twin;

		if ((XFindContext(dpy, eventp->xbutton.subwindow,
				  VirtualContext, (caddr_t *) &twin) == XCNOENT))
		{
		    DoAudible(); /* was 'XBell()' - djhjr - 6/22/01 */
		    break;
		}

		context = C_WINDOW;
		tmp_win = twin;
		resizefromcenter = 1;
	    }
	    else if (Context == C_ICONMGR && tmp_win->list)
	    {
		/* added the second argument - djhjr - 5/28/00 */
		if (!warp_if_warpunmapped(tmp_win, F_NOFUNCTION))
		{
		    DoAudible(); /* was 'XBell()' - djhjr - 6/22/01 */
		    break;
		}

		resizefromcenter = 1;
	    }

	    if (resizefromcenter)
	    {
		WarpScreenToWindow(tmp_win);

		XWarpPointer(dpy, None, Scr->Root, 0, 0, 0, 0,
			     tmp_win->frame_x + tmp_win->frame_width / 2,
			     tmp_win->frame_y + tmp_win->frame_height / 2);

		/* grr - djhjr - 5/27/03 */
		focused = Scr->Focus;
		Scr->Focus = tmp_win;
		SetBorder(Scr->Focus, True);

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
	    fromtitlebar = belongs_to_twm_window(tmp_win,
						 eventp->xbutton.window);

	    if (resizefromcenter == 2)
	    {
		MenuStartResize(tmp_win, origDragX, origDragY,
				DragWidth, DragHeight, Context);

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

	    /* substantially re-worked - djhjr - 5/27/03 */
	    while (TRUE)
	    {
		/* added exposure event masks - djhjr - 10/11/01 */
		XMaskEvent(dpy, ButtonPressMask | ButtonReleaseMask |
			   EnterWindowMask | LeaveWindowMask |
			   ExposureMask | VisibilityChangeMask |
			   movementMask, &Event);

/*
 * See down below, after this loop - djhjr - 5/27/03
 */
#if 0
		/* discard crossing events before a release - djhjr - 10/11/01 */
		if (Event.xany.type == EnterNotify ||
				Event.xany.type == LeaveNotify)
		{
		    /* this can't be the proper place - djhjr - 10/2/02 */
		    SetBorder(tmp_win, True);

		    continue;
		}
#endif

		/*
		 * Don't discard exposure events before release
		 * or window borders and/or their titles in the
		 * virtual desktop won't get redrawn - djhjr
		 */

		/* discard any extra motion events before a release */
		if (Event.type == MotionNotify)
		{
		    /* was 'ButtonMotionMask' - djhjr - 10/11/01 */
		    while (XCheckMaskEvent(dpy, releaseEvent | movementMask,
					   &Event))
		    {
			if (Event.type == releaseEvent)
			    break;
		    }
		}

/*
 * See above, before this loop - djhjr - 5/27/03
 */
#if 0
		if (fromtitlebar && Event.type == ButtonPress) {
		    fromtitlebar = False;
		    continue;
		}
#endif

		if (Event.type == releaseEvent)
		{
		    if (Cancel)
		    {
			if (tmp_win->opaque_resize)
			{
			    ConstrainSize(tmp_win, &origWidth, &origHeight);
			    SetupWindow(tmp_win, origx, origy,
					origWidth, origHeight, -1);
			    ResizeTwmWindowContents(tmp_win,
						    origWidth, origHeight);
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
			    /* added passing of 'Context' - djhjr - 9/30/02 */
			    MenuEndResize(tmp_win, Context);
			}
			else
			    EndResize();

			/* DispatchEvent2() is depreciated - djhjr - 10/6/02 */
			DispatchEvent();

			/* djhjr - 5/27/03 11/2/03 */
			if (!Scr->NoRaiseResize && !Scr->RaiseOnStart &&
				WindowMoved)
			{
			    XRaiseWindow(dpy, tmp_win->frame);
			    SetRaiseWindow(tmp_win);
			}
		    }

		    break;
		}

		/* DispatchEvent2() is depreciated - djhjr - 10/6/02 */
		if (!DispatchEvent()) continue;

		if (Event.type != MotionNotify) continue;

		XQueryPointer(dpy, Scr->Root,
			      &JunkRoot, &JunkChild, &JunkX, &JunkY,
			      &AddingX, &AddingY, &JunkMask);

		if (!resizing_window &&
				(abs(AddingX - ResizeOrigX) < Scr->MoveDelta &&
				 abs(AddingY - ResizeOrigY) < Scr->MoveDelta))
		{
		    continue;
		}

		resizing_window = 1;
		WindowMoved = TRUE;

		/* djhjr - 5/27/03 11/3/03 */
		if ((!Scr->NoRaiseResize && Scr->RaiseOnStart)
			/* trap a Shape extention bug - djhjr - 5/27/03 */
			|| (tmp_win->opaque_resize &&
			(HasShape &&
			(tmp_win->wShaped || tmp_win->squeeze_info)))
		   )
		{
		    XRaiseWindow(dpy, tmp_win->frame);
		    SetRaiseWindow(tmp_win);
		    if (Scr->Virtual && tmp_win->VirtualDesktopDisplayWindow.win)
			XRaiseWindow(dpy, tmp_win->VirtualDesktopDisplayWindow.win);
		}

/* djhjr - 6/22/01 */
#ifndef NO_SOUND_SUPPORT
		if (did_playsound == FALSE)
		{
		    PlaySound(func);
		    did_playsound = TRUE;
		}
#endif

		/* MenuDoResize() is depreciated - djhjr - 10/6/02 */
		DoResize(AddingX, AddingY, tmp_win);
	    }

/* djhjr - 6/4/98
	    return TRUE;
*/

/* djhjr - 7/17/98
	    * djhjr - 4/7/98 *
	    if (!Scr->NoGrabServer) XUngrabServer(dpy);
*/
	    if (!tmp_win->opaque_resize) XUngrabServer(dpy);

	    /*
	     * All this stuff from resize.c:EndResize() - djhjr - 10/6/02
	     */

	    if (!tmp_win->opaque_resize)
		UninstallRootColormap();

	    /* discard queued enter and leave events - djhjr - 5/27/03 */
	    while (XCheckMaskEvent(dpy, EnterWindowMask | LeaveWindowMask,
				   &Event))
		;

	    if (!Scr->NoRaiseResize)
	    {
		RaiseStickyAbove (); /* DSE */
		RaiseAutoPan();
	    }

	    /* update virtual coords */
	    tmp_win->virtual_frame_x = Scr->VirtualDesktopX + tmp_win->frame_x;
	    tmp_win->virtual_frame_y = Scr->VirtualDesktopY + tmp_win->frame_y;

	    /* UpdateDesktop(tmp_win); Stig */
/* djhjr - 5/27/03
	    MoveResizeDesktop(tmp_win,  Scr->NoRaiseResize); * Stig *
*/
	    MoveResizeDesktop(tmp_win,  Cancel | Scr->NoRaiseResize); /* Stig */

	    /* djhjr - 9/30/02 10/6/02 */
	    if (Context == C_VIRTUAL_WIN)
	    {
		/*
		 * Mask a bug that calls MoveOutline(zeros) after the
		 * border has been repainted, leaving artifacts. I think
		 * I know what the bug is, but I can't seem to fix it.
		 */
		if (Scr->BorderBevelWidth > 0) PaintBorders(tmp_win, False);

		JunkX = tmp_win->virtual_frame_x + tmp_win->frame_width / 2;
		JunkY = tmp_win->virtual_frame_y + tmp_win->frame_height / 2;
		XWarpPointer(dpy, None, Scr->VirtualDesktopDisplayOuter,
			     0, 0, 0, 0, SCALE_D(JunkX), SCALE_D(JunkY));

		/* grr - djhjr - 5/27/03 */
		SetBorder(Scr->Focus, False);
		Scr->Focus = focused;
	    }

	    /* djhjr - 6/4/98 */
	    /* don't re-map if the window is the virtual desktop - djhjr - 2/28/99 */
	    if (Scr->VirtualReceivesMotionEvents &&
			/* !tmp_win->opaque_resize && */
			tmp_win->w != Scr->VirtualDesktopDisplayOuter)
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

	/* djhjr - 4/1/00 */
	PopDownMenu();

	fullzoom(tmp_win, func);
	/* UpdateDesktop(tmp_win); Stig */
	MoveResizeDesktop(tmp_win, Scr->NoRaiseMove); /* Stig */
	break;

    case F_MOVE:
    case F_FORCEMOVE:
	{
	    static Time last_time = 0;
	    Window rootw;
	    Bool fromtitlebar = False;
	    int moving_icon = FALSE;
	    int constMoveDir, constMoveX, constMoveY;
	    int constMoveXL, constMoveXR, constMoveYT, constMoveYB;
	    int origX, origY;
	    long releaseEvent;
	    long movementMask;
	    int xl, yt, xr, yb;
	    int movefromcenter = 0;	/* djhjr - 10/4/02 */
/* djhjr - 6/22/01 */
#ifndef NO_SOUND_SUPPORT
	    int did_playsound = FALSE;
#endif

	    if (DeferExecution(context, func, Scr->MoveCursor))
		return TRUE;

	    PopDownMenu();
	    rootw = eventp->xbutton.root;

	    if (pulldown)
		XWarpPointer(dpy, None, Scr->Root,
			     0, 0, 0, 0, eventp->xbutton.x_root,
			     eventp->xbutton.y_root);

	    EventHandler[EnterNotify] = HandleUnknown;
	    EventHandler[LeaveNotify] = HandleUnknown;

/* djhjr - 4/7/98
	    if (!Scr->NoGrabServer || !Scr->OpaqueMove) XGrabServer(dpy);
*/
/* djhjr - 7/17/98
	    if (!Scr->NoGrabServer) XGrabServer(dpy);
*/
	    if (!tmp_win->opaque_move) XGrabServer(dpy);

/* use initialized size... djhjr - 5/9/96
	    * djhjr - 4/27/96 *
	    Scr->SizeStringOffset = SIZE_HINDENT;
	    XResizeWindow(dpy, Scr->SizeWindow.win,
			  Scr->SizeStringWidth + SIZE_HINDENT * 2, 
			  Scr->SizeFont.height + SIZE_VINDENT * 2);
*/

	    XGrabPointer(dpy, eventp->xbutton.root, True,
			 ButtonPressMask | ButtonReleaseMask |
			 ButtonMotionMask | PointerMotionMask,
			 /* PointerMotionHintMask */
			 GrabModeAsync, GrabModeAsync,
			 Scr->Root, Scr->MoveCursor, CurrentTime);

	    /* added this 'if (...) else' - djhjr - 10/11/01 */
	    if (context == C_VIRTUAL_WIN)
	    {
		TwmWindow *twin;

		if ((XFindContext(dpy, eventp->xbutton.subwindow,
				VirtualContext, (caddr_t *) &twin) == XCNOENT))
		{
		    DoAudible(); /* was 'XBell()' - djhjr - 6/22/01 */
		    break;
		}

		tmp_win = twin;
		moveFromCenterWrapper(tmp_win);
		/* these two - djhjr - 10/4/02 */
		w = tmp_win->frame;
		movefromcenter = 1;
	    }
	    else

	    /* added this 'if (...) else' - djhjr - 9/15/99 */
	    if (context == C_ICONMGR && tmp_win->list)
	    {
		/* added the second argument - djhjr - 5/28/00 */
		if (!warp_if_warpunmapped(tmp_win, F_NOFUNCTION))
		{
		    DoAudible(); /* was 'XBell()' - djhjr - 6/22/01 */
		    break;
		}

		moveFromCenterWrapper(tmp_win); /* djhjr - 10/11/01 */
		/* these two - djhjr - 10/4/02 */
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
		XTranslateCoordinates(dpy, w, tmp_win->frame,
				      eventp->xbutton.x,
				      eventp->xbutton.y,
				      &DragX, &DragY, &JunkChild);

		w = tmp_win->frame;
	    }

	    XMapRaised (dpy, Scr->SizeWindow.win);

	    DragWindow = None;

	    MoveFunction = func;	/* set for DispatchEvent() */

	    XGetGeometry(dpy, w, &JunkRoot, &origDragX, &origDragY,
			 (unsigned int *)&DragWidth,
			 (unsigned int *)&DragHeight,
			 &JunkBW, &JunkDepth);

	    /* added this 'if (...) else' - djhjr - 10/4/02 */
	    if (menuFromFrameOrWindowOrTitlebar ||
			movefromcenter || (moving_icon && fromMenu))
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
	    if ( ConstrainedMoveTime &&
			eventp->xbutton.time - last_time < ConstrainedMoveTime)
	    {
		int width, height;

		ConstMove = TRUE;
		constMoveDir = MOVE_NONE;
		constMoveX = eventp->xbutton.x_root - DragX - JunkBW;
		constMoveY = eventp->xbutton.y_root - DragY - JunkBW;
		width = DragWidth + 2 * JunkBW;
		height = DragHeight + 2 * JunkBW;
		constMoveXL = constMoveX + width/3;
		constMoveXR = constMoveX + 2*(width/3);
		constMoveYT = constMoveY + height/3;
		constMoveYB = constMoveY + 2*(height/3);

		XWarpPointer(dpy, None, w,
			     0, 0, 0, 0, DragWidth/2, DragHeight/2);

		XQueryPointer(dpy, w, &JunkRoot, &JunkChild,
			      &JunkX, &JunkY, &DragX, &DragY, &JunkMask);
	    }
	    last_time = eventp->xbutton.time;

/* djhjr - 4/7/98
	    if (!Scr->OpaqueMove)
*/
	    if (!tmp_win->opaque_move)
	    {
		InstallRootColormap();
		/*if (!Scr->MoveDelta)*/	/* djhjr - 10/2/02 */
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
				tmp_win->frame_bw,
/* djhjr - 4/22/96
				moving_icon ? 0 : tmp_win->title_height);
*/
				moving_icon ? 0 : tmp_win->title_height + tmp_win->frame_bw3D);

		    /*
		     * This next line causes HandleButtonRelease to call
		     * XRaiseWindow().  This is solely to preserve the
		     * previous behaviour that raises a window being moved
		     * on button release even if you never actually moved
		     * any distance (unless you move less than MoveDelta or
		     * NoRaiseMove is set or OpaqueMove is set).
		     *
		     * It's set way down below; no need to force it here.
		     * djhjr - 10/4/02
		     *
		     * The code referred to above is 'if 0'd out now anyway.
		     * djhjr - 10/6/02
		     */
		    /*DragWindow = w;*/
		}
	    }

	    /*
	     * see if this is being done from the titlebar
	     */
	    fromtitlebar = belongs_to_twm_window(tmp_win,
						 eventp->xbutton.window);

	    /* added 'movefromcenter' and 'moving_icon' - djhjr - 10/4/02 */
	    if ((menuFromFrameOrWindowOrTitlebar && !fromtitlebar) ||
			movefromcenter || (moving_icon && fromMenu))
	    {
		/* warp the pointer to the middle of the window */
		XWarpPointer(dpy, None, Scr->Root, 0, 0, 0, 0,
			     origDragX + DragWidth / 2,
			     origDragY + DragHeight / 2);

		SetBorder(tmp_win, True);	/* grr */

		XFlush(dpy);
	    }

	    /* djhjr - 4/27/96 */
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
			   EnterWindowMask | LeaveWindowMask |
			   ExposureMask | VisibilityChangeMask |
			   movementMask, &Event);

/*
 * See down below, after this loop - djhjr - 5/23/03
 */
#if 0
		/* throw away enter and leave events until release */
		if (Event.xany.type == EnterNotify ||
				Event.xany.type == LeaveNotify)
		{
		    continue;
		}
#endif

		/*
		 * Don't discard exposure events before release
		 * or window borders and/or their titles in the
		 * virtual desktop won't get redrawn - djhjr
		 */

		/* discard any extra motion events before a release */
		if (Event.type == MotionNotify)
		{
		    while (XCheckMaskEvent(dpy, movementMask | releaseEvent,
					   &Event))
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
		 * djhjr - 10/6/02
		 */

		if (fromtitlebar && Event.type == ButtonPress)
		{
		    fromtitlebar = False;
		    CurrentDragX = origX = Event.xbutton.x_root;
		    CurrentDragY = origY = Event.xbutton.y_root;
		    XTranslateCoordinates(dpy, rootw, tmp_win->frame,
					  origX, origY,
					  &DragX, &DragY, &JunkChild);

		    continue;
		}

		/* DispatchEvent2() is depreciated - djhjr - 10/6/02 */
		if (!DispatchEvent()) continue;

		/* re-wrote this stuff - djhjr - 10/4/02 5/24/03 11/2/03 */
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
			    XMoveWindow(dpy, tmp_win->icon_w.win,
					CurrentDragX, CurrentDragY);

			    if (!Scr->NoRaiseMove && !Scr->RaiseOnStart)
			    {
				XRaiseWindow(dpy, tmp_win->icon_w.win);
				SetRaiseWindow(tmp_win->icon_w.win);
			    }
			}
			else
			{
			    if (movefromcenter)
			    {
				tmp_win->frame_x = Event.xbutton.x_root -
						   DragWidth / 2;
				tmp_win->frame_y = Event.xbutton.y_root -
						   DragHeight / 2;
			    }
			    else
			    {
				tmp_win->frame_x = CurrentDragX;
				tmp_win->frame_y = CurrentDragY;
			    }

			    XMoveWindow(dpy, tmp_win->frame,
					tmp_win->frame_x, tmp_win->frame_y);
			    SendConfigureNotify(tmp_win, tmp_win->frame_x,
						tmp_win->frame_y);

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
		if (Event.type != MotionNotify) continue;

		XQueryPointer(dpy, rootw, &(eventp->xmotion.root), &JunkChild,
			      &(eventp->xmotion.x_root),
			      &(eventp->xmotion.y_root),
			      &JunkX, &JunkY, &JunkMask);

		if (DragWindow == None &&
			abs(eventp->xmotion.x_root - origX) < Scr->MoveDelta &&
			abs(eventp->xmotion.y_root - origY) < Scr->MoveDelta)
		{
		    continue;
		}

		WindowMoved = TRUE;
		DragWindow = w;

/* djhjr - 4/7/98
		if (!Scr->NoRaiseMove && Scr->OpaqueMove)
*/
/* djhjr - 10/6/02
		if (!Scr->NoRaiseMove && tmp_win->opaque_move)
		    XRaiseWindow(dpy, DragWindow);
*/
		/* djhjr - 5/24/03 11/3/03 */
		if (!Scr->NoRaiseMove && Scr->RaiseOnStart)
		{
		    if (moving_icon)
		    {
			XRaiseWindow(dpy, tmp_win->icon_w.win);
			SetRaiseWindow(tmp_win->icon_w.win);
		    }
		    else
		    {
			XRaiseWindow(dpy, tmp_win->frame);
			SetRaiseWindow(tmp_win);
			if (Scr->Virtual &&
				tmp_win->VirtualDesktopDisplayWindow.win)
			    XRaiseWindow(dpy,
					 tmp_win->VirtualDesktopDisplayWindow.win);
		    }
		}

		if (ConstMove)
		{
		    switch (constMoveDir)
		    {
			case MOVE_NONE:
			    if (eventp->xmotion.x_root < constMoveXL ||
					eventp->xmotion.x_root > constMoveXR)
			    {
				constMoveDir = MOVE_HORIZ;
			    }
			    if (eventp->xmotion.y_root < constMoveYT ||
					eventp->xmotion.y_root > constMoveYB)
			    {
				constMoveDir = MOVE_VERT;
			    }
			    XQueryPointer(dpy, DragWindow, &JunkRoot,
					  &JunkChild, &JunkX, &JunkY,
					  &DragX, &DragY, &JunkMask);
			    break;
			case MOVE_VERT:
			    constMoveY = eventp->xmotion.y_root - DragY -
					 JunkBW;
			    break;
			case MOVE_HORIZ:
			    constMoveX = eventp->xmotion.x_root - DragX -
					 JunkBW;
			    break;
		    }

		    xl = constMoveX;
		    yt = constMoveY;
		}
		else if (DragWindow != None)
		{
		    /* added 'movefromcenter' and 'moving_icon' - djhjr - 10/4/02 */
		    if (!menuFromFrameOrWindowOrTitlebar &&
				!movefromcenter && !(moving_icon && fromMenu))
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

		if ((ConstMove && constMoveDir != MOVE_NONE) ||
			DragWindow != None)
		{
		    int width = DragWidth + 2 * JunkBW;
		    int height = DragHeight + 2 * JunkBW;

		    if (Scr->DontMoveOff && MoveFunction != F_FORCEMOVE)
		    {
			xr = xl + width;
			yb = yt + height;

			if (xl < 0) xl = 0;
			if (xr > Scr->MyDisplayWidth)
			    xl = Scr->MyDisplayWidth - width;

			if (yt < 0) yt = 0;
			if (yb > Scr->MyDisplayHeight)
			    yt = Scr->MyDisplayHeight - height;
		    }

		    CurrentDragX = xl;
		    CurrentDragY = yt;

/* djhjr - 6/22/01 10/6/02 */
#ifndef NO_SOUND_SUPPORT
		    if ((!ConstMove || constMoveDir != MOVE_NONE) &&
				did_playsound == FALSE)
		    {
			PlaySound(func);
			did_playsound = TRUE;
		    }
#endif

/* djhjr - 4/7/98
		    if (Scr->OpaqueMove)
*/
		    if (tmp_win->opaque_move)
			XMoveWindow(dpy, DragWindow, xl, yt);
		    else
			MoveOutline(eventp->xmotion.root, xl, yt,
				    width, height, tmp_win->frame_bw,
/* djhjr - 4/22/96
				    moving_icon ? 0 : tmp_win->title_height);
*/
				    moving_icon ? 0 : tmp_win->title_height + tmp_win->frame_bw3D);

/* djhjr - 4/17/98
		    * move the small representation window
		    * this knows a bit much about the internals i guess
		    * XMoveWindow(dpy, tmp_win->VirtualDesktopDisplayWindow.win, SCALE_D(xl), SCALE_D(yt));
*/
		    if (Scr->VirtualReceivesMotionEvents)
		    {
			tmp_win->virtual_frame_x = R_TO_V_X(xl);
			tmp_win->virtual_frame_y = R_TO_V_Y(yt);
/* djhjr - 5/24/03
			MoveResizeDesktop(tmp_win, Scr->NoRaiseMove);
*/
			MoveResizeDesktop(tmp_win, TRUE);
		    }

		    /* djhjr - 4/27/96 */
		    DisplayPosition (xl, yt);
		}
	    }

/* djhjr - 7/17/98
	    * djhjr - 4/7/98 *
	    if (!Scr->NoGrabServer) XUngrabServer(dpy);
*/
	    if (!tmp_win->opaque_move) XUngrabServer(dpy);

	    /* djhjr - 4/27/96 */
	    XUnmapWindow (dpy, Scr->SizeWindow.win);

	    MovedFromKeyPress = False;

	    if (!tmp_win->opaque_move)
		UninstallRootColormap();

	    /* discard queued enter and leave events - djhjr - 5/23/03 */
	    while (XCheckMaskEvent(dpy, EnterWindowMask | LeaveWindowMask,
				   &Event))
		;

	    /* from events.c:HandleButtonRelease() - djhjr - 10/6/02 */
	    if (!Scr->NoRaiseMove)
	    {
		RaiseStickyAbove(); /* DSE */
		RaiseAutoPan();
	    }

	    /* update virtual coords */
	    tmp_win->virtual_frame_x = Scr->VirtualDesktopX + tmp_win->frame_x;
	    tmp_win->virtual_frame_y = Scr->VirtualDesktopY + tmp_win->frame_y;

	    /* UpdateDesktop() hoses the stacking order - djhjr - 10/6/02 */
/* djhjr - 5/24/03
	    MoveResizeDesktop(tmp_win, Scr->NoRaiseMove);
*/
	    MoveResizeDesktop(tmp_win, Cancel | Scr->NoRaiseMove);

	    /* djhjr - 10/4/02 10/6/02 */
	    if (Context == C_VIRTUAL_WIN)
	    {
		/*
		 * Mask a bug that calls MoveOutline(zeros) after the
		 * border has been repainted, leaving artifacts. I think
		 * I know what the bug is, but I can't seem to fix it.
		 */
		if (Scr->BorderBevelWidth > 0) PaintBorders(tmp_win, False);

		JunkX = tmp_win->virtual_frame_x + tmp_win->frame_width / 2;
		JunkY = tmp_win->virtual_frame_y + tmp_win->frame_height / 2;
		XWarpPointer(dpy, None, Scr->VirtualDesktopDisplayOuter,
			     0, 0, 0, 0, SCALE_D(JunkX), SCALE_D(JunkY));
	    }

	    /* djhjr - 6/4/98 */
	    /* don't re-map if the window is the virtual desktop - djhjr - 2/28/99 */
	    if (Scr->VirtualReceivesMotionEvents &&
			/* !tmp_win->opaque_move && */
			tmp_win->w != Scr->VirtualDesktopDisplayOuter)
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
		fprintf (stderr, "%s: couldn't find function \"%s\"\n",
			 ProgramName, action);
		return TRUE;
	    }

/*
 * Changed this 'if ()' for deferred keyboard events (see also events.c)
 * Submitted by Michel Eyckmans
 *
		if (NeedToDefer(mroot) && DeferExecution(context, func, Scr->SelectCursor))
 */
		if ((cursor = NeedToDefer(mroot)) != None && DeferExecution(context, func, cursor))
			return TRUE;
		else
		{
			for (mitem = mroot->first; mitem != NULL; mitem = mitem->next)
			{
				if (!ExecuteFunction (mitem->func, mitem->action, w,
						tmp_win, eventp, context, pulldown))
				break;
			}
		}
	}
	break;

    case F_DEICONIFY:
    case F_ICONIFY:
	if (DeferExecution(context, func, Scr->SelectCursor))
	    return TRUE;

	/* added '|| (...)' - djhjr - 6/3/03 */
	if (tmp_win->icon ||
		(func == F_DEICONIFY && tmp_win == tmp_win->list->twm))
	{
/* djhjr - 6/3/03 */
#ifndef NO_SOUND_SUPPORT
	    PlaySound(func);
#endif

	    DeIconify(tmp_win);

		/*
		 * now HERE's a fine bit of kludge! it's to mask a hole in the
		 * code I can't find that messes up when trying to warp to the
		 * de-iconified window not in the real screen when WarpWindows
		 * isn't used. see also the change in DeIconify().
		 * djhjr - 1/24/98
		 */
		if (!Scr->WarpWindows && (Scr->WarpCursor ||
				LookInList(Scr->WarpCursorL, tmp_win->full_name, &tmp_win->class)))
		{
			RaiseStickyAbove();                                 /* DSE */
			RaiseAutoPan();                                     

			WarpToWindow(tmp_win);                              /* PF */
		}
	}
	else if (func == F_ICONIFY)
	{
		/* djhjr - 9/10/99 */
		TwmDoor *d;
		TwmWindow *tmgr = NULL, *twin = NULL;
		MenuRoot *mr;

		/* sanity check for what's next - djhjr - 9/10/99 */
		if (XFindContext(dpy, tmp_win->w, DoorContext,
				(caddr_t *)&d) != XCNOENT)
		{
			twin = tmp_win;
			tmp_win = d->twin;
		}

		/*
		 * don't iconify if there's no way to get it back - not fool-proof
		 * djhjr - 9/10/99
		 */
		if (tmp_win->iconify_by_unmapping)
		{
			/* iconified by unmapping */

			if (tmp_win->list) tmgr = tmp_win->list->iconmgr->twm_win;

			if ((tmgr && !tmgr->mapped && tmgr->iconify_by_unmapping) ||
					((Scr->IconManagerDontShow ||
					LookInList(Scr->IconMgrNoShow, tmp_win->full_name, &tmp_win->class)) &&
					LookInList(Scr->IconMgrShow, tmp_win->full_name, &tmp_win->class) == (char *)NULL))
			{
				/* icon manager not mapped or not shown in one */

				if (have_twmwindows == -1)
				{
					have_twmwindows = 0;

					/* better than two calls to FindMenuRoot() */
					for (mr = Scr->MenuList; mr != NULL; mr = mr->next)
						if (strcmp(mr->name, TWM_WINDOWS) == 0 ||
								strcmp(mr->name, VTWM_WINDOWS) == 0)
						{
							/* djhjr - 9/21/99 */
							have_twmwindows = FindMenuOrFuncInBindings(C_ALL_BITS, mr, F_NOFUNCTION);
							break;
						}
				}
				/* djhjr - 9/21/99 */
				if (have_showdesktop == -1)
					have_showdesktop = FindMenuOrFuncInBindings(C_ALL_BITS, NULL, F_SHOWDESKTOP);
				if (have_showlist == -1)
					have_showlist = FindMenuOrFuncInBindings(C_ALL_BITS, NULL, F_SHOWLIST);

				/* djhjr - 9/21/99 */
				if (!FindMenuOrFuncInWindows(tmp_win, have_twmwindows, mr, F_NOFUNCTION) ||
						LookInList(Scr->DontShowInTWMWindows, tmp_win->full_name, &tmp_win->class))
				{
					/* no TwmWindows menu or not shown in it */

					if (tmp_win->w == Scr->VirtualDesktopDisplayOuter &&
							FindMenuOrFuncInWindows(tmp_win, have_showdesktop, NULL, F_SHOWDESKTOP))
						;
					else if (tmp_win->iconmgr &&
							FindMenuOrFuncInWindows(tmp_win, have_showlist, NULL, F_SHOWLIST))
						;
					else if (tmgr &&
							FindMenuOrFuncInWindows(tmgr, have_showlist, NULL, F_SHOWLIST))
						;
					else
					{
						/* no f.showdesktop or f.showiconmgr */

						DoAudible(); /* was 'XBell()' - djhjr - 6/22/01 */

						if (twin) tmp_win = twin;
						break;
					}
				}
			}
		}

		if (twin) tmp_win = twin;

		if (tmp_win->list || !Scr->NoIconifyIconManagers) /* PF */
		{
/* djhjr - 6/3/03 */
#ifndef NO_SOUND_SUPPORT
			PlaySound(func);
#endif

			Iconify (tmp_win, eventp->xbutton.x_root - EDGE_OFFSET, /* DSE */
					eventp->xbutton.y_root - EDGE_OFFSET); /* DSE */
		}
	}
	break;

    case F_RAISELOWER:
	if (DeferExecution(context, func, Scr->SelectCursor))
	    return TRUE;

	if (!WindowMoved) {
	    XWindowChanges xwc;

	    xwc.stack_mode = Opposite;
	    if (w != tmp_win->icon_w.win)
	      w = tmp_win->frame;
	    XConfigureWindow (dpy, w, CWStackMode, &xwc);
	    XConfigureWindow (dpy, tmp_win->VirtualDesktopDisplayWindow.win, CWStackMode, &xwc);
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

	RaiseStickyAbove(); /* DSE */
	RaiseAutoPan();

	break;

    case F_LOWER:
	if (DeferExecution(context, func, Scr->SelectCursor))
	    return TRUE;

	if (!(Scr->StickyAbove && tmp_win->nailed)) { /* DSE */
		if (w == tmp_win->icon_w.win)
		    XLowerWindow(dpy, tmp_win->icon_w.win);
		else
		{    XLowerWindow(dpy, tmp_win->frame);
			XLowerWindow(dpy, tmp_win->VirtualDesktopDisplayWindow.win);
			XLowerWindow(dpy, Scr->VirtualDesktopDScreen);
		}
	} /* DSE */

	break;

    case F_FOCUS:
	if (DeferExecution(context, func, Scr->SelectCursor))
	    return TRUE;

	if (tmp_win->icon == FALSE)
	{
	    if (!Scr->FocusRoot && Scr->Focus == tmp_win)
	    {
		FocusOnRoot();
	    }
	    else
	    {
		if (Scr->Focus != NULL) {
		    SetBorder (Scr->Focus, False);

/* djhjr - 4/25/96
		    if (Scr->Focus->hilite_w)
		      XUnmapWindow (dpy, Scr->Focus->hilite_w);
*/
			PaintTitleHighlight(Scr->Focus, off);

		}

		InstallWindowColormaps (0, tmp_win);

/* djhjr - 4/25/96
		if (tmp_win->hilite_w) XMapWindow (dpy, tmp_win->hilite_w);
*/
		PaintTitleHighlight(tmp_win, on);

		SetBorder (tmp_win, True);
		SetFocus (tmp_win, eventp->xbutton.time);
		Scr->FocusRoot = FALSE;
		Scr->Focus = tmp_win;
	    }
	}
	break;

    case F_DESTROY:
	if (DeferExecution(context, func, Scr->DestroyCursor))
	    return TRUE;

/* djhjr - 6/22/01 */
#ifndef NO_SOUND_SUPPORT
	/* flag for the handler */
	if (PlaySound(func)) destroySoundFromFunction = TRUE;
#endif

	/* djhjr - 9/10/96 */
	if (tmp_win == Scr->VirtualDesktopDisplayTwin)
	{
		/* added this 'if (...) ...' and 'if (...) else' - djhjr - 9/21/99 */
		if (have_showdesktop == -1)
			have_showdesktop = FindMenuOrFuncInBindings(C_ALL_BITS, NULL, F_SHOWDESKTOP);
		if (FindMenuOrFuncInWindows(tmp_win, have_showdesktop, NULL, F_SHOWDESKTOP))
			XUnmapWindow(dpy, Scr->VirtualDesktopDisplayTwin->frame);
		else

			DoAudible(); /* was 'XBell()' - djhjr - 6/22/01 */
		break;
	}

	{
		TwmDoor *d;

		if (XFindContext(dpy, tmp_win->w, DoorContext,
				(caddr_t *) &d) != XCNOENT)
		{
/* djhjr - 9/10/99
			XBell(dpy, 0);
*/
			/* for some reason, we don't get the button up event - djhjr - 9/10/99 */
			ButtonPressed = -1;
			door_delete(tmp_win->w, d);

			break;
		}
	}

	if (tmp_win->iconmgr)		/* don't send ourself a message */
	{
		/* added this 'if (...) ...' and 'if (...) else ...' - djhjr - 9/21/99 */
		if (have_showlist == -1)
			have_showlist = FindMenuOrFuncInBindings(C_ALL_BITS, NULL, F_SHOWLIST);
		if (FindMenuOrFuncInWindows(tmp_win, have_showlist, NULL, F_SHOWLIST))

			/* added argument - djhjr - 9/21/99 */
			HideIconManager(tmp_win);

		else
			DoAudible(); /* was 'XBell()' - djhjr - 6/22/01 */
	}
	else
	{
		/* djhjr - 4/26/99 */
		AppletDown(tmp_win);

	    XKillClient(dpy, tmp_win->w);
	}
	break;

    case F_DELETE:
	if (DeferExecution(context, func, Scr->DestroyCursor))
	    return TRUE;

/* djhjr - 6/22/01 */
#ifndef NO_SOUND_SUPPORT
	/* flag for the handler */
	if (PlaySound(func)) destroySoundFromFunction = TRUE;
#endif

	/* djhjr - 9/21/99 */
	if (tmp_win == Scr->VirtualDesktopDisplayTwin)
	{
		if (have_showdesktop == -1)
			have_showdesktop = FindMenuOrFuncInBindings(C_ALL_BITS, NULL, F_SHOWDESKTOP);
		if (FindMenuOrFuncInWindows(tmp_win, have_showdesktop, NULL, F_SHOWDESKTOP))
			XUnmapWindow(dpy, Scr->VirtualDesktopDisplayTwin->frame);
		else
			DoAudible(); /* was 'XBell()' - djhjr - 6/22/01 */

		break;
	}

	/* djhjr - 9/10/99 */
	{
		TwmDoor *d;

		if (XFindContext(dpy, tmp_win->w, DoorContext,
				(caddr_t *) &d) != XCNOENT)
		{
			/* for some reason, we don't get the button up event - djhjr - 9/10/99 */
			ButtonPressed = -1;
			door_delete(tmp_win->w, d);

			break;
		}
	}

	if (tmp_win->iconmgr)		/* don't send ourself a message */
	{
		/* added this 'if (...) ...' and 'if (...) else ...' - djhjr - 9/21/99 */
		if (have_showlist == -1)
			have_showlist = FindMenuOrFuncInBindings(C_ALL_BITS, NULL, F_SHOWLIST);
		if (FindMenuOrFuncInWindows(tmp_win, have_showlist, NULL, F_SHOWLIST))

			/* added argument - djhjr - 9/21/99 */
			HideIconManager(tmp_win);

		else
			DoAudible(); /* was 'XBell()' - djhjr - 6/22/01 */
	}
	else if (tmp_win->protocols & DoesWmDeleteWindow)
	{
		/* djhjr - 4/26/99 */
		AppletDown(tmp_win);

	  SendDeleteWindowMessage (tmp_win, LastTimestamp());
	}
	else
	  DoAudible(); /* was 'XBell()' - djhjr - 6/22/01 */
	break;

    case F_SAVEYOURSELF:
	if (DeferExecution (context, func, Scr->SelectCursor))
	  return TRUE;

	if (tmp_win->protocols & DoesWmSaveYourself)
	  SendSaveYourselfMessage (tmp_win, LastTimestamp());
	else
	  DoAudible(); /* was 'XBell()' - djhjr - 6/22/01 */
	break;

    case F_CIRCLEUP:
	XCirculateSubwindowsUp(dpy, Scr->Root);
	break;

    case F_CIRCLEDOWN:
	XCirculateSubwindowsDown(dpy, Scr->Root);
	break;

    case F_EXEC:
	PopDownMenu();
	if (!Scr->NoGrabServer) {
	    XUngrabServer (dpy);
	    XSync (dpy, 0);
	}

/* djhjr - 6/22/01 */
#ifndef NO_SOUND_SUPPORT
	/* flag for the handler */
	if (PlaySound(func)) createSoundFromFunction = TRUE;
#endif

	Execute(action);
	break;

    case F_UNFOCUS:
	FocusOnRoot();
	break;

    case F_CUT:
	strcpy(tmp, action);
	strcat(tmp, "\n");
	XStoreBytes(dpy, tmp, strlen(tmp));
	break;

    case F_CUTFILE:
	ptr = XFetchBytes(dpy, &count);
	if (ptr) {
	    if (sscanf (ptr, "%s", tmp) == 1) {
		XFree (ptr);
		ptr = ExpandFilename(tmp);
		if (ptr) {
		    fd = open (ptr, 0);
		    if (fd >= 0) {
			count = read (fd, buff, MAX_FILE_SIZE - 1);
			if (count > 0) XStoreBytes (dpy, buff, count);
			close(fd);
		    } else {
			fprintf (stderr,
				 "%s:  unable to open cut file \"%s\"\n",
				 ProgramName, tmp);
		    }
		    if (ptr != tmp) free (ptr);
		}
	    } else {
		XFree(ptr);
	    }
	} else {
	    fprintf(stderr, "%s:  cut buffer is empty\n", ProgramName);
	}
	break;

    case F_WARPTOSCREEN:
	{
	    if (strcmp (action, WARPSCREEN_NEXT) == 0) {
		WarpToScreen (Scr->screen + 1, 1);
	    } else if (strcmp (action, WARPSCREEN_PREV) == 0) {
		WarpToScreen (Scr->screen - 1, -1);
	    } else if (strcmp (action, WARPSCREEN_BACK) == 0) {
		WarpToScreen (PreviousScreen, 0);
	    } else {
		WarpToScreen (atoi (action), 0);
	    }
	}
	break;

    case F_COLORMAP:
	{
	    if (strcmp (action, COLORMAP_NEXT) == 0) {
		BumpWindowColormap (tmp_win, 1);
	    } else if (strcmp (action, COLORMAP_PREV) == 0) {
		BumpWindowColormap (tmp_win, -1);
	    } else {
		BumpWindowColormap (tmp_win, 0);
	    }
	}
	break;

    case F_WARPCLASSNEXT: /* PF */
    case F_WARPCLASSPREV: /* PF */
		WarpClass(func == F_WARPCLASSNEXT, tmp_win, action);
		break;

    case F_WARPTONEWEST: /* PF */
		/* added '&& warp_if_warpunmapped()' - djhjr - 5/13/98 */
		/* added the second argument - djhjr - 5/28/00 */
		if (Scr->Newest && warp_if_warpunmapped(Scr->Newest, F_NOFUNCTION))
		{
			RaiseStickyAbove();
			RaiseAutoPan();

/* djhjr - 6/3/03 */
#ifndef NO_SOUND_SUPPORT
			PlaySound(func);
#endif

		    WarpToWindow(Scr->Newest);
		}
		else
	    	DoAudible(); /* was 'XBell()' - djhjr - 6/22/01 */
		break;
	
    case F_WARPTO:                                              
	{
	    register TwmWindow *t;                                  
	    /* djhjr - 6/3/03 */
	    int did_warpto = FALSE;

	    for (t = Scr->TwmRoot.next; t != NULL; t = t->next)
	    {
		/*
		 * This used to fall through into F_WARP, but the
		 * warp_if_warpunmapped() meant this loop couldn't
		 * continue to look for a match in the window list.
		 * djhjr - 10/27/02
		 */

		/* jason@tfs.com */
		if (MatchWinName(action, t) == 0 &&
				warp_if_warpunmapped(t, func))
		{
			tmp_win = t;                 /* PF */
			RaiseStickyAbove();          /* DSE */
			RaiseAutoPan();

			/* djhjr - 6/3/03 */
#ifndef NO_SOUND_SUPPORT
			PlaySound(func);
#endif
			did_warpto = TRUE;

			WarpToWindow(tmp_win);       /* PF */
			break;
		}
	    }

	    /* djhjr - 6/3/03 */
	    if (!did_warpto)
		DoAudible();
	}
	break;

    case F_WARP:                                                /* PF */
	{                                                           /* PF */
		/* added '&& warp_if_warpunmapped()' - djhjr - 5/13/98 */
		/* added the second argument - djhjr - 5/28/00 */
		if (tmp_win && warp_if_warpunmapped(tmp_win, F_NOFUNCTION)) /* PF */
		{
			RaiseStickyAbove();                                 /* DSE */
			RaiseAutoPan();                                     

/* djhjr - 6/3/03 */
#ifndef NO_SOUND_SUPPORT
			PlaySound(func);
#endif

			WarpToWindow(tmp_win);                              /* PF */
		} else {                                                
		DoAudible(); /* was 'XBell()' - djhjr - 6/22/01 */                                         
	    }                                                       
	}                                                           /* PF */
	break;

    case F_WARPTOICONMGR:
	{
	    TwmWindow *t;
	    int len;

/* djhjr - 5/13/98
	    Window raisewin = None, iconwin = None;
*/
/*
 * raisewin now points to the window's icon manager entry, and
 * iconwin now points to raisewin's icon manager - djhjr - 5/30/00
 *
	    TwmWindow *raisewin = None;
	    Window iconwin = None;
*/
	    WList *raisewin = NULL;
	    TwmWindow *iconwin = None;

	    len = strlen(action);
	    if (len == 0) {
		if (tmp_win && tmp_win->list) {
/* djhjr - 5/13/98
		    raisewin = tmp_win->list->iconmgr->twm_win->frame;
*/
/* djhjr - 5/30/00
		    raisewin = tmp_win->list->iconmgr->twm_win;
		    iconwin = tmp_win->list->icon;
*/
		    raisewin = tmp_win->list;
		} else if (Scr->iconmgr.active) {
/* djhjr - 5/13/98
		    raisewin = Scr->iconmgr.twm_win->frame;
*/
/* djhjr - 5/30/00
		    raisewin = Scr->iconmgr.twm_win;
		    iconwin = Scr->iconmgr.active->w;
*/
		    raisewin = Scr->iconmgr.active;
		}
	    } else {
		for (t = Scr->TwmRoot.next; t != NULL; t = t->next) {
		    if (strncmp (action, t->icon_name, len) == 0) {
			if (t->list && t->list->iconmgr->twm_win->mapped) {

/* djhjr - 5/13/98
			    raisewin = t->list->iconmgr->twm_win->frame;
*/
/* djhjr - 5/30/00
			    raisewin = t->list->iconmgr->twm_win;
			    iconwin = t->list->icon;
*/
			    raisewin = t->list;
			    break;
			}
		    }
		}
	    }

		/* djhjr - 6/14/00 */
		if (!raisewin)
		{
			DoAudible(); /* was 'XBell()' - djhjr - 6/22/01 */
			break;
		}

		/* djhjr - 5/30/00 */
		iconwin = raisewin->iconmgr->twm_win;

		/* added '&& warp_if_warpunmapped()' - djhjr - 5/13/98 */
		/* added the second argument - djhjr - 5/28/00 */
		/* was 'raisewin' - djhjr - 5/30/00 */
		if (iconwin && warp_if_warpunmapped(iconwin, F_NOFUNCTION)) {
/* djhjr - 6/3/03 */
#ifndef NO_SOUND_SUPPORT
			PlaySound(func);
#endif

/* djhjr - 5/30/00
			XWarpPointer (dpy, None, iconwin, 0, 0, 0, 0,
					EDGE_OFFSET, EDGE_OFFSET); * DSE *
*/
			WarpInIconMgr(raisewin, iconwin);
	    } else {
		DoAudible(); /* was 'XBell()' - djhjr - 6/22/01 */
	    }
	}
	break;

	case F_SQUEEZELEFT:/*RFB*/
	{
	    static SqueezeInfo left_squeeze = { J_LEFT, 0, 0 };
	    
	    /* too much dup'd code - djhjr - 9/17/02 */
	    if (do_squeezetitle(context, func, tmp_win, &left_squeeze))
		return TRUE;	/* deferred */
	}
	break;

	case F_SQUEEZERIGHT:/*RFB*/
	{
	    static SqueezeInfo right_squeeze = { J_RIGHT, 0, 0 };

	    /* too much dup'd code - djhjr - 9/17/02 */
	    if (do_squeezetitle(context, func, tmp_win, &right_squeeze))
		return TRUE;	/* deferred */
	}
	break;
	
	case F_SQUEEZECENTER:/*RFB*/
	{
	    static SqueezeInfo center_squeeze = { J_CENTER, 0, 0 };

	    /* too much dup'd code - djhjr - 9/17/02 */
	    if (do_squeezetitle(context, func, tmp_win, &center_squeeze))
		return TRUE;	/* deferred */
	}
	break;

	case F_RING:/*RFB*/
	if (DeferExecution (context, func, Scr->SelectCursor))
	  return TRUE;
	if ( tmp_win->ring.next || tmp_win->ring.prev )
		RemoveWindowFromRing(tmp_win);
	else
		AddWindowToRing(tmp_win);
#ifdef ORIGINAL_WARPRINGCOORDINATES /* djhjr - 5/11/98 */
    tmp_win->ring.cursor_valid = False;
#endif
	break;

    case F_WARPRING:
	switch (action[0]) {
	  case 'n':
	    WarpAlongRing (&eventp->xbutton, True);
	    break;
	  case 'p':
	    WarpAlongRing (&eventp->xbutton, False);
	    break;
	  default:
	    DoAudible(); /* was 'XBell()' - djhjr - 6/22/01 */
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
	    fprintf (stderr, "%s:  unable to open file \"%s\"\n",
		     ProgramName, action);
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
	    w = XCreateWindow (dpy, Scr->Root, 0, 0,
			       (unsigned int) Scr->MyDisplayWidth,
			       (unsigned int) Scr->MyDisplayHeight,
			       (unsigned int) 0,
			       CopyFromParent, (unsigned int) CopyFromParent,
			       (Visual *) CopyFromParent, valuemask,
			       &attributes);
	    XMapWindow (dpy, w);
	    XDestroyWindow (dpy, w);
	    XFlush (dpy);
	}
	break;

    case F_WINREFRESH:
	if (DeferExecution(context, func, Scr->SelectCursor))
	    return TRUE;

	if (context == C_ICON && tmp_win->icon_w.win)
	    w = XCreateSimpleWindow(dpy, tmp_win->icon_w.win,
		0, 0, 9999, 9999, 0, Scr->Black, Scr->Black);
	else
	    w = XCreateSimpleWindow(dpy, tmp_win->frame,
		0, 0, 9999, 9999, 0, Scr->Black, Scr->Black);

	XMapWindow(dpy, w);
	XDestroyWindow(dpy, w);
	XFlush(dpy);
	break;

     case F_NAIL:
 	if (DeferExecution(context, func, Scr->SelectCursor))
 	    return TRUE;

 	tmp_win->nailed = !tmp_win->nailed;
 	/* update the vd display */
	/* UpdateDesktop(tmp_win); Stig */
 	NailDesktop(tmp_win); /* Stig */

#ifdef DEBUG
 	fprintf(stdout, "%s:  nail state of %s is now %s\n",
 		ProgramName, tmp_win->name, (tmp_win->nailed ? "nailed" : "free"));
#endif /* DEBUG */

	RaiseStickyAbove(); /* DSE */
	RaiseAutoPan(); /* DSE */

 	break;

	/*
	 * move a percentage in a particular direction
	 */
    case F_PANDOWN:
 	PanRealScreen(0, (atoi(action) * Scr->MyDisplayHeight) / 100
 		/* DSE */ ,NULL,NULL);
 	break;
    case F_PANLEFT:
 	PanRealScreen(-((atoi(action) * Scr->MyDisplayWidth) / 100), 0
 		/* DSE */ ,NULL,NULL);
 	break;
    case F_PANRIGHT:
 	PanRealScreen((atoi(action) * Scr->MyDisplayWidth) / 100, 0
 		/* DSE */ ,NULL,NULL);
 	break;
    case F_PANUP:
 	PanRealScreen(0, -((atoi(action) * Scr->MyDisplayHeight) / 100)
 		/* DSE */ ,NULL,NULL);
 	break;
 	
    case F_RESETDESKTOP:
 		SetRealScreen(0, 0);
 		break;

/*SNUG*/ 	/* Robert Forsman added these two functions <thoth@ufl.edu> */
/*SNUG*/ 	{
/*SNUG*/ 	  TwmWindow	*scan;
/*SNUG*/ 	  int		right, left, up, down;
/*SNUG*/ 	  int		inited;
/*SNUG*/    case F_SNUGDESKTOP:
/*SNUG*/
/*SNUG*/ 	  inited = 0;
/*SNUG*/ 	  for (scan = Scr->TwmRoot.next; scan!=NULL; scan = scan->next)
/*SNUG*/ 	    {
/*SNUG*/ 	      if (scan->nailed)
/*SNUG*/ 		continue;
/*SNUG*/ 	      if (scan->frame_x > Scr->MyDisplayWidth ||
/*SNUG*/ 		  scan->frame_y > Scr->MyDisplayHeight)
/*SNUG*/ 		continue;
/*SNUG*/ 	      if (scan->frame_x+scan->frame_width < 0 ||
/*SNUG*/ 		  scan->frame_y+scan->frame_height < 0)
/*SNUG*/ 		continue;
/*SNUG*/ 	      if ( inited==0 || scan->frame_x<right )
/*SNUG*/ 		right = scan->frame_x;
/*SNUG*/ 	      if ( inited==0 || scan->frame_y<up )
/*SNUG*/ 		up = scan->frame_y;
/*SNUG*/ 	      if ( inited==0 || scan->frame_x+scan->frame_width>left )
/*SNUG*/ 		left = scan->frame_x+scan->frame_width;
/*SNUG*/ 	      if ( inited==0 || scan->frame_y+scan->frame_height>down )
/*SNUG*/ 		down = scan->frame_y+scan->frame_height;
/*SNUG*/ 	      inited = 1;
/*SNUG*/ 	    }
/*SNUG*/ 	  if (inited)
/*SNUG*/ 	    {
/*SNUG*/ 	      int	dx,dy;
/*SNUG*/ 	      if (left-right < Scr->MyDisplayWidth && (right<0 || left>Scr->MyDisplayWidth) )
/*SNUG*/ 		dx = right - ( Scr->MyDisplayWidth - (left-right) ) /2;
/*SNUG*/ 	      else
/*SNUG*/ 		dx = 0;
/*SNUG*/ 	      if (down-up < Scr->MyDisplayHeight && (up<0 || down>Scr->MyDisplayHeight) )
/*SNUG*/ 		dy = up - (Scr->MyDisplayHeight - (down-up) ) /2;
/*SNUG*/ 	      else
/*SNUG*/ 		dy = 0;
/*SNUG*/ 	      if (dx!=0 || dy!=0)
/*SNUG*/ 		PanRealScreen(dx,dy,NULL,NULL);
/*SNUG*/ 		                    /* DSE */
/*SNUG*/ 	      else
/*SNUG*/ 		DoAudible(); /* was 'XBell()' - djhjr - 6/22/01 */
/*SNUG*/ 	    }
/*SNUG*/ 	  else
/*SNUG*/ 	    DoAudible(); /* was 'XBell()' - djhjr - 6/22/01 */
/*SNUG*/ 	  break;
/*SNUG*/
/*SNUG*/     case F_SNUGWINDOW:
/*SNUG*/ 	  if (DeferExecution(context, func, Scr->SelectCursor))
/*SNUG*/ 	    return TRUE;
/*SNUG*/
/*SNUG*/ 	  inited = 0;
/*SNUG*/ 	  right = tmp_win->frame_x;
/*SNUG*/ 	  left = tmp_win->frame_x + tmp_win->frame_width;
/*SNUG*/ 	  up = tmp_win->frame_y;
/*SNUG*/ 	  down = tmp_win->frame_y + tmp_win->frame_height;
/*SNUG*/ 	  inited = 1;
/*SNUG*/ 	  if (inited)
/*SNUG*/ 	    {
/*SNUG*/ 	      int	dx,dy;
/*SNUG*/ 	      dx = 0;
/*SNUG*/ 	      if (left-right < Scr->MyDisplayWidth)
/*SNUG*/ 		{
/*SNUG*/ 		if (right<0)
/*SNUG*/ 		  dx = right;
/*SNUG*/ 		else if (left>Scr->MyDisplayWidth)
/*SNUG*/ 		  dx = left - Scr->MyDisplayWidth;
/*SNUG*/ 		}
/*SNUG*/
/*SNUG*/ 	      dy = 0;
/*SNUG*/ 	      if (down-up < Scr->MyDisplayHeight)
/*SNUG*/ 		{
/*SNUG*/ 		if (up<0)
/*SNUG*/ 		  dy = up;
/*SNUG*/ 		else if (down>Scr->MyDisplayHeight)
/*SNUG*/ 		  dy = down - Scr->MyDisplayHeight;
/*SNUG*/ 		}
/*SNUG*/
/*SNUG*/ 	      if (dx!=0 || dy!=0)
/*SNUG*/ 		PanRealScreen(dx,dy,NULL,NULL);
/*SNUG*/ 		                    /* DSE */
/*SNUG*/ 	      else
/*SNUG*/ 		DoAudible(); /* was 'XBell()' - djhjr - 6/22/01 */
/*SNUG*/ 	    }
/*SNUG*/ 	  else
/*SNUG*/ 	    DoAudible(); /* was 'XBell()' - djhjr - 6/22/01 */
/*SNUG*/
/*SNUG*/ 	break;
/*SNUG*/ 	}

    /* Next four submitted by Seth Robertson - 9/9/02 */
    case F_BINDBUTTONS:
	{
	    int i, j;
         
	    if (DeferExecution(context, func, Scr->SelectCursor))
		return TRUE;
	    for (i = 0; i < NumButtons+1; i++)
		for (j = 0; j < MOD_SIZE; j++)
		    if (Scr->Mouse[MOUSELOC(i,C_WINDOW,j)].func != F_NOFUNCTION)
			XGrabButton(dpy, i, j, tmp_win->frame,
				    True, ButtonPressMask | ButtonReleaseMask,
				    GrabModeAsync, GrabModeAsync, None,
				    Scr->FrameCursor);  
	    break;
	}
    case F_BINDKEYS:
	{
	    FuncKey *tmp;

	    if (DeferExecution(context, func, Scr->SelectCursor))
		return TRUE;
	    for (tmp = Scr->FuncKeyRoot.next; tmp != NULL; tmp = tmp->next)
		if (tmp->cont == C_WINDOW)
/* djhjr - 9/10/03
		    XGrabKey(dpy, tmp->keycode, tmp->mods, tmp_win->w, True,
			     GrabModeAsync, GrabModeAsync);
*/
		    GrabModKeys(tmp_win->w, tmp);
	    break;
	}
    case F_UNBINDBUTTONS:
	{
	    int i, j;

	    if (DeferExecution(context, func, Scr->SelectCursor))
		return TRUE;
	    for (i = 0; i < NumButtons+1; i++)
		for (j = 0; j < MOD_SIZE; j++)
		    if (Scr->Mouse[MOUSELOC(i,C_WINDOW,j)].func != F_NOFUNCTION)
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
/* djhjr - 9/10/03
		    XUngrabKey(dpy, tmp->keycode, tmp->mods, tmp_win->w);
*/
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
		/* added exposure event masks - djhjr - 10/11/01 */
		XMaskEvent(dpy, ButtonPressMask | ButtonReleaseMask |
			   EnterWindowMask | LeaveWindowMask |
			   ExposureMask | VisibilityChangeMask |
			   movementMask, &Event);

		/*
		 * Don't discard exposure events before release
		 * or window borders and/or their titles in the
		 * virtual desktop won't get redrawn - djhjr
		 */

		/* discard any extra motion events before a release */
		if (Event.type == MotionNotify)
		{
		    /* was 'ButtonMotionMask' - djhjr - 10/11/01 */
		    while (XCheckMaskEvent(dpy, releaseEvent | movementMask,
					   &Event))
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

		if (!DispatchEvent()) continue;

		if (Event.type != MotionNotify) continue;

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
	    while (XCheckMaskEvent(dpy, EnterWindowMask | LeaveWindowMask,
				   &Event))
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
		Scr->snapRealScreen = ! Scr->snapRealScreen;
		break;

	/* djhjr - 12/14/98 */
	case F_STATICICONPOSITIONS:
		Scr->StaticIconPositions = ! Scr->StaticIconPositions;
		break;

	/* djhjr - 12/14/98 */
	case F_STRICTICONMGR:
	{
		TwmWindow *t;
			
		Scr->StrictIconManager = ! Scr->StrictIconManager;
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
		JunkMask = XParseGeometry (action, &JunkX, &JunkY, &JunkWidth, &JunkHeight);

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
	if (Scr->Virtual) {
		XMapWindow(dpy, Scr->VirtualDesktopDisplayTwin->frame);

		/* djhjr - 9/14/96 */
		if (Scr->VirtualDesktopDisplayTwin->icon)
		    DeIconify(Scr->VirtualDesktopDisplayTwin);
	}
	break;

    case F_ENTERDOOR:
	{
		TwmDoor *d;

		if (XFindContext(dpy, tmp_win->w, DoorContext,
				 (caddr_t *) &d) != XCNOENT)
 			door_enter(tmp_win->w, d);
		break;
	}

    case F_DELETEDOOR:
	{	/*marcel@duteca.et.tudelft.nl*/
		TwmDoor *d;

		if (DeferExecution(context, func, Scr->DestroyCursor))
	    		return TRUE;

/* djhjr - 6/22/01 */
#ifndef NO_SOUND_SUPPORT
	/* flag for the handler */
	if (PlaySound(func)) destroySoundFromFunction = TRUE;
#endif

		if (XFindContext(dpy, tmp_win->w, DoorContext,
				 (caddr_t *) &d) != XCNOENT)
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

	/* djhjr - 4/20/98 */
	case F_NAMEDOOR:
	{
		TwmDoor *d;

		if (XFindContext(dpy, tmp_win->w, DoorContext,
				(caddr_t *) &d) != XCNOENT)
			door_paste_name(tmp_win->w, d);
		break;
	}

     case F_QUIT:
/* djhjr - 9/14/96 - it's in Done()...
	SetRealScreen(0,0);
*/

/* djhjr - 6/22/01 */
#ifndef NO_SOUND_SUPPORT
	if (PlaySound(func))
	{
		/* allow time to emit */
		if (Scr->PauseOnQuit) sleep(Scr->PauseOnQuit);
	}
	else
		PlaySoundDone();
#endif

	Done();
	break;

     case F_VIRTUALGEOMETRIES:
	Scr->GeometriesAreVirtual = ! Scr->GeometriesAreVirtual;
	break;

	/* submitted by Ugen Antsilevitch - 5/28/00 */
	case F_WARPVISIBLE:
		Scr->WarpVisible = ! Scr->WarpVisible;
		break;

	/* djhjr - 5/30/00 */
	case F_WARPSNUG:
		Scr->WarpSnug = ! Scr->WarpSnug;
		break;

/* djhjr - 6/22/01 */
#ifndef NO_SOUND_SUPPORT
	case F_SOUNDS:
		ToggleSounds();
		break;
    
	/* djhjr - 11/15/02 */
	case F_PLAYSOUND:
		PlaySoundAdhoc(action);
		break;
#endif
    }

    if (ButtonPressed == -1) XUngrabPointer(dpy, CurrentTime);
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
DeferExecution(context, func, cursor)
int context, func;
Cursor cursor;
{
  if (context == C_ROOT)
    {
	LastCursor = cursor;
	XGrabPointer(dpy, Scr->Root, True,
	    ButtonPressMask | ButtonReleaseMask,
	    GrabModeAsync, GrabModeAsync,
	    Scr->Root, cursor, CurrentTime);

	RootFunction = func;
	Action = actionHack; /* Submitted by Michel Eyckmans */

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

void ReGrab()
{
    XGrabPointer(dpy, Scr->Root, True,
	ButtonPressMask | ButtonReleaseMask,
	GrabModeAsync, GrabModeAsync,
	Scr->Root, LastCursor, CurrentTime);
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

/* was of type 'int' - Submitted by Michel Eyckmans */
Cursor
NeedToDefer(root)
MenuRoot *root;
{
    MenuItem *mitem;

    for (mitem = root->first; mitem != NULL; mitem = mitem->next)
    {
	switch (mitem->func)
	{
	case F_RESIZE:
	  return Scr->ResizeCursor; /* Submitted by Michel Eyckmans */
	case F_MOVE:
	case F_FORCEMOVE:
	  return Scr->MoveCursor; /* Submitted by Michel Eyckmans */
    /* these next four - Submitted by Michel Eyckmans */
	case F_DELETE:
	case F_DELETEDOOR:
	case F_DESTROY:
	  return Scr->DestroyCursor;
	case F_IDENTIFY: /* was with 'F_RESIZE' - Submitted by Michel Eyckmans */
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
Execute(s)
    char *s;
{
    static char buf[256];
    char *ds = DisplayString (dpy);
    char *colon, *dot1;
    char oldDisplay[256];
    char *doisplay;
    int restorevar = 0;
    
    char *append_this = " &";
    char *es = (char *)malloc(strlen(s)+strlen(append_this)+1);
    sprintf(es,s);
    	/* a new copy of s, with extra space incase -- DSE */

	if (Scr->EnhancedExecResources) /* DSE */
		{    
	    /* chop all space characters from the end of the string */
    	while ( isspace ( es[strlen(es)-1] ) )
	    	{
    		es[strlen(es)-1] = '\0';
			}
		switch ( es[strlen(es)-1] ) /* last character */
			{
			case ';':
				es[strlen(es)-1] = '\0'; /* remove the semicolon */
				break;
			case '&': /* already there so do nothing */
				break;
			default:
				strcat(es,append_this); /* don't block the window manager */
				break;
			}
		}

    oldDisplay[0] = '\0';
    doisplay=getenv("DISPLAY");
    if (doisplay)
		strcpy (oldDisplay, doisplay);

    /*
     * Build a display string using the current screen number, so that
     * X programs which get fired up from a menu come up on the screen
     * that they were invoked from, unless specifically overridden on
     * their command line.
     */
    colon = rindex (ds, ':');
    if (colon) {			/* if host[:]:dpy */
	strcpy (buf, "DISPLAY=");
	strcat (buf, ds);
	colon = buf + 8 + (colon - ds);	/* use version in buf */
	dot1 = index (colon, '.');	/* first period after colon */
	if (!dot1) dot1 = colon + strlen (colon);  /* if not there, append */
	(void) sprintf (dot1, ".%d", Scr->screen);
	putenv (buf);
	restorevar = 1;
    }

    (void) system (es); /* DSE */
    free (es); /* DSE */

    if (restorevar) {		/* why bother? */
	(void) sprintf (buf, "DISPLAY=%s", oldDisplay);
	putenv (buf);
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
FocusOnRoot()
{
    SetFocus ((TwmWindow *) NULL, LastTimestamp());
    if (Scr->Focus != NULL)
    {
	SetBorder (Scr->Focus, False);

/* djhjr - 4/25/96
	if (Scr->Focus->hilite_w) XUnmapWindow (dpy, Scr->Focus->hilite_w);
*/
	PaintTitleHighlight(Scr->Focus, off);

    }
    InstallWindowColormaps(0, &Scr->TwmRoot);
    Scr->Focus = NULL;
    Scr->FocusRoot = TRUE;
}

void DeIconify(tmp_win)
TwmWindow *tmp_win;
{
    TwmWindow *t;

    /*
     * De-iconify the main window first
     */

    /* re-vamped the zoom stuff - djhjr - 10/11/01 */
    if (Scr->DoZoom && Scr->ZoomCount > 0)
    {
	IconMgr *ipf = NULL;
	Window wt = None, wf = None;

	if (tmp_win->icon)
	{
	    if (tmp_win->icon_on)
	    {
		wf = tmp_win->icon_w.win; wt = tmp_win->frame;
	    }
	    else if (tmp_win->list) /* djhjr - 10/11/01 */
	    {
		wf = tmp_win->list->w.win; wt = tmp_win->frame;
		ipf = tmp_win->list->iconmgr;
	    }
	    else if (tmp_win->group != None)
	    {
		for (t = Scr->TwmRoot.next; t != NULL; t = t->next)
		    if (tmp_win->group == t->w)
		    {
			if (t->icon_on)
			    wf = t->icon_w.win;
			else if (t->list) /* djhjr - 10/11/01 */
			{
			    wf = t->list->w.win;
			    ipf = t->list->iconmgr;
			}

			wt = tmp_win->frame;
			break;
		    }
	    }
	}

	/* added Zoom()s args to iconmgrs - djhjr - 10/11/01 */
	if (Scr->ZoomZoom || (wf != None && wt != None))
	    Zoom(wf, ipf, wt, NULL);	/* RFBZOOM */
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

    if (tmp_win->icon_w.win) {
	XUnmapWindow(dpy, tmp_win->icon_w.win);
	IconDown (tmp_win);
    }

    tmp_win->icon = FALSE;
    tmp_win->icon_on = FALSE;

    if (tmp_win->list)
	XUnmapWindow(dpy, tmp_win->list->icon);

    /*
     * RemoveIconManager() done in events.c:HandleMapNotify()
     */

    UpdateDesktop(tmp_win);

    /*
     * Now de-iconify transients
     */

    for (t = Scr->TwmRoot.next; t != NULL; t = t->next)
    {
	if (t->transient && t->transientfor == tmp_win->w)
	{
	    /* this 'if (...) else' (see also Iconify()) - djhjr - 6/22/99 */
	    if (Scr->DontDeiconifyTransients && t->icon_w.win &&
			t->icon == TRUE && t->icon_on == FALSE)
	    {
		IconUp(t);
		XMapRaised(dpy, t->icon_w.win);
		t->icon_on = TRUE;
	    }
	    else
	    {
		/* added Zoom()s args to iconmgrs - djhjr - 10/11/01 */
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
		    IconDown (t);
		}

		t->icon = FALSE;
		t->icon_on = FALSE;

		if (t->list) XUnmapWindow(dpy, t->list->icon);

		/*
		 * RemoveIconManager() done in events.c:HandleMapNotify()
		 */

		UpdateDesktop(t);
	    }
	}
    }

    RaiseStickyAbove(); /* DSE */
    RaiseAutoPan();

    /*
     * added '&& Scr->WarpWindows'.
     * see the kludge in ExecuteFunction(F_ICONIFY, ...).
     * djhjr - 1/24/98
     */
    if (((Scr->WarpCursor ||
		LookInList(Scr->WarpCursorL, tmp_win->full_name,
				&tmp_win->class)) &&
		tmp_win->icon) && Scr->WarpWindows)
	WarpToWindow (tmp_win);

    XSync (dpy, 0);
}



void Iconify(tmp_win, def_x, def_y)
TwmWindow *tmp_win;
int def_x, def_y;
{
    TwmWindow *t;
    int iconify;
    XWindowAttributes winattrs;
    unsigned long eventMask;
    /* djhjr - 6/22/99 */
    short fake_icon;

    iconify = ((!tmp_win->iconify_by_unmapping) || tmp_win->transient);
    if (iconify)
    {
	if (tmp_win->icon_w.win == None)
	    CreateIconWindow(tmp_win, def_x, def_y);
	else
	    IconUp(tmp_win);

	XMapRaised(dpy, tmp_win->icon_w.win);
	
	RaiseStickyAbove(); /* DSE */
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

	    /* RemoveFromDesktop(t); Stig */

	    /*
	     * Prevent the receipt of an UnmapNotify, since that would
	     * cause a transition to the Withdrawn state.
	     */
	    t->mapped = FALSE;
	    XSelectInput(dpy, t->w, eventMask & ~StructureNotifyMask);
	    XUnmapWindow(dpy, t->w);
	    XSelectInput(dpy, t->w, eventMask);
	    XUnmapWindow(dpy, t->frame);

	    /* moved to make zooms more aesthetically pleasing -- DSE */
	    if (iconify)
	    {
		/* added Zoom()s args to iconmgrs - djhjr - 10/11/01 */
		if (t->icon_on)
		  Zoom(t->icon_w.win, NULL, tmp_win->icon_w.win, NULL);
		else
		  Zoom(t->frame, NULL, tmp_win->icon_w.win, NULL);
	    }

	    if (t->icon_w.win)
	      XUnmapWindow(dpy, t->icon_w.win);
	    SetMapStateProp(t, IconicState);
	    SetBorder (t, False);
	    if (t == Scr->Focus)
	      {
		SetFocus ((TwmWindow *) NULL, LastTimestamp());
		Scr->Focus = NULL;
		Scr->FocusRoot = TRUE;
	      }

	    /*
	     * let current status ride, but "fake out" UpdateDesktop()
	     * (see also DeIconify()) - djhjr - 6/22/99
	     */
	    fake_icon = t->icon;

	    t->icon = TRUE;
	    t->icon_on = FALSE;

	    /* djhjr - 10/2/01 */
	    if (Scr->StrictIconManager)
		if (!t->list)
		    AddIconManager(t);

	    if (t->list) XMapWindow(dpy, t->list->icon);

	    UpdateDesktop(t);

	    /* restore icon status - djhjr - 6/22/99 */
	    t->icon = fake_icon;
	  }
      }

    /*
     * Now iconify the main window
     */

/*    if (iconify) RFBZOOM*/

    /* RemoveFromDesktop(tmp_win); Stig */

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

    SetBorder (tmp_win, False);
    if (tmp_win == Scr->Focus)
    {
	SetFocus ((TwmWindow *) NULL, LastTimestamp());
	Scr->Focus = NULL;
	Scr->FocusRoot = TRUE;
    }

    tmp_win->icon = TRUE;
    if (iconify)
	tmp_win->icon_on = TRUE;
    else
	tmp_win->icon_on = FALSE;

    /* djhjr - 10/2/01 */
    if (Scr->StrictIconManager)
	if (!tmp_win->list)
	    AddIconManager(tmp_win);

    /* moved to make zooms more aesthetically pleasing -- DSE */
    /* moved again to ensure an icon manager entry exists - djhjr - 10/11/01 */
    /* added Zoom()s args to iconmgrs - djhjr - 10/11/01 */
    if (iconify)
	Zoom(tmp_win->frame, NULL, tmp_win->icon_w.win, NULL);
    else if (tmp_win->list) /* djhjr - 10/11/01 */
	Zoom(tmp_win->frame, NULL, tmp_win->list->w, tmp_win->list->iconmgr);

    if (tmp_win->list)
	XMapWindow(dpy, tmp_win->list->icon);

    UpdateDesktop(tmp_win);
    XSync (dpy, 0);
}



static void Identify (t)
TwmWindow *t;
{
    int i, n, twidth, width, height;
    int x, y;
    unsigned int wwidth, wheight, bw, depth;
    Window junk;
    int px, py, dummy;
    unsigned udummy;

/* djhjr - 6/22/01 */
#ifndef NO_SOUND_SUPPORT
    PlaySound(F_IDENTIFY);
#endif

    n = 0;
    (void) sprintf(Info[n++], "%s", Version);
    Info[n++][0] = '\0';

    if (t) {
	XGetGeometry (dpy, t->w, &JunkRoot, &JunkX, &JunkY,
		      &wwidth, &wheight, &bw, &depth);
	(void) XTranslateCoordinates (dpy, t->w, Scr->Root, 0, 0,
				      &x, &y, &junk);

/* looks bad with variable fonts... djhjr - 5/10/96
	(void) sprintf(Info[n++], "Name             = \"%s\"", t->full_name);
	(void) sprintf(Info[n++], "Class.res_name   = \"%s\"", t->class.res_name);
	(void) sprintf(Info[n++], "Class.res_class  = \"%s\"", t->class.res_class);
	Info[n++][0] = '\0';
	(void) sprintf(Info[n++], "Geometry/root    = %dx%d+%d+%d", wwidth, wheight, x, y);
	(void) sprintf(Info[n++], "Border width     = %d", bw);
	(void) sprintf(Info[n++], "Depth            = %d", depth);
*/
	(void) sprintf(Info[n++], "Name:  \"%s\"", t->full_name);
	(void) sprintf(Info[n++], "Class.res_name:  \"%s\"", t->class.res_name);
	(void) sprintf(Info[n++], "Class.res_class:  \"%s\"", t->class.res_class);
	Info[n++][0] = '\0';
	(void) sprintf(Info[n++], "Geometry/root:  %dx%d+%d+%d", wwidth, wheight, x, y);
	(void) sprintf(Info[n++], "Border width:  %d", bw);
	(void) sprintf(Info[n++], "Depth:  %d", depth);

	Info[n++][0] = '\0';
    }
/* djhjr - 9/19/96 */
#ifndef NO_BUILD_INFO
	else
	{
		char is_m4, is_xpm;
		char is_rplay; /* djhjr - 6/22/01 */
		char is_regex; /* djhjr - 10/20/01 */

/* djhjr - 6/22/99 */
#ifdef WE_REALLY_DO_WANT_TO_SEE_THIS
		(void) sprintf(Info[n++], "X Server:  %s Version %d.%d Release %d",
				ServerVendor(dpy), ProtocolVersion(dpy), ProtocolRevision(dpy),
				VendorRelease(dpy));
#endif

		/*
		 * Was a 'do ... while()' that accessed unallocated memory.
		 * This and the change to Imakefile submitted by Takeharu Kato
		 */
		i = 0;
		while (lastmake[i][0] != '\0')
			(void) sprintf(Info[n++], "%s", lastmake[i++]);

/* djhjr - 1/31/99 */
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
/* djhjr - 6/22/01 */
#ifdef NO_SOUND_SUPPORT
		is_rplay = '-';
#else
		is_rplay = '+';
#endif
/* djhjr - 6/22/01 */
#ifdef NO_REGEX_SUPPORT
		is_regex = '-';
#else
		is_regex = '+';
#endif
		(void) sprintf(Info[n++],
			       "Options:  %cm4 %cregex %crplay %cxpm",
			       is_m4, is_regex, is_rplay, is_xpm);

		Info[n++][0] = '\0';
	}
#endif

    (void) sprintf(Info[n++], "Click to dismiss...");

    /* figure out the width and height of the info window */

#ifdef TWM_USE_SPACING
    height = n * (120*Scr->InfoFont.height/100); /*baselineskip 1.2*/
    height += Scr->InfoFont.height - Scr->InfoFont.y;
#else
    /* was 'Scr->use3Dborders' - djhjr - 8/11/98 */
    i = (Scr->InfoBevelWidth > 0) ? Scr->InfoBevelWidth + 8 : 10;
    height = (n * (Scr->InfoFont.height+2)) + i; /* some padding */
#endif

    width = 1;
    for (i = 0; i < n; i++)
    {
	twidth = MyFont_TextWidth(&Scr->InfoFont,
	    Info[i], strlen(Info[i]));
	if (twidth > width)
	    width = twidth;
    }
    if (InfoLines) XUnmapWindow(dpy, Scr->InfoWindow.win);

#ifdef TWM_USE_SPACING
    i = Scr->InfoFont.height;
#else
    /* was 'Scr->use3Dborders' - djhjr - 8/11/98 */
    i = (Scr->InfoBevelWidth > 0) ? Scr->InfoBevelWidth + 18 : 20;
#endif
    width += i; /* some padding */

    if (XQueryPointer (dpy, Scr->Root, &JunkRoot, &JunkChild, &px, &py,
		       &dummy, &dummy, &udummy)) {
	px -= (width / 2);
	py -= (height / 3);

	/* added this 'if ()' - djhjr - 4/29/98 */
	/* was 'Scr->use3Dborders' - djhjr - 8/11/98 */
	if (Scr->InfoBevelWidth > 0)
	{
		if (px + width + 2 * Scr->InfoBevelWidth >= Scr->MyDisplayWidth)
		  px = Scr->MyDisplayWidth - width - 2 * Scr->InfoBevelWidth;
		if (py + height + 2 * Scr->InfoBevelWidth >= Scr->MyDisplayHeight)
		  py = Scr->MyDisplayHeight - height - 2 * Scr->InfoBevelWidth;
	}
	else
	{
		if (px + width + BW2 >= Scr->MyDisplayWidth)
		  px = Scr->MyDisplayWidth - width - BW2;
		if (py + height + BW2 >= Scr->MyDisplayHeight)
		  py = Scr->MyDisplayHeight - height - BW2;
	}

	if (px < 0) px = 0;
	if (py < 0) py = 0;
    } else {
	px = py = 0;
    }

    XMoveResizeWindow(dpy, Scr->InfoWindow.win, px, py, width, height);

/* done in HandleExpose() in events.c - djhjr - 4/30/98 */
#ifdef NEVER
	/* djhjr - 5/9/96 */
	if (Scr->use3Dborders > 0)
	{
		XGetGeometry (dpy, Scr->InfoWindow.win, &JunkRoot, &JunkX, &JunkY,
				&JunkWidth, &JunkHeight, &JunkBW, &JunkDepth);
	    Draw3DBorder(Scr->InfoWindow.win, 0, 0, JunkWidth, JunkHeight,
/* djhjr - 4/29/98
			 BW, Scr->DefaultC, off, False, False);
*/
			 Scr->InfoBevelWidth, Scr->DefaultC, off, False, False);
	}
#endif

    XMapRaised(dpy, Scr->InfoWindow.win);
    InfoLines = n;
}



void SetMapStateProp(tmp_win, state)
TwmWindow *tmp_win;
int state;
{
    unsigned long data[2];		/* "suggested" by ICCCM version 1 */

    data[0] = (unsigned long) state;
    data[1] = (unsigned long) (tmp_win->iconify_by_unmapping ? None :
			   tmp_win->icon_w.win);

    XChangeProperty (dpy, tmp_win->w, _XA_WM_STATE, _XA_WM_STATE, 32,
		 PropModeReplace, (unsigned char *) data, 2);
}



Bool GetWMState (w, statep, iwp)
    Window w;
    int *statep;
    Window *iwp;
{
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytesafter;
    unsigned long *datap = NULL;
    Bool retval = False;

	/* used to test for '!datap' - djhjr - 1/10/98 */
    if (XGetWindowProperty (dpy, w, _XA_WM_STATE, 0L, 2L, False, _XA_WM_STATE,
			    &actual_type, &actual_format, &nitems, &bytesafter,
			    (unsigned char **) &datap) != Success ||
			    actual_type == None)
      return False;

    if (nitems <= 2) {			/* "suggested" by ICCCM version 1 */
	*statep = (int) datap[0];
	*iwp = (Window) datap[1];
	retval = True;
    }

    XFree ((char *) datap);
    return retval;
}



/*
 * BumpWindowColormap - rotate our internal copy of WM_COLORMAP_WINDOWS
 */

void BumpWindowColormap (tmp, inc)
    TwmWindow *tmp;
    int inc;
{
    int i, j, previously_installed;
    ColormapWindow **cwins;

    if (!tmp) return;

    if (inc && tmp->cmaps.number_cwins > 0) {
	cwins = (ColormapWindow **) malloc(sizeof(ColormapWindow *)*
					   tmp->cmaps.number_cwins);
	if (cwins) {
	    if ((previously_installed =
		(Scr->cmapInfo.cmaps == &tmp->cmaps) &&
	        tmp->cmaps.number_cwins)) {
		for (i = tmp->cmaps.number_cwins; i-- > 0; )
		    tmp->cmaps.cwins[i]->colormap->state = 0;
	    }

	    for (i = 0; i < tmp->cmaps.number_cwins; i++) {
		j = i - inc;
		if (j >= tmp->cmaps.number_cwins)
		    j -= tmp->cmaps.number_cwins;
		else if (j < 0)
		    j += tmp->cmaps.number_cwins;
		cwins[j] = tmp->cmaps.cwins[i];
	    }

	    free((char *) tmp->cmaps.cwins);

	    tmp->cmaps.cwins = cwins;

	    if (tmp->cmaps.number_cwins > 1)
		memset( tmp->cmaps.scoreboard, 0,
		       ColormapsScoreboardLength(&tmp->cmaps));

	    if (previously_installed)
		InstallWindowColormaps(PropertyNotify, (TwmWindow *) NULL);
	}
    } else
	FetchWmColormapWindows (tmp);
}



void HideIconManager(tmp_win)
TwmWindow *tmp_win;
{
	/* added this 'if (...) else ...' - djhjr - 9/21/99 */
	if (tmp_win == NULL)
	{
		name_list *list;

		HideIconMgr(&Scr->iconmgr);

		/*
		 * New code in list.c necessitates 'next_entry()' and
		 * 'contents_of_entry()' - djhjr - 10/20/01
		 */
		for (list = Scr->IconMgrs; list != NULL; list = next_entry(list))
			HideIconMgr((IconMgr *)contents_of_entry(list));
	}
	else
	{
		IconMgr *ip;

		if ((ip = (IconMgr *)LookInList(Scr->IconMgrs, tmp_win->full_name,
				&tmp_win->class)) == NULL)
			ip = &Scr->iconmgr;

		HideIconMgr(ip);
	}
}

/* djhjr - 9/21/99 */
void HideIconMgr(ip)
IconMgr *ip;
{
	/* djhjr - 6/10/98 */
	if (ip->count == 0)
	    return;

    SetMapStateProp (ip->twm_win, WithdrawnState);
    XUnmapWindow(dpy, ip->twm_win->frame);
    if (ip->twm_win->icon_w.win)
      XUnmapWindow (dpy, ip->twm_win->icon_w.win);
    ip->twm_win->mapped = FALSE;
    ip->twm_win->icon = TRUE;
}

/* djhjr - 9/21/99 */
void ShowIconMgr(ip)
IconMgr *ip;
{
	/* added the second condition - djhjr - 6/10/98 */
	if (Scr->NoIconManagers || ip->count == 0)
		return;

	DeIconify(ip->twm_win);
	XRaiseWindow(dpy, ip->twm_win->frame);
	XRaiseWindow(dpy, ip->twm_win->VirtualDesktopDisplayWindow.win);
}


void SetBorder (tmp, onoroff)
TwmWindow	*tmp;
Bool		onoroff;
{
	if (tmp->highlight)
	{
		/* djhjr - 4/22/96 */
		/* was 'Scr->use3Dborders' - djhjr - 8/11/98 */
		if (Scr->BorderBevelWidth > 0)
			PaintBorders (tmp, onoroff);
		else
		{
			if (onoroff)
			{
/* djhjr - 4/24/96
				XSetWindowBorder (dpy, tmp->frame, tmp->border);
*/
/* djhjr - 11/17/97
				XSetWindowBorder (dpy, tmp->frame, tmp->border_tile.back);
*/
				XSetWindowBorder (dpy, tmp->frame, tmp->border.back);

				if (tmp->title_w.win)
/* djhjr - 4/24/96
					XSetWindowBorder (dpy, tmp->title_w.win, tmp->border);
*/
/* djhjr - 11/17/97
					XSetWindowBorder (dpy, tmp->title_w.win, tmp->border_tile.back);
*/
					XSetWindowBorder (dpy, tmp->title_w.win, tmp->border.back);
			}
			else
			{
				XSetWindowBorderPixmap (dpy, tmp->frame, tmp->gray);

				if (tmp->title_w.win)
					XSetWindowBorderPixmap (dpy, tmp->title_w.win, tmp->gray);
			}
		}

		/* djhjr - 11/17/97 */
		/* rem'd out test for button color - djhjr - 9/15/99 */
		if (/*Scr->ButtonColorIsFrame && */tmp->titlebuttons)
		{
			int i, nb = Scr->TBInfo.nleft + Scr->TBInfo.nright;
			TBWindow *tbw;

			/* collapsed two functions - djhjr - 8/10/98 */
			for (i = 0, tbw = tmp->titlebuttons; i < nb; i++, tbw++)
				PaintTitleButton(tmp, tbw, (onoroff) ? 2 : 1);
		}
	}
}



void DestroyMenu (menu)
    MenuRoot *menu;
{
    MenuItem *item;

    if (menu->w.win) {
#ifdef TWM_USE_XFT
	if (Scr->use_xft > 0)
	    MyXftDrawDestroy (menu->w.xft);
#endif
	XDeleteContext (dpy, menu->w.win, MenuContext);
	XDeleteContext (dpy, menu->w.win, ScreenContext);
	if (Scr->Shadow) XDestroyWindow (dpy, menu->shadow);
	XDestroyWindow(dpy, menu->w.win);
    }

    for (item = menu->first; item; ) {
	MenuItem *tmp = item;
	item = item->next;
	free ((char *) tmp);
    }
}



/*
 * warping routines
 */

/* for moves and resizes from center - djhjr - 10/4/02 */
void WarpScreenToWindow(t)
TwmWindow *t;
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

/* was in-lined in WarpToWindow() - djhjr - 5/30/00 */
void WarpWindowOrScreen(t)
TwmWindow *t;
{
	
	/* 
	 * we are either moving the window onto the screen, or the screen to the
     * window, the distances remain the same
     */

	if ((t->frame_x < Scr->MyDisplayWidth)
	    && (t->frame_y < Scr->MyDisplayHeight)
	    && (t->frame_x + t->frame_width >= 0)
	    && (t->frame_y + t->frame_height >= 0))
	{
		
		/*
		 *	window is visible; you can simply
		 *	snug it if WarpSnug or WarpWindows is set -- DSE
		 */
		
		if (Scr->WarpSnug || Scr->WarpWindows)
		{ 
			int right,left,up,down,dx,dy;

			/*
			 * Adjustment for border widths submitted by Steve Ratcliffe
			 * Note: Do not include the 3D border width!
			 */
			right = t->frame_x;
			left = t->frame_x + t->frame_width + 2 * t->frame_bw;
			up = t->frame_y;
			down = t->frame_y + t->frame_height + 2 * t->frame_bw;
	
			dx = 0;
			if (left-right < Scr->MyDisplayWidth)
			{
				if (right<0)
					dx = right;
				else if (left>Scr->MyDisplayWidth)
					dx = left - Scr->MyDisplayWidth;
			}
	
			dy = 0;
			if (down-up < Scr->MyDisplayHeight)
			{
				if (up<0)
					dy = up;
				else if (down>Scr->MyDisplayHeight)
					dy = down - Scr->MyDisplayHeight;
			}
	
			if (dx!=0 || dy!=0) {
				/* added 'Scr->WarpSnug ||' - djhjr - 5/30/00 */
				if (Scr->WarpSnug || Scr->WarpWindows)
				{
					/* move the window */
					VirtualMoveWindow(t, t->virtual_frame_x - dx,
					    t->virtual_frame_y - dy);
				}
				else
				{
					/* move the screen */
					PanRealScreen(dx,dy,NULL,NULL);
				}
			}
		}
	}
	else
	{

		/*
		 *	Window is invisible; we need to move it or the screen.
		 */
		 
		int xdiff, ydiff;

		xdiff = ((Scr->MyDisplayWidth - t->frame_width) / 2) - t->frame_x;
		ydiff = ((Scr->MyDisplayHeight - t->frame_height) / 2) - t->frame_y;

		/* added 'Scr->WarpSnug ||' - djhjr - 5/30/00 */
		if (Scr->WarpSnug || Scr->WarpWindows)
		{
			/* move the window */
			VirtualMoveWindow(t, t->virtual_frame_x + xdiff,
			    t->virtual_frame_y + ydiff);
		}
		else
		{
			/* move the screen */
			PanRealScreen(-xdiff, -ydiff,NULL,NULL); /* DSE */
		}
	}

	if (t->auto_raise || !Scr->NoRaiseWarp)
		AutoRaiseWindow (t);
}

/* for icon manager management - djhjr - 5/30/00 */
void WarpInIconMgr(w, t)
WList *w;
TwmWindow *t;
{
	int x, y, pan_margin = 0;
	/* djhjr - 9/9/02 */
	int bw = t->frame_bw3D + t->frame_bw;

	RaiseStickyAbove();
	RaiseAutoPan();                                     

	WarpWindowOrScreen(t);

	/* was 'Scr->BorderWidth' - djhjr - 9/9/02 */
	x = w->x + bw + EDGE_OFFSET + 5;
	x += (Scr->IconMgrBevelWidth > 0) ? Scr->IconMgrBevelWidth : bw;
	y = w->y + bw + w->height / 2;
	y += (Scr->IconMgrBevelWidth > 0) ? Scr->IconMgrBevelWidth : bw;
	y += w->iconmgr->twm_win->title_height;

	/*
	 * adjust the pointer for partially visible windows and the
	 * AutoPan border width
	 */

	if (Scr->AutoPanX) pan_margin = Scr->AutoPanBorderWidth;

	if (x + t->frame_x >= Scr->MyDisplayWidth - pan_margin)
		x = Scr->MyDisplayWidth - t->frame_x - pan_margin - 2;
	if (x + t->frame_x <= pan_margin)
		x = -t->frame_x + pan_margin + 2;
	if (y + t->frame_y >= Scr->MyDisplayHeight - pan_margin)
		y = Scr->MyDisplayHeight - t->frame_y - pan_margin - 2;
	if (y + t->frame_y <= pan_margin)
		y = -t->frame_y + pan_margin + 2;

	XWarpPointer(dpy, None, t->frame, 0, 0, 0, 0, x, y); /* DSE */
}

/*
 * substantially re-written and added passing 'next' to next_by_class()
 *
 * djhjr - 5/13/98 6/6/98 6/15/98
 */
#ifdef ORIGINAL_WARPCLASS
void WarpClass (next, t, class)
    int next;
    TwmWindow *t;
    char *class;
{
    int len = strlen(class);

    if (!strncmp(class, t->class.res_class, len))
	t = next_by_class(t, class);
    else
	t = next_by_class((TwmWindow *)NULL, class);
    if (t) {
	if (Scr->WarpUnmapped || t->mapped) {
	    if (!t->mapped) DeIconify (t);
	    if (!Scr->NoRaiseWarp)
		{
		    XRaiseWindow (dpy, t->frame);
		}
	    XRaiseWindow (dpy, t->VirtualDesktopDisplayWindow.win);

	    RaiseStickyAbove(); /* DSE */
	    RaiseAutoPan();

	    WarpToWindow (t);
	}
    }
}
#else /* ORIGINAL_WARPCLASS */
void WarpClass(next, t, class)
    int next;
    TwmWindow *t;
    char *class;
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
		else if (Scr->Focus)
		{
			i = XGetClassHint(dpy, Scr->Focus->w, &ch);
			if (i && !strncmp(class, ch.res_class, strlen(class)))
				class = ch.res_class;
		}
		/* djhjr - 6/21/00 */
		else
		{
			DoAudible(); /* was 'XBell()' - djhjr - 6/22/01 */
			return;
		}
	}
	if (!strlen(class) || !strncmp(class, "VTWM", 4))
		class = "VTWM";

/* djhjr - 8/3/98
	if (!(tt = next_by_class(next, t, class)))
		if (t) tt = t;
*/

	/* djhjr - 5/28/00 */
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

	/* added the second argument - djhjr - 5/28/00 */
	if (tt && warp_if_warpunmapped(tt, (next) ? F_WARPCLASSNEXT: F_WARPCLASSPREV))
	{
		RaiseStickyAbove(); /* DSE */
		RaiseAutoPan();

/* djhjr - 6/3/03 */
#ifndef NO_SOUND_SUPPORT
		PlaySound((next) ? F_WARPCLASSNEXT: F_WARPCLASSPREV);
#endif

		WarpToWindow(tt);

		/* djhjr - 5/28/00 */
		break;
    }
/* djhjr - 5/28/00
	else
		XBell(dpy, 0);
*/
	t = tt;
	} /* while (1) */
}
#endif /* ORIGINAL_WARPCLASS */



/* moved from add_window.c - djhjr - 10/27/02 */
void AddWindowToRing(tmp_win)
TwmWindow *tmp_win;
{
	if (Scr->Ring)
	{
		/* link window in after Scr->Ring */
		tmp_win->ring.prev = Scr->Ring;
		tmp_win->ring.next = Scr->Ring->ring.next;

		/* Scr->Ring's next's prev points to this */
		/*if (Scr->Ring->ring.next->ring.prev)*/
			Scr->Ring->ring.next->ring.prev = tmp_win;

		/* Scr->Ring's next points to this */
		Scr->Ring->ring.next = tmp_win;
	}
	else
		tmp_win->ring.next = tmp_win->ring.prev = Scr->Ring = tmp_win;
}

/* moved from events.c - djhjr - 10/27/02 */
void RemoveWindowFromRing(tmp_win)
TwmWindow *tmp_win;
{
	/* unlink window */
	if (tmp_win->ring.prev)
		tmp_win->ring.prev->ring.next = tmp_win->ring.next;
	if (tmp_win->ring.next)
		tmp_win->ring.next->ring.prev = tmp_win->ring.prev;

	/*  if window was only thing in ring, null out ring */
	if (Scr->Ring == tmp_win)
		Scr->Ring = (tmp_win->ring.next != tmp_win) ?
			    tmp_win->ring.next : (TwmWindow *)NULL;

	/* if window was ring leader, set to next (or null) */
	if (!Scr->Ring || Scr->RingLeader == tmp_win)
		Scr->RingLeader = Scr->Ring;

	tmp_win->ring.next = tmp_win->ring.prev = NULL;
}

void WarpAlongRing (ev, forward)
    XButtonEvent *ev;
    Bool forward;
{
    TwmWindow *r, *head;

    /*
     * Re-vamped much of this to properly handle icon managers, and
     * clean up dumb code I added some time back.
     * djhjr - 11/8/01
     * Cleaned it up again. I musta been high. Twice.
     * djhjr - 10/27/02
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
#ifdef ORIGINAL_WARPRINGCOORDINATES /* djhjr - 5/11/98 */
	TwmWindow *p = Scr->RingLeader, *t;
#endif

/* done in WarpToWindow - djhjr - 10/27/02
	Scr->RingLeader = r;
*/

	RaiseStickyAbove();
	RaiseAutoPan();

/* djhjr - 6/3/03 */
#ifndef NO_SOUND_SUPPORT
	PlaySound(F_WARPRING);
#endif

	WarpToWindow (r);

#ifdef ORIGINAL_WARPRINGCOORDINATES /* djhjr - 5/11/98 */
	if (p && p->mapped &&
	    XFindContext (dpy, ev->window, TwmContext, (caddr_t *)&t) == XCSUCCESS &&
	    p == t)
	{
	    p->ring.cursor_valid = True;
	    p->ring.curs_x = ev->x_root - t->frame_x;
	    p->ring.curs_y = ev->y_root - t->frame_y;
	    if (p->ring.curs_x < -p->frame_bw ||
		p->ring.curs_x >= p->frame_width + p->frame_bw ||
		p->ring.curs_y < -p->frame_bw ||
		p->ring.curs_y >= p->frame_height + p->frame_bw)
	    {
		/* somehow out of window */
		p->ring.curs_x = p->frame_width / 2;
		p->ring.curs_y = p->frame_height / 2;
	    }
	}
#endif
    }
    else
	DoAudible(); /* was 'XBell()' - djhjr - 6/22/01 */
}



void WarpToScreen (n, inc)
    int n, inc;
{
    Window dumwin;
    int x, y, dumint;
    unsigned int dummask;
    ScreenInfo *newscr = NULL;

    while (!newscr) {
					/* wrap around */
	if (n < 0)
	  n = NumScreens - 1;
	else if (n >= NumScreens)
	  n = 0;

	newscr = ScreenList[n];
	if (!newscr) {			/* make sure screen is managed */
	    if (inc) {			/* walk around the list */
		n += inc;
		continue;
	    }
	    fprintf (stderr, "%s:  unable to warp to unmanaged screen %d\n",
		     ProgramName, n);
	    DoAudible(); /* was 'XBell()' - djhjr - 6/22/01 */
	    return;
	}
    }

    if (Scr->screen == n) return;	/* already on that screen */

    PreviousScreen = Scr->screen;
    XQueryPointer (dpy, Scr->Root, &dumwin, &dumwin, &x, &y,
		   &dumint, &dumint, &dummask);

/* djhjr - 6/3/03 */
#ifndef NO_SOUND_SUPPORT
    PlaySound(F_WARPTOSCREEN);
#endif

    XWarpPointer (dpy, None, newscr->Root, 0, 0, 0, 0, x, y);
    return;
}



void WarpToWindow (t)
TwmWindow *t;
{
	int x, y;
	int pan_margin = 0;			/* djhjr - 5/28/00 */
	int bw = t->frame_bw3D + t->frame_bw;	/* djhjr - 9/9/02 */
	Window w = t->frame;			/* djhjr - 5/30/00 */
	
	WarpWindowOrScreen(t);	/* djhjr - 5/30/00 */

#ifdef ORIGINAL_WARPRINGCOORDINATES /* djhjr - 5/11/98 */
	if (t->ring.cursor_valid) {
		x = t->ring.curs_x;
		y = t->ring.curs_y;
		}
	else {
		x = t->frame_width / 2;
		y = t->frame_height / 2;
		}
#else
	/* added this 'if (...) else' - djhjr - 6/10/98 */
	if (t->iconmgr)
	{
/* djhjr - 5/30/00
		if (t->iconmgrp->count > 0)
			XWarpPointer(dpy, None, t->iconmgrp->first->icon, 0,0,0,0,
					EDGE_OFFSET, EDGE_OFFSET);

		return;
*/
		if (t->iconmgrp->count > 0)
		{
			w = t->iconmgrp->twm_win->frame;

			/* was 'Scr->BorderWidth' - djhjr - 9/9/02 */
			x = t->iconmgrp->x + bw + EDGE_OFFSET + 5;
			x += (Scr->IconMgrBevelWidth > 0) ? Scr->IconMgrBevelWidth : bw;
			y = t->iconmgrp->y + bw + t->iconmgrp->first->height / 2;
			y += (Scr->IconMgrBevelWidth > 0) ? Scr->IconMgrBevelWidth : bw;
			y += t->iconmgrp->twm_win->title_height;
		}
	}
	else if (!t->title_w.win)
	{
		/* added this 'if (...) else' - djhjr - 10/16/02 */
		if (Scr->WarpCentered & WARPC_UNTITLED)
		{
			x = t->frame_width / 2;
			y = t->frame_height / 2;
		}
		else
		{
			x = t->frame_width / 2;
			y = (t->wShaped) ? bw : bw / 2;	/* djhjr - 9/9/02 */
		}
	}
	else
	{
		/* added this 'if (...) else' - djhjr - 10/16/02 */
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
			 * Submitted by Steve Ratcliffe
			 * was '(t->frame_bw3D + t->frame_bw)' - djhjr - 9/9/02
			 */
			x = t->title_x + t->title_width / 2 + bw;
			y = t->title_height / 2 + bw;
		}
	}

	/* was 'Scr->use3Dborders' - djhjr - 8/11/98 */
	if (!Scr->BorderBevelWidth > 0) y -= t->frame_bw;
#endif

	/*
	 * adjust the pointer for partially visible windows and the
	 * AutoPan border width - djhjr - 5/30/00
	 * was '(t->frame_bw3D + t->frame_bw)' - djhjr - 9/9/02
	 */

	if (Scr->AutoPanX) pan_margin = Scr->AutoPanBorderWidth;

	if (x + t->frame_x >= Scr->MyDisplayWidth - pan_margin)
		x = Scr->MyDisplayWidth - t->frame_x - pan_margin - 2;
	if (x + t->frame_x <= pan_margin)
	{
		if (t->title_w.win)
			x = t->title_width - (t->frame_x + t->title_width) +
			    pan_margin + 2;
		else
			x = -t->frame_x + pan_margin + 2;
	}

	/* added test for centered warps - djhjr - 10/16/02 */
	if (t->title_w.win && !(Scr->WarpCentered & WARPC_TITLED) &&
			(x < t->title_x || x > t->title_x + t->title_width))
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

	/* was 't->frame' - djhjr - 5/30/00 */
	XWarpPointer (dpy, None, w, 0, 0, 0, 0, x, y);

	/* djhjr - 10/27/02 */
	if (t->ring.next) Scr->RingLeader = t;
}



/*
 * substantially re-written and added receiving and using 'next'
 *
 * djhjr - 5/13/98 5/19/98 6/6/98 6/15/98
 */
#ifdef ORIGINAL_WARPCLASS
TwmWindow *
next_by_class (t, class)
TwmWindow *t;
char *class;
{
    TwmWindow *tt;
    int len = strlen(class);

    if (t)
	for (tt = t->next; tt != NULL; tt = tt->next)
	    if (!strncmp(class, tt->class.res_class, len)) return tt;
    for (tt = Scr->TwmRoot.next; tt != NULL; tt = tt->next)
	if (!strncmp(class, tt->class.res_class, len)) return tt;
    return NULL;
}
#else /* ORIGINAL_WARPCLASS */
static TwmWindow *
next_by_class (next, t, class)
int next;
TwmWindow *t;
char *class;
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
	for (tt = (next) ? ((t) ? t->next : tl) : ((t) ? t->prev : tl);
			tt != NULL;
			tt = (next) ? tt->next : tt->prev)
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
				if (i == 0) break;
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
	i = 0; if (tl) i = XGetClassHint(dpy, tl->w, &ch);
	fprintf(stderr, "prev wrapped to \"%s\ \"%s\"\n", (i) ? ch.res_class : "NO RES_CLASS!", (i) ? ch.res_name : "NO RES_CLASS!");
#endif
	tp = tl;
	return tp;
}
#endif /* ORIGINAL_WARPCLASS */

/* this was inlined in many places, and even more now - djhjr - 5/13/98 */
/* added the second argument - djhjr - 5/28/00 */
static int warp_if_warpunmapped(w, func)
TwmWindow *w;
int func;
{
	/* skip empty icon managers - 10/27/02 */
	if (w && (w->iconmgr && w->iconmgrp->count == 0))
		return (0);

	if (Scr->WarpUnmapped || w->mapped)
	{
		/* submitted by Ugen Antsilevitch - 5/28/00 */
		/* if F_NOFUNCTION, override WarpVisible - djhjr - 5/28/00 */
		if (func != F_NOFUNCTION && Scr->WarpVisible)
		{
			int pan_margin = 0;

			if (Scr->AutoPanX) pan_margin = Scr->AutoPanBorderWidth;

			if (w->frame_x >= Scr->MyDisplayWidth - pan_margin ||
				w->frame_y >= Scr->MyDisplayHeight - pan_margin ||
				w->frame_x + w->frame_width <= pan_margin ||
				w->frame_y + w->frame_height <= pan_margin)
			return 0;
		}

		if (!w->mapped) DeIconify(w);
		if (!Scr->NoRaiseWarp) XRaiseWindow(dpy, w->frame);
		XRaiseWindow(dpy, w->VirtualDesktopDisplayWindow.win);

		return (1);
	}

	return (0);
}

/* djhjr - 9/17/02 */
static int
do_squeezetitle(context, func, tmp_win, squeeze)
int context, func;
TwmWindow *tmp_win;
SqueezeInfo *squeeze;
{
    if (DeferExecution (context, func, Scr->SelectCursor))
	return TRUE;

    /* honor "Don't Squeeze" resources - djhjr - 9/17/02 */
    if (Scr->SqueezeTitle &&
		!LookInList(Scr->DontSqueezeTitleL, tmp_win->full_name, &tmp_win->class))
    {
	if ( tmp_win->title_height )	/* Not for untitled windows! */
	{
	    PopDownMenu();	/* djhjr - 9/17/02 */

#ifndef NO_SOUND_SUPPORT
	    PlaySound(func);
#endif

	    tmp_win->squeeze_info = squeeze;
	    SetFrameShape( tmp_win );

	    /* Can't go in SetFrameShape()... - djhjr - 4/1/00 */
	    if (Scr->WarpCursor ||
			LookInList(Scr->WarpCursorL, tmp_win->full_name, &tmp_win->class))
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

static void setup_restart(time)
	Time time;
{
/* djhjr - 6/22/01 */
#ifndef NO_SOUND_SUPPORT
	CloseSound();
#endif

	SetRealScreen(0,0);
	XSync (dpy, 0);
	Reborder (time);
	XSync (dpy, 0);

	/* djhjr - 3/13/97 */
	XCloseDisplay(dpy);

	/* djhjr - 12/2/01 */
	delete_pidfile();
}

void RestartVtwm(time)
	Time time;
{
	setup_restart(time);

	execvp(*Argv, Argv);
	fprintf (stderr, "%s:  unable to restart \"%s\"\n", ProgramName, *Argv);
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

static void send_clientmessage (w, a, timestamp)
    Window w;
    Atom a;
    Time timestamp;
{
    XClientMessageEvent ev;

    ev.type = ClientMessage;
    ev.window = w;
    ev.message_type = _XA_WM_PROTOCOLS;
    ev.format = 32;
    ev.data.l[0] = a;
    ev.data.l[1] = timestamp;
    XSendEvent (dpy, w, False, 0L, (XEvent *) &ev);
}

void SendDeleteWindowMessage (tmp, timestamp)
    TwmWindow *tmp;
    Time timestamp;
{
    send_clientmessage (tmp->w, _XA_WM_DELETE_WINDOW, timestamp);
}

void SendSaveYourselfMessage (tmp, timestamp)
    TwmWindow *tmp;
    Time timestamp;
{
    send_clientmessage (tmp->w, _XA_WM_SAVE_YOURSELF, timestamp);
}

void SendTakeFocusMessage (tmp, timestamp)
    TwmWindow *tmp;
    Time timestamp;
{
    send_clientmessage (tmp->w, _XA_WM_TAKE_FOCUS, timestamp);
}


/* djhjr - 4/27/96 */
void DisplayPosition (x, y)
int x, y;
{
    char str [100];
	int i;

/* djhjr - 5/10/96
    char signx = '+';
    char signy = '+';

    if (x < 0) {
	x = -x;
	signx = '-';
    }
    if (y < 0) {
	y = -y;
	signy = '-';
    }

    i = sprintf (str, " %c%-4d %c%-4d ", signx, x, signy, y);
*/
/*
 * Non-SysV systems - specifically, BSD-derived systems - return a
 * pointer to the string, not its length. Submitted by Goran Larsson
    i = sprintf (str, "%+6d %-+6d", x, y);
 */
    sprintf (str, "%+6d %-+6d", x, y);
    i = strlen (str);

    XRaiseWindow (dpy, Scr->SizeWindow.win);
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

/* djhjr - 9/21/99 */
int FindMenuOrFuncInBindings(contexts, mr, func)
int contexts;
MenuRoot *mr;
int func;
{
	MenuRoot *start;
	FuncKey *key;
	int found = 0; /* context bitmap for menu or function */
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
		if ((contexts & (1 << j)) == 0) continue;

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
					if (Scr->Mouse[MOUSELOC(i,j,k)].func == F_MENU)
						l = FindMenuInMenus(Scr->Mouse[MOUSELOC(i,j,k)].menu, mr);
				}
				else
				{
					if (Scr->Mouse[MOUSELOC(i,j,k)].func == func)
						l = 1;
					else if (Scr->Mouse[MOUSELOC(i,j,k)].func == F_MENU)
						l = FindFuncInMenus(Scr->Mouse[MOUSELOC(i,j,k)].menu, func);
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

/* djhjr - 9/21/99 */
int FindMenuOrFuncInWindows(tmp_win, contexts, mr, func)
TwmWindow *tmp_win;
int contexts;
MenuRoot *mr;
int func;
{
	TwmWindow *twin;
	TwmDoor *d;
	TBWindow *tbw;
	int i, nb;

	if (contexts & C_ROOT_BIT) return 1;

	for (twin = Scr->TwmRoot.next; twin != NULL; twin = twin->next)
		if (twin != tmp_win)
		{
			/*
			 * if this window is an icon manager,
			 * skip the windows that aren't in it
			 */
			if (tmp_win->iconmgr && twin->list &&
					tmp_win != twin->list->iconmgr->twm_win)
				continue;

			if (twin->mapped)
			{
				for (i = 1; i < C_ALL_BITS; i = (1 << i))
				{
					if ((contexts & i) == 0) continue;

					switch (i)
					{
						case C_WINDOW_BIT:
						case C_FRAME_BIT:
							break;
						case C_TITLE_BIT:
							if (!twin->title_height) continue;
							break;
						case C_VIRTUAL_BIT:
							if (twin->w != Scr->VirtualDesktopDisplayOuter)
								continue;
							break;
						case C_DOOR_BIT:
							if (XFindContext(dpy, twin->w, DoorContext,
									(caddr_t *)&d) == XCNOENT)
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

				if (contexts & C_ICON_BIT) return 1;
			}
		}

	return 0;
}

/* djhjr - 9/21/99 */
int FindMenuInMenus(start, sought)
MenuRoot *start, *sought;
{
	MenuItem *mi;

	if (!start) return 0;	/* submitted by Jonathan Paisley - 11/11/02 */
	if (start == sought) return 1;

	for (mi = start->first; mi != NULL; mi = mi->next)
		if (mi->sub)
			if (FindMenuInMenus(mi->sub, sought))
				return 1;

	return 0;
}

/* djhjr - 9/21/99 */
int FindFuncInMenus(mr, func)
MenuRoot *mr;
int func;
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

/* djhjr - 6/22/01 */
void DoAudible()
{
#ifndef NO_SOUND_SUPPORT
	if (PlaySound(S_BELL)) return;
#endif

	XBell(dpy, 0);
}

