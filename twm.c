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
 * $XConsortium: twm.c,v 1.124 91/05/08 11:01:54 dave Exp $
 *
 * twm - "Tom's Window Manager"
 *
 * 27-Oct-87 Thomas E. LaStrange	File created
 * 10-Oct-90 David M. Sternlicht        Storing saved colors on root
 ***********************************************************************/

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include "twm.h"
#include "add_window.h"
#include "gc.h"
#include "parse.h"
#include "version.h"
#include "menus.h"
#include "events.h"
#include "util.h"
#include "gram.h"
#include "screen.h"
#include "iconmgr.h"
#include "desktop.h"
#include <X11/Xproto.h>
#include <X11/Xatom.h>

#ifndef PIXMAP_DIRECTORY
#define PIXMAP_DIRECTORY "/usr/lib/X11/twm"
#endif

Display *dpy;			/* which display are we talking to */
Window ResizeWindow;		/* the window we are resizing */

int MultiScreen = TRUE;		/* try for more than one screen? */
int NumScreens;			/* number of screens in ScreenList */
int HasShape;			/* server supports shape extension? */
int ShapeEventBase, ShapeErrorBase;
ScreenInfo **ScreenList;	/* structures for each screen */
ScreenInfo *Scr = NULL;		/* the cur and prev screens */
int PreviousScreen;		/* last screen that we were on */
int FirstScreen;		/* TRUE ==> first screen of display */
Window windowmask = (Window) 0;		/* window to mask the screen at startup */
Bool PrintErrorMessages = False;	/* controls error messages */
static int RedirectError;	/* TRUE ==> another window manager running */
static int CatchRedirectError();	/* for settting RedirectError */
static int TwmErrorHandler();	/* for everything else */
char Info[INFO_LINES][INFO_SIZE];		/* info strings to print */
int InfoLines;
char *InitFile = NULL;

Cursor UpperLeftCursor;		/* upper Left corner cursor */
Cursor RightButt;
Cursor MiddleButt;
Cursor LeftButt;

XContext TwmContext;		/* context for twm windows */
XContext MenuContext;		/* context for all menu windows */
XContext IconManagerContext;	/* context for all window list windows */
XContext VirtualContext;	/* context for all desktop display windows */
XContext ScreenContext;		/* context to get screen data */
XContext ColormapContext;	/* context for colormap operations */
XContext DoorContext;		/* context for doors */

XClassHint NoClass;		/* for applications with no class */

XGCValues Gcv;

char *Home;			/* the HOME environment variable */
int HomeLen;			/* length of Home */
int ParseError;			/* error parsing the .twmrc file */

int HandlingEvents = FALSE;	/* are we handling events yet? */

Window JunkRoot;		/* junk window */
Window JunkChild;		/* junk window */
int JunkX;			/* junk variable */
int JunkY;			/* junk variable */
unsigned int JunkWidth, JunkHeight, JunkBW, JunkDepth, JunkMask;

char *ProgramName;
int Argc;
char **Argv;
char **Environ;

Bool RestartPreviousState = False;	/* try to restart in previous state */

unsigned long black, white;

extern void assign_var_savecolor();
SIGNAL_T Restart();

/***********************************************************************
 *
 *  Procedure:
 *	main - start of twm
 *
 ***********************************************************************
 */

