
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
#include "image_formats.h"
#include "util.h"		/* for Image structure */
#include "iconmgr.h"
#include "doors.h"

#if defined TWM_USE_XINERAMA || defined TWM_USE_XRANDR

/* Xinerama and Xrandr enable screen tiling: */
#define TILED_SCREEN
#endif


typedef struct _StdCmap
{
  struct _StdCmap *next;	/* next link in chain */
  Atom atom;			/* property from which this came */
  int nmaps;			/* number of maps below */
  XStandardColormap *maps;	/* the actual maps */
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
  int d_depth;			/* copy of DefaultDepth(dpy, screen) */
  Visual *d_visual;		/* copy of DefaultVisual(dpy, screen) */
  int Monochrome;		/* is the display monochrome ? */
  int MyDisplayWidth;		/* my copy of DisplayWidth(dpy, screen) */
  int MyDisplayHeight;		/* my copy of DisplayHeight(dpy, screen) */
  int MaxWindowWidth;		/* largest window to allow */
  int MaxWindowHeight;		/* ditto */

  int ResizeX;			/* coordinate of resize/position window */
  int ResizeY;			/* ditto */

  int VirtualDesktopMaxWidth;	/* max width of virtual desktop */
  int VirtualDesktopMaxHeight;	/* max height of virtual desktop */
  int VirtualDesktopWidth;	/* width of virtual desktop */
  int VirtualDesktopHeight;	/* height of virtual desktop */
  int VirtualDesktopX;		/* top left x of my screen on the desktop */
  int VirtualDesktopY;		/* top left y of my screen on the desktop */
  int VirtualDesktopPanDistanceX;	/* distance to pan screen */
  int VirtualDesktopPanDistanceY;	/* distance to pan screen */

  /* these are for the little vd display */
  int VirtualDesktopDScale;	/* scale of the virtual desktop display */
  int VirtualDesktopDX;		/* position of the vd display */
  int VirtualDesktopDY;		/* position of the vd display */

  /* the autopan stuff */
  int AutoPanX;			/* how far should autopan travel */
  /* AutoPanX is also "whether autopan configured". */
  int AutoPanY;			/* how far should autopan travel */
  Window VirtualDesktopAutoPan[4];	/* the autopan windows */
  /* 0 = left, 1 = right, 2 = top, 3 = bottom */

  int VirtualDesktopPanResistance;	/* how much effort it takes to pan */

  TwmWindow TwmRoot;		/* the head of the twm window list */

  Window Root;			/* the root window */
  MyWindow SizeWindow;		/* the resize dimensions window */
  MyWindow InfoWindow;		/* the information window */
  Window VirtualDesktopDisplayOuter;	/* wrapper for display of the virtual desktop */
  Window VirtualDesktopDisplay;	/* display of the virtual desktop */
  Window VirtualDesktopDScreen;	/* display of the real screen on the vd */
  TwmWindow *VirtualDesktopDisplayTwin;	/* twm window for the above */

  name_list *ImageCache;	/* list of pixmaps */
  name_list *Icons;		/* list of icon pixmaps */

  int pullW, pullH;		/* size of pull right menu icon */

/* added the unknowns - djhjr - 8/13/98 */
  char *unknownName;		/* name of unknown icon pixmap */

  char *hiliteName;		/* name of built-in focus highlight pixmap */
  char *iconMgrIconName;	/* name of built-in iconmgr iconify pixmap */
  char *menuIconName;		/* name of built-in pull right menu pixmap */

  Image *hilitePm;		/* focus highlight window image structure */
  Image *virtualPm;		/* panner background window image structure */
  Image *realscreenPm;		/* real screen window image structure */

  MenuRoot *MenuList;		/* head of the menu list */
  MenuRoot *LastMenu;		/* the last menu (mostly unused?) */
  MenuRoot *Windows;		/* the TwmWindows menu */

  TwmWindow *Ring;		/* one of the windows in window ring */
  TwmWindow *RingLeader;	/* current winodw in ring */

  MouseButton *Mouse;		/* [MAX_BUTTONS+1][NUM_CONTEXTS][MOD_SIZE]; */
#define MOUSELOC(but,con,siz) (but*NUM_CONTEXTS*MOD_SIZE+con*MOD_SIZE+siz)
  MouseButton DefaultFunction;
  MouseButton WindowFunction;

  struct
  {
    Colormaps *cmaps;		/* current list of colormap windows */
    int maxCmaps;		/* maximum number of installed colormaps */
    unsigned long first_req;	/* seq # for first XInstallColormap() req in
				 * pass thru loading a colortable list */
    int root_pushes;		/* current push level to install root
				 * colormap windows */
    TwmWindow *pushed_window;	/* saved window to install when pushes drops
				 * to zero */
  } cmapInfo;

  struct
  {
    StdCmap *head, *tail;	/* list of maps */
    StdCmap *mru;		/* most recently used in list */
    int mruindex;		/* index of mru in entry */
  } StdCmapInfo;

  struct
  {
    int nleft, nright;		/* numbers of buttons in list */
    TitleButton *head;		/* start of list */
    int border;			/* button border */
    int pad;			/* button-padding */
    int width;			/* width of single button & border */
    int leftx;			/* start of left buttons */
    int titlex;			/* start of title string */
    int rightoff;		/* offset back from right edge */
  } TBInfo;
  ColorPair BorderTileC;	/* border tile colors */
  ColorPair TitleC;		/* titlebar colors */
  ColorPair MenuC;		/* menu colors */
  ColorPair MenuTitleC;		/* menu title colors */
  ColorPair IconC;		/* icon colors */
  ColorPair IconManagerC;	/* icon manager colors */
  ColorPair DefaultC;		/* default colors */

  ColorPair BorderColorC;	/* color of window borders */

  ColorPair VirtualDesktopDisplayC;	/* desktop display color */
  ColorPair DoorC;		/* default door colors */
  ColorPair VirtualC;		/* default virtual colors *//*RFB VCOLOR */
  ColorPair RealScreenC;	/* "real screen" in panner RFB 4/92 */
  Pixel VirtualDesktopDisplayBorder;	/* desktop display default border */
  Pixel BorderColor;		/* color of window borders */
  Pixel MenuShadowColor;	/* menu shadow color */
  Pixel IconBorderColor;	/* icon border color */
  Pixel IconManagerHighlight;	/* icon manager highlight */

  short ClearBevelContrast;	/* The contrast of the clear shadow */
  short DarkBevelContrast;	/* The contrast of the dark shadow */

  Cursor TitleCursor;		/* title bar cursor */
  Cursor FrameCursor;		/* frame cursor */
  Cursor IconCursor;		/* icon cursor */
  Cursor IconMgrCursor;		/* icon manager cursor */
  Cursor ButtonCursor;		/* title bar button cursor */
  Cursor MoveCursor;		/* move cursor */
  Cursor ResizeCursor;		/* resize cursor */
  Cursor WaitCursor;		/* wait a while cursor */
  Cursor MenuCursor;		/* menu cursor */
  Cursor SelectCursor;		/* dot cursor for f.move, etc. from menus */
  Cursor DestroyCursor;		/* skull and cross bones, f.destroy */
  Cursor DoorCursor;
    /*RFBCURSOR*/ Cursor VirtualCursor;
    /*RFBCURSOR*/ Cursor DesktopCursor;
    /*RFBCURSOR*/ Cursor NoCursor;	/* a black cursor - used on desktop display */

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

  name_list *NoBorder;		/* list of window without borders          */

  name_list *OpaqueMoveL;	/* list of windows moved as a solid */
  name_list *NoOpaqueMoveL;	/* list of windows moved as an outline */
  name_list *OpaqueResizeL;	/* list of windows resized as a solid */
  name_list *NoOpaqueResizeL;	/* list of windows resized as an outline */

  name_list *NoTitle;		/* list of window names with no title bar */
  name_list *MakeTitle;		/* list of window names with title bar */
  name_list *AutoRaise;		/* list of window names to auto-raise */
  name_list *IconNames;		/* list of window names and icon names */
  name_list *NoHighlight;	/* list of windows to not highlight */
  name_list *NoStackModeL;	/* windows to ignore stack mode requests */
  name_list *NoTitleHighlight;	/* list of windows to not highlight the TB */
  name_list *DontIconify;	/* don't iconify by unmapping */
  name_list *IconMgrNoShow;	/* don't show in the icon manager */
  name_list *IconMgrShow;	/* show in the icon manager */
  name_list *IconifyByUn;	/* windows to iconify by unmapping */
  name_list *StartIconified;	/* windows to start iconic */
  name_list *IconManagerHighlightL;	/* icon manager highlight colors */
  name_list *SqueezeTitleL;	/* windows of which to squeeze title */
  name_list *DontSqueezeTitleL;	/* windows of which not to squeeze */
  name_list *WindowRingL;	/* windows in ring */

  name_list *NoWindowRingL;	/* windows not added to ring */

  name_list *WarpCursorL;	/* windows to warp cursor to on deiconify */
  name_list *NailedDown;	/* windows that are nailed down */
  name_list *VirtualDesktopColorFL;	/* color of representations on the vd display */
  name_list *VirtualDesktopColorBL;	/* color of representations on the vd display */
  name_list *VirtualDesktopColorBoL;	/* color of representations on the vd display */
  name_list *DontShowInDisplay;	/* don't show these in the desktop display */

  name_list *DontShowInTWMWindows;	/* don't show these in the TWMWindows menu */

  name_list *DoorForegroundL;	/* doors foreground */
  name_list *DoorBackgroundL;	/* doors background */

  name_list *UsePPositionL;	/* windows with UsePPosition set */

  GC NormalGC;			/* normal GC for everything */
  GC RootGC;			/* root GC for title buttons */
  GC MenuGC;			/* gc for menus */
  GC DrawGC;			/* GC to draw lines for move and resize */

  GC GreyGC;			/* for shadowing on monochrome displays */
  GC ShadGC;			/* for shadowing on with patterns */

  unsigned long Black;
  unsigned long White;
  unsigned long XORvalue;	/* number to use when drawing xor'ed */
  MyFont TitleBarFont;		/* title bar font structure */
  MyFont MenuFont;		/* menu font structure */
  MyFont IconFont;		/* icon font structure */
  MyFont SizeFont;		/* resize font structure */
  MyFont IconManagerFont;	/* window list font structure */
  MyFont VirtualFont;		/* virtual display windows */
  MyFont DoorFont;		/* for drawing in doors */
  MyFont MenuTitleFont;		/* DSE -- for menu titles */
  MyFont InfoFont;		/* for the info window */
  MyFont DefaultFont;

#ifdef TWM_USE_XFT
  int use_xft;			/* >0 if using Xft fonts, otherwise X11 core fonts */
#endif

#ifdef TILED_SCREEN
  short use_tiles;		/* TRUE if screen decomposed into tiles */
  int tiles_bb[4];		/* (x0,y0) and (x1,y1) coordinates of tiled screen bounding-box */
  int ntiles;			/* number of tiles */
  int (*tiles)[4];		/* tiles vertices coordinates: (x0,y0), (x1,y1) */
#endif

#ifdef TWM_USE_XRANDR
  short RRScreenChangeRestart;	/* if TRUE restart vtwm on RRScreenChangeNotify events */
#endif

  IconMgr iconmgr;		/* default icon manager */
  struct RootRegion *FirstIconRegion;	/* pointer to icon regions */
  struct RootRegion *LastIconRegion;	/* pointer to the last icon region */
  char *IconDirectory;		/* icon directory to search */

  struct RootRegion *FirstAppletRegion;	/* pointer to applet regions */
  struct RootRegion *LastAppletRegion;	/* pointer to the last applet region */

  char *BitmapFilePath;		/* local copy of the X database resource */

#ifdef TWM_USE_OPACITY
  int MenuOpacity;		/* make use of "_NET_WM_WINDOW_OPACITY" */
  int IconOpacity;		/* property for twm menus, icons */
#endif

  int SizeStringOffset;		/* x offset in size window for drawing */
  int SizeStringWidth;		/* minimum width of size window */
  int BorderWidth;		/* border width of twm windows */

  /* widths of the various 3D shadows - djhjr - 5/2/98 */
  int BorderBevelWidth;
  int TitleBevelWidth;
  int MenuBevelWidth;
  int IconMgrBevelWidth;
  int InfoBevelWidth;

  int IconBevelWidth;
  int ButtonBevelWidth;

  int DoorBevelWidth;
  int VirtualDesktopBevelWidth;

  int MenuScrollBorderWidth;	/* top and bottom margins for menu scrolling */
  int MenuScrollJump;		/* number of entries for menu scroll */

  int IconBorderWidth;		/* border width of icon windows */
  int TitleHeight;		/* height of the title bar window */
  TwmWindow *Newest;		/* the most newly added twm window -- PF */
  int EntryHeight;		/* menu entry height */
  int FramePadding;		/* distance between decorations and border */
  int TitlePadding;		/* distance between items in titlebar */
  int ButtonIndent;		/* amount to shrink buttons on each side */
  int NumAutoRaises;		/* number of autoraise windows on screen */

  short SqueezeTitle;		/* make title as small as possible */
  short MoveDelta;		/* number of pixels before f.move starts */
  short ZoomCount;		/* zoom outline count */

  int PauseOnExit;		/* delay before shutting down via Done() */
  int PauseOnQuit;		/* delay before shuttind down via f.quit */

  struct
  {
    unsigned int NoDefaultMouseOrKeyboardBindings:1;
    unsigned int NoDefaultTitleButtons:1;
    unsigned int UsePPosition:2;
    unsigned int OldFashionedTwmWindowsMenu:1;
    unsigned int AutoRelativeResize:1;

    unsigned int WarpCentered:2;

    unsigned int WarpCursor:1;
    unsigned int ForceIcon:1;
    unsigned int NoGrabServer:1;
    unsigned int NoRaiseMove:1;
    unsigned int NoRaiseResize:1;
    unsigned int NoRaiseDeicon:1;
    unsigned int NoRaiseWarp:1;
    unsigned int DontMoveOff:1;
    unsigned int DoZoom:1;
    unsigned int TitleFocus:1;

    unsigned int IconManagerFocus:1;

    unsigned int StaticIconPositions:1;

    unsigned int StrictIconManager:1;

    unsigned int NoBorders:1;

    unsigned int NoTitlebar:1;
    unsigned int DecorateTransients:1;
    unsigned int IconifyByUnmapping:1;
    unsigned int ShowIconManager:1;
    unsigned int IconManagerDontShow:1;
    unsigned int NoIconifyIconManagers:1;
    unsigned int BackingStore:1;
    unsigned int SaveUnder:1;
    unsigned int RandomPlacement:1;
    unsigned int PointerPlacement:1;
    unsigned int OpaqueMove:1;

    unsigned int OpaqueResize:1;

    unsigned int Highlight:1;

    unsigned int IconMgrHighlight:1;

    unsigned int StackMode:1;
    unsigned int TitleHighlight:1;
    unsigned int SortIconMgr:1;
    unsigned int Shadow:1;
    unsigned int InterpolateMenuColors:1;
    unsigned int NoIconManagers:1;
    unsigned int ClientBorderWidth:1;
    unsigned int HaveFonts:1;
    unsigned int FirstTime:1;
    unsigned int CaseSensitive:1;
    unsigned int WarpUnmapped:1;
    unsigned int DeIconifyToScreen:1;
    unsigned int WarpWindows:1;
    unsigned int snapRealScreen:1;
    unsigned int GeometriesAreVirtual:1;
    unsigned int Virtual:1;
    unsigned int NamesInVirtualDesktop:1;
    unsigned int AutoRaiseDefault:1;
    unsigned int UseWindowRing:1;
    unsigned int StayUpMenus:1;
    unsigned int StayUpOptionalMenus:1;
    unsigned int WarpToTransients:1;
    unsigned int WarpToLocalTransients:1;
    unsigned int EnhancedExecResources:1;
    unsigned int RightHandSidePulldownMenus:1;
    unsigned int LessRandomZoomZoom:1;
    unsigned int PrettyZoom:1;
    unsigned int StickyAbove:1;
    unsigned int DontInterpolateTitles:1;

    /* djhjr - 1/6/98 */
    unsigned int FixManagedVirtualGeometries:1;

    unsigned int FixTransientVirtualGeometries:1;
    unsigned int WarpSnug:1;
    unsigned int ShallowReliefWindowButton:2;


    unsigned int BeNiceToColormap:1;

    /* for rader - djhjr - 2/9/99 */
    unsigned int NoPrettyTitles:1;

    unsigned int ButtonColorIsFrame:1;

    unsigned int VirtualReceivesMotionEvents:1;
    unsigned int VirtualSendsMotionEvents:1;

    unsigned int DontDeiconifyTransients:1;

    unsigned int WarpVisible:1;

    unsigned int ZoomZoom:1;

    unsigned int NoBorderDecorations:1;

    unsigned int RaiseOnStart:1;
  } userflags;
#define NoDefaultMouseOrKeyboardBindings	userflags.NoDefaultMouseOrKeyboardBindings
#define NoDefaultTitleButtons				userflags.NoDefaultTitleButtons
#define UsePPosition						userflags.UsePPosition
#define OldFashionedTwmWindowsMenu			userflags.OldFashionedTwmWindowsMenu


#define AutoRelativeResize					userflags.AutoRelativeResize

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

#define IconManagerFocus					userflags.IconManagerFocus

#define StaticIconPositions					userflags.StaticIconPositions

#define StrictIconManager					userflags.StrictIconManager

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

#define OpaqueResize						userflags.OpaqueResize

#define Highlight							userflags.Highlight

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
#define WarpToLocalTransients					userflags.WarpToLocalTransients
#define EnhancedExecResources				userflags.EnhancedExecResources
#define RightHandSidePulldownMenus			userflags.RightHandSidePulldownMenus
#define LessRandomZoomZoom					userflags.LessRandomZoomZoom
#define PrettyZoom							userflags.PrettyZoom
#define StickyAbove							userflags.StickyAbove
#define DontInterpolateTitles				userflags.DontInterpolateTitles

#define FixManagedVirtualGeometries			userflags.FixManagedVirtualGeometries

#define FixTransientVirtualGeometries		userflags.FixTransientVirtualGeometries
#define WarpSnug							userflags.WarpSnug
#define ShallowReliefWindowButton			userflags.ShallowReliefWindowButton


#define BeNiceToColormap					userflags.BeNiceToColormap

#define NoPrettyTitles						userflags.NoPrettyTitles

#define ButtonColorIsFrame					userflags.ButtonColorIsFrame

#define VirtualReceivesMotionEvents			userflags.VirtualReceivesMotionEvents
#define VirtualSendsMotionEvents			userflags.VirtualSendsMotionEvents

#define DontDeiconifyTransients				userflags.DontDeiconifyTransients

#define WarpVisible							userflags.WarpVisible

#define ZoomZoom					userflags.ZoomZoom

#define NoBorderDecorations				userflags.NoBorderDecorations

#define RaiseOnStart					userflags.RaiseOnStart

  int IgnoreModifiers;		/* binding modifiers to ignore */

  FuncKey FuncKeyRoot;
  TwmDoor *Doors;		/* a list of doors on this screen */

  int AutoPanBorderWidth;	/* of autopan windows, really - DSE */
  int AutoPanExtraWarp;		/* # of extra pixels to warp - DSE */
  int RealScreenBorderWidth;	/* in virtual desktop - DSE */
  int AutoPanWarpWithRespectToRealScreen;	/* percent - DSE */
} ScreenInfo;

