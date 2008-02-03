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
 * vtwm - Virtually "Tom's Window Manager"
 *
 * 27-Oct-87 Thomas E. LaStrange	File created
 * 10-Oct-90 David M. Sternlicht        Storing saved colors on root
 ***********************************************************************/

#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h> /* for sleep() */
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
/* djhjr - 6/22/01 */
#ifndef NO_SOUND_SUPPORT
#include "sound.h"
#endif
#include <X11/Xresource.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/Xmu/Error.h>
#include <X11/Xlocale.h>

Display *dpy;			/* which display are we talking to */
Window ResizeWindow;		/* the window we are resizing */

int MultiScreen = TRUE;		/* try for more than one screen? */
int NumScreens;			/* number of screens in ScreenList */
int NumButtons;			/* number of mouse buttons */
int HasShape;			/* server supports shape extension? */
int ShapeEventBase, ShapeErrorBase;
ScreenInfo **ScreenList;	/* structures for each screen */
ScreenInfo *Scr = NULL;		/* the cur and prev screens */
int PreviousScreen;		/* last screen that we were on */
int FirstScreen;		/* TRUE ==> first screen of display */
Bool PrintPID = False;		/* controls PID file - djhjr - 12/2/01 */
Bool PrintErrorMessages = False;	/* controls error messages */
static int RedirectError;	/* TRUE ==> another window manager running */
static int CatchRedirectError();	/* for settting RedirectError */
static int TwmErrorHandler();	/* for everything else */
void InitVariables();
void InternUsefulAtoms();

char Info[INFO_LINES][INFO_SIZE];		/* info strings to print */
int InfoLines;

char *InitFile = NULL;
int parseInitFile = TRUE;	/* djhjr - 10/7/02 */

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

char *ProgramName, *PidName = "vtwm.pid"; /* PID file - djhjr - 12/2/01 */
int Argc;
char **Argv;
char **Environ;

Bool RestartPreviousState = False;	/* try to restart in previous state */

unsigned long black, white;

Bool use_fontset;

extern void assign_var_savecolor();

/* djhjr - 4/26/99 */
extern void FreeRegions();

/***********************************************************************
 *
 *  Procedure:
 *	main - start of twm
 *
 ***********************************************************************
 */