main(argc, argv, environ)
    int argc;
    char **argv;
    char **environ;
{
    Window root, parent, *children;
    unsigned int nchildren;
    int i, j;
    char *display_name = NULL;
    unsigned long valuemask;	/* mask for create windows */
    XSetWindowAttributes attributes;	/* attributes for create windows */
    int numManaged, firstscrn, lastscrn, scrnum;
    extern ColormapWindow *CreateColormapWindow();
    char geom [256];
    char *welcomefile;
    int  screenmasked;
    int rsx, rsy;

    ProgramName = argv[0];
    Argc = argc;
    Argv = argv;
    Environ = environ;

    for (i = 1; i < argc; i++) {
	if (argv[i][0] == '-') {
	    switch (argv[i][1]) {
	      case 'd':				/* -display dpy */
		if (++i >= argc) goto usage;
		display_name = argv[i];
		continue;
	      case 's':				/* -single */
		MultiScreen = FALSE;
		continue;
	      case 'f':				/* -file twmrcfilename */
		if (++i >= argc) goto usage;
		InitFile = argv[i];
		continue;
	      case 'v':				/* -verbose */
		PrintErrorMessages = True;
		continue;
	      case 'q':				/* -quiet */
		PrintErrorMessages = False;
		continue;
	    }
	}
      usage:
	fprintf (stderr,
		 "usage:  %s [-display dpy] [-f file] [-s] [-q] [-v]\n",
		 ProgramName);
	exit (1);
    }

#define newhandler(sig, action) \
    if (signal (sig, SIG_IGN) != SIG_IGN) (void) signal (sig, action)

    newhandler (SIGINT, Done);
    newhandler (SIGHUP, Restart);
    newhandler (SIGQUIT, Done);
    newhandler (SIGTERM, Done);

#undef newhandler

    Home = getenv("HOME");
    if (Home == NULL)
	Home = "./";

    HomeLen = strlen(Home);

    NoClass.res_name = NoName;
    NoClass.res_class = NoName;

    if (!(dpy = XOpenDisplay(display_name))) {
	fprintf (stderr, "%s:  unable to open display \"%s\"\n",
		 ProgramName, XDisplayName(display_name));
	exit (1);
    }

    if (fcntl(ConnectionNumber(dpy), F_SETFD, 1) == -1) {
	fprintf (stderr,
		 "%s:  unable to mark display connection as close-on-exec\n",
		 ProgramName);
	exit (1);
    }

    HasShape = XShapeQueryExtension (dpy, &ShapeEventBase, &ShapeErrorBase);
    TwmContext = XUniqueContext();
    MenuContext = XUniqueContext();
    IconManagerContext = XUniqueContext();
    VirtualContext = XUniqueContext();
    ScreenContext = XUniqueContext();
    ColormapContext = XUniqueContext();
    DoorContext = XUniqueContext();

    InternUsefulAtoms ();


    /* Set up the per-screen global information. */

    NumScreens = ScreenCount(dpy);

    if (MultiScreen)
    {
	firstscrn = 0;
	lastscrn = NumScreens - 1;
    }
    else
    {
	firstscrn = lastscrn = DefaultScreen(dpy);
    }

    InfoLines = 0;

    /* for simplicity, always allocate NumScreens ScreenInfo struct pointers */
    ScreenList = (ScreenInfo **) calloc (NumScreens, sizeof (ScreenInfo *));
    if (ScreenList == NULL)
    {
	fprintf (stderr, "%s: Unable to allocate memory for screen list, exiting.\n",
		 ProgramName);
	exit (1);
    }
    numManaged = 0;
    PreviousScreen = DefaultScreen(dpy);
    FirstScreen = TRUE;
    for (scrnum = firstscrn ; scrnum <= lastscrn; scrnum++)
    {
        /* Make sure property priority colors is empty */
        XChangeProperty (dpy, RootWindow(dpy, scrnum), _XA_MIT_PRIORITY_COLORS,
			 XA_CARDINAL, 32, PropModeReplace, NULL, 0);
	RedirectError = FALSE;
	XSetErrorHandler(CatchRedirectError);
	XSelectInput(dpy, RootWindow (dpy, scrnum),
	    ColormapChangeMask | EnterWindowMask | PropertyChangeMask |
	    SubstructureRedirectMask | KeyPressMask |
	    ButtonPressMask | ButtonReleaseMask);
	XSync(dpy, 0);
	XSetErrorHandler(TwmErrorHandler);

	if (RedirectError)
	{
	    fprintf (stderr, "%s:  another window manager is already running",
		     ProgramName);
	    if (MultiScreen && NumScreens > 0)
		fprintf(stderr, " on screen %d?\n", scrnum);
	    else
		fprintf(stderr, "?\n");
	    continue;
	}

	numManaged ++;

	/* Note:  ScreenInfo struct is calloc'ed to initialize to zero. */
	Scr = ScreenList[scrnum] =
	    (ScreenInfo *) calloc(1, sizeof(ScreenInfo));
  	if (Scr == NULL)
  	{
  	    fprintf (stderr, "%s: unable to allocate memory for ScreenInfo structure for screen %d.\n",
  		     ProgramName, scrnum);
  	    continue;
  	}

	/* initialize list pointers, remember to put an initialization
	 * in InitVariables also
	 */
	Scr->BorderColorL = NULL;
	Scr->IconBorderColorL = NULL;
	Scr->BorderTileForegroundL = NULL;
	Scr->BorderTileBackgroundL = NULL;
	Scr->TitleForegroundL = NULL;
	Scr->TitleBackgroundL = NULL;
	Scr->IconForegroundL = NULL;
	Scr->IconBackgroundL = NULL;
	Scr->NoIconTitle = NULL;
	Scr->NoTitle = NULL;
	Scr->MakeTitle = NULL;
	Scr->AutoRaise = NULL;
	Scr->IconNames = NULL;
	Scr->NoHighlight = NULL;
	Scr->NoStackModeL = NULL;
	Scr->NoTitleHighlight = NULL;
	Scr->DontIconify = NULL;
	Scr->IconMgrNoShow = NULL;
	Scr->IconMgrShow = NULL;
	Scr->IconifyByUn = NULL;
	Scr->IconManagerFL = NULL;
	Scr->IconManagerBL = NULL;
	Scr->IconMgrs = NULL;
	Scr->StartIconified = NULL;
	Scr->SqueezeTitleL = NULL;
	Scr->DontSqueezeTitleL = NULL;
	Scr->WindowRingL = NULL;
	Scr->WarpCursorL = NULL;
	Scr->OpaqueMoveList = NULL;
	Scr->NoOpaqueMoveList = NULL;
	Scr->OpaqueResizeList = NULL;
	Scr->NoOpaqueResizeList = NULL;

	/* remember to put an initialization in InitVariables also
	 */

	Scr->screen = scrnum;
	Scr->d_depth = DefaultDepth(dpy, scrnum);
	Scr->d_visual = DefaultVisual(dpy, scrnum);
	Scr->Root = RootWindow(dpy, scrnum);
	XSaveContext (dpy, Scr->Root, ScreenContext, (caddr_t) Scr);

	Scr->TwmRoot.cmaps.number_cwins = 1;
	Scr->TwmRoot.cmaps.cwins =
		(ColormapWindow **) malloc(sizeof(ColormapWindow *));
	Scr->TwmRoot.cmaps.cwins[0] =
		CreateColormapWindow(Scr->Root, True, False);
	Scr->TwmRoot.cmaps.cwins[0]->visibility = VisibilityPartiallyObscured;

	Scr->cmapInfo.cmaps = NULL;
	Scr->cmapInfo.maxCmaps =
		MaxCmapsOfScreen(ScreenOfDisplay(dpy, Scr->screen));
	Scr->cmapInfo.root_pushes = 0;
	InstallWindowColormaps(0, &Scr->TwmRoot);

	Scr->StdCmapInfo.head = Scr->StdCmapInfo.tail =
	  Scr->StdCmapInfo.mru = NULL;
	Scr->StdCmapInfo.mruindex = 0;
	LocateStandardColormaps();

	Scr->TBInfo.nleft = Scr->TBInfo.nright = 0;
	Scr->TBInfo.head = NULL;
	Scr->TBInfo.border = -100; /* trick to have different default value if ThreeDTitles
	    is set or not */
	Scr->TBInfo.width = 0;
	Scr->TBInfo.leftx = 0;
	Scr->TBInfo.titlex = 0;

	Scr->MyDisplayWidth = DisplayWidth(dpy, scrnum);
	Scr->MyDisplayHeight = DisplayHeight(dpy, scrnum);
	Scr->MaxWindowWidth = 32767 - Scr->MyDisplayWidth;
	Scr->MaxWindowHeight = 32767 - Scr->MyDisplayHeight;

	Scr->XORvalue = (((unsigned long) 1) << Scr->d_depth) - 1;

	if (DisplayCells(dpy, scrnum) < 3)
	    Scr->Monochrome = MONOCHROME;
	else
	    Scr->Monochrome = COLOR;

	/* setup default colors */
	Scr->FirstTime = TRUE;
	GetColor(Scr->Monochrome, &black, "black");
	Scr->Black = black;
	GetColor(Scr->Monochrome, &white, "white");
	Scr->White = white;

	if (FirstScreen)
	{
	    SetFocus ((TwmWindow *)NULL, CurrentTime);

	    /* define cursors */

	    NewFontCursor(&UpperLeftCursor, "top_left_corner");
	    NewFontCursor(&RightButt, "rightbutton");
	    NewFontCursor(&LeftButt, "leftbutton");
	    NewFontCursor(&MiddleButt, "middlebutton");
	}

	Scr->iconmgr = NULL;
	AllocateIconManager ("TWM", "Icons", "", 1);

	Scr->IconDirectory = NULL;
	Scr->PixmapDirectory = PIXMAP_DIRECTORY;
	Scr->siconifyPm = None;
	Scr->pullPm = None;
	Scr->hilitePm = None;
	Scr->virtualPm = None;/*RFB PIXMAP*/
	Scr->RealScreenPm = None;/*RFB PIXMAP*/
	Scr->tbpm.xlogo = None;
	Scr->tbpm.resize = None;
	Scr->tbpm.question = None;
	Scr->tbpm.menu = None;
	Scr->tbpm.delete = None;

	screenmasked = 0;
	if ((welcomefile = getenv ("VTWM_WELCOME_FILE")) != NULL) {
	    screenmasked = 1;
	    MaskScreen (welcomefile);
	}
	InitVariables();
	InitMenus();

	/* Parse it once for each screen. */
	ParseTwmrc(InitFile);
	if (! screenmasked) MaskScreen (NULL);

	if (Scr->use3Dtitles) {
	    if (Scr->FramePadding  == -100) Scr->FramePadding  = 0;
	    if (Scr->TitlePadding  == -100) Scr->TitlePadding  = 0;
	    if (Scr->ButtonIndent  == -100) Scr->ButtonIndent  = 0;
	    if (Scr->TBInfo.border == -100) Scr->TBInfo.border = 0;
	}
	else {
	    if (Scr->FramePadding  == -100) Scr->FramePadding  = 2; /* values that look */
	    if (Scr->TitlePadding  == -100) Scr->TitlePadding  = 8; /* "nice" on */
	    if (Scr->ButtonIndent  == -100) Scr->ButtonIndent  = 1; /* 75 and 100dpi displays */
	    if (Scr->TBInfo.border == -100) Scr->TBInfo.border = 1;
	}
/*
	sprintf (geom, "%dx%d+0+0", Scr->MyDisplayWidth, Scr->MyDisplayHeight);
	AddIconRegion (geom, D_NORTH, D_WEST, 80, 70);
*/

	if (Scr->use3Dtitles && !Scr->BeNiceToColormap) GetShadeColors (&Scr->TitleC);
	if (Scr->use3Dmenus  && !Scr->BeNiceToColormap) GetShadeColors (&Scr->MenuC);
	if (Scr->use3Dmenus  && !Scr->BeNiceToColormap) GetShadeColors (&Scr->MenuTitleC);

	assign_var_savecolor(); /* storeing pixels for twmrc "entities" */
	if (Scr->SqueezeTitle == -1) Scr->SqueezeTitle = FALSE;
	if (!Scr->HaveFonts) CreateFonts();
	CreateGCs();
	MakeMenus();

	Scr->TitleBarFont.y += Scr->FramePadding;
	Scr->TitleHeight = Scr->TitleBarFont.height + Scr->FramePadding * 2;
	if (Scr->use3Dtitles) Scr->TitleHeight += 4;
	/* make title height be odd so buttons look nice and centered */
	if (!(Scr->TitleHeight & 1)) Scr->TitleHeight++;

	InitTitlebarButtons ();		/* menus are now loaded! */

	XGrabServer(dpy);
	XSync(dpy, 0);

	JunkX = 0;
	JunkY = 0;

	XQueryTree(dpy, Scr->Root, &root, &parent, &children, &nchildren);
	CreateIconManagers();
	if (!Scr->NoIconManagers)
	    Scr->iconmgr->twm_win->isicon = TRUE;

 	if (Scr->VirtualDesktopWidth > 0)
 		CreateDesktopDisplay();

	/* create all of the door windows */
	door_open_all();

	/*
	 * weed out icon windows
	 */
	for (i = 0; i < nchildren; i++) {
	    if (children[i]) {
		XWMHints *wmhintsp = XGetWMHints (dpy, children[i]);

		if (wmhintsp) {
		    if (wmhintsp->flags & IconWindowHint) {
			for (j = 0; j < nchildren; j++) {
			    if (children[j] == wmhintsp->icon_window) {
				children[j] = None;
				break;
			    }
			}
		    }
		    XFree ((char *) wmhintsp);
		}
	    }
	}

	/*
	 * map all of the non-override windows
	 */
	for (i = 0; i < nchildren; i++)
	{
	    if (children[i] && MappedNotOverride(children[i]))
	    {
		XUnmapWindow(dpy, children[i]);
		SimulateMapRequest(children[i]);
	    }
	}

	if (Scr->ShowIconManager && !Scr->NoIconManagers)
	{
	    Scr->iconmgr->twm_win->isicon = FALSE;
	    if (Scr->iconmgr->count)
	    {
		SetMapStateProp (Scr->iconmgr->twm_win, NormalState);
		XMapWindow(dpy, Scr->iconmgr->w);
		XMapWindow(dpy, Scr->iconmgr->twm_win->frame);
	    }
	}


	attributes.border_pixel = Scr->DefaultC.fore;
	attributes.background_pixel = Scr->DefaultC.back;
	attributes.event_mask = (ExposureMask | ButtonPressMask |
				 KeyPressMask | ButtonReleaseMask);
	attributes.backing_store = NotUseful;
	attributes.cursor = XCreateFontCursor (dpy, XC_hand2);
	valuemask = (CWBorderPixel | CWBackPixel | CWEventMask | 
		     CWBackingStore | CWCursor);
	Scr->InfoWindow = XCreateWindow (dpy, Scr->Root, 0, 0,
					 (unsigned int) 5, (unsigned int) 5,
					 (unsigned int) BW, 0,
					 (unsigned int) CopyFromParent,
					 (Visual *) CopyFromParent,
					 valuemask, &attributes);

	Scr->SizeStringWidth = XTextWidth (Scr->SizeFont.font,
					   " +-8888 x +-8888 ", 13);
	valuemask = (CWBorderPixel | CWBackPixel | CWBitGravity | CWSaveUnder);
	attributes.bit_gravity = NorthWestGravity;
	attributes.save_under = True;
	if (Scr->CenteredInfoBox) {
	    rsx = (Scr->MyDisplayWidth - Scr->SizeStringWidth)/2;
	    rsy = (Scr->MyDisplayHeight - (Scr->SizeFont.height + SIZE_VINDENT*2))/2;
	} else {
	    rsx = rsy = 0;
	}
	Scr->SizeWindow = XCreateWindow (dpy, Scr->Root, rsx, rsy,
					 (unsigned int) Scr->SizeStringWidth,
					 (unsigned int) (Scr->SizeFont.height +
							 SIZE_VINDENT*2),
					 (unsigned int) BW, 0,
					 (unsigned int) CopyFromParent,
					 (Visual *) CopyFromParent,
					 valuemask, &attributes);

	UnmaskScreen ();
	XUngrabServer(dpy);

	FirstScreen = FALSE;
    	Scr->FirstTime = FALSE;
    } /* for */

    if (numManaged == 0) {
	if (MultiScreen && NumScreens > 0)
	  fprintf (stderr, "%s:  unable to find any unmanaged screens\n",
		   ProgramName);
	exit (1);
    }

    RestartPreviousState = True;
    HandlingEvents = TRUE;
    InitEvents();
    HandleEvents();
}

