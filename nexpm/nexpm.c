/**********************************************************/
/*                                                        */
/*  pxpm.c: this is sxpm.c hacked by Ralph Betza so that  */
/*  it can be used to set a background picture for the    */
/*  vtwm panner window,                                   */
/*                                                        */
/*  Works for vtwm, not for tvtwm.                        */
/*                                                        */
/*  And then also changed    to set backgrounds for       */
/*  *any* window.                                         */
/*  Ralph Betza, June 1993    gnohmon@ssiny.com           */
/**********************************************************/

/* Copyright 1990,91 GROUPE BULL -- See licence conditions in file COPYRIGHT */
/* Since most of the code is from sxpm.c, the above copyright should
** still apply.
*/

/*****************************************************************************\
* sxpm.c:                                                                     *
*                                                                             *
*  Show XPM File program                                                      *
*                                                                             *
*  Developed by Arnaud Le Hors                                                *
\*****************************************************************************/

#include <stdio.h>

#ifdef VMS
#include "decw$include:Xlib.h"
#include "decw$include:Intrinsic.h"
#include "decw$include:Shell.h"
#include "decw$include:shape.h"
#else
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <X11/extensions/shape.h>
#include <X11/cursorfont.h>
#endif

#include <X11/xpm.h>

Window window_by_name();
Window point_to_window();

/* XPM */
/* plaid pixmap */
static char *plaid[] =
{
/* width height ncolors chars_per_pixel */
 "22 22 4 2",
/* colors */
 "   c red 	m white  s light_color",
 "Y  c green	m black  s lines_in_mix",
 "+  c yellow	m white  s lines_in_dark",
 "x 		m black  s dark_color",
/* pixels */
 "x   x   x x x   x   x x x x x x + x x x x x ",
 "  x   x   x   x   x   x x x x x x x x x x x ",
 "x   x   x x x   x   x x x x x x + x x x x x ",
 "  x   x   x   x   x   x x x x x x x x x x x ",
 "x   x   x x x   x   x x x x x x + x x x x x ",
 "Y Y Y Y Y x Y Y Y Y Y + x + x + x + x + x + ",
 "x   x   x x x   x   x x x x x x + x x x x x ",
 "  x   x   x   x   x   x x x x x x x x x x x ",
 "x   x   x x x   x   x x x x x x + x x x x x ",
 "  x   x   x   x   x   x x x x x x x x x x x ",
 "x   x   x x x   x   x x x x x x + x x x x x ",
 "          x           x   x   x Y x   x   x ",
 "          x             x   x   Y   x   x   ",
 "          x           x   x   x Y x   x   x ",
 "          x             x   x   Y   x   x   ",
 "          x           x   x   x Y x   x   x ",
 "x x x x x x x x x x x x x x x x x x x x x x ",
 "          x           x   x   x Y x   x   x ",
 "          x             x   x   Y   x   x   ",
 "          x           x   x   x Y x   x   x ",
 "          x             x   x   Y   x   x   ",
 "          x           x   x   x Y x   x   x "
};

/* #define win XtWindow(topw) */
Window win;
Window Compensate();
#define dpy XtDisplay(topw)
#define screen XtScreen(topw)
#define colormap XDefaultColormapOfScreen(screen)
#define root XRootWindowOfScreen(screen)
#define xrdb XtDatabase(dpy)

int sDoByName;
int sDoByNumber;
char * pTargetWindowName;

void Usage();
void ErrorMessage();
void Punt();

#define IWIDTH      50
#define IHEIGHT     50

typedef struct _XpmIcon {
    Pixmap pixmap;
    Pixmap mask;
    XpmAttributes attributes;
}        XpmIcon;

static char **command;
static Widget topw;
static XpmIcon view;

char *point_txt[] = {
"           ====> Please select the window",
"           ====> whose background you wish to",
"           ====> change by clicking the",
"           ====> pointer in that window.",
0};

XColor NameToXColor();
unsigned long NameToPixel();