/* Changes for m4 pre-processing submitted by Jason Gloudon */
int
main(argc, argv, environ)
    int argc;
    char **argv;
    char **environ;
{
    Window root, parent, *children;
    unsigned int nchildren;
    int i, j;
    char *def, *display_name = NULL;
    unsigned long valuemask;	/* mask for create windows */
    XSetWindowAttributes attributes;	/* attributes for create windows */
    int numManaged, firstscrn, lastscrn, scrnum;
    extern ColormapWindow *CreateColormapWindow();
#ifndef NO_M4_SUPPORT
    int m4_preprocess = False;	/* filter the *twmrc file through m4 */
    char *m4_option = NULL; /* pass these options to m4 - djhjr - 2/20/98 */
#endif
/* djhjr - 6/22/01 */
#ifndef NO_SOUND_SUPPORT
    int sound_state = 0;
#endif
    extern char *defTwmrc[];	/* djhjr - 10/7/02 */
    char *loc;

    /* djhjr - 7/21/98 */
    SIGNAL_T QueueRestartVtwm();
    
	if ((ProgramName = strrchr(argv[0], '/')))
		ProgramName++;
	else
		ProgramName = argv[0];
    Argc = argc;
    Argv = argv;
    Environ = environ;

    for (i = 1; i < argc; i++) {
	if (argv[i][0] == '-') {
	    switch (argv[i][1]) {
	      case 'd':				/* -display display */
		if (++i >= argc) goto usage;
		display_name = argv[i];
		continue;
	      case 'f':				/* -file [initfile] */
		/* this isn't really right, but hey... - djhjr - 10/7/02 */
		if (i + 1 < argc &&
				(argv[i + 1][0] != '-' ||
				(argv[i + 1][0] == '-' && !strchr("dfmpsv", argv[i + 1][1]))))
			InitFile = argv[++i];
		else
			parseInitFile = FALSE;
		continue;
#ifndef NO_M4_SUPPORT
	      case 'm':				/* -m4 [options] */
		m4_preprocess = True;
		/* this isn't really right, but hey... - djhjr - 2/20/98 */
		if (i + 1 < argc &&
				(argv[i + 1][0] != '-' ||
				(argv[i + 1][0] == '-' && !strchr("dfmpsv", argv[i + 1][1]))))
			m4_option = argv[++i];
		continue;
#endif
	      case 'p':				/* -pidfile - djhjr - 12/2/01 */
		PrintPID = True;
		continue;
	      case 's':				/* -single */
		MultiScreen = FALSE;
		continue;
	      case 'v':				/* -verbose */
		PrintErrorMessages = True;
		continue;
#ifdef NEVER /* djhjr - 2/20/99 */
	      case 'q':				/* -quiet */
		PrintErrorMessages = False;
		continue;
#endif
	    }
	}
      usage:
	fprintf (stderr,
#ifndef NO_M4_SUPPORT
		 "usage:  %s [-d display] [-f [initfile]] [-m [options]] [-p] [-s] [-v]\n",
#else
		 "usage:  %s [-d display] [-f [initfile]] [-p] [-s] [-v]\n",
#endif
		 ProgramName);
	exit (1);
    }

    loc = setlocale(LC_ALL, "");
    if (!loc || !strcmp(loc, "C") || !strcmp(loc, "POSIX") ||
		!XSupportsLocale())
	use_fontset = False;
    else
	use_fontset = True;

    if (PrintErrorMessages)
	fprintf(stderr, "%s: L10N %sabled.\n",
		ProgramName, (use_fontset) ? "en" : "dis");

/* djhjr - 6/22/01 */
#ifndef NO_SOUND_SUPPORT
#define sounddonehandler(sig) \
    if (signal (sig, SIG_IGN) != SIG_IGN) (void) signal (sig, PlaySoundDone)
#else
#define sounddonehandler(sig) \
    if (signal (sig, SIG_IGN) != SIG_IGN) (void) signal (sig, Done)
#endif
#define donehandler(sig) \
    if (signal (sig, SIG_IGN) != SIG_IGN) (void) signal (sig, Done)

    sounddonehandler (SIGINT);
    sounddonehandler (SIGHUP);
    sounddonehandler (SIGQUIT);
    sounddonehandler (SIGTERM);

    /* djhjr - 12/2/01 */
    donehandler (SIGABRT);
    donehandler (SIGFPE);
    donehandler (SIGSEGV);
    donehandler (SIGILL);
    donehandler (SIGTSTP);
    donehandler (SIGPIPE);
#undef sounddonehandler
#undef donehandler

    /* djhjr - 7/31/98 */
    signal (SIGUSR1, QueueRestartVtwm);

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
    {
      unsigned char pmap[256];		/* there are 8 bits of buttons */
      NumButtons = XGetPointerMapping(dpy, pmap, 256);
    }

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
        Scr->Mouse = calloc((NumButtons+1)*NUM_CONTEXTS*MOD_SIZE,sizeof(MouseButton));
	if (!Scr->Mouse)
	{
	  fprintf (stderr, "%s: Unable to allocate memory for mouse buttons, exiting.\n",
		   ProgramName);
	  exit (1);
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

	/* submitted by Jonathan Paisley - 10/27/02 */
	Scr->NoWindowRingL = NULL;

	Scr->WarpCursorL = NULL;

	/* djhjr - 4/22/96 */
	Scr->ImageCache = NULL;

	/* djhjr - 4/7/98 */
	Scr->OpaqueMoveL = NULL;
	Scr->NoOpaqueMoveL = NULL;
	Scr->OpaqueResizeL = NULL;
	Scr->NoOpaqueResizeL = NULL;

	/* djhjr - 5/2/98 */
	Scr->NoBorder = NULL;

	/* djhjr - 9/24/02 */
	Scr->UsePPositionL = NULL;

	/* remember to put an initialization in InitVariables also
	 */

	Scr->screen = scrnum;
	Scr->d_depth = DefaultDepth(dpy, scrnum);
	Scr->d_visual = DefaultVisual(dpy, scrnum);
	Scr->Root = RootWindow(dpy, scrnum);
	XSaveContext (dpy, Scr->Root, ScreenContext, (caddr_t) Scr);

	/* djhjr - 1/31/99 */
	if ((def = XGetDefault(dpy, "*", "bitmapFilePath")))
		Scr->BitmapFilePath = strdup(def);
	else
		Scr->BitmapFilePath = NULL;

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

/* djhjr - 4/19/96
	Scr->TBInfo.border = 1;
*/
	Scr->TBInfo.border = -100;

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

	Scr->iconmgr.x = 0;
	Scr->iconmgr.y = 0;
	Scr->iconmgr.width = 150;
	Scr->iconmgr.height = 5;
	Scr->iconmgr.next = NULL;
	Scr->iconmgr.prev = NULL;
	Scr->iconmgr.lasti = &(Scr->iconmgr);
	Scr->iconmgr.first = NULL;
	Scr->iconmgr.last = NULL;
	Scr->iconmgr.active = NULL;
	Scr->iconmgr.scr = Scr;
	Scr->iconmgr.columns = 1;
	Scr->iconmgr.count = 0;
	Scr->iconmgr.name = "VTWM";
	Scr->iconmgr.icon_name = "Icons";

	Scr->IconDirectory = NULL;

	/* djhjr - 10/30/02 */
	Scr->hiliteName = NULL;
	Scr->menuIconName = TBPM_MENU;
	Scr->iconMgrIconName = TBPM_XLOGO;

/* djhjr - 10/30/02
	Scr->siconifyPm = NULL;
	Scr->pullPm = NULL;
*/

/* djhjr - 5/17/98 */
#ifdef ORIGINAL_PIXMAPS
	Scr->hilitePm = None;
	Scr->virtualPm = None; /* RFB PIXMAP */
	Scr->RealScreenPm = None; /* RFB PIXMAP */
#else /* ORIGINAL_PIXMAPS */
	/* djhjr - 10/25/02 */
	Scr->hiliteName = NULL;

	Scr->hilitePm = NULL;
	Scr->virtualPm = NULL;
	Scr->realscreenPm = NULL;
#endif /* ORIGINAL_PIXMAPS */

	if ( Scr->FirstTime )
	{	/* retain max size on restart. */
		Scr->VirtualDesktopMaxWidth = 0;
		Scr->VirtualDesktopMaxHeight = 0;
	}

	InitVariables();
	InitMenus();

	/* added this 'if (...) else' - djhjr - 10/7/02 */
	if (!parseInitFile)
		ParseStringList(defTwmrc);
	else
	{
		/* Parse it once for each screen. */
#ifndef NO_M4_SUPPORT
		/* added 'm4_option' - djhjr - 2/20/99 */
		ParseTwmrc(InitFile, display_name, m4_preprocess, m4_option);
#else
		ParseTwmrc(InitFile);
#endif
	}

/* djhjr - 6/22/01 */
#ifndef NO_SOUND_SUPPORT
	OpenSound();

	if (PlaySound(S_START))
	{
		/*
		 * Save setting from resource file, and turn sound off
		 */
		sound_state = ToggleSounds();
		sound_state ^= 1;
		if (sound_state == 0) ToggleSounds();
	}
#endif

	assign_var_savecolor(); /* storeing pixels for twmrc "entities" */

/* djhjr - 10/17/02 */
#if 0
	/* djhjr - 4/19/96 */
	/* was 'Scr->use3Dtitles' - djhjr - 8/11/98 */
	if (Scr->TitleBevelWidth > 0) {
/* djhjr - 10/17/02
	    if (Scr->TBInfo.border == -100) Scr->TBInfo.border = 0;
*/

/* djhjr - 3/12/97
	    if (Scr->ButtonIndent  == -100) Scr->ButtonIndent  = 0;
	    if (Scr->FramePadding  == -100) Scr->FramePadding  = 0;
	    if (Scr->TitlePadding  == -100) Scr->TitlePadding  = 0;
*/
	    Scr->ButtonIndent  = 0;
	    Scr->FramePadding  = 0;
	    Scr->TitlePadding  = 0;

/* djhjr - 4/3/98
		* djhjr - 4/26/96 *
		if (Scr->SunkFocusWindowTitle) Scr->TitleHighlight = FALSE;
*/
	}
	else {
		/* djhjr - 4/5/98 */
		Scr->SunkFocusWindowTitle = FALSE;
	}
#endif

	/* was only if Scr->TitleBevelWidth == 0 - djhjr - 10/17/02 */
	if (Scr->FramePadding  == -100) Scr->FramePadding  = 2; /* values that look */
	if (Scr->TitlePadding  == -100) Scr->TitlePadding  = 8; /* "nice" on */
	if (Scr->ButtonIndent  == -100) Scr->ButtonIndent  = 1; /* 75 and 100dpi displays */
	if (Scr->TBInfo.border == -100) Scr->TBInfo.border = 1;

	/* was 'Scr->use3D*' - djhjr - 8/11/98 */
	/* rem'd 'Scr->*BevelWidth > 0' - djhjr - 10/30/02 */
	if (/*Scr->TitleBevelWidth > 0 && */!Scr->BeNiceToColormap) GetShadeColors (&Scr->TitleC);
	if (/*Scr->MenuBevelWidth > 0 && */!Scr->BeNiceToColormap) GetShadeColors (&Scr->MenuC);
	if (Scr->MenuBevelWidth > 0 && !Scr->BeNiceToColormap) GetShadeColors (&Scr->MenuTitleC);
	if (Scr->BorderBevelWidth > 0 && !Scr->BeNiceToColormap) GetShadeColors (&Scr->BorderColorC);

	/* djhjr - 2/7/99 */
	if (Scr->DoorBevelWidth > 0 && !Scr->BeNiceToColormap) GetShadeColors (&Scr->DoorC);
	if (Scr->VirtualDesktopBevelWidth > 0 && !Scr->BeNiceToColormap) GetShadeColors (&Scr->VirtualC);

/* djhjr - 8/11/98
	* was 'Scr->use3Dborders' - djhjr - 8/11/98 *
	if (Scr->BorderBevelWidth == 0)
		Scr->ThreeDBorderWidth = 0;
	else
*/
	{
/* djhjr - 8/11/98
		* djhjr - 4/29/98 *
		if (2 * Scr->BorderBevelWidth > Scr->ThreeDBorderWidth)
			Scr->ThreeDBorderWidth = 2 * Scr->BorderBevelWidth;
*/
		if (2 * Scr->BorderBevelWidth > Scr->BorderWidth)
			Scr->BorderWidth = 2 * Scr->BorderBevelWidth;

    	if (!Scr->BeNiceToColormap)
			GetShadeColors(&Scr->DefaultC);
	}

	/* djhjr - 5/5/98 */
	/* was 'Scr->use3Dicons' - djhjr - 8/11/98 */
	if (Scr->IconBevelWidth > 0)
		Scr->IconBorderWidth = 0;

	if (Scr->SqueezeTitle == -1) Scr->SqueezeTitle = FALSE;
	if (!Scr->HaveFonts) CreateFonts();
	CreateGCs();
	MakeMenus();

/* djhjr - 10/18/02 */
#if 0
	/*
	 * Set titlebar height from font height and padding,
	 * then adjust to titlebutton height - djhjr - 12/10/98
	 */
	Scr->TitleBarFont.y += Scr->FramePadding;
	i = Scr->TitleBarFont.height;
	do
	{
		Scr->TitleBarFont.y += (i - Scr->TitleBarFont.height) / 2;
		Scr->TitleHeight = i + Scr->FramePadding * 2;

/* djhjr - 4/29/98
		* djhjr - 4/19/96 *
		if (Scr->use3Dtitles) Scr->TitleHeight += 4;
*/
/* djhjr - 10/18/02
		* was 'Scr->use3Dtitles' - djhjr - 8/11/98 *
		if (Scr->TitleBevelWidth > 0)
			Scr->TitleHeight += 2 * Scr->TitleBevelWidth + 2;
*/

		/* make title height be odd so buttons look nice and centered */
		if (!(Scr->TitleHeight & 1)) Scr->TitleHeight++;
	} while ((i = InitTitlebarButtons()) > Scr->TitleHeight - Scr->FramePadding * 2);
#else
	/* set titlebar height to font height plus frame padding */
	Scr->TitleHeight = Scr->TitleBarFont.height + Scr->FramePadding * 2;
	if (!(Scr->TitleHeight & 1)) Scr->TitleHeight++;

	i = InitTitlebarButtons();	/* returns the button height */
	
 	/* adjust titlebar height to button height */
	if (i > Scr->TitleHeight) Scr->TitleHeight = i + Scr->FramePadding * 2;
	if (!(Scr->TitleHeight & 1)) Scr->TitleHeight++;

	/* adjust font baseline */
	Scr->TitleBarFont.y += ((Scr->TitleHeight - Scr->TitleBarFont.height) / 2);
#endif

	XGrabServer(dpy);
	XSync(dpy, 0);

	JunkX = 0;
	JunkY = 0;

	XQueryTree(dpy, Scr->Root, &root, &parent, &children, &nchildren);
	CreateIconManagers();
	if (!Scr->NoIconManagers)
	    Scr->iconmgr.twm_win->icon = TRUE;

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
	    Scr->iconmgr.twm_win->icon = FALSE;
	    if (Scr->iconmgr.count)
	    {
		SetMapStateProp (Scr->iconmgr.twm_win, NormalState);
		XMapWindow(dpy, Scr->iconmgr.w);
		XMapWindow(dpy, Scr->iconmgr.twm_win->frame);
	    }
	}

	/* djhjr - 5/9/96 */
	/* was 'Scr->use3Dborders' - djhjr - 8/11/98 */
	if (!Scr->BorderBevelWidth > 0)
		attributes.border_pixel = Scr->DefaultC.fore;

	attributes.background_pixel = Scr->DefaultC.back;
	attributes.event_mask = (ExposureMask | ButtonPressMask |
				 KeyPressMask | ButtonReleaseMask);
	attributes.backing_store = NotUseful;

#ifdef ORIGINAL_INFOCURSOR
	attributes.cursor = XCreateFontCursor (dpy, Scr->WaitCursor);

	/* djhjr - 5/9/96 */
	/* was 'Scr->use3Dborders' - djhjr - 8/11/98 */
	if (!Scr->BorderBevelWidth > 0)
		valuemask = (CWBorderPixel | CWBackPixel | CWEventMask |
			     CWBackingStore | CWCursor);
	else
		valuemask = (CWBackPixel | CWEventMask | CWBackingStore | CWCursor);
#else

	/* djhjr - 5/9/96 */
	/* was 'Scr->use3Dborders' - djhjr - 8/11/98 */
	if (!Scr->BorderBevelWidth > 0)
		valuemask = (CWBorderPixel | CWBackPixel | CWEventMask |
			     CWBackingStore);
	else
		valuemask = (CWBackPixel | CWEventMask | CWBackingStore);
#endif

	Scr->InfoWindow = XCreateWindow (dpy, Scr->Root, 0, 0,
					 (unsigned int) 5, (unsigned int) 5,

					 /* djhjr - 5/9/96 */
					 /* was 'Scr->use3Dborders' - djhjr - 8/11/98 */
					 (unsigned int) (Scr->InfoBevelWidth > 0) ? 0 : BW, 0,

					 (unsigned int) CopyFromParent,
					 (Visual *) CopyFromParent,
					 valuemask, &attributes);

	Scr->SizeStringWidth = MyFont_TextWidth (&Scr->SizeFont,
/* djhjr - 5/9/96
					   " 8888 x 8888 ", 13);
*/
/* djhjr - 4/29/98
					   "nnnnnnnnnnnnn", 13);
*/
					   /* was 'Scr->use3Dborders' - djhjr - 8/11/98 */
					   "nnnnnnnnnnnnn", 13) + ((Scr->InfoBevelWidth > 0) ? 2 * Scr->InfoBevelWidth : 0);

	/* djhjr - 5/9/96 */
	/* was 'Scr->use3Dborders' - djhjr - 8/11/98 */
	if (!Scr->InfoBevelWidth > 0)
		valuemask = (CWBorderPixel | CWBackPixel | CWBitGravity);
	else
		valuemask = (CWBackPixel | CWBitGravity);

	/* djhjr - 5/15/96 */
	switch (Scr->ResizeX)
	{
		case R_NORTHWEST:
			Scr->ResizeX = 20;
			Scr->ResizeY = 20;
			break;
		case R_NORTHEAST:
			Scr->ResizeX = (Scr->MyDisplayWidth - Scr->SizeStringWidth) - 20;
			Scr->ResizeY = 20;
			break;
		case R_SOUTHWEST:
			Scr->ResizeX = 20;
			Scr->ResizeY = (Scr->MyDisplayHeight - (Scr->SizeFont.height + SIZE_VINDENT*2)) - 20;
			break;
		case R_SOUTHEAST:
			Scr->ResizeX = (Scr->MyDisplayWidth - Scr->SizeStringWidth) - 20;
			Scr->ResizeY = (Scr->MyDisplayHeight - (Scr->SizeFont.height + SIZE_VINDENT*2)) - 20;
			break;
		case R_CENTERED:
			Scr->ResizeX = (Scr->MyDisplayWidth - Scr->SizeStringWidth) / 2;
			Scr->ResizeY = (Scr->MyDisplayHeight - (Scr->SizeFont.height + SIZE_VINDENT*2)) / 2;
			break;
	}

	attributes.bit_gravity = NorthWestGravity;
	Scr->SizeWindow = XCreateWindow (dpy, Scr->Root,

/* djhjr - 5/15/96
					 0,0,
*/
					 Scr->ResizeX, Scr->ResizeY,

					 (unsigned int) Scr->SizeStringWidth,

/* djhjr - 4/29/98
					 (unsigned int) (Scr->SizeFont.height + SIZE_VINDENT*2),
*/
					 /* was 'Scr->use3Dborders' - djhjr - 8/11/98 */
					 (unsigned int) (Scr->SizeFont.height + SIZE_VINDENT*2) +
						((Scr->InfoBevelWidth > 0) ? 2 * Scr->InfoBevelWidth : 0),

					 /* djhjr - 5/9/96 */
					 /* was 'Scr->use3Dborders' - djhjr - 8/11/98 */
					 (unsigned int) (Scr->InfoBevelWidth > 0) ? 0 : BW, 0,

					 (unsigned int) CopyFromParent,
					 (Visual *) CopyFromParent,
					 valuemask, &attributes);

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

    RestartPreviousState = False;
    HandlingEvents = TRUE;

	RaiseStickyAbove(); /* DSE */    
    RaiseAutoPan(); /* autopan windows should have been raised
                       after [re]starting vtwm -- DSE */

    InitEvents();

	/* profile function stuff by DSE */
#define VTWM_PROFILE "VTWM Profile"
	if (FindMenuRoot (VTWM_PROFILE)) {
		ExecuteFunction (F_FUNCTION, VTWM_PROFILE, Event.xany.window,
			&Scr->TwmRoot, &Event, C_NO_CONTEXT, FALSE);
	}

/* djhjr - 6/22/01 */
#ifndef NO_SOUND_SUPPORT
	/* restore setting from resource file */
	if (sound_state == 1) ToggleSounds();
#endif

    
    /* write out a PID file - djhjr - 12/2/01 */
    if (PrintPID)
    {
	int fd, err = 0;
	char buf[10], *fn = malloc(HomeLen + strlen(PidName) + 2);

	/* removed group and other permissions - djhjr - 10/20/02 */
	sprintf(fn, "%s/%s", Home, PidName);
	if ((fd = open(fn,
		O_WRONLY|O_EXCL|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR)) != -1)
	{
	    sprintf(buf, "%d\n", getpid());
	    err = write(fd, buf, strlen(buf));
	    close(fd);
	}

	if (fd == -1 || err == -1)
	{
	    fprintf(stderr, "%s: cannot write to %s\n", ProgramName, fn);
	    DoAudible();
	}

	free(fn);
    }

    HandleEvents();

	return (0);
}