/***********************************************************************
 *
 *  Procedure:
 *	InitVariables - initialize twm variables
 *
 ***********************************************************************
 */

InitVariables()
{
    FreeList(&Scr->BorderColorL);
    FreeList(&Scr->IconBorderColorL);
    FreeList(&Scr->BorderTileForegroundL);
    FreeList(&Scr->BorderTileBackgroundL);
    FreeList(&Scr->TitleForegroundL);
    FreeList(&Scr->TitleBackgroundL);
    FreeList(&Scr->IconForegroundL);
    FreeList(&Scr->IconBackgroundL);
    FreeList(&Scr->IconManagerFL);
    FreeList(&Scr->IconManagerBL);
    FreeList(&Scr->IconMgrs);
    FreeList(&Scr->NoIconTitle);
    FreeList(&Scr->NoTitle);
    FreeList(&Scr->MakeTitle);
    FreeList(&Scr->AutoRaise);
    FreeList(&Scr->IconNames);
    FreeList(&Scr->NoHighlight);
    FreeList(&Scr->NoStackModeL);
    FreeList(&Scr->NoTitleHighlight);
    FreeList(&Scr->DontIconify);
    FreeList(&Scr->IconMgrNoShow);
    FreeList(&Scr->IconMgrShow);
    FreeList(&Scr->IconifyByUn);
    FreeList(&Scr->StartIconified);
    FreeList(&Scr->IconManagerHighlightL);
    FreeList(&Scr->SqueezeTitleL);
    FreeList(&Scr->DontSqueezeTitleL);
    FreeList(&Scr->WindowRingL);
    FreeList(&Scr->WarpCursorL);
    FreeList(&Scr->NailedDown);
    FreeList(&Scr->VirtualDesktopColorFL);
    FreeList(&Scr->VirtualDesktopColorBL);
    FreeList(&Scr->VirtualDesktopColorBoL);
    FreeList(&Scr->DontShowInDisplay);
    FreeList(&Scr->DoorForegroundL);
    FreeList(&Scr->DoorBackgroundL);

    NewFontCursor(&Scr->FrameCursor, "top_left_arrow");
    NewFontCursor(&Scr->TitleCursor, "top_left_arrow");
    NewFontCursor(&Scr->IconCursor, "top_left_arrow");
    NewFontCursor(&Scr->IconMgrCursor, "top_left_arrow");
    NewFontCursor(&Scr->MoveCursor, "fleur");
    NewFontCursor(&Scr->ResizeCursor, "fleur");
    NewFontCursor(&Scr->MenuCursor, "sb_left_arrow");
    NewFontCursor(&Scr->ButtonCursor, "hand2");
    NewFontCursor(&Scr->WaitCursor, "watch");
    NewFontCursor(&Scr->SelectCursor, "dot");
    NewFontCursor(&Scr->DestroyCursor, "pirate");
    NewFontCursor(&Scr->DoorCursor, "exchange");/*RFBCURSOR*/
    NewFontCursor(&Scr->VirtualCursor, "rtl_logo");/*RFBCURSOR*/
    NewFontCursor(&Scr->DesktopCursor, "dotbox");/*RFBCURSOR*/
    Scr->NoCursor = NoCursor();

    Scr->Ring = NULL;
    Scr->RingLeader = NULL;

    Scr->DefaultC.fore = black;
    Scr->DefaultC.back = white;
    Scr->BorderColor = black;
    Scr->BorderTileC.fore = black;
    Scr->BorderTileC.back = white;
    Scr->TitleC.fore = black;
    Scr->TitleC.back = white;
    Scr->MenuC.fore = black;
    Scr->MenuC.back = white;
    Scr->MenuTitleC.fore = black;
    Scr->MenuTitleC.back = white;
    Scr->MenuShadowColor = black;
    Scr->IconC.fore = black;
    Scr->IconC.back = white;
    Scr->IconBorderColor = black;
    Scr->IconManagerC.fore = black;
    Scr->IconManagerC.back = white;
    Scr->IconManagerHighlight = black;
    Scr->VirtualC.fore = black;/*RFB VCOLOR*/
    Scr->VirtualC.back = white;/*RFB VCOLOR*/
	Scr->RealScreenC.back = black;/*RFB 4/92 */
	Scr->RealScreenC.fore = white;/*RFB 4/92 */
    Scr->VirtualDesktopDisplayC.fore = black;
    Scr->VirtualDesktopDisplayC.back = white;
    Scr->VirtualDesktopDisplayBorder = black;
    Scr->DoorC.fore = black;
    Scr->DoorC.back = white;

    Scr->FramePadding = -100;	/* trick to have different default value if ThreeDTitles
				is set or not */
    Scr->TitlePadding = -100;
    Scr->ButtonIndent = -100;
    Scr->SizeStringOffset = 0;
    Scr->BorderWidth = BW;
    Scr->IconBorderWidth = BW;
    Scr->VirtualDesktopDisplayBorderWidth = 1;
    Scr->UnknownWidth = 0;
    Scr->UnknownHeight = 0;
    Scr->NumAutoRaises = 0;
    Scr->TransientOnTop = 30;
    Scr->NoDefaults = FALSE;
    Scr->UsePPosition = PPOS_OFF;
    Scr->FocusRoot = TRUE;
    Scr->Focus = NULL;
    Scr->WarpCursor = FALSE;
    Scr->ForceIcon = FALSE;
    Scr->NoGrabServer = FALSE;
    Scr->NoRaiseDesktop = FALSE;
    Scr->NoRaiseMove = FALSE;
    Scr->NoRaiseResize = FALSE;
    Scr->NoRaiseDeicon = FALSE;
    Scr->NoRaiseWarp = FALSE;
    Scr->DontMoveOff = FALSE;
    Scr->DoZoom = FALSE;
    Scr->TitleFocus = TRUE;
    Scr->NoIconTitlebar = FALSE;
    Scr->NoTitlebar = FALSE;
    Scr->DecorateTransients = FALSE;
    Scr->IconifyByUnmapping = FALSE;
    Scr->ShowIconManager = FALSE;
    Scr->IconManagerDontShow =FALSE;
    Scr->BackingStore = TRUE;
    Scr->SaveUnder = TRUE;
    Scr->RandomPlacement = FALSE;
    Scr->DoOpaqueMove = FALSE;
    Scr->OpaqueMove = FALSE;
    Scr->OpaqueMoveThreshold = 1000;
    Scr->OpaqueResize = FALSE;
    Scr->DoOpaqueResize = FALSE;
    Scr->OpaqueResizeThreshold = 1000;
    Scr->Highlight = TRUE;
    Scr->StackMode = TRUE;
    Scr->TitleHighlight = TRUE;
    Scr->MoveDelta = 1;		/* so that f.deltastop will work */
    Scr->ZoomCount = 8;
    Scr->SortIconMgr = FALSE;
    Scr->Shadow = TRUE;
    Scr->InterpolateMenuColors = FALSE;
    Scr->NoIconManagers = FALSE;
    Scr->ClientBorderWidth = FALSE;
    Scr->SqueezeTitle = -1;
    Scr->FirstRegion = NULL;
    Scr->LastRegion = NULL;
    Scr->FirstTime = TRUE;
    Scr->HaveFonts = FALSE;		/* i.e. not loaded yet */
    Scr->CaseSensitive = TRUE;
    Scr->WarpUnmapped = FALSE;
    Scr->DeIconifyToScreen = FALSE;
    Scr->WarpWindows = FALSE;
    Scr->SnapRealScreen = FALSE;
    Scr->GeometriesAreVirtual = TRUE;
    Scr->HighlightDesktopFocus = TRUE;
    Scr->use3Diconmanagers = FALSE;
    Scr->use3Dmenus = FALSE;
    Scr->use3Dtitles = FALSE;
    Scr->SunkFocusWindowTitle = FALSE;
    Scr->ClearShadowContrast = 50;
    Scr->DarkShadowContrast  = 40;
    Scr->BeNiceToColormap = FALSE;

    /* setup default fonts; overridden by defaults from system.twmrc */
    Scr->DefaultFont.font = NULL;
    Scr->DefaultFont.name = "variable";
    Scr->TitleBarFont.font = NULL;
    Scr->TitleBarFont.name = NULL;
    Scr->MenuFont.font = NULL;
    Scr->MenuFont.name = NULL;
    Scr->IconFont.font = NULL;
    Scr->IconFont.name = NULL;
    Scr->SizeFont.font = NULL;
    Scr->SizeFont.name = NULL;
    Scr->IconManagerFont.font = NULL;
    Scr->IconManagerFont.name = NULL;
    Scr->VirtualFont.font = NULL;
    Scr->VirtualFont.name = NULL;
    Scr->DoorFont.font = NULL;
    Scr->DoorFont.name = NULL;
    Scr->InfoFont.font = NULL;
    Scr->InfoFont.name = NULL;

    /* no names unless they say so */
    Scr->NamesInVirtualDesktop = FALSE;

    /* by default we emulate the old twm - ie. no virtual desktop */
    Scr->Virtual = FALSE;

    /* this makes some of the algorithms for checking if windows
     * are on the screen simpler */
    Scr->VirtualDesktopWidth = Scr->MyDisplayWidth;
    Scr->VirtualDesktopHeight = Scr->MyDisplayHeight;

    /* start at the top left of the virtual desktop */
    Scr->VirtualDesktopX = 0;
    Scr->VirtualDesktopY = 0;

    /* pan defaults to half screen size */
    Scr->VirtualDesktopPanDistanceX = 50;
    Scr->VirtualDesktopPanDistanceY = 50;

    /* default scale is 1:25 */
    Scr->VirtualDesktopDScale = 25;

    /* and the display should appear at 0, 0 */
    Scr->VirtualDesktopDX = 0;
    Scr->VirtualDesktopDY = 0;

    /* by default no autopan */
    Scr->AutoPan = 0;
}