extern int MultiScreen;
extern int NumScreens;
extern ScreenInfo **ScreenList;
extern ScreenInfo *Scr;
extern int FirstScreen;

#define PPOS_OFF 0
#define PPOS_ON 1
#define PPOS_NON_ZERO 2
#define PPOS_ON_SCREEN 3

/* may eventually want an option for having the PPosition be the initial
   location for the drag lines */

#define WARPC_OFF	0
#define WARPC_TITLED	1
#define WARPC_UNTITLED	2
#define WARPC_ON	3


/*
 * Check if "TwmWindow *win" is on the root window of "ScreenInfo *scr":
 */
#define ClientIsOnScreen(win,scr)   ((win)->attr.root == (scr)->Root)

#ifdef TILED_SCREEN

#define AreaHeight(a)	    (Top(a) - Bot(a) + 1)
#define AreaWidth(a)	    (Rht(a) - Lft(a) + 1)

#define Lft(t)		    ((t)[0])	/* See "int (*tiles)[4];" above and            */
#define Rht(t)		    ((t)[2])	/* initialisation of (x0,y0), (x1,y1) in twm.c */

#define Bot(t)		    ((t)[1])	/* As a matter of notation: Reflect y-axis as  */
#define Top(t)		    ((t)[3])	/* "Text-Editor- to Cartesian Coordinates"     */
				     /* transform: 'Top' must be >= 'Bot'.          */

#define xmin(a,b)	    ((a) < (b) ? (a) : (b))
#define xmax(a,b)	    ((a) > (b) ? (a) : (b))

/* if overlapping return intersection;  else return gapsize as negative value: */
#define Distance1D(a,b,u,v) (xmin((b),(v)) - xmax((a),(u)))

/* if overlapping return area; else return (larger) gapsize as negative value: */
#define Distance2D(dx,dy)   ((dx)>=0?((dy)>=0?((dx)+1)*((dy)+1):(dy)):((dy)>=0?(dx):((dx)>(dy)?(dy):(dx))))


#endif /*TILED_SCREEN */

#endif /* _SCREEN_ */