/***********************************************************************
 *
 *  Procedure:
 *	InitVariables - initialize twm variables
 *
 ***********************************************************************
 */

void InitVariables()
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

    /* submitted by Jonathan Paisley - 10/27/02 */
    FreeList(&Scr->NoWindowRingL);

    FreeList(&Scr->WarpCursorL);
    FreeList(&Scr->NailedDown);
    FreeList(&Scr->VirtualDesktopColorFL);
    FreeList(&Scr->VirtualDesktopColorBL);
    FreeList(&Scr->VirtualDesktopColorBoL);
    FreeList(&Scr->DontShowInDisplay);

	/* Submitted by Erik Agsjo <erik.agsjo@aktiedirekt.com> */
    FreeList(&Scr->DontShowInTWMWindows);

    FreeList(&Scr->DoorForegroundL);
    FreeList(&Scr->DoorBackgroundL);

	/* djhjr - 4/22/96 */
	FreeList(&Scr->ImageCache);

	/* djhjr - 4/7/98 */
	FreeList(&Scr->OpaqueMoveL);
	FreeList(&Scr->NoOpaqueMoveL);
	FreeList(&Scr->OpaqueResizeL);
	FreeList(&Scr->NoOpaqueResizeL);

	/* djhjr - 5/2/98 */
	FreeList(&Scr->NoBorder);

	/* djhjr - 9/24/02 */
	FreeList(&Scr->UsePPositionL);

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

	/* djhjr - 4/19/96 */
    Scr->FramePadding = -100;
    Scr->TitlePadding = -100;
    Scr->ButtonIndent = -100;