CreateFonts ()
{
    GetFont(&Scr->DefaultFont);
    GetFont(&Scr->TitleBarFont);
    GetFont(&Scr->MenuFont);
    GetFont(&Scr->IconFont);
    GetFont(&Scr->SizeFont);
    GetFont(&Scr->IconManagerFont);
    GetFont(&Scr->VirtualFont);
    GetFont(&Scr->DoorFont);
    GetFont(&Scr->InfoFont);
    Scr->HaveFonts = TRUE;
}


RestoreWithdrawnLocation (tmp)
    TwmWindow *tmp;
{
    int gravx, gravy;
    unsigned int bw, mask;
    XWindowChanges xwc;

    if (XGetGeometry (dpy, tmp->w, &JunkRoot, &xwc.x, &xwc.y,
		      &JunkWidth, &JunkHeight, &bw, &JunkDepth)) {

	GetGravityOffsets (tmp, &gravx, &gravy);
	if (gravy < 0) xwc.y -= tmp->title_height;

	if (bw != tmp->old_bw) {
	    int xoff, yoff;

	    if (!Scr->ClientBorderWidth) {
		xoff = gravx;
		yoff = gravy;
	    } else {
		xoff = 0;
		yoff = 0;
	    }

	    xwc.x -= (xoff + 1) * tmp->old_bw;
	    xwc.y -= (yoff + 1) * tmp->old_bw;
	}
	if (!Scr->ClientBorderWidth) {
	    xwc.x += gravx * tmp->frame_bw;
	    xwc.y += gravy * tmp->frame_bw;
	}

	mask = (CWX | CWY);
	if (bw != tmp->old_bw) {
	    xwc.border_width = tmp->old_bw;
	    mask |= CWBorderWidth;
	}

	XConfigureWindow (dpy, tmp->w, mask, &xwc);

	if (tmp->wmhints && (tmp->wmhints->flags & IconWindowHint)) {
	    XUnmapWindow (dpy, tmp->wmhints->icon_window);
	}

    }
}