main(argc, argv)
    unsigned int argc;
    char **argv;
{
    int ErrorStatus;
    unsigned int stdinf = 0;
    unsigned int sSolid = 0;
    unsigned int w_rtn;
    unsigned int h_rtn;
    char *input = NULL;
    unsigned int numsymbols = 0;
    XpmColorSymbol symbols[10];
    char *stype;
    XrmValue val;
    unsigned long valuemask = 0;
    int n;
    char *solid_color = NULL;
    Arg args[3];

    topw = XtInitialize(argv[0], "Nexpm",
			(void*)0, 0, &argc, argv);

    if (!topw) {
	fprintf(stderr, "Nexpm Error... [ Undefined DISPLAY ]\n");
	exit(1);
    }

    /*
     * arguments parsing
     */

    command = argv;
    if (argc < 2)
	Usage();
    for (n = 1; n < argc; n++) {

	if ( ! strcmp( argv[ n ], "-target" ))
	{	sDoByName = 1;
		pTargetWindowName = argv[++n];
		continue;
	}
	if ( ! strcmp( argv[ n ], "-vtwm" ))
	{	sDoByName = 1;
		pTargetWindowName = "Virtual Desktop";
		continue;
	}
	if ( ! strcmp( argv[ n ], "-id" ))
	{	sDoByNumber = 1;
		pTargetWindowName = argv[++n];
		continue;
	}

	if (!strcmp("-solid", argv[n])) {
	    solid_color = argv[++n];
		sSolid = 1;
	    continue;
	}
	if (strncmp(argv[n], "-plaid", 3) == 0) {
	    continue;
	}
	if (strncmp(argv[n], "-in", 3) == 0) {
	    input = argv[++n];
	    continue;
	}
	if (strncmp(argv[n], "-stdin", 5) == 0) {
	    stdinf = 1;
	    continue;
	}
	if (strncmp(argv[n], "-s", 2) == 0) {
	    if (n < argc - 2) {
		valuemask |= XpmColorSymbols;
		symbols[numsymbols].name = argv[++n];
		symbols[numsymbols++].value = argv[++n];
		continue;
	    }
	}
	if (strncmp(argv[n], "-p", 2) == 0) {
	    if (n < argc - 2) {
		valuemask |= XpmColorSymbols;
		symbols[numsymbols].name = argv[++n];
		symbols[numsymbols].value = NULL;
		symbols[numsymbols++].pixel = atol(argv[++n]);
		continue;
	    }
	}
	if (strncmp(argv[n], "-rgb", 3) == 0) {
	    if (n < argc - 1) {
		valuemask |= XpmRgbFilename;
		view.attributes.rgb_fname = argv[++n];
		continue;
	    }
	}
	Usage();
    }

    n = 0;
	XtSetArg(args[n], XtNwidth, 1);
	n++;
	XtSetArg(args[n], XtNheight, 1);
	n++;
    XtSetArg(args[n], XtNmappedWhenManaged, False);
    n++;
    XtSetValues(topw, args, n);
    XtRealizeWidget(topw);

    view.attributes.colorsymbols = symbols;
    view.attributes.numsymbols = numsymbols;
    view.attributes.valuemask = valuemask;

    if ( sDoByName )
    {	win = window_by_name( root, pTargetWindowName );
		win = Compensate( win );
	}
	else if ( sDoByNumber )
	{
		win = strtol( pTargetWindowName, (void *)0, 0 );
	}
	else
	{	char ** ptr;
		for (ptr = point_txt; *ptr; ptr++)
	    	printf("%s\n",*ptr);

		win = point_to_window(); /* use pointer to get window */
		win = Compensate( win );
	}

	if ( ! sSolid )
	{
	    if (input || stdinf) {
		view.attributes.valuemask |= XpmReturnInfos;
		view.attributes.valuemask |= XpmReturnPixels;
		ErrorStatus = XpmReadFileToPixmap(dpy, win, input,
						  &view.pixmap, &view.mask,
						  &view.attributes);
		ErrorMessage(ErrorStatus, "Read");
	    } else {

		ErrorStatus = XpmCreatePixmapFromData(dpy, win, plaid,
						      &view.pixmap, &view.mask,
						      &view.attributes);
		ErrorMessage(ErrorStatus, "Plaid");
	    }
/* Now we are ready to display it... */
		XSetWindowBackgroundPixmap(dpy, win, view.pixmap);
	}
	else
	{
		XSetWindowBackground(dpy, win, NameToPixel( solid_color,
		    0 ));
	}

	XClearWindow(dpy, win);
	XFlush( dpy );
    Punt(0);
}

void
Usage()
{
    fprintf(stderr, "\nUsage:  %s [options...]\n", command[0]);
    fprintf(stderr, "%s\n", "Where options are:");
    fprintf(stderr, "%s\n",
    	"[-vtwm]                      Target is the vtwm panner.");
    fprintf(stderr, "%s\n",
    	"[-id window_id               target Window's numeric ID.");
    fprintf(stderr, "%s\n",
		"[-target window_name]        target Window's name.");
    fprintf(stderr, "%s\n",
	"    (if none of the above specified, target is selected by mouse)");
    fprintf(stderr, "%s\n",
"[-solid color_name]          Ignore all pixmap parameters, set\
 background color.");
    fprintf(stderr, "%s\n",
	    "[-d host:display]            Display to connect to.");
    fprintf(stderr, "%s\n",
	    "[-s symbol_name color_name]  Overwrite color defaults.");
    fprintf(stderr, "%s\n",
	    "[-p symbol_name pixel_value] Overwrite color defaults.");
    fprintf(stderr, "%s\n",
	    "[-plaid]                     Read the included plaid pixmap.");
    fprintf(stderr, "%s\n",
	  "[-in filename]               Read input from file `filename`.");
    fprintf(stderr, "%s\n",
	    "[-stdin]                     Read input from stdin.");
    fprintf(stderr, "%s\n\n",
	    "[-rgb filename]              Search color names in the \
rgb text file `filename`.");
    exit(0);
}