/* djhjr - 8/11/98
    Scr->ThreeDBorderWidth = 6;
*/

	/* djhjr - 5/15/96 */
	Scr->ResizeX = Scr->ResizeY = 0;

    Scr->VirtualC.fore = black;/*RFB VCOLOR*/
    Scr->VirtualC.back = white;/*RFB VCOLOR*/
	Scr->RealScreenC.back = black;/*RFB 4/92 */
	Scr->RealScreenC.fore = white;/*RFB 4/92 */
    Scr->VirtualDesktopDisplayC.fore = black;
    Scr->VirtualDesktopDisplayC.back = white;
    Scr->VirtualDesktopDisplayBorder = black;
    Scr->DoorC.fore = black;
    Scr->DoorC.back = white;

	Scr->AutoRaiseDefault = FALSE;/*RAISEDELAY*/
    Scr->FramePadding = 2;		/* values that look "nice" on */
    Scr->TitlePadding = 8;		/* 75 and 100dpi displays */
    Scr->ButtonIndent = 1;
    Scr->SizeStringOffset = 0;
    Scr->BorderWidth = BW;
    Scr->IconBorderWidth = BW;

/* djhjr - 8/13/98 */
#ifdef ORIGINAL_PIXMAPS
    Scr->UnknownWidth = 0;
    Scr->UnknownHeight = 0;
