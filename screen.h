/*
 * Copyright 1989 Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.  M.I.T. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * M.I.T. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL M.I.T.
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/***********************************************************************
 *
 * $XConsortium: screen.h,v 1.62 91/05/01 17:33:09 keith Exp $
 *
 * twm per-screen data include file
 *
 * 11-3-88 Dave Payne, Apple Computer			File created
 *
 ***********************************************************************/

#ifndef _SCREEN_
#define _SCREEN_

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include "list.h"
#include "menus.h"
/* djhjr - 5/17/98 */
#ifndef ORIGINAL_PIXMAPS
#include "util.h" /* for Image structure */
#endif
#include "iconmgr.h"
#include "doors.h"


typedef struct _StdCmap {
    struct _StdCmap *next;		/* next link in chain */
    Atom atom;				/* property from which this came */
    int nmaps;				/* number of maps below */
    XStandardColormap *maps;		/* the actual maps */
} StdCmap;

#define SIZE_HINDENT 10

#ifdef ORIGINAL_SIZEVINDENT
#define SIZE_VINDENT 2
#else
#define SIZE_VINDENT 5
#endif

typedef struct ScreenInfo
{
    int screen;			/* the default screen */
    int d_depth;		/* copy of DefaultDepth(dpy, screen) */
    Visual *d_visual;		/* copy of DefaultVisual(dpy, screen) */
    int Monochrome;		/* is the display monochrome ? */
    int MyDisplayWidth;		/* my copy of DisplayWidth(dpy, screen) */
    int MyDisplayHeight;	/* my copy of DisplayHeight(dpy, screen) */
    int MaxWindowWidth;		/* largest window to allow */
    int MaxWindowHeight;	/* ditto */

	/* djhjr - 5/15/96 */
	int ResizeX;		/* coordinate of resize/position window */
	int ResizeY;		/* ditto */

    int VirtualDesktopMaxWidth;		/* max width of virtual desktop */
    int VirtualDesktopMaxHeight;	/* max height of virtual desktop */
    int VirtualDesktopWidth;	/* width of virtual desktop */
    int VirtualDesktopHeight;	/* height of virtual desktop */
    int VirtualDesktopX;	/* top left x of my screen on the desktop */
    int VirtualDesktopY;	/* top left y of my screen on the desktop */
    int VirtualDesktopPanDistanceX; /* distance to pan screen */
    int VirtualDesktopPanDistanceY; /* distance to pan screen */

    /* these are for the little vd display */
    int VirtualDesktopDScale;	/* scale of the virtual desktop display */
    int VirtualDesktopDX;	/* position of the vd display */
    int VirtualDesktopDY;	/* position of the vd display */

    /* the autopan stuff */
    int AutoPanX;		/* how far should autopan travel */
		/* AutoPanX is also "whether autopan configured". */
    int AutoPanY;		/* how far should autopan travel */
    Window VirtualDesktopAutoPan[4]; /* the autopan windows */
    				/* 0 = left, 1 = right, 2 = top, 3 = bottom */

    /* djhjr - 9/8/98 */
    int VirtualDesktopPanResistance;	/* how much effort it takes to pan */

    TwmWindow TwmRoot;		/* the head of the twm window list */

    Window Root;		/* the root window */
    Window SizeWindow;		/* the resize dimensions window */
    Window InfoWindow;		/* the information window */
    Window VirtualDesktopDisplayOuter; /* wrapper for display of the virtual desktop */
    Window VirtualDesktopDisplay; /* display of the virtual desktop */
    Window VirtualDesktopDScreen; /* display of the real screen on the vd */
    TwmWindow *VirtualDesktopDisplayTwin; /* twm window for the above */

    name_list *ImageCache;  /* list of pixmaps */
    name_list *Icons;		/* list of icon pixmaps */

    int	pullW, pullH;		/* size of pull right menu icon */

/* djhjr - 5/17/98 */
/* added the unknowns - djhjr - 8/13/98 */
#ifdef ORIGINAL_PIXMAPS
    Pixmap UnknownPm;		/* the unknown icon pixmap */
    int UnknownWidth;		/* width of the unknown icon */
    int UnknownHeight;		/* height of the unknown icon */
    Pixmap hilitePm;		/* focus highlight window background */
    int hilite_pm_width, hilite_pm_height;  /* cache the size */
    Pixmap virtualPm;		/* panner background pixmap RFB PIXMAP */
    int virtual_pm_width, virtual_pm_height; /* RFB PIXMAP */
    Pixmap RealScreenPm;		/* panner background pixmap RFB PIXMAP */
    int RealScreen_pm_width, RealScreen_pm_height; /* RFB PIXMAP */
#else /* ORIGINAL_PIXMAPS */
    char *unknownName;		/* name of unknown icon pixmap */

    /* djhjr - 10/25/02 */
    char *hiliteName;		/* name of built-in focus highlight pixmap */
    /* two more - djhjr - 10/30/02 */
    char *iconMgrIconName;	/* name of built-in iconmgr iconify pixmap */
    char *menuIconName;		/* name of built-in pull right menu pixmap */

/* depreciated - djhjr - 10/30/02
    Image *siconifyPm;		* the icon manager iconify pixmap *
    Image *pullPm;		* pull right menu icon *
*/

    Image *hilitePm;		/* focus highlight window image structure */
    Image *virtualPm;		/* panner background window image structure */
    Image *realscreenPm;	/* real screen window image structure */
#endif /* ORIGINAL_PIXMAPS */

    MenuRoot *MenuList;		/* head of the menu list */
    MenuRoot *LastMenu;		/* the last menu (mostly unused?) */
    MenuRoot *Windows;		/* the TwmWindows menu */

    TwmWindow *Ring;		/* one of the windows in window ring */
    TwmWindow *RingLeader;	/* current winodw in ring */

    MouseButton *Mouse;         // [MAX_BUTTONS+1][NUM_CONTEXTS][MOD_SIZE];
#define MOUSELOC(but,con,siz) (but*NUM_CONTEXTS*MOD_SIZE+con*MOD_SIZE+siz)
    MouseButton DefaultFunction;
    MouseButton WindowFunction;

    struct {
      Colormaps *cmaps; 	/* current list of colormap windows */
      int maxCmaps;		/* maximum number of installed colormaps */
      unsigned long first_req;	/* seq # for first XInstallColormap() req in
				   pass thru loading a colortable list */
      int root_pushes;		/* current push level to install root
				   colormap windows */
      TwmWindow *pushed_window;	/* saved window to install when pushes drops
				   to zero */
    } cmapInfo;

    struct {
	StdCmap *head, *tail;		/* list of maps */
	StdCmap *mru;			/* most recently used in list */
	int mruindex;			/* index of mru in entry */
    } StdCmapInfo;

    struct {
	int nleft, nright;		/* numbers of buttons in list */
	TitleButton *head;		/* start of list */
	int border;			/* button border */
	int pad;			/* button-padding */
	int width;			/* width of single button & border */
	int leftx;			/* start of left buttons */
	int titlex;			/* start of title string */
	int rightoff;			/* offset back from right edge */
    } TBInfo;
    ColorPair BorderTileC;	/* border tile colors */
    ColorPair TitleC;		/* titlebar colors */
    ColorPair MenuC;		/* menu colors */
    ColorPair MenuTitleC;	/* menu title colors */
    ColorPair IconC;		/* icon colors */
    ColorPair IconManagerC;	/* icon manager colors */
    ColorPair DefaultC;		/* default colors */

    /* djhjr - 4/19/96 */
    ColorPair BorderColorC;	/* color of window borders */

    ColorPair VirtualDesktopDisplayC; /* desktop display color */
    ColorPair DoorC;		/* default door colors */
    ColorPair VirtualC;     /* default virtual colors *//*RFB VCOLOR*/
	ColorPair RealScreenC;	/* "real screen" in panner RFB 4/92 */
    Pixel VirtualDesktopDisplayBorder; /* desktop display default border */
    Pixel BorderColor;		/* color of window borders */
    Pixel MenuShadowColor;	/* menu shadow color */
    Pixel IconBorderColor;	/* icon border color */
    Pixel IconManagerHighlight;	/* icon manager highlight */

    /* djhjr - 4/19/96 */
    short ClearBevelContrast;  /* The contrast of the clear shadow */
    short DarkBevelContrast;   /* The contrast of the dark shadow */

    Cursor TitleCursor;		/* title bar cursor */
    Cursor FrameCursor;		/* frame cursor */
    Cursor IconCursor;		/* icon cursor */
    Cursor IconMgrCursor;	/* icon manager cursor */
    Cursor ButtonCursor;	/* title bar button cursor */
    Cursor MoveCursor;		/* move cursor */
    Cursor ResizeCursor;	/* resize cursor */
    Cursor WaitCursor;		/* wait a while cursor */
    Cursor MenuCursor;		/* menu cursor */
    Cursor SelectCursor;	/* dot cursor for f.move, etc. from menus */
    Cursor DestroyCursor;	/* skull and cross bones, f.destroy */
	Cursor DoorCursor;/*RFBCURSOR*/
	Cursor VirtualCursor;/*RFBCURSOR*/
	Cursor DesktopCursor;/*RFBCURSOR*/
    Cursor NoCursor;		/* a black cursor - used on desktop display */

    name_list *BorderColorL;
    name_list *IconBorderColorL;
    name_list *BorderTileForegroundL;
    name_list *BorderTileBackgroundL;
    name_list *TitleForegroundL;
    name_list *TitleBackgroundL;
    name_list *IconForegroundL;
    name_list *IconBackgroundL;
    name_list *IconManagerFL;
    name_list *IconManagerBL;
    name_list *IconMgrs;

	/* djhjr - 4/19/96 */
    name_list *NoBorder;	/* list of window without borders          */

	/* djhjr - 4/7/98 */
	name_list *OpaqueMoveL;		/* list of windows moved as a solid */
	name_list *NoOpaqueMoveL;	/* list of windows moved as an outline */
	name_list *OpaqueResizeL;	/* list of windows resized as a solid */
	name_list *NoOpaqueResizeL;	/* list of windows resized as an outline */

    name_list *NoTitle;		/* list of window names with no title bar */
    name_list *MakeTitle;	/* list of window names with title bar */
    name_list *AutoRaise;	/* list of window names to auto-raise */
    name_list *IconNames;	/* list of window names and icon names */
    name_list *NoHighlight;	/* list of windows to not highlight */
    name_list *NoStackModeL;	/* windows to ignore stack mode requests */
    name_list *NoTitleHighlight;/* list of windows to not highlight the TB*/
    name_list *DontIconify;	/* don't iconify by unmapping */
    name_list *IconMgrNoShow;	/* don't show in the icon manager */
    name_list *IconMgrShow;	/* show in the icon manager */
    name_list *IconifyByUn;	/* windows to iconify by unmapping */
    name_list *StartIconified;	/* windows to start iconic */
    name_list *IconManagerHighlightL;	/* icon manager highlight colors */
    name_list *SqueezeTitleL;		/* windows of which to squeeze title */
    name_list *DontSqueezeTitleL;	/* windows of which not to squeeze */
    name_list *WindowRingL;	/* windows in ring */

    /* submitted by Jonathan Paisley - 10/27/02 */
    name_list *NoWindowRingL;	/* windows not added to ring */

    name_list *WarpCursorL;	/* windows to warp cursor to on deiconify */
    name_list *NailedDown;      /* windows that are nailed down */
    name_list *VirtualDesktopColorFL;  /* color of representations on the vd display */
    name_list *VirtualDesktopColorBL;  /* color of representations on the vd display */
    name_list *VirtualDesktopColorBoL; /* color of representations on the vd display */
    name_list *DontShowInDisplay;      /* don't show these in the desktop display */

	/* Submitted by Erik Agsjo <erik.agsjo@aktiedirekt.com> */
    name_list *DontShowInTWMWindows;   /* don't show these in the TWMWindows menu */

    name_list *DoorForegroundL; /* doors foreground */
    name_list *DoorBackgroundL; /* doors background */

    /* djhjr - 9/24/02 */
    name_list *UsePPositionL;	/* windows with UsePPosition set */

    GC NormalGC;		/* normal GC for everything */
    /* sjr - 9/25/06 */
    GC RootGC;			/* root GC for title buttons */
    GC MenuGC;			/* gc for menus */
    GC DrawGC;			/* GC to draw lines for move and resize */

    /* djhjr - 4/19/96 */
    GC GreyGC;			/* for shadowing on monochrome displays */
    GC ShadGC;			/* for shadowing on with patterns */

    unsigned long Black;
    unsigned long White;
    unsigned long XORvalue;	/* number to use when drawing xor'ed */
    MyFont TitleBarFont;	/* title bar font structure */
    MyFont MenuFont;		/* menu font structure */
    MyFont IconFont;		/* icon font structure */
    MyFont SizeFont;		/* resize font structure */
    MyFont IconManagerFont;	/* window list font structure */
    MyFont VirtualFont;		/* virtual display windows */
    MyFont DoorFont;		/* for drawing in doors */
    MyFont MenuTitleFont;   /* DSE -- for menu titles */
    MyFont InfoFont;        /* for the info window */
    MyFont DefaultFont;
    IconMgr iconmgr;		/* default icon manager */
    struct RootRegion *FirstIconRegion;	/* pointer to icon regions */
    struct RootRegion *LastIconRegion;	/* pointer to the last icon region */
    char *IconDirectory;	/* icon directory to search */

	/* djhjr - 4/26/99 */
	struct RootRegion *FirstAppletRegion;	/* pointer to applet regions */
	struct RootRegion *LastAppletRegion;	/* pointer to the last applet region */

	/* djhjr - 12/26/98 */
	char *BitmapFilePath;	/* local copy of the X database resource */

    int SizeStringOffset;	/* x offset in size window for drawing */
    int SizeStringWidth;	/* minimum width of size window */
    int BorderWidth;		/* border width of twm windows */

/* djhjr - 8/11/98
    * djhjr - 4/18/96 *
    int ThreeDBorderWidth;	* 3D border width of twm windows *
*/

    /* widths of the various 3D shadows - djhjr - 5/2/98 */
    int BorderBevelWidth;
    int TitleBevelWidth;
    int MenuBevelWidth;
    int IconMgrBevelWidth;
    int InfoBevelWidth;

    /* djhjr - 8/11/98 */
    int IconBevelWidth;
    int ButtonBevelWidth;

    /* djhjr - 2/7/99 */
    int DoorBevelWidth;
    int VirtualDesktopBevelWidth;

    /* djhjr - 5/22/00 */
    int MenuScrollBorderWidth;	/* top and bottom margins for menu scrolling */
    int MenuScrollJump;		/* number of entries for menu scroll */

    int IconBorderWidth;	/* border width of icon windows */
    int TitleHeight;		/* height of the title bar window */
    TwmWindow *Focus;		/* the twm window that has focus */
    TwmWindow *Newest;		/* the most newly added twm window -- PF */
    int EntryHeight;		/* menu entry height */
    int FramePadding;		/* distance between decorations and border */
    int TitlePadding;		/* distance between items in titlebar */
    int ButtonIndent;		/* amount to shrink buttons on each side */
    int NumAutoRaises;		/* number of autoraise windows on screen */

    short SqueezeTitle;		/* make title as small as possible */
    short MoveDelta;		/* number of pixels before f.move starts */
    short ZoomCount;		/* zoom outline count */

    /* djhjr - 6/22/01 */
    int PauseOnExit;		/* delay before shutting down via Done() */
    int PauseOnQuit;		/* delay before shuttind down via f.quit */

/* djhjr - 5/17/96 */
#ifdef ORIGINAL_SHORTS
	/* short NoDefaults; - DSE */
	short NoDefaultMouseOrKeyboardBindings; /* do not add default UI mouse and keyboard stuff - DSE */
	short NoDefaultTitleButtons; /* do not add default resize and iconify title buttons - DSE */
    short UsePPosition;		/* what do with PPosition, see values below */
    short OldFashionedTwmWindowsMenu;

/* djhjr - 2/15/99
	short UseRealScreenBorder;
*/

    short AutoRelativeResize;	/* start resize relative to position in quad */
    short FocusRoot;		/* is the input focus on the root ? */

    /* djhjr - 10/16/02 */
    short WarpCentered;		/* warp to center of windows? */

    short WarpCursor;		/* warp cursor on de-iconify? */
    short ForceIcon;		/* force the icon to the user specified */
    short NoGrabServer;		/* don't do server grabs */
    short NoRaiseMove;		/* don't raise window following move */
    short NoRaiseResize;	/* don't raise window following resize */
    short NoRaiseDeicon;	/* don't raise window on deiconify */
    short NoRaiseWarp;		/* don't raise window on warp */
    short DontMoveOff;		/* don't allow windows to be moved off */
    short DoZoom;		/* zoom in and out of icons */
    short TitleFocus;		/* focus on window in title bar ? */

    /* djhjr - 5/27/98 */
    short IconManagerFocus;	/* focus on window of the icon manager entry? */

    /* djhjr - 12/14/98 */
    short StaticIconPositions;	/* non-nailed icons stay put */

    /* djhjr - 10/2/01 */
    short StrictIconManager;	/* show only the iconified */

    /* djhjr - 8/23/02 */
    short NoBorders;		/* put borders on windows */

    short NoTitlebar;		/* put title bars on windows */
    short DecorateTransients;	/* put title bars on transients */
    short IconifyByUnmapping;	/* simply unmap windows when iconifying */
    short ShowIconManager;	/* display the window list */
    short IconManagerDontShow;	/* show nothing in the icon manager */
    short NoIconifyIconManagers; /* don't iconify the icon manager -- PF */
    short BackingStore;		/* use backing store for menus */
    short SaveUnder;		/* use save under's for menus */
    short RandomPlacement;	/* randomly place windows that no give hints */
    short PointerPlacement;	/* place near mouse pointer */
    short OpaqueMove;		/* move the window rather than outline */

	/* djhjr - 4/6/98 */
	short OpaqueResize;		/* resize the window rather than outline */

    short Highlight;		/* should we highlight the window borders */

    /* djhjr - 1/27/98 */
    short IconMgrHighlight;	/* should we highlight icon manager entries */

    short StackMode;		/* should we honor stack mode requests */
    short TitleHighlight;	/* should we highlight the titlebar */
    short SortIconMgr;		/* sort entries in the icon manager */
    short Shadow;		/* show the menu shadow */
    short InterpolateMenuColors;/* make pretty menus */
    short NoIconManagers;	/* Don't create any icon managers */
    short ClientBorderWidth;	/* respect client window border width */
    short HaveFonts;		/* set if fonts have been loaded */
    short FirstTime;		/* first time we've read .twmrc */
    short CaseSensitive;	/* be case-sensitive when sorting names */
    short WarpUnmapped;		/* allow warping to unmapped windows */
    short DeIconifyToScreen;	/* if deiconified, should this goto the screen ? */
    short WarpWindows;		/* should windows or the screen be warped ? */
    short snapRealScreen;       /* should the real screen snap to a pandistance grid ? */
    short GeometriesAreVirtual; /* should geometries be interpreted as virtual or real ? */
    short Virtual;		/* are we virtual ? (like, hey man....) */
    short NamesInVirtualDesktop;/* show names in virtual desktop display ? */
    short AutoRaiseDefault;   /* AutoRaise all windows if true *//*RAISEDELAY*/
	short UseWindowRing;   /* put all windows in the ring? */
	short StayUpMenus;
    short StayUpOptionalMenus; /* PF */
    short WarpToTransients;    /* PF */
	short EnhancedExecResources;      /* instead of normal behavior - DSE */
	short RightHandSidePulldownMenus; /* instead of left-right center - DSE */
	short LessRandomZoomZoom;         /* makes zoomzoom a better visual bell - DSE */
	short PrettyZoom;                 /* nicer-looking animation - DSE */
	short StickyAbove;                /* sticky windows above other windows - DSE */
	short DontInterpolateTitles;      /* menu titles are excluded from color interpolation - DSE */

	/* djhjr - 1/6/98 */
	short FixManagedVirtualGeometries; /* bug workaround */

	short FixTransientVirtualGeometries; /* bug workaround - DSE */
	short WarpSnug;                   /* make sure entire window is on screen when warping - DSE */

	/* djhjr - 6/25/96 */
	short	ShallowReliefWindowButton;

/* obsoleted by the *BevelWidth resources - djhjr - 8/11/98
    * djhjr - 4/18/96 *
    short 	use3Dmenus;
    short 	use3Dtitles;
    short 	use3Diconmanagers;
    short 	use3Dborders;
*/

    short	BeNiceToColormap;

/* obsoleted by the *BevelWidth resources - djhjr - 8/11/98
    * djhjr - 5/5/98 *
    short 	use3Dicons;
*/

/* obsoleted by the ":xpm:*" built-in pixmaps - djhjr - 10/26/02
	* djhjr - 4/25/96 *
	short SunkFocusWindowTitle;
*/

	/* for rader - djhjr - 2/9/99 */
	short NoPrettyTitles;

	/* djhjr - 9/21/96 */
	short ButtonColorIsFrame;

	/* djhjr - 4/17/98 */
	short VirtualReceivesMotionEvents;
	short VirtualSendsMotionEvents;

	/* djhjr - 6/22/99 */
	short DontDeiconifyTransients;

	/* submitted by Ugen Antsilevitch - 5/28/00 */
	short WarpVisible;

	/* djhjr - 10/11/01 */
	short ZoomZoom;         /* fallback on random zooms on iconify */

	/* djhjr - 10/20/02 */
	short NoBorderDecorations;

	/* djhjr - 11/3/03 */
	short RaiseOnStart;
#else
	struct
	{
		unsigned int NoDefaultMouseOrKeyboardBindings	: 1;
		unsigned int NoDefaultTitleButtons				: 1;
		unsigned int UsePPosition						: 2;
    	unsigned int OldFashionedTwmWindowsMenu			: 1;

/* djhjr - 2/15/99
		unsigned int UseRealScreenBorder				: 1;
*/

    	unsigned int AutoRelativeResize					: 1;
    	unsigned int FocusRoot							: 1;

	/* djhjr - 10/16/02 */
	unsigned int WarpCentered						: 2;

    	unsigned int WarpCursor							: 1;
    	unsigned int ForceIcon							: 1;
    	unsigned int NoGrabServer						: 1;
    	unsigned int NoRaiseMove						: 1;
    	unsigned int NoRaiseResize						: 1;
    	unsigned int NoRaiseDeicon						: 1;
    	unsigned int NoRaiseWarp						: 1;
    	unsigned int DontMoveOff						: 1;
    	unsigned int DoZoom								: 1;
    	unsigned int TitleFocus							: 1;

	/* djhjr - 5/27/98 */
	unsigned int IconManagerFocus					: 1;

	/* djhjr - 12/14/98 */
	unsigned int StaticIconPositions				: 1;

	/* djhjr - 10/2/01 */
	unsigned int StrictIconManager					: 1;

	/* djhjr - 8/23/02 */
    	unsigned int NoBorders						: 1;

    	unsigned int NoTitlebar							: 1;
    	unsigned int DecorateTransients					: 1;
    	unsigned int IconifyByUnmapping					: 1;
    	unsigned int ShowIconManager					: 1;
    	unsigned int IconManagerDontShow				: 1;
    	unsigned int NoIconifyIconManagers				: 1;
    	unsigned int BackingStore						: 1;
    	unsigned int SaveUnder							: 1;
    	unsigned int RandomPlacement					: 1;
    	unsigned int PointerPlacement					: 1;
    	unsigned int OpaqueMove							: 1;

        /* djhjr - 4/6/98 */
    	unsigned int OpaqueResize						: 1;

    	unsigned int Highlight							: 1;

    	/* djhjr - 1/27/98 */
    	unsigned int IconMgrHighlight					: 1;

    	unsigned int StackMode							: 1;
    	unsigned int TitleHighlight						: 1;
    	unsigned int SortIconMgr						: 1;
    	unsigned int Shadow								: 1;
    	unsigned int InterpolateMenuColors				: 1;
    	unsigned int NoIconManagers						: 1;
    	unsigned int ClientBorderWidth					: 1;
    	unsigned int HaveFonts							: 1;
    	unsigned int FirstTime							: 1;
    	unsigned int CaseSensitive						: 1;
    	unsigned int WarpUnmapped						: 1;
    	unsigned int DeIconifyToScreen					: 1;
    	unsigned int WarpWindows						: 1;
    	unsigned int snapRealScreen						: 1;
    	unsigned int GeometriesAreVirtual				: 1;
    	unsigned int Virtual							: 1;
    	unsigned int NamesInVirtualDesktop				: 1;
    	unsigned int AutoRaiseDefault					: 1;
		unsigned int UseWindowRing						: 1;
		unsigned int StayUpMenus						: 1;
    	unsigned int StayUpOptionalMenus				: 1;
    	unsigned int WarpToTransients					: 1;
		unsigned int EnhancedExecResources				: 1;
		unsigned int RightHandSidePulldownMenus			: 1;
		unsigned int LessRandomZoomZoom					: 1;
		unsigned int PrettyZoom							: 1;
		unsigned int StickyAbove						: 1;
		unsigned int DontInterpolateTitles				: 1;

		/* djhjr - 1/6/98 */
		unsigned int FixManagedVirtualGeometries		: 1;

		unsigned int FixTransientVirtualGeometries		: 1;
		unsigned int WarpSnug							: 1;
		unsigned int ShallowReliefWindowButton			: 2;

/* obsoleted by the *BevelWidth resources - djhjr - 8/11/98
	    unsigned int use3Dmenus							: 1;
    	unsigned int use3Dtitles						: 1;
    	unsigned int use3Diconmanagers					: 1;
    	unsigned int use3Dborders						: 1;

        * djhjr - 5/5/98 *
    	unsigned int use3Dicons							: 1;
*/

    	unsigned int BeNiceToColormap					: 1;

/* obsoleted by the ":xpm:*" built-in pixmaps - djhjr - 10/26/02
		unsigned int SunkFocusWindowTitle				: 1;
*/

		/* for rader - djhjr - 2/9/99 */
		unsigned int NoPrettyTitles						: 1;

		unsigned int ButtonColorIsFrame					: 1;

		/* djhjr - 4/17/98 */
		unsigned int VirtualReceivesMotionEvents		: 1;
		unsigned int VirtualSendsMotionEvents			: 1;

		/* djhjr - 6/22/99 */
		unsigned int DontDeiconifyTransients			: 1;

		/* submitted by Ugen Antsilevitch - 5/28/00 */
		unsigned int WarpVisible						: 1;

		/* djhjr - 10/11/01 */
		unsigned int ZoomZoom					: 1;

		/* djhjr - 10/20/02 */
		unsigned int NoBorderDecorations			: 1;

		/* djhjr - 11/3/03 */
		unsigned int RaiseOnStart				: 1;
	} userflags;
#define NoDefaultMouseOrKeyboardBindings	userflags.NoDefaultMouseOrKeyboardBindings
#define NoDefaultTitleButtons				userflags.NoDefaultTitleButtons
#define UsePPosition						userflags.UsePPosition
#define OldFashionedTwmWindowsMenu			userflags.OldFashionedTwmWindowsMenu

/* djhjr - 2/15/99
#define UseRealScreenBorder					userflags.UseRealScreenBorder
*/

#define AutoRelativeResize					userflags.AutoRelativeResize
#define FocusRoot							userflags.FocusRoot

/* djhjr - 10/16/02 */
#define WarpCentered							userflags.WarpCentered

#define WarpCursor							userflags.WarpCursor
#define ForceIcon							userflags.ForceIcon
#define NoGrabServer						userflags.NoGrabServer
#define NoRaiseMove							userflags.NoRaiseMove
#define NoRaiseResize						userflags.NoRaiseResize
#define NoRaiseDeicon						userflags.NoRaiseDeicon
#define NoRaiseWarp							userflags.NoRaiseWarp
#define DontMoveOff							userflags.DontMoveOff
#define DoZoom								userflags.DoZoom
#define TitleFocus							userflags.TitleFocus

/* djhjr - 5/27/98 */
#define IconManagerFocus					userflags.IconManagerFocus

/* djhjr - 12/14/98 */
#define StaticIconPositions					userflags.StaticIconPositions

/* djhjr - 10/2/01 */
#define StrictIconManager					userflags.StrictIconManager

/* djhjr - 8/23/02 */
#define NoBorders						userflags.NoBorders

#define NoTitlebar							userflags.NoTitlebar
#define DecorateTransients					userflags.DecorateTransients
#define IconifyByUnmapping					userflags.IconifyByUnmapping
#define ShowIconManager						userflags.ShowIconManager
#define IconManagerDontShow					userflags.IconManagerDontShow
#define NoIconifyIconManagers				userflags.NoIconifyIconManagers
#define BackingStore						userflags.BackingStore
#define SaveUnder							userflags.SaveUnder
#define RandomPlacement						userflags.RandomPlacement
#define PointerPlacement						userflags.PointerPlacement
#define OpaqueMove							userflags.OpaqueMove

/* djhjr - 4/6/98 */
#define OpaqueResize						userflags.OpaqueResize

#define Highlight							userflags.Highlight

/* djhjr - 1/27/98 */
#define IconMgrHighlight					userflags.IconMgrHighlight

#define StackMode							userflags.StackMode
#define TitleHighlight						userflags.TitleHighlight
#define SortIconMgr							userflags.SortIconMgr
#define Shadow								userflags.Shadow
#define InterpolateMenuColors				userflags.InterpolateMenuColors
#define NoIconManagers						userflags.NoIconManagers
#define ClientBorderWidth					userflags.ClientBorderWidth
#define HaveFonts							userflags.HaveFonts
#define FirstTime							userflags.FirstTime
#define CaseSensitive						userflags.CaseSensitive
#define WarpUnmapped						userflags.WarpUnmapped
#define DeIconifyToScreen					userflags.DeIconifyToScreen
#define WarpWindows							userflags.WarpWindows
#define snapRealScreen						userflags.snapRealScreen
#define GeometriesAreVirtual				userflags.GeometriesAreVirtual
#define Virtual								userflags.Virtual
#define NamesInVirtualDesktop				userflags.NamesInVirtualDesktop
#define AutoRaiseDefault					userflags.AutoRaiseDefault
#define UseWindowRing						userflags.UseWindowRing
#define StayUpMenus							userflags.StayUpMenus
#define StayUpOptionalMenus					userflags.StayUpOptionalMenus
#define WarpToTransients					userflags.WarpToTransients
#define EnhancedExecResources				userflags.EnhancedExecResources
#define RightHandSidePulldownMenus			userflags.RightHandSidePulldownMenus
#define LessRandomZoomZoom					userflags.LessRandomZoomZoom
#define PrettyZoom							userflags.PrettyZoom
#define StickyAbove							userflags.StickyAbove
#define DontInterpolateTitles				userflags.DontInterpolateTitles

/* djhjr - 1/6/98 */
#define FixManagedVirtualGeometries			userflags.FixManagedVirtualGeometries

#define FixTransientVirtualGeometries		userflags.FixTransientVirtualGeometries
#define WarpSnug							userflags.WarpSnug
#define ShallowReliefWindowButton			userflags.ShallowReliefWindowButton

/* obsoleted by the *BevelWidth resources - djhjr - 8/11/98
#define use3Dmenus							userflags.use3Dmenus
#define use3Dtitles							userflags.use3Dtitles
#define use3Diconmanagers					userflags.use3Diconmanagers
#define use3Dborders						userflags.use3Dborders

* djhjr - 5/5/98 *
#define use3Dicons							userflags.use3Dicons
*/

#define BeNiceToColormap					userflags.BeNiceToColormap

/* obsoleted by the ":xpm:*" built-in pixmaps - djhjr - 10/26/02
#define SunkFocusWindowTitle				userflags.SunkFocusWindowTitle
*/

/* for rader - djhjr - 2/9/99 */
#define NoPrettyTitles						userflags.NoPrettyTitles

#define ButtonColorIsFrame					userflags.ButtonColorIsFrame

/* djhjr - 4/17/98 */
#define VirtualReceivesMotionEvents			userflags.VirtualReceivesMotionEvents
#define VirtualSendsMotionEvents			userflags.VirtualSendsMotionEvents

/* djhjr - 6/22/99 */
#define DontDeiconifyTransients				userflags.DontDeiconifyTransients

/* submitted by Ugen Antsilevitch - 5/28/00 */
#define WarpVisible							userflags.WarpVisible

/* djhjr - 10/11/01 */
#define ZoomZoom					userflags.ZoomZoom

/* djhjr - 10/20/02 */
#define NoBorderDecorations				userflags.NoBorderDecorations

/* djhjr - 11/3/03 */
#define RaiseOnStart					userflags.RaiseOnStart
#endif

    /* djhjr - 9/10/03 */
    int IgnoreModifiers;	/* binding modifiers to ignore */

    FuncKey FuncKeyRoot;
    TwmDoor *Doors;		/* a list of doors on this screen */

	int AutoPanBorderWidth;           /* of autopan windows, really - DSE */
	int AutoPanExtraWarp;             /* # of extra pixels to warp - DSE */
	int RealScreenBorderWidth;        /* in virtual desktop - DSE */
	int AutoPanWarpWithRespectToRealScreen; /* percent - DSE */
} ScreenInfo;

extern int MultiScreen;
extern int NumScreens;
extern ScreenInfo **ScreenList;
extern ScreenInfo *Scr;
extern int FirstScreen;

#define PPOS_OFF 0
#define PPOS_ON 1
#define PPOS_NON_ZERO 2
/* may eventually want an option for having the PPosition be the initial
   location for the drag lines */

/* djhjr - 10/16/02 */
#define WARPC_OFF	0
#define WARPC_TITLED	1
#define WARPC_UNTITLED	2
#define WARPC_ON	3

#endif /* _SCREEN_ */