/***********************************************************************
 *
 *  Procedure:
 *	Done - cleanup and exit twm
 *
 *  Returned Value:
 *	none
 *
 *  Inputs:
 *	none
 *
 *  Outputs:
 *	none
 *
 *  Special Considerations:
 *	none
 *
 ***********************************************************************
 */

void Reborder (time)
Time time;
{
    TwmWindow *tmp;			/* temp twm window structure */
    int scrnum;

    /* put a border back around all windows */

    XGrabServer (dpy);
    for (scrnum = 0; scrnum < NumScreens; scrnum++)
    {
	if ((Scr = ScreenList[scrnum]) == NULL)
	    continue;

	InstallWindowColormaps (0, &Scr->TwmRoot);	/* force reinstall */
	for (tmp = Scr->TwmRoot.next; tmp != NULL; tmp = tmp->next)
	{
	    RestoreWithdrawnLocation (tmp);
	    XMapWindow (dpy, tmp->w);
	}
    }

    XUngrabServer (dpy);
    SetFocus ((TwmWindow*)NULL, time);
}

SIGNAL_T Done()
{
    SetRealScreen(0,0);
    Reborder (CurrentTime);
    XCloseDisplay(dpy);
    exit(0);
}


SIGNAL_T Restart()
{
    XSync (dpy, 0);
    Reborder (CurrentTime);
    XSync (dpy, 0);
    execvp(*Argv, Argv);
    fprintf (stderr, "%s:  unable to restart:  %s\n", ProgramName, *Argv);
    exit (1);
}