#else
    Scr->unknownName = NULL;
#endif

    Scr->NumAutoRaises = 0;
/*  Scr->NoDefaults = FALSE;  */
	Scr->NoDefaultMouseOrKeyboardBindings = FALSE; /* DSE */
	Scr->NoDefaultTitleButtons = FALSE; /* DSE */
    Scr->UsePPosition = PPOS_OFF;
    Scr->FocusRoot = TRUE;
    Scr->Newest = NULL; /* PF */
    Scr->Focus = NULL;

    /* djhjr - 9/10/03 */
    Scr->IgnoreModifiers = 0;

    /* djhjr - 10/16/02 */
    Scr->WarpCentered = WARPC_OFF;

    Scr->WarpCursor = FALSE;
    Scr->ForceIcon = FALSE;
    Scr->NoGrabServer = FALSE;
    Scr->NoRaiseMove = FALSE;
    Scr->NoRaiseResize = FALSE;
    Scr->NoRaiseDeicon = FALSE;
    Scr->NoRaiseWarp = FALSE;
    Scr->DontMoveOff = FALSE;
    Scr->DoZoom = FALSE;
    Scr->TitleFocus = TRUE;

    /* djhjr - 5/27/98 */
    Scr->IconManagerFocus = TRUE;

    /* djhjr - 12/14/98 */
    Scr->StaticIconPositions = FALSE;

    /* djhjr - 10/2/01 */
    Scr->StrictIconManager = FALSE;

    /* djhjr - 8/23/02 */
    Scr->NoBorders = FALSE;

    Scr->NoTitlebar = FALSE;
    Scr->DecorateTransients = FALSE;
    Scr->IconifyByUnmapping = FALSE;
    Scr->ShowIconManager = FALSE;
    Scr->IconManagerDontShow =FALSE;
    Scr->BackingStore = TRUE;
    Scr->SaveUnder = TRUE;
    Scr->RandomPlacement = FALSE;
    Scr->PointerPlacement = FALSE;
    Scr->OpaqueMove = FALSE;

	/* djhjr - 4/6/98 */
	Scr->OpaqueResize = FALSE;

    Scr->Highlight = TRUE;

	/* djhjr - 1/27/98 */
    Scr->IconMgrHighlight = TRUE;

    Scr->StackMode = TRUE;
    Scr->TitleHighlight = TRUE;
    Scr->MoveDelta = 1;		/* so that f.deltastop will work */
    Scr->ZoomCount = 8;
    Scr->SortIconMgr = FALSE;
    Scr->Shadow = TRUE;
    Scr->InterpolateMenuColors = FALSE;
    Scr->NoIconManagers = FALSE;
    Scr->NoIconifyIconManagers = FALSE; /* PF */
    Scr->ClientBorderWidth = FALSE;
    Scr->SqueezeTitle = -1;