void
ErrorMessage(ErrorStatus, tag)
    int ErrorStatus;
    char *tag;
{
    char *error = NULL;
    char *warning = NULL;

    switch (ErrorStatus) {
    case XpmSuccess:
	return;
    case XpmColorError:
	warning = "Could not parse or alloc requested color";
	break;
    case XpmOpenFailed:
	error = "Cannot open file";
	break;
    case XpmFileInvalid:
	error = "invalid XPM file";
	break;
    case XpmNoMemory:
	error = "Not enough memory";
	break;
    case XpmColorFailed:
	error = "Color not found";
	break;
    }

    if (warning)
	printf("%s Xpm Warning: %s.\n", tag, warning);

    if (error) {
	printf("%s Xpm Error: %s.\n", tag, error);
	Punt(1);
    }
}

void
Punt(i)
    int i;
{
    if (view.pixmap) {
	XFreePixmap(dpy, view.pixmap);
	if (view.mask)
	    XFreePixmap(dpy, view.mask);

	XFreeColors(dpy, colormap,
		    view.attributes.pixels, view.attributes.npixels, 0);

	XpmFreeAttributes(&view.attributes);
    }
    exit(i);
}

Window window_by_name(wdw,name)
Window wdw;
char *name;
{
/**********************************************************/
/*                                                        */
/*  Copied from xcursor.c, who copied it from xwininfo.   */
/*                                                        */
/**********************************************************/

    Window *offspring;		/* Any children */
    Window junk;		/* Just that */
    Window w = 0;		/* Found window */
    int count;			/* Number of kids */
    int loop;			/* Loop counter */
    char *wdw_name;		/* Returnewd name */
    if (XFetchName(dpy,wdw,&wdw_name) && !strcmp(wdw_name,name))
      return(wdw);
    if (!XQueryTree(dpy,wdw,&junk,&junk,&offspring,&count))
      return(0);
    for (loop = 0; loop < count; loop++)
      {
	  w = window_by_name(offspring[loop],name);
	  if (w)
	    break;
      }
    if (offspring)
      XFree(offspring);
	/* fprintf( stderr, "w=%x\n", w ); */
    return(w);
}

Window point_to_window()
{	/* from xcursor.c, also from blast.c */
    int status;
    Cursor cursor;
    XEvent event;
    Window target_win = None;
    int buttons = 0;

    /* Make the target cursor */
    cursor = XCreateFontCursor(dpy, XC_crosshair);

    /* Grab the pointer using target cursor, letting it room all over */
    status = XGrabPointer(dpy, root, False,
			  ButtonPressMask|ButtonReleaseMask, GrabModeSync,
			  GrabModeAsync, None, cursor, CurrentTime);
    if (status != GrabSuccess)
      {
	  fprintf(stderr,"Can't grab the mouse.");
	  exit(4);
      }

    /* Let the user select a window... */
    while ((target_win == None) || (buttons != 0)) /* allow one more event */
      {
	  XAllowEvents(dpy, SyncPointer, CurrentTime);
	  XWindowEvent(dpy, root,
		       ButtonPressMask|ButtonReleaseMask, &event);
	  switch (event.type)
	    {
	      case ButtonPress:
		if (target_win == None)
		  {
		      /* window selected */
		      target_win = event.xbutton.subwindow;
		      if (target_win == None)
			target_win = root;
		  }
		buttons++;
		break;

		/* there may have been some down before we started */
	      case ButtonRelease:
		if (buttons > 0)
		  buttons--;
		break;
	    }
      }

    XUngrabPointer(dpy, CurrentTime);      /* Done with pointer */

	target_win = XmuClientWindow( dpy, target_win );
/* XmuClientWindow() is suggested by blast.c, but ifdeffed out.
** What happens is that blast gets the ID of the frame window,
** created by the window manager, not the ID of the window you
** really pointed to; but for blast it's OK: the shape *should* be
** given to the frame.
*/

    return(target_win);
}

Window
Compensate( win )
Window win;
{
	Window dummy;
	Window dummy2;
	Window * children;
	unsigned int n;

	if ( XQueryTree( dpy, win, &dummy, &dummy2, &children, &n ))
	{
		win = *children;
		XFree( children );
	}
/**********************************************************/
/*                                                        */
/*  It is possible that Compensate() is vtwm-specific.    */
/*                                                        */
/*  In any case, xlswins() reveals that the window ID we  */
/*  get from the pointer or from the name is always the   */
/*  *parent* of the window we really want, and since it   */
/*  is always at the bottom of the stacking order we      */
/*  know that it is the first window returned by          */
/*  XQueryTree.                                           */
/*                                                        */
/**********************************************************/
	return win;
}


unsigned long NameToPixel(name, pixel)
    char *name;
    unsigned long pixel;
{
    XColor ecolor;

    if (!name || !*name)
	return pixel;
    if (!XParseColor(dpy,colormap,name,&ecolor)) {
	fprintf(stderr,"nexpm:  unknown color \"%s\"\n",name);
	exit(1);
	/*NOTREACHED*/
    }
    if (!XAllocColor(dpy, colormap,&ecolor))
    {
	    fprintf(stderr, "nexpm:  unable to allocate color for \"%s\"\n",
		    name);
	    exit(1);
	    /*NOTREACHED*/
	}
    return(ecolor.pixel);
}