/*
 * Error Handlers.  If a client dies, we'll get a BadWindow error (except for
 * GetGeometry which returns BadDrawable) for most operations that we do before
 * manipulating the client's window.
 */

Bool ErrorOccurred = False;
XErrorEvent LastErrorEvent;

static int TwmErrorHandler(dpy, event)
    Display *dpy;
    XErrorEvent *event;
{
    LastErrorEvent = *event;
    ErrorOccurred = True;

    if (PrintErrorMessages && 			/* don't be too obnoxious */
	event->error_code != BadWindow &&	/* watch for dead puppies */
	(event->request_code != X_GetGeometry &&	 /* of all styles */
	 event->error_code != BadDrawable))
      XmuPrintDefaultErrorMessage (dpy, event, stderr);
    return 0;
}


/* ARGSUSED*/
static int CatchRedirectError(dpy, event)
    Display *dpy;
    XErrorEvent *event;
{
    RedirectError = TRUE;
    LastErrorEvent = *event;
    ErrorOccurred = True;
    return 0;
}

Atom _XA_MIT_PRIORITY_COLORS;
Atom _XA_WM_CHANGE_STATE;
Atom _XA_WM_STATE;
Atom _XA_WM_COLORMAP_WINDOWS;
Atom _XA_WM_PROTOCOLS;
Atom _XA_WM_TAKE_FOCUS;
Atom _XA_WM_SAVE_YOURSELF;
Atom _XA_WM_DELETE_WINDOW;

InternUsefulAtoms ()
{
    /*
     * Create priority colors if necessary.
     */
    _XA_MIT_PRIORITY_COLORS = XInternAtom(dpy, "_MIT_PRIORITY_COLORS", False);
    _XA_WM_CHANGE_STATE = XInternAtom (dpy, "WM_CHANGE_STATE", False);
    _XA_WM_STATE = XInternAtom (dpy, "WM_STATE", False);
    _XA_WM_COLORMAP_WINDOWS = XInternAtom (dpy, "WM_COLORMAP_WINDOWS", False);
    _XA_WM_PROTOCOLS = XInternAtom (dpy, "WM_PROTOCOLS", False);
    _XA_WM_TAKE_FOCUS = XInternAtom (dpy, "WM_TAKE_FOCUS", False);
    _XA_WM_SAVE_YOURSELF = XInternAtom (dpy, "WM_SAVE_YOURSELF", False);
    _XA_WM_DELETE_WINDOW = XInternAtom (dpy, "WM_DELETE_WINDOW", False);
}