/* djhjr - 4/26/99
    Scr->FirstIconRegion = NULL;
    Scr->LastIconRegion = NULL;
*/
    FreeRegions(Scr->FirstIconRegion, Scr->LastIconRegion);

	/* djhjr - 4/26/99 */
    FreeRegions(Scr->FirstAppletRegion, Scr->LastAppletRegion);

    Scr->FirstTime = TRUE;
    Scr->HaveFonts = FALSE;		/* i.e. not loaded yet */
    Scr->CaseSensitive = TRUE;
    Scr->WarpUnmapped = FALSE;
    Scr->DeIconifyToScreen = FALSE;
    Scr->WarpWindows = FALSE;
    Scr->WarpToTransients = FALSE; /* PF */

	/* djhjr - 6/25/96 */
    Scr->ShallowReliefWindowButton = 2;

/* obsoleted by the *BevelWidth resources - djhjr - 8/11/98
	* djhjr - 4/19/96 *
    Scr->use3Diconmanagers = FALSE;
    Scr->use3Dmenus = FALSE;
    Scr->use3Dtitles = FALSE;
    Scr->use3Dborders = FALSE;
*/

    Scr->ClearBevelContrast = 50;
    Scr->DarkBevelContrast  = 40;
    Scr->BeNiceToColormap = FALSE;

/* obsoleted by the *BevelWidth resources - djhjr - 8/11/98
	* djhjr - 5/5/98 *
    Scr->use3Dicons = FALSE;
*/

/* obsoleted by the ":xpm:*" built-in pixmaps - djhjr - 10/26/02
	* djhjr - 4/26/96 *
    Scr->SunkFocusWindowTitle = FALSE;
*/

	/* for rader - djhjr - 2/9/99 */
	Scr->NoPrettyTitles = FALSE;

	/* djhjr - 9/21/96 */
    Scr->ButtonColorIsFrame = FALSE;

    Scr->snapRealScreen = FALSE;
	Scr->OldFashionedTwmWindowsMenu = FALSE;
    Scr->GeometriesAreVirtual = TRUE;
	Scr->UseWindowRing = FALSE;

    /* setup default fonts; overridden by defaults from system.twmrc */
#define DEFAULT_NICE_FONT "variable"
#define DEFAULT_FAST_FONT "fixed"
#define DEFAULT_SMALL_FONT "5x8"

    Scr->TitleBarFont.font = NULL;
    Scr->TitleBarFont.name = DEFAULT_NICE_FONT;
    Scr->MenuFont.font = NULL;
    Scr->MenuFont.name = DEFAULT_NICE_FONT;
    Scr->MenuTitleFont.font = NULL; /* DSE */
    Scr->MenuTitleFont.name = NULL; /* uses MenuFont unless set -- DSE */
    Scr->IconFont.font = NULL;
    Scr->IconFont.name = DEFAULT_NICE_FONT;
    Scr->SizeFont.font = NULL;
    Scr->SizeFont.name = DEFAULT_FAST_FONT;

	/* djhjr - 5/10/96 */
    Scr->InfoFont.font = NULL;
    Scr->InfoFont.name = DEFAULT_FAST_FONT;

    Scr->IconManagerFont.font = NULL;
    Scr->IconManagerFont.name = DEFAULT_NICE_FONT;
    Scr->VirtualFont.font = NULL;
    Scr->VirtualFont.name = DEFAULT_SMALL_FONT;
    Scr->DoorFont.font = NULL;
    Scr->DoorFont.name = DEFAULT_NICE_FONT;
    Scr->DefaultFont.font = NULL;
    Scr->DefaultFont.name = DEFAULT_FAST_FONT;

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

	/* djhjr - 9/8/98 */
	Scr->VirtualDesktopPanResistance = 0;

    /* default scale is 1:25 */
    Scr->VirtualDesktopDScale = 25;

    /* and the display should appear at 0, 0 */
    Scr->VirtualDesktopDX = 0;
    Scr->VirtualDesktopDY = 0;

    /* by default no autopan */
    Scr->AutoPanX = 0;
    Scr->StayUpMenus = FALSE;
    Scr->StayUpOptionalMenus = FALSE; /* PF */

	Scr->AutoPanWarpWithRespectToRealScreen = 0;   /* DSE */
	Scr->AutoPanBorderWidth = 5;                   /* DSE */
	Scr->AutoPanExtraWarp = 2;                     /* DSE */
	Scr->EnhancedExecResources = FALSE;            /* DSE */
	Scr->RightHandSidePulldownMenus = FALSE;       /* DSE */

	/* was '2' for when UseRealScreenBorderWidth existed - djhjr - 2/15/99 */
	Scr->RealScreenBorderWidth = 0;                /* DSE */

	/* djhjr - 10/11/01 */
	Scr->ZoomZoom = FALSE;

	Scr->LessRandomZoomZoom = FALSE;               /* DSE */
	Scr->PrettyZoom = FALSE;                       /* DSE */
	Scr->StickyAbove = FALSE;                      /* DSE */
	Scr->DontInterpolateTitles = FALSE;            /* DSE */

	/* djhjr - 1/6/98 */
	Scr->FixManagedVirtualGeometries = FALSE;

	Scr->FixTransientVirtualGeometries = FALSE;    /* DSE */
	Scr->WarpSnug = FALSE;                         /* DSE */

	/* djhjr - 4/17/98 */
	Scr->VirtualReceivesMotionEvents = FALSE;
	Scr->VirtualSendsMotionEvents = FALSE;

	/* djhjr - 5/2/98 */
	Scr->BorderBevelWidth = 0;
	Scr->TitleBevelWidth = 0;
	Scr->MenuBevelWidth = 0;
	Scr->IconMgrBevelWidth = 0;
	Scr->InfoBevelWidth = 0;

	/* djhjr - 8/11/98 */
	Scr->IconBevelWidth = 0;
	Scr->ButtonBevelWidth = 0;

	/* djhjr - 2/7/99 */
	Scr->DoorBevelWidth = 0;
	Scr->VirtualDesktopBevelWidth = 0;

	/* djhjr - 5/22/00 */
	Scr->MenuScrollBorderWidth = 2;
	Scr->MenuScrollJump = 3;

	/* djhjr - 6/22/99 */
	Scr->DontDeiconifyTransients = FALSE;

	/* submitted by Ugen Antsilevitch - 5/28/00 */
	Scr->WarpVisible = FALSE;

	/* djhjr - 6/22/01 */
	Scr->PauseOnExit = 0;
	Scr->PauseOnQuit = 0;

	/* djhjr - 11/3/03 */
	Scr->RaiseOnStart = FALSE;
}


void CreateFonts ()
{
    GetFont(&Scr->TitleBarFont);
    GetFont(&Scr->MenuFont);
    GetFont(&Scr->IconFont);
    GetFont(&Scr->SizeFont);

	/* djhjr - 5/10/96 */
    GetFont(&Scr->InfoFont);

    GetFont(&Scr->IconManagerFont);
    GetFont(&Scr->VirtualFont);
    GetFont(&Scr->DoorFont);
    GetFont(&Scr->DefaultFont);
    GetFont(&Scr->MenuTitleFont); /* DSE */
    Scr->HaveFonts = TRUE;
}


void RestoreWithdrawnLocation (tmp)
    TwmWindow *tmp;
{
    int gravx, gravy;
    unsigned int bw, mask;
    XWindowChanges xwc;

    if (XGetGeometry (dpy, tmp->w, &JunkRoot, &xwc.x, &xwc.y,
		      &JunkWidth, &JunkHeight, &bw, &JunkDepth)) {

	GetGravityOffsets (tmp, &gravx, &gravy);
	if (gravy < 0) xwc.y -= tmp->title_height;

	/* djhjr - 4/19/96 */
	xwc.x += gravx * tmp->frame_bw3D;
	xwc.y += gravy * tmp->frame_bw3D;

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

/* delete the PID file - djhjr - 12/2/01 */
void delete_pidfile()
{
    char *fn;

    if (PrintPID)
    {    
	fn = malloc(HomeLen + strlen(PidName) + 2);
	sprintf(fn, "%s/%s", Home, PidName);
	unlink(fn);
	free(fn);
    }
}


/*
 * Exit handlers. Clean up and exit VTWM.
 *
 *    PlaySoundDone()
 *    Done()
 *    QueueRestartVtwm()
 */

/* djhjr - 6/22/01 */
#ifndef NO_SOUND_SUPPORT
SIGNAL_T PlaySoundDone()
{
    if (PlaySound(S_STOP))
    {
	/* allow time to emit */
	if (Scr->PauseOnExit) sleep(Scr->PauseOnExit);
    }

    Done();
    SIGNAL_RETURN;
}

void Done()
{
    CloseSound();

    SetRealScreen(0,0);
    Reborder (CurrentTime);
    XCloseDisplay(dpy);
    
    delete_pidfile(); /* djhjr - 12/2/01 */

    exit(0);
}
#else
SIGNAL_T Done()
{
    SetRealScreen(0,0);
    Reborder (CurrentTime);
    XCloseDisplay(dpy);
    
    delete_pidfile(); /* djhjr - 12/2/01 */

    exit(0);
    SIGNAL_RETURN;
}
#endif

/* djhjr - 7/31/98 */
SIGNAL_T
QueueRestartVtwm()
{
    XClientMessageEvent ev;

    delete_pidfile(); /* djhjr - 12/2/01 */

    ev.type = ClientMessage;
    ev.window = Scr->Root;
    ev.message_type = _XA_TWM_RESTART;
    ev.format = 32;
    ev.data.b[0] = (char)0;

    XSendEvent (dpy, Scr->VirtualDesktopDisplay, False, 0L, (XEvent *) &ev);
    XFlush(dpy);
    SIGNAL_RETURN;
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

/* djhjr - 7/31/98 */
Atom _XA_TWM_RESTART;

void InternUsefulAtoms ()
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

    /* djhjr - 7/31/98 */
    _XA_TWM_RESTART = XInternAtom (dpy, "_TWM_RESTART", False);
}
