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
 * $XConsortium: parse.c,v 1.52 91/07/12 09:59:37 dave Exp $
 *
 * parse the .twmrc file
 *
 * 17-Nov-87 Thomas E. LaStrange       File created
 * 10-Oct-90 David M. Sternlicht       Storing saved colors on root
 ***********************************************************************/

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <X11/Xos.h>
#include <X11/Xmu/CharSet.h>
#include "twm.h"
#include "screen.h"
#include "menus.h"
#include "list.h"
#include "image_formats.h"
#include "util.h"
#include "gram.h"
#include "parse.h"
#include "prototypes.h"
#include <X11/Xatom.h>

#ifndef NO_M4_SUPPORT
#include <sys/wait.h>
#include <unistd.h>
#include <netdb.h>
#include <X11/Xmu/SysUtil.h>
#endif


#define BUF_LEN 300

static FILE *twmrc;
static int ptr = 0;
static int len = 0;
static char buff[BUF_LEN+1];
static char overflowbuff[20];		/* really only need one */
static int overflowlen;
static char **stringListSource, *currentString;
static void put_pixel_on_root(Pixel pixel);

int RaiseDelay = 0;           /* msec, for AutoRaise */
extern int mods;
int (*twmInputFunc)(void);

int ConstrainedMoveTime = 400;		/* milliseconds, event times */

static int twmFileInput(void), twmStringListInput(void);

extern char *defTwmrc[];		/* default bindings */

#ifndef NO_M4_SUPPORT
#define Resolution(pixels, mm) ((((pixels) * 100000 / (mm)) + 50) / 100)
#define M4_MAXDIGITS 21 /* greater than the number of digits in a long int */
extern Bool PrintErrorMessages;
static char *make_m4_cmdline(char *display_name, char *cp, char *m4_option);
#endif

#ifndef NO_SOUND_SUPPORT
extern void SetSoundHost();
extern void SetSoundVolume();
#endif

/***********************************************************************
 *
 *  Procedure:
 *	ParseTwmrc - parse the .twmrc file
 *
 *  Inputs:
 *	filename  - the filename to parse.  A NULL indicates $HOME/.twmrc
 *
 ***********************************************************************
 */

static int 
doparse (int (*ifunc)(void), char *srctypename, char *srcname)
{
    mods = 0;
    ptr = 0;
    len = 0;
#ifdef NEED_YYLINENO_V
    yylineno = 1;
#endif
    ParseError = FALSE;
    twmInputFunc = ifunc;
    overflowlen = 0;

    yyparse();

    if (ParseError) {
	fprintf (stderr, "%s:  errors found in twm %s",
		 ProgramName, srctypename);
	if (srcname) fprintf (stderr, " \"%s\"", srcname);
	fprintf (stderr, "\n");
    }
    return (ParseError ? 0 : 1);
}

/* Changes for m4 pre-processing submitted by Jason Gloudon */
/* added support for user-defined parameters - djhjr - 2/20/99 */
#ifndef NO_M4_SUPPORT
int ParseTwmrc (char *filename, char *display_name, int m4_preprocess, char *m4_option)
#else
int ParseTwmrc (char *filename)
#endif
{
    int i;
    char *cp = NULL;
    char tmpfilename[257];
#ifndef NO_M4_SUPPORT
    char *m4_cmdline;
    int m4_status;
#endif

    /*
     * If filename given, try it, else try ~/.vtwmrc.#, else try ~/.vtwmrc,
     * else try system.vtwmrc, else try ~/.twmrc.#, else ~/.twmrc, else
     * system.twmrc; finally using built-in defaults.
     *
     * This choice allows user, then system, versions of .vtwmrc, followed
     * by user, then system, versions of .twmrc.
     * Thus, sites that have both twm and vtwm can allow users without
     * private .vtwmrc or .twmrc files to fall back to system-wide
     * defaults (very important when there are many users), yet the
     * presence of a private .twmrc file for twm will not prevent
     * features of a system-wide .vtwmrc file from exploiting the mew
     * features of vtwm. Submitted by Nelson H. F. Beebe
     */
    for (twmrc = NULL, i = 0; !twmrc && i <= 6; i++) {
	switch (i) {
	  case 0:			/* -f filename */
	    cp = filename;
	    break;

	  case 1:			/* ~/.vtwmrc.screennum */
	    if (!filename) {
	      cp = tmpfilename;
	      (void) sprintf (tmpfilename, "%s/.vtwmrc.%d",
			      Home, Scr->screen);
	      break;
	    }
	    continue;

	  case 2:			/* ~/.vtwmrc */
	    if (!filename) {
	      tmpfilename[HomeLen + 8] = '\0';
	    }
	    break;

	  case 3:			/* system.vtwmrc */
	    cp = SYSTEM_VTWMRC;
	    break;

	  case 4:			/* ~/.twmrc.screennum */
	    if (!filename) {
	      cp = tmpfilename;
	      (void) sprintf (tmpfilename, "%s/.twmrc.%d",
			      Home, Scr->screen);
	      break;
	    }
	    continue;

	  case 5:			/* ~/.twmrc */
	    if(!filename){
	      tmpfilename[HomeLen + 7] = '\0';
	    }
	    break;

	  case 6:			/* system.twmrc */
	    cp = SYSTEM_TWMRC;
	    break;
	}

#ifndef NO_M4_SUPPORT
	if (cp) {
	    if (m4_preprocess && (access(cp, R_OK) == 0) ) {
		    if ((m4_cmdline = make_m4_cmdline(display_name, cp, m4_option)) != NULL) {
			    twmrc = popen(m4_cmdline, "r");
			    free(m4_cmdline);
		    }
		    else {
			    m4_preprocess = 0;
			    twmrc = fopen (cp, "r");
		    }
	    }
	    else {
		    twmrc = fopen (cp, "r");
	    }
	}
#else
	if (cp) twmrc = fopen (cp, "r");
#endif
    }

    if (twmrc) {
	int status;

	if (filename && cp != filename) {
	    fprintf (stderr,
		     "%s:  unable to open twmrc file %s, using %s instead\n",
		     ProgramName, filename, cp);
	}
	status = doparse (twmFileInput, "file", cp);

#ifndef NO_M4_SUPPORT
	if(m4_preprocess){
	    m4_status = pclose (twmrc);
	    if(!WIFEXITED(m4_status) ||
	       (WIFEXITED(m4_status) && WEXITSTATUS(m4_status)) ){
		fprintf(stderr,
			"%s: m4 returned %d\n",
			ProgramName, WEXITSTATUS(m4_status));
		exit(-1);
	    }
	}
	else {
	    fclose (twmrc);
	}
#else
	fclose (twmrc);
#endif

	return status;
    } else {
	if (filename) {
	    fprintf (stderr,
	"%s:  unable to open twmrc file %s, using built-in defaults instead\n",
		     ProgramName, filename);
	}
	return ParseStringList (defTwmrc);
    }
}

int 
ParseStringList (char **sl)
{
    stringListSource = sl;
    currentString = *sl;
    return doparse (twmStringListInput, "string list", (char *)NULL);
}


/***********************************************************************
 *
 *  Procedure:
 *	twmFileInput - redefinition of the lex input routine for file input
 *
 *  Returned Value:
 *	the next input character
 *
 ***********************************************************************
 */

static int 
twmFileInput (void)
{
    if (overflowlen) return (int) overflowbuff[--overflowlen];

    while (ptr == len)
    {
	if (fgets(buff, BUF_LEN, twmrc) == NULL)
	    return 0;

#ifdef NEED_YYLINENO_V
	yylineno++;
#endif

	ptr = 0;
	len = strlen(buff);
    }
    return ((int)buff[ptr++]);
}

static int 
twmStringListInput (void)
{
    if (overflowlen) return (int) overflowbuff[--overflowlen];

    /*
     * return the character currently pointed to
     */
    if (currentString) {
	unsigned int c = (unsigned int) *currentString++;

	if (c) return c;		/* if non-nul char */
	currentString = *++stringListSource;  /* advance to next bol */
	return '\n';			/* but say that we hit last eol */
    }
    return 0;				/* eof */
}


/***********************************************************************
 *
 *  Procedure:
 *	twmUnput - redefinition of the lex unput routine
 *
 *  Inputs:
 *	c	- the character to push back onto the input stream
 *
 ***********************************************************************
 */

void 
twmUnput (int c)
{
    if (overflowlen < sizeof overflowbuff) {
	overflowbuff[overflowlen++] = (char) c;
    } else {
	twmrc_error_prefix ();
	fprintf (stderr, "unable to unput character (%d)\n",
		 c);
    }
}


/***********************************************************************
 *
 *  Procedure:
 *	TwmOutput - redefinition of the lex output routine
 *
 *  Inputs:
 *	c	- the character to print
 *
 ***********************************************************************
 */

void 
TwmOutput (int c)
{
    putchar(c);
}


/**********************************************************************
 *
 *  Parsing table and routines
 *
 ***********************************************************************/

typedef struct _TwmKeyword {
    char *name;
    int value;
    int subnum;
} TwmKeyword;

#define kw0_NoDefaults			1
#define kw0_AutoRelativeResize		2
#define kw0_ForceIcons			3
#define kw0_NoIconManagers		4
#define kw0_OpaqueMove			5
#define kw0_InterpolateMenuColors	6
#define kw0_NoVersion			7
#define kw0_SortIconManager		8
#define kw0_NoGrabServer		9
#define kw0_NoMenuShadows		10
#define kw0_NoRaiseOnMove		11
#define kw0_NoRaiseOnResize		12
#define kw0_NoRaiseOnDeiconify		13
#define kw0_DontMoveOff			14
#define kw0_NoBackingStore		15
#define kw0_NoSaveUnders		16
#define kw0_RestartPreviousState	17
#define kw0_ClientBorderWidth		18
#define kw0_NoTitleFocus		19
#define kw0_RandomPlacement		20
#define kw0_DecorateTransients		21
#define kw0_ShowIconManager		22
#define kw0_NoCaseSensitive		23
#define kw0_NoRaiseOnWarp		24
#define kw0_WarpUnmapped		25
#define kw0_DeIconifyToScreen           26
#define kw0_WarpWindows                 27
#define kw0_SnapRealScreen		28
#define kw0_NotVirtualGeometries	29
#define kw0_OldFashionedTwmWindowsMenu 30
#define kw0_StayUpMenus                32
#define kw0_NaturalAutopanBehavior     33
#define kw0_EnhancedExecResources      34
#define kw0_RightHandSidePulldownMenus 35
#define kw0_LessRandomZoomZoom         36
#define kw0_PrettyZoom                 37

#define kw0_NoDefaultTitleButtons            38
#define kw0_NoDefaultMouseOrKeyboardBindings 39

#define kw0_StickyAbove                      40
#define kw0_DontInterpolateTitles            41
#define kw0_FixTransientVirtualGeometries    42

#define kw0_WarpToTransients                 43
#define kw0_NoIconifyIconManagers            44
#define kw0_StayUpOptionalMenus              45

#define kw0_WarpSnug                         46

#define kw0_PointerPlacement		47
#define kw0_ShallowReliefWindowButton		52
#define kw0_ButtonColorIsFrame				53
#define kw0_FixManagedVirtualGeometries		54
#define kw0_BeNiceToColormap				55
#define kw0_VirtualReceivesMotionEvents		56
#define kw0_VirtualSendsMotionEvents		57
#define kw0_NoIconManagerFocus			59
#define kw0_StaticIconPositions			60
#define kw0_NoPrettyTitles				61
#define kw0_DontDeiconifyTransients		62
#define kw0_WarpVisible			63
#define kw0_StrictIconManager		64
#define kw0_ZoomZoom			65
#define kw0_NoBorderDecorations		66
#define kw0_RaiseOnStart		67

#ifdef TWM_USE_XFT
#define kw0_EnableXftFontRenderer	68
#endif

#ifdef TWM_USE_SLOPPYFOCUS
#define kw0_SloppyFocus			69
#endif

#define kw0_WarpToLocalTransients	70

#ifdef TWM_USE_XRANDR
#define kw0_RestartOnScreenChangeNotify	71
#endif

#define kws_IconFont			2
#define kws_ResizeFont			3
#define kws_MenuFont			4
#define kws_TitleFont			5
#define kws_IconManagerFont		6
#define kws_UnknownIcon			7
#define kws_IconDirectory		8
#define kws_MaxWindowSize		9
#define kws_VirtualFont			10
#define kws_DoorFont			11
#define kws_MenuTitleFont       12

#define kws_InfoFont			13

#define kws_ResizeRegion		14

#ifndef NO_SOUND_SUPPORT
#define kws_SoundHost			15
#endif

#define kws_DefaultFont			16

#define kwn_ConstrainedMoveTime     1
#define kwn_MoveDelta               2
#define kwn_XorValue                3
#define kwn_FramePadding            4
#define kwn_TitlePadding            5
#define kwn_ButtonIndent            6
#define kwn_BorderWidth             7
#define kwn_IconBorderWidth         8
#define kwn_TitleButtonBorderWidth  9
#define kwn_PanDistanceX           10
#define kwn_PanDistanceY           11
#define kwn_AutoPan                12
#define kwn_RaiseDelay             13
#define kwn_AutoPanBorderWidth     14
#define kwn_AutoPanExtraWarp       15
#define kwn_RealScreenBorderWidth  16
#define kwn_AutoPanWarpWithRespectToRealScreen 17
#define kwn_ClearBevelContrast		19
#define kwn_DarkBevelContrast		20
#define kwn_BorderBevelWidth		21
#define kwn_IconManagerBevelWidth	22
#define kwn_InfoBevelWidth			23
#define kwn_MenuBevelWidth			24
#define kwn_TitleBevelWidth		25
#define kwn_IconBevelWidth			26
#define kwn_ButtonBevelWidth		27
#define kwn_DoorBevelWidth				28
#define kwn_VirtualDesktopBevelWidth	29
#define kwn_PanResistance		30
#define kwn_MenuScrollBorderWidth	31
#define kwn_MenuScrollJump			32
#ifndef NO_SOUND_SUPPORT
#define kwn_SoundVolume			33
#endif
#define kwn_PauseOnExit			34
#define kwn_PauseOnQuit			35

#ifdef TWM_USE_OPACITY
#define kwn_MenuOpacity			36
#define kwn_IconOpacity			37
#endif

#define kwcl_BorderColor		1
#define kwcl_IconManagerHighlight	2
#define kwcl_BorderTileForeground	3
#define kwcl_BorderTileBackground	4
#define kwcl_TitleForeground		5
#define kwcl_TitleBackground		6
#define kwcl_IconForeground		7
#define kwcl_IconBackground		8
#define kwcl_IconBorderColor		9
#define kwcl_IconManagerForeground	10
#define kwcl_IconManagerBackground	11
#define kwcl_VirtualDesktopBackground   12
#define kwcl_VirtualDesktopForeground   13
#define kwcl_VirtualDesktopBorder	14
#define kwcl_DoorForeground		15
#define kwcl_DoorBackground		16

#define kwc_DefaultForeground		1
#define kwc_DefaultBackground		2
#define kwc_MenuForeground		3
#define kwc_MenuBackground		4
#define kwc_MenuTitleForeground		5
#define kwc_MenuTitleBackground		6
#define kwc_MenuShadowColor		7
#define kwc_VirtualForeground	8
#define kwc_VirtualBackground	9
#define kwc_RealScreenBackground		10
#define kwc_RealScreenForeground		11

#define kwm_Name			LTYPE_NAME
#define kwm_ResName			LTYPE_RES_NAME
#define kwm_ResClass			LTYPE_RES_CLASS

/*
 * The following is sorted alphabetically according to name (which must be
 * in lowercase and only contain the letters a-z).  It is fed to a binary
 * search to parse keywords.
 */
static TwmKeyword keytable[] = {
    { "all",			ALL, 0 },

    /* djhjr - 4/26/99 */
    { "appletregion",	APPLET_REGION, 0 },

    { "autopan",		NKEYWORD, kwn_AutoPan },
    { "autopanborderwidth", NKEYWORD, kwn_AutoPanBorderWidth },
    { "autopanextrawarp", NKEYWORD, kwn_AutoPanExtraWarp },
    { "autopanwarpwithrespecttorealscreen", NKEYWORD,
	  kwn_AutoPanWarpWithRespectToRealScreen },
    { "autoraise",		AUTO_RAISE, 0 },
	{ "autoraisedelay",       NKEYWORD, kwn_RaiseDelay },
    { "autorelativeresize",	KEYWORD, kw0_AutoRelativeResize },
    { "benicetocolormap",	KEYWORD, kw0_BeNiceToColormap },
    { "borderbevelwidth",	NKEYWORD, kwn_BorderBevelWidth },

    { "bordercolor",		CLKEYWORD, kwcl_BorderColor },
    { "bordertilebackground",	CLKEYWORD, kwcl_BorderTileBackground },
    { "bordertileforeground",	CLKEYWORD, kwcl_BorderTileForeground },
    { "borderwidth",		NKEYWORD, kwn_BorderWidth },
    { "button",			BUTTON, 0 },
    { "buttonbevelwidth",	NKEYWORD, kwn_ButtonBevelWidth },
    { "buttoncolorisframe",	KEYWORD, kw0_ButtonColorIsFrame },
    { "buttonindent",		SNKEYWORD, kwn_ButtonIndent },
    { "c",			CONTROL, 0 },
    { "center",			JKEYWORD, J_CENTER },
    { "clearbevelcontrast",	NKEYWORD, kwn_ClearBevelContrast },
    { "clientborderwidth",	KEYWORD, kw0_ClientBorderWidth },
    { "color",			COLOR, 0 },
    { "constrainedmovetime",	NKEYWORD, kwn_ConstrainedMoveTime },
    { "control",		CONTROL, 0 },
    { "cursors",		CURSORS, 0 },
    { "d",			VIRTUAL_WIN, 0 },
    { "darkbevelcontrast",	NKEYWORD, kwn_DarkBevelContrast },
    { "decoratetransients",	KEYWORD, kw0_DecorateTransients },
    { "defaultbackground",	CKEYWORD, kwc_DefaultBackground },
    { "defaultfont",		SKEYWORD, kws_DefaultFont },
    { "defaultforeground",	CKEYWORD, kwc_DefaultForeground },
    { "defaultfunction",	DEFAULT_FUNCTION, 0 },
    { "deiconifytoscreen",      KEYWORD, kw0_DeIconifyToScreen },
    { "desktop",		VIRTUAL_WIN, 0 },
    { "desktopdisplaybackground",
	  CLKEYWORD, kwcl_VirtualDesktopBackground },
    { "desktopdisplayborder",
	  CLKEYWORD, kwcl_VirtualDesktopBorder },
    { "desktopdisplayforeground",
	  CLKEYWORD, kwcl_VirtualDesktopForeground },
    { "destroy",		KILL, 0 },

    { "dontdeiconifytransients", KEYWORD, kw0_DontDeiconifyTransients },

    { "donticonifybyunmapping",	DONT_ICONIFY_BY_UNMAPPING, 0 },
    { "dontinterpolatetitles", KEYWORD, kw0_DontInterpolateTitles },
    { "dontmoveoff",		KEYWORD, kw0_DontMoveOff },
    { "dontshowindisplay",	NO_SHOW_IN_DISPLAY, 0  },

    { "dontshowintwmwindows",	NO_SHOW_IN_TWMWINDOWS, 0  },
    { "dontshowinvtwmwindows",	NO_SHOW_IN_TWMWINDOWS, 0  },

    { "dontsqueezetitle",	DONT_SQUEEZE_TITLE, 0 },
    { "door",			DOOR, 0 },
    { "doorbackground",		CLKEYWORD, kwcl_DoorBackground },

    { "doorbevelwidth",		NKEYWORD, kwn_DoorBevelWidth },

    { "doorfont",		SKEYWORD, kws_DoorFont },
    { "doorforeground",		CLKEYWORD, kwcl_DoorForeground },
    { "doors",			DOORS, 0 },
    { "east",			DKEYWORD, D_EAST },
#ifdef TWM_USE_XFT
    { "enablexftfontrenderer",	KEYWORD, kw0_EnableXftFontRenderer },
#endif
    { "enhancedexecresources", KEYWORD, kw0_EnhancedExecResources },
    { "f",			FRAME, 0 },
    { "f.autopan",		FKEYWORD, F_AUTOPAN },
    { "f.autoraise",		FKEYWORD, F_AUTORAISE },
    { "f.backiconmgr",		FKEYWORD, F_BACKICONMGR },
    { "f.beep",			FKEYWORD, F_BEEP },
    { "f.bindbuttons",		FKEYWORD, F_BINDBUTTONS },
    { "f.bindkeys",		FKEYWORD, F_BINDKEYS },

    { "f.bottomzoom",		FKEYWORD, F_BOTTOMZOOM },
    { "f.circledown",		FKEYWORD, F_CIRCLEDOWN },
    { "f.circleup",		FKEYWORD, F_CIRCLEUP },
    { "f.colormap",		FSKEYWORD, F_COLORMAP },
    { "f.cut",			FSKEYWORD, F_CUT },
    { "f.cutfile",		FKEYWORD, F_CUTFILE },
    { "f.deiconify",		FKEYWORD, F_DEICONIFY },
    { "f.delete",		FKEYWORD, F_DELETE },
    { "f.deletedoor",		FKEYWORD, F_DELETEDOOR },
    { "f.deltastop",		FKEYWORD, F_DELTASTOP },
    { "f.destroy",		FKEYWORD, F_DESTROY },
    { "f.downiconmgr",		FKEYWORD, F_DOWNICONMGR },
    { "f.enterdoor",		FKEYWORD, F_ENTERDOOR },
    { "f.exec",			FSKEYWORD, F_EXEC },
    { "f.file",			FSKEYWORD, F_FILE },
    { "f.focus",		FKEYWORD, F_FOCUS },
    { "f.forcemove",		FKEYWORD, F_FORCEMOVE },
    { "f.forwiconmgr",		FKEYWORD, F_FORWICONMGR },
    { "f.fullzoom",		FKEYWORD, F_FULLZOOM },
    { "f.function",		FSKEYWORD, F_FUNCTION },
    { "f.hbzoom",		FKEYWORD, F_BOTTOMZOOM },
    { "f.hidedesktopdisplay",	FKEYWORD, F_HIDEDESKTOP },
    { "f.hideiconmgr",		FKEYWORD, F_HIDELIST },
    { "f.horizoom",		FKEYWORD, F_HORIZOOM },
    { "f.htzoom",		FKEYWORD, F_TOPZOOM },
    { "f.hzoom",		FKEYWORD, F_HORIZOOM },
    { "f.iconify",		FKEYWORD, F_ICONIFY },
    { "f.identify",		FKEYWORD, F_IDENTIFY },
    { "f.lefticonmgr",		FKEYWORD, F_LEFTICONMGR },
    { "f.leftzoom",		FKEYWORD, F_LEFTZOOM },
    { "f.lower",		FKEYWORD, F_LOWER },
    { "f.menu",			FSKEYWORD, F_MENU },
    { "f.move",			FKEYWORD, F_MOVE },
    { "f.movescreen",		FKEYWORD, F_MOVESCREEN },
    { "f.nail",			FKEYWORD, F_NAIL },
    { "f.nailedabove", FKEYWORD, F_STICKYABOVE },

    { "f.namedoor",		FKEYWORD, F_NAMEDOOR },

    { "f.newdoor",		FKEYWORD, F_NEWDOOR },
    { "f.nexticonmgr",		FKEYWORD, F_NEXTICONMGR },
    { "f.nop",			FKEYWORD, F_NOP },
    { "f.pandown",		FSKEYWORD, F_PANDOWN },
    { "f.panleft",		FSKEYWORD, F_PANLEFT },
    { "f.panright",		FSKEYWORD, F_PANRIGHT },
    { "f.panup",		FSKEYWORD, F_PANUP },

    { "f.playsound",		FSKEYWORD, F_PLAYSOUND },

    { "f.previconmgr",		FKEYWORD, F_PREVICONMGR },
    { "f.quit",			FKEYWORD, F_QUIT },
    { "f.raise",		FKEYWORD, F_RAISE },
    { "f.raiselower",		FKEYWORD, F_RAISELOWER },
    { "f.refresh",		FKEYWORD, F_REFRESH },
    { "f.resetdesktop",		FKEYWORD, F_RESETDESKTOP },
    { "f.resize",		FKEYWORD, F_RESIZE },
    { "f.restart",		FKEYWORD, F_RESTART },
    { "f.righticonmgr",		FKEYWORD, F_RIGHTICONMGR },
    { "f.rightzoom",		FKEYWORD, F_RIGHTZOOM },
    { "f.ring",		FKEYWORD, F_RING },
    { "f.saveyourself",		FKEYWORD, F_SAVEYOURSELF },

    { "f.separator",		FKEYWORD, F_SEPARATOR },

    { "f.setrealscreen",	FSKEYWORD, F_SETREALSCREEN },
    { "f.showdesktopdisplay",	FKEYWORD, F_SHOWDESKTOP },
    { "f.showiconmgr",		FKEYWORD, F_SHOWLIST },
#ifdef TWM_USE_SLOPPYFOCUS
    { "f.sloppyfocus",		FKEYWORD, F_SLOPPYFOCUS },
#endif
    { "f.snap",			FKEYWORD, F_SNAP },
    { "f.snaprealscreen",			FKEYWORD, F_SNAPREALSCREEN },
    { "f.snugdesktop",        FKEYWORD, F_SNUGDESKTOP },
    { "f.snugwindow",     FKEYWORD, F_SNUGWINDOW },
    { "f.sorticonmgr",		FKEYWORD, F_SORTICONMGR },

#ifndef NO_SOUND_SUPPORT
    { "f.sounds",		FKEYWORD, F_SOUNDS },
#endif

    { "f.source",		FSKEYWORD, F_BEEP },  /* XXX - don't work */
    { "f.squeezecenter",		FKEYWORD, F_SQUEEZECENTER },
    { "f.squeezeleft",		FKEYWORD, F_SQUEEZELEFT },
    { "f.squeezeright",		FKEYWORD, F_SQUEEZERIGHT },

    { "f.startwm",		FSKEYWORD, F_STARTWM },

	{ "f.staticiconpositions",	FKEYWORD, F_STATICICONPOSITIONS },

    { "f.stick",			FKEYWORD, F_NAIL },
    { "f.stickyabove", FKEYWORD, F_STICKYABOVE },

    { "f.stricticonmgr",	FKEYWORD, F_STRICTICONMGR },

    { "f.title",		FKEYWORD, F_TITLE },
    { "f.topzoom",		FKEYWORD, F_TOPZOOM },
    { "f.twmrc",		FKEYWORD, F_RESTART },

    { "f.unbindbuttons",	FKEYWORD, F_UNBINDBUTTONS },
    { "f.unbindkeys",		FKEYWORD, F_UNBINDKEYS },

    { "f.unfocus",		FKEYWORD, F_UNFOCUS },
    { "f.upiconmgr",		FKEYWORD, F_UPICONMGR },
    { "f.version",		FKEYWORD, F_VERSION },
    { "f.virtualgeometries",	FKEYWORD, F_VIRTUALGEOMETRIES },
    { "f.vlzoom",		FKEYWORD, F_LEFTZOOM },
    { "f.vrzoom",		FKEYWORD, F_RIGHTZOOM },
    { "f.warp",			FKEYWORD, F_WARP },
    { "f.warpclassnext",	FSKEYWORD, F_WARPCLASSNEXT },
    { "f.warpclassprev",	FSKEYWORD, F_WARPCLASSPREV },
    { "f.warpring",		FSKEYWORD, F_WARPRING },

    { "f.warpsnug",        FKEYWORD, F_WARPSNUG },

    { "f.warpto",		FSKEYWORD, F_WARPTO },
    { "f.warptoiconmgr",	FSKEYWORD, F_WARPTOICONMGR },
    { "f.warptonewest", FKEYWORD, F_WARPTONEWEST },
    { "f.warptoscreen",		FSKEYWORD, F_WARPTOSCREEN },

    { "f.warpvisible",		FKEYWORD, F_WARPVISIBLE },

    { "f.winrefresh",		FKEYWORD, F_WINREFRESH },
    { "f.zoom",			FKEYWORD, F_ZOOM },
    { "f.zoomzoom",			FKEYWORD, F_ZOOMZOOM },

    { "fixmanagedvirtualgeometries", KEYWORD, kw0_FixManagedVirtualGeometries },

    { "fixtransientvirtualgeometries", KEYWORD,
	kw0_FixTransientVirtualGeometries },
    { "forceicons",		KEYWORD, kw0_ForceIcons },
    { "frame",			FRAME, 0 },
    { "framepadding",		NKEYWORD, kwn_FramePadding },
    { "function",		FUNCTION, 0 },
    { "i",			ICON, 0 },
    { "icon",			ICON, 0 },
    { "iconbackground",		CLKEYWORD, kwcl_IconBackground },

    { "iconbevelwidth",	NKEYWORD, kwn_IconBevelWidth },

    { "iconbordercolor",	CLKEYWORD, kwcl_IconBorderColor },
    { "iconborderwidth",	NKEYWORD, kwn_IconBorderWidth },
    { "icondirectory",		SKEYWORD, kws_IconDirectory },
    { "iconfont",		SKEYWORD, kws_IconFont },
    { "iconforeground",		CLKEYWORD, kwcl_IconForeground },
    { "iconifybyunmapping",	ICONIFY_BY_UNMAPPING, 0 },
    { "iconmanagerbackground",	CLKEYWORD, kwcl_IconManagerBackground },

    { "iconmanagerbevelwidth",	NKEYWORD, kwn_IconManagerBevelWidth },

    { "iconmanagerdontshow",	ICONMGR_NOSHOW, 0 },
    { "iconmanagerfont",	SKEYWORD, kws_IconManagerFont },
    { "iconmanagerforeground",	CLKEYWORD, kwcl_IconManagerForeground },
    { "iconmanagergeometry",	ICONMGR_GEOMETRY, 0 },
    { "iconmanagerhighlight",	CLKEYWORD, kwcl_IconManagerHighlight },

    { "iconmanagerpixmap",	ICONMGRICONMAP, 0 },

    { "iconmanagers",		ICONMGRS, 0 },
    { "iconmanagershow",	ICONMGR_SHOW, 0 },
    { "iconmgr",		ICONMGR, 0 },
#ifdef TWM_USE_OPACITY
    { "iconopacity",		NKEYWORD, kwn_IconOpacity },
#endif
    { "iconregion",		ICON_REGION, 0 },
    { "icons",			ICONS, 0 },

    { "ignoremodifiers",	IGNORE_MODS, 0 },

    { "infobevelwidth",	NKEYWORD, kwn_InfoBevelWidth },

    { "infofont",		SKEYWORD, kws_InfoFont },

    { "interpolatemenucolors",	KEYWORD, kw0_InterpolateMenuColors },
    { "l",			LOCK, 0 },
    { "left",			JKEYWORD, J_LEFT },
    { "lefttitlebutton",	LEFT_TITLEBUTTON, 0 },
    { "lessrandomzoomzoom", KEYWORD, kw0_LessRandomZoomZoom },
    { "lock",			LOCK, 0 },
    { "m",			META, 0 },
    { "maketitle",		MAKE_TITLE, 0 },
    { "maxwindowsize",		SKEYWORD, kws_MaxWindowSize },
    { "menu",			MENU, 0 },
    { "menubackground",		CKEYWORD, kwc_MenuBackground },

    { "menubevelwidth",	NKEYWORD, kwn_MenuBevelWidth },

    { "menufont",		SKEYWORD, kws_MenuFont },
    { "menuforeground",		CKEYWORD, kwc_MenuForeground },

    { "menuiconpixmap",		MENUICONMAP, 0 },

#ifdef TWM_USE_OPACITY
    { "menuopacity",		NKEYWORD, kwn_MenuOpacity },
#endif
    { "menuscrollborderwidth",	NKEYWORD, kwn_MenuScrollBorderWidth },
    { "menuscrolljump",		NKEYWORD, kwn_MenuScrollJump },

    { "menushadowcolor",	CKEYWORD, kwc_MenuShadowColor },
    { "menutitlebackground",	CKEYWORD, kwc_MenuTitleBackground },
    { "menutitlefont", SKEYWORD, kws_MenuTitleFont },
    { "menutitleforeground",	CKEYWORD, kwc_MenuTitleForeground },
    { "meta",			META, 0 },
    { "mod",			META, 0 },  /* fake it */
    { "monochrome",		MONOCHROME, 0 },
    { "move",			MOVE, 0 },
    { "movedelta",		NKEYWORD, kwn_MoveDelta },
    { "nailedabove",    KEYWORD, kw0_StickyAbove },
    { "naileddown",		NAILEDDOWN, 0},

    { "name",			MKEYWORD, kwm_Name },

    { "naturalautopanbehavior", KEYWORD,
	kw0_NaturalAutopanBehavior },
    { "nobackingstore",		KEYWORD, kw0_NoBackingStore },

    { "noborder",		NO_BORDER, 0 },

    { "noborderdecorations",	KEYWORD, kw0_NoBorderDecorations },

    { "nocasesensitive",	KEYWORD, kw0_NoCaseSensitive },
    { "nodefaultmouseorkeyboardbindings", KEYWORD,
	kw0_NoDefaultMouseOrKeyboardBindings },
    { "nodefaults",		KEYWORD, kw0_NoDefaults },
    { "nodefaulttitlebuttons", KEYWORD, kw0_NoDefaultTitleButtons },
    { "nograbserver",		KEYWORD, kw0_NoGrabServer },
    { "nohighlight",		NO_HILITE, 0 },
    { "noiconifyiconmanagers",	KEYWORD, kw0_NoIconifyIconManagers },

    { "noiconmanagerfocus",		KEYWORD, kw0_NoIconManagerFocus },

    { "noiconmanagerhighlight",		NO_ICONMGR_HILITE, 0 },

    { "noiconmanagers",		KEYWORD, kw0_NoIconManagers },
    { "nomenushadows",		KEYWORD, kw0_NoMenuShadows },

    { "noopaquemove",		NO_OPAQUE_MOVE, 0 },
	{ "noopaqueresize",		NO_OPAQUE_RESIZE, 0 },

    { "noprettytitles",		KEYWORD, kw0_NoPrettyTitles },

    { "noraiseondeiconify",	KEYWORD, kw0_NoRaiseOnDeiconify },
    { "noraiseonmove",		KEYWORD, kw0_NoRaiseOnMove },
    { "noraiseonresize",	KEYWORD, kw0_NoRaiseOnResize },
    { "noraiseonwarp",		KEYWORD, kw0_NoRaiseOnWarp },
    { "north",			DKEYWORD, D_NORTH },
    { "nosaveunders",		KEYWORD, kw0_NoSaveUnders },
    { "nostackmode",		NO_STACKMODE, 0 },
    { "notitle",		NO_TITLE, 0 },
    { "notitlefocus",		KEYWORD, kw0_NoTitleFocus },
    { "notitlehighlight",	NO_TITLE_HILITE, 0 },
    { "notvirtualgeometries",	KEYWORD, kw0_NotVirtualGeometries },
    { "noversion",		KEYWORD, kw0_NoVersion },

    { "nowindowring",		NO_WINDOW_RING, 0 },

	{ "oldfashionedtwmwindowsmenu", KEYWORD,
			kw0_OldFashionedTwmWindowsMenu },

	{ "oldfashionedvtwmwindowsmenu", KEYWORD,
			kw0_OldFashionedTwmWindowsMenu },

    { "opaquemove",			OPAQUE_MOVE, 0 },

	{ "opaqueresize",		OPAQUE_RESIZE, 0 },

    { "pandistancex",		NKEYWORD, kwn_PanDistanceX },
    { "pandistancey",		NKEYWORD, kwn_PanDistanceY },

    { "panresistance",		NKEYWORD, kwn_PanResistance },

    { "pauseonexit",		NKEYWORD, kwn_PauseOnExit },
    { "pauseonquit",		NKEYWORD, kwn_PauseOnQuit },

	{ "pixmaps",		PIXMAPS, 0 },
    { "pointerplacement",	KEYWORD, kw0_PointerPlacement },
	{ "prettyzoom", KEYWORD, kw0_PrettyZoom },
    { "r",			ROOT, 0 },
	{ "raisedelay",       NKEYWORD, kwn_RaiseDelay },

    { "raiseonstart",		KEYWORD, kw0_RaiseOnStart },

    { "randomplacement",	KEYWORD, kw0_RandomPlacement },
    { "realscreenbackground", CKEYWORD, kwc_RealScreenBackground },
    { "realscreenborderwidth", NKEYWORD, kwn_RealScreenBorderWidth },
    { "realscreenforeground", CKEYWORD, kwc_RealScreenForeground },
    { "realscreenpixmap",		REALSCREENMAP, 0 },

    { "resclass",		MKEYWORD, kwm_ResClass },

    { "resize",			RESIZE, 0 },
    { "resizefont",		SKEYWORD, kws_ResizeFont },

    { "resizeregion",		SKEYWORD, kws_ResizeRegion },

    { "resname",		MKEYWORD, kwm_ResName },

#ifdef TWM_USE_XRANDR
    { "restartonscreenchangenotify", KEYWORD, kw0_RestartOnScreenChangeNotify },
#endif
    { "restartpreviousstate",	KEYWORD, kw0_RestartPreviousState },
    { "rhspulldownmenus", KEYWORD, kw0_RightHandSidePulldownMenus },
    { "right",			JKEYWORD, J_RIGHT },
    { "righthandsidepulldownmenus", KEYWORD, kw0_RightHandSidePulldownMenus },
    { "righttitlebutton",	RIGHT_TITLEBUTTON, 0 },
    { "root",			ROOT, 0 },
    { "s",			SHIFT, 0 },
    { "savecolor",              SAVECOLOR, 0},
    { "select",			SELECT, 0 },

    { "shallowreliefwindowbutton",	KEYWORD, kw0_ShallowReliefWindowButton },

    { "shift",			SHIFT, 0 },
    { "showiconmanager",	KEYWORD, kw0_ShowIconManager },
#ifdef TWM_USE_SLOPPYFOCUS
    { "sloppyfocus",		KEYWORD, kw0_SloppyFocus },
#endif
    { "snaprealscreen",		KEYWORD, kw0_SnapRealScreen },
    { "sorticonmanager",	KEYWORD, kw0_SortIconManager },

#ifndef NO_SOUND_SUPPORT
    { "soundhost",		SKEYWORD, kws_SoundHost },
    { "sounds",			SOUNDS, 0 },
    { "soundvolume",		NKEYWORD, kwn_SoundVolume },
#endif

    { "south",			DKEYWORD, D_SOUTH },
    { "squeezetitle",		SQUEEZE_TITLE, 0 },
    { "starticonified",		START_ICONIFIED, 0 },

    { "staticiconpositions",	KEYWORD, kw0_StaticIconPositions },

    { "stayupmenus",		KEYWORD, kw0_StayUpMenus },
    { "stayupoptionalmenus",	KEYWORD, kw0_StayUpOptionalMenus },
    { "sticky",             NAILEDDOWN, 0 },
    { "stickyabove",        KEYWORD, kw0_StickyAbove },

    { "stricticonmanager",	KEYWORD, kw0_StrictIconManager },

    { "t",			TITLE, 0 },

    { "title",			TITLE, 0 },
    { "titlebackground",	CLKEYWORD, kwcl_TitleBackground },

    { "titlebevelwidth",	NKEYWORD, kwn_TitleBevelWidth },

    { "titlebuttonborderwidth",	NKEYWORD, kwn_TitleButtonBorderWidth },
    { "titlefont",		SKEYWORD, kws_TitleFont },
    { "titleforeground",	CLKEYWORD, kwcl_TitleForeground },
    { "titlehighlight",		TITLE_HILITE, 0 },
    { "titlepadding",		NKEYWORD, kwn_TitlePadding },
    { "unknownicon",		SKEYWORD, kws_UnknownIcon },
    { "usepposition",		USE_PPOSITION, 0 },
    { "v",			VIRTUAL, 0 },
    { "virtual",		VIRTUAL, 0 },
    { "virtualbackground",	CKEYWORD, kwc_VirtualBackground },
    { "virtualbackgroundpixmap",		VIRTUALMAP, 0 },
    { "virtualdesktop",		VIRTUALDESKTOP, 0 },

    { "virtualdesktopbevelwidth",	NKEYWORD, kwn_VirtualDesktopBevelWidth },

    { "virtualdesktopfont",	SKEYWORD, kws_VirtualFont },
    { "virtualforeground",	CKEYWORD, kwc_VirtualForeground },

	{ "virtualreceivesmotionevents", KEYWORD,
			kw0_VirtualReceivesMotionEvents },
	{ "virtualsendsmotionevents", KEYWORD,
			kw0_VirtualSendsMotionEvents },

    { "w",			WINDOW, 0 },
    { "wait",			WAIT, 0 },

    { "warpcentered",		WARP_CENTERED, 0 },

    { "warpcursor",		WARP_CURSOR, 0 },
    { "warpsnug",		KEYWORD, kw0_WarpSnug },
    { "warptolocaltransients",	KEYWORD, kw0_WarpToLocalTransients },
    { "warptotransients",	KEYWORD, kw0_WarpToTransients },
    { "warpunmapped",		KEYWORD, kw0_WarpUnmapped },

    { "warpvisible",		KEYWORD, kw0_WarpVisible },

    { "warpwindows",		KEYWORD, kw0_WarpWindows },
    { "west",			DKEYWORD, D_WEST },
    { "window",			WINDOW, 0 },
    { "windowfunction",		WINDOW_FUNCTION, 0 },
    { "windowring",		WINDOW_RING, 0 },
    { "xorvalue",		NKEYWORD, kwn_XorValue },
    { "zoom",			ZOOM, 0 },

    { "zoomzoom",		KEYWORD, kw0_ZoomZoom },
};

static int numkeywords = (sizeof(keytable)/sizeof(keytable[0]));

int 
parse_keyword (char *s, int *nump)
{
    register int lower = 0, upper = numkeywords - 1;

    XmuCopyISOLatin1Lowered (s, s);
    while (lower <= upper) {
	int middle = (lower + upper) / 2;
	TwmKeyword *p = &keytable[middle];
	int res = strcmp (p->name, s);

	if (res < 0) {
	    lower = middle + 1;
	} else if (res == 0) {
	    *nump = p->subnum;
	    return p->value;
	} else {
	    upper = middle - 1;
	}
    }
    return ERRORTOKEN;
}



/*
 * action routines called by grammar
 */

int 
do_single_keyword (int keyword)
{
    switch (keyword) {
      case kw0_NoDefaults:
	Scr->NoDefaultMouseOrKeyboardBindings = TRUE;
	Scr->NoDefaultTitleButtons = TRUE;
	return 1;

      case kw0_NoDefaultMouseOrKeyboardBindings:
	Scr->NoDefaultMouseOrKeyboardBindings = TRUE;
	return 1;

      case kw0_NoDefaultTitleButtons:
	Scr->NoDefaultTitleButtons = TRUE;
	return 1;

	case kw0_StayUpMenus:
		if (Scr->FirstTime) Scr->StayUpMenus = TRUE;
		return 1;

	case kw0_StayUpOptionalMenus:
		if (Scr->FirstTime) Scr->StayUpOptionalMenus = Scr->StayUpMenus = TRUE;
		return 1;


	case kw0_OldFashionedTwmWindowsMenu:
		Scr->OldFashionedTwmWindowsMenu = TRUE;
		return 1;

      case kw0_AutoRelativeResize:
	Scr->AutoRelativeResize = TRUE;
	return 1;

      case kw0_ForceIcons:
	if (Scr->FirstTime) Scr->ForceIcon = TRUE;
	return 1;

      case kw0_NoIconManagers:
	Scr->NoIconManagers = TRUE;
	return 1;

      case kw0_NoIconifyIconManagers:
	Scr->NoIconifyIconManagers = TRUE;
	return 1;

      case kw0_InterpolateMenuColors:
	if (Scr->FirstTime) Scr->InterpolateMenuColors = TRUE;
	return 1;

      case kw0_NoVersion:
	/* obsolete */
	return 1;

      case kw0_SortIconManager:
	if (Scr->FirstTime) Scr->SortIconMgr = TRUE;
	return 1;

      case kw0_NoGrabServer:
	Scr->NoGrabServer = TRUE;
	return 1;

      case kw0_NoMenuShadows:
	if (Scr->FirstTime) Scr->Shadow = FALSE;
	return 1;

      case kw0_NoRaiseOnMove:
	if (Scr->FirstTime) Scr->NoRaiseMove = TRUE;
	return 1;

      case kw0_NoRaiseOnResize:
	if (Scr->FirstTime) Scr->NoRaiseResize = TRUE;
	return 1;

      case kw0_NoRaiseOnDeiconify:
	if (Scr->FirstTime) Scr->NoRaiseDeicon = TRUE;
	return 1;

      case kw0_DontMoveOff:
	Scr->DontMoveOff = TRUE;
	return 1;

      case kw0_NoBackingStore:
	Scr->BackingStore = FALSE;
	return 1;

      case kw0_NoSaveUnders:
	Scr->SaveUnder = FALSE;
	return 1;

      case kw0_RestartPreviousState:
	RestartPreviousState = True;
	return 1;

      case kw0_ClientBorderWidth:
	if (Scr->FirstTime) Scr->ClientBorderWidth = TRUE;
	return 1;

      case kw0_NoTitleFocus:
	Scr->TitleFocus = FALSE;
	return 1;

      case kw0_RandomPlacement:
	Scr->RandomPlacement = TRUE;
	return 1;

      case kw0_PointerPlacement:
	Scr->PointerPlacement = TRUE;
	return 1;

      case kw0_DecorateTransients:
	Scr->DecorateTransients = TRUE;
	return 1;

      case kw0_WarpToTransients:
	Scr->WarpToTransients = TRUE;
	return 1;

      case kw0_WarpToLocalTransients:
	Scr->WarpToLocalTransients = TRUE;
	return 1;

      case kw0_ShowIconManager:
	Scr->ShowIconManager = TRUE;
	return 1;

      case kw0_NoCaseSensitive:
	Scr->CaseSensitive = FALSE;
	return 1;

      case kw0_NoRaiseOnWarp:
	Scr->NoRaiseWarp = TRUE;
	return 1;

      case kw0_WarpUnmapped:
	Scr->WarpUnmapped = TRUE;
	return 1;

      case kw0_DeIconifyToScreen:
	Scr->DeIconifyToScreen = TRUE;
	return 1;

      case kw0_WarpWindows:
	Scr->WarpWindows = TRUE;
	return 1;

      case kw0_SnapRealScreen:
	Scr->snapRealScreen = TRUE;
	return 1;

      case kw0_NotVirtualGeometries:
	Scr->GeometriesAreVirtual = FALSE;
	return 1;

	case kw0_NaturalAutopanBehavior:
		Scr->AutoPanWarpWithRespectToRealScreen = 100;
		return 1;
	case kw0_EnhancedExecResources:
		Scr->EnhancedExecResources = TRUE;
		return 1;
	case kw0_RightHandSidePulldownMenus:
		Scr->RightHandSidePulldownMenus = TRUE;
		return 1;
	case kw0_LessRandomZoomZoom:
		Scr->LessRandomZoomZoom = TRUE;
		return 1;
	case kw0_PrettyZoom:
		Scr->PrettyZoom = TRUE;
		return 1;
	case kw0_StickyAbove:
		Scr->StickyAbove = TRUE;
		return 1;
	case kw0_DontInterpolateTitles:
		Scr->DontInterpolateTitles = TRUE;
		return 1;

	case kw0_FixManagedVirtualGeometries:
		Scr->FixManagedVirtualGeometries = TRUE;
		return 1;

	case kw0_FixTransientVirtualGeometries:
		Scr->FixTransientVirtualGeometries = TRUE;
		return 1;
	case kw0_WarpSnug:
		Scr->WarpSnug = TRUE;
		return 1;

	case kw0_ShallowReliefWindowButton:
		Scr->ShallowReliefWindowButton = 1;
		return 1;

	case kw0_ButtonColorIsFrame:
		Scr->ButtonColorIsFrame = TRUE;
		return 1;

	case kw0_BeNiceToColormap:
		Scr->BeNiceToColormap = TRUE;
		return 1;

	case kw0_VirtualReceivesMotionEvents:
		Scr->VirtualReceivesMotionEvents = TRUE;
		return 1;
	case kw0_VirtualSendsMotionEvents:
		Scr->VirtualSendsMotionEvents = TRUE;
		return 1;

	case kw0_NoIconManagerFocus:
		Scr->IconManagerFocus = FALSE;
		return 1;

	case kw0_StaticIconPositions:
		Scr->StaticIconPositions = TRUE;
		return 1;

	case kw0_NoPrettyTitles:
		Scr->NoPrettyTitles = TRUE;
		return 1;

	case kw0_DontDeiconifyTransients:
		Scr->DontDeiconifyTransients = TRUE;
		return 1;

	case kw0_WarpVisible:
		Scr->WarpVisible = TRUE;
		return 1;

	case kw0_StrictIconManager:
		Scr->StrictIconManager = TRUE;
		return 1;

	case kw0_ZoomZoom:
		Scr->ZoomZoom = TRUE;
		return 1;

	case kw0_NoBorderDecorations:
		Scr->NoBorderDecorations = TRUE;
		return 1;

	case kw0_RaiseOnStart:
		Scr->RaiseOnStart = TRUE;
		return 1;

#ifdef TWM_USE_XFT
	case kw0_EnableXftFontRenderer:
		if (Scr->use_xft == 0) /*Xrender available?*/
		    Scr->use_xft = +1;
		return 1;
#endif
#ifdef TWM_USE_SLOPPYFOCUS
	case kw0_SloppyFocus:
		SloppyFocus = TRUE;
		return 1;
#endif
#ifdef TWM_USE_XRANDR
	case kw0_RestartOnScreenChangeNotify:
		Scr->RRScreenChangeRestart = TRUE;
		return 1;
#endif
    }

    return 0;
}


int 
do_string_keyword (int keyword, char *s)
{
    if (s == NULL || s[0] == '\0')
	return 0;

    switch (keyword) {

      case kws_IconFont:
	if (!Scr->HaveFonts) Scr->IconFont.name = s;
	return 1;

      case kws_ResizeFont:
	if (!Scr->HaveFonts) Scr->SizeFont.name = s;
	return 1;

      case kws_MenuFont:
	if (!Scr->HaveFonts) Scr->MenuFont.name = s;
	return 1;

      case kws_TitleFont:
	if (!Scr->HaveFonts) Scr->TitleBarFont.name = s;
	return 1;

      case kws_IconManagerFont:
	if (!Scr->HaveFonts) Scr->IconManagerFont.name = s;
	return 1;

      case kws_MenuTitleFont:
	if (!Scr->HaveFonts) Scr->MenuTitleFont.name = s;
	return 1;

      case kws_InfoFont:
	if (!Scr->HaveFonts) Scr->InfoFont.name = s;
	return 1;

      case kws_DefaultFont:
	if (!Scr->HaveFonts) Scr->DefaultFont.name = s;
	return 1;

      case kws_UnknownIcon:
	if (Scr->FirstTime) GetUnknownIcon (s);
	return 1;

      case kws_IconDirectory:
	if (Scr->FirstTime) Scr->IconDirectory = ExpandFilename (s);
	return 1;

      case kws_MaxWindowSize:
	JunkMask = XParseGeometry (s, &JunkX, &JunkY, &JunkWidth, &JunkHeight);
	if ((JunkMask & (WidthValue | HeightValue)) !=
	    (WidthValue | HeightValue)) {
	    twmrc_error_prefix();
	    fprintf (stderr, "bad MaxWindowSize \"%s\"\n", s);
	    return 0;
	}
	if (JunkWidth <= 0 || JunkHeight <= 0) {
	    twmrc_error_prefix();
	    fprintf (stderr, "MaxWindowSize \"%s\" must be positive\n", s);
	    return 0;
	}
	Scr->MaxWindowWidth = JunkWidth;
	Scr->MaxWindowHeight = JunkHeight;
	return 1;

      case kws_VirtualFont:
	Scr->NamesInVirtualDesktop = True;
	if (!Scr->HaveFonts) Scr->VirtualFont.name = s;
	return 1;

      case kws_DoorFont:
	if (!Scr->HaveFonts) Scr->DoorFont.name = s;
	return 1;

#ifndef NO_SOUND_SUPPORT
	case kws_SoundHost:
		if (Scr->FirstTime) SetSoundHost(s);
		return 1;
#endif

	case kws_ResizeRegion:
		XmuCopyISOLatin1Lowered (s, s);
		if (strcmp (s, "northwest") == 0)
		{
			Scr->ResizeX = R_NORTHWEST;
			return 1;
		}
		else if (strcmp (s, "northeast") == 0)
		{
			Scr->ResizeX = R_NORTHEAST;
			return 1;
		}
		if (strcmp (s, "southwest") == 0)
		{
			Scr->ResizeX = R_SOUTHWEST;
			return 1;
		}
		else if (strcmp (s, "southeast") == 0)
		{
			Scr->ResizeX = R_SOUTHEAST;
			return 1;
		}
		else if (strcmp (s, "centered") == 0)
		{
			Scr->ResizeX = R_CENTERED;
			return 1;
		}
		else
		{
			twmrc_error_prefix();
			fprintf (stderr, "Invalid ResizeRegion \"%s\"\n", s);
			return 0;
		}
	}

    return 0;
}


int 
do_number_keyword (int keyword, int num)
{
    switch (keyword) {
      case kwn_ConstrainedMoveTime:
	ConstrainedMoveTime = num;
	return 1;

	  case kwn_AutoPanBorderWidth:
		Scr->AutoPanBorderWidth = (num<1)?1:num;
		return 1;

	  case kwn_AutoPanExtraWarp:
		Scr->AutoPanExtraWarp = (num<0)?0:num;
	    return 1;

	  case kwn_RealScreenBorderWidth:
		Scr->RealScreenBorderWidth = (num < 0) ? 0 : num;
	    return 1;

      case kwn_MoveDelta:
	Scr->MoveDelta = num;
	return 1;

      case kwn_XorValue:
	if (Scr->FirstTime) Scr->XORvalue = num;
	return 1;

      case kwn_FramePadding:
	if (Scr->FirstTime) Scr->FramePadding = num;
	return 1;

      case kwn_TitlePadding:
	if (Scr->FirstTime) Scr->TitlePadding = num;
	return 1;

      case kwn_ButtonIndent:
	if (Scr->FirstTime) Scr->ButtonIndent = num;
	return 1;

      case kwn_BorderWidth:
	if (Scr->FirstTime) Scr->BorderWidth = num;
	return 1;

      case kwn_IconBorderWidth:
	if (Scr->FirstTime) Scr->IconBorderWidth = num;
	return 1;

      case kwn_TitleButtonBorderWidth:
	if (Scr->FirstTime) Scr->TBInfo.border = num;
	return 1;

      case kwn_PanDistanceX:
	if (Scr->FirstTime)
	{
		Scr->VirtualDesktopPanDistanceX = (num * Scr->MyDisplayWidth) / 100;
		if (Scr->VirtualDesktopPanDistanceX <= 0) Scr->VirtualDesktopPanDistanceX = 1;
	}
	return 1;

      case kwn_PanDistanceY:
	if (Scr->FirstTime)
	{
		Scr->VirtualDesktopPanDistanceY = (num * Scr->MyDisplayHeight) / 100;
		if (Scr->VirtualDesktopPanDistanceY <= 0) Scr->VirtualDesktopPanDistanceY = 1;
	}
	return 1;

	case kwn_PanResistance:
		if (Scr->FirstTime) Scr->VirtualDesktopPanResistance = num;
		if (Scr->VirtualDesktopPanResistance < 0)
			Scr->VirtualDesktopPanResistance = 0;
		return 1;

	case kwn_RaiseDelay: RaiseDelay = num; return 1;

      case kwn_AutoPan:
	if (Scr->FirstTime)
	{
		Scr->AutoPanX = (num * Scr->MyDisplayWidth) / 100;
		Scr->AutoPanY = (num * Scr->MyDisplayHeight) / 100;
		if (Scr->AutoPanX <= 0) Scr->AutoPanX = 1;
		if (Scr->AutoPanY <= 0) Scr->AutoPanY = 1;
	}
	return 1;

	case kwn_AutoPanWarpWithRespectToRealScreen:
		Scr->AutoPanWarpWithRespectToRealScreen = (num<0)?0:(num>100)?100:num;
		return 1;

	case kwn_ClearBevelContrast:
		if (Scr->FirstTime) Scr->ClearBevelContrast = num;
		if (Scr->ClearBevelContrast <   0) Scr->ClearBevelContrast =   0;
		if (Scr->ClearBevelContrast > 100) Scr->ClearBevelContrast = 100;
		return 1;
	case kwn_DarkBevelContrast:
		if (Scr->FirstTime) Scr->DarkBevelContrast = num;
		if (Scr->DarkBevelContrast <   0) Scr->DarkBevelContrast =   0;
		if (Scr->DarkBevelContrast > 100) Scr->DarkBevelContrast = 100;
		return 1;

	case kwn_BorderBevelWidth:
		if (Scr->FirstTime) Scr->BorderBevelWidth = num;
		if (Scr->BorderBevelWidth < 0) Scr->BorderBevelWidth = 0;
		if (Scr->BorderBevelWidth > 9) Scr->BorderBevelWidth = 9;
		return 1;
	case kwn_IconManagerBevelWidth:
		if (Scr->FirstTime) Scr->IconMgrBevelWidth = num;
		if (Scr->IconMgrBevelWidth < 0) Scr->IconMgrBevelWidth = 0;
		if (Scr->IconMgrBevelWidth > 9) Scr->IconMgrBevelWidth = 9;
		return 1;
	case kwn_InfoBevelWidth:
		if (Scr->FirstTime) Scr->InfoBevelWidth = num;
		if (Scr->InfoBevelWidth < 0) Scr->InfoBevelWidth = 0;
		if (Scr->InfoBevelWidth > 9) Scr->InfoBevelWidth = 9;
		return 1;
	case kwn_MenuBevelWidth:
		if (Scr->FirstTime) Scr->MenuBevelWidth = num;
		if (Scr->MenuBevelWidth < 0) Scr->MenuBevelWidth = 0;
		if (Scr->MenuBevelWidth > 9) Scr->MenuBevelWidth = 9;
		return 1;
	case kwn_TitleBevelWidth:
		if (Scr->FirstTime) Scr->TitleBevelWidth = num;
		if (Scr->TitleBevelWidth < 0) Scr->TitleBevelWidth = 0;
		if (Scr->TitleBevelWidth > 9) Scr->TitleBevelWidth = 9;
		return 1;

	case kwn_ButtonBevelWidth:
		if (Scr->FirstTime) Scr->ButtonBevelWidth = num;
		if (Scr->ButtonBevelWidth < 0) Scr->ButtonBevelWidth = 0;
		if (Scr->ButtonBevelWidth > 9) Scr->ButtonBevelWidth = 9;
		return 1;
	case kwn_IconBevelWidth:
		if (Scr->FirstTime) Scr->IconBevelWidth = num;
		if (Scr->IconBevelWidth < 0) Scr->IconBevelWidth = 0;
		if (Scr->IconBevelWidth > 9) Scr->IconBevelWidth = 9;
		return 1;

	case kwn_DoorBevelWidth:
		if (Scr->FirstTime) Scr->DoorBevelWidth = num;
		if (Scr->DoorBevelWidth < 0) Scr->DoorBevelWidth = 0;
		if (Scr->DoorBevelWidth > 9) Scr->DoorBevelWidth = 9;
		return 1;
	case kwn_VirtualDesktopBevelWidth:
		if (Scr->FirstTime) Scr->VirtualDesktopBevelWidth = num;
		if (Scr->VirtualDesktopBevelWidth < 0) Scr->VirtualDesktopBevelWidth = 0;
		if (Scr->VirtualDesktopBevelWidth > 9) Scr->VirtualDesktopBevelWidth = 9;
		return 1;

	case kwn_MenuScrollBorderWidth:
		if (Scr->FirstTime) Scr->MenuScrollBorderWidth = num;
		return 1;
	case kwn_MenuScrollJump:
		if (Scr->FirstTime) Scr->MenuScrollJump = num;
		return 1;

#ifndef NO_SOUND_SUPPORT
	case kwn_SoundVolume:
		if (Scr->FirstTime) SetSoundVolume(num);
		return 1;
#endif

	case kwn_PauseOnExit:
		if (Scr->FirstTime) Scr->PauseOnExit = num;
		return 1;
	case kwn_PauseOnQuit:
		if (Scr->FirstTime) Scr->PauseOnQuit = num;
		return 1;

#ifdef TWM_USE_OPACITY
	case kwn_MenuOpacity: /* clamp into range: 0 = transparent ... 255 = opaque */
		if (Scr->FirstTime) Scr->MenuOpacity = (num > 255 ? 255 : (num < 0 ? 0 : num));
		return 1;
	case kwn_IconOpacity:
		if (Scr->FirstTime) Scr->IconOpacity = (num > 255 ? 255 : (num < 0 ? 0 : num));
		return 1;
#endif
    }

    return 0;
}

name_list **
do_colorlist_keyword (int keyword, int colormode, char *s)
{
    switch (keyword) {
      case kwcl_BorderColor:
	GetColor (colormode, &Scr->BorderColor, s);
	return &Scr->BorderColorL;

      case kwcl_IconManagerHighlight:
	GetColor (colormode, &Scr->IconManagerHighlight, s);
	return &Scr->IconManagerHighlightL;

      case kwcl_BorderTileForeground:
	GetColor (colormode, &Scr->BorderTileC.fore, s);
	return &Scr->BorderTileForegroundL;

      case kwcl_BorderTileBackground:
	GetColor (colormode, &Scr->BorderTileC.back, s);
	return &Scr->BorderTileBackgroundL;

      case kwcl_TitleForeground:
	GetColor (colormode, &Scr->TitleC.fore, s);
	return &Scr->TitleForegroundL;

      case kwcl_TitleBackground:
	GetColor (colormode, &Scr->TitleC.back, s);
	return &Scr->TitleBackgroundL;

      case kwcl_IconForeground:
	GetColor (colormode, &Scr->IconC.fore, s);
	return &Scr->IconForegroundL;

      case kwcl_IconBackground:
	GetColor (colormode, &Scr->IconC.back, s);
	return &Scr->IconBackgroundL;

      case kwcl_IconBorderColor:
	GetColor (colormode, &Scr->IconBorderColor, s);
	return &Scr->IconBorderColorL;

      case kwcl_IconManagerForeground:
	GetColor (colormode, &Scr->IconManagerC.fore, s);
	return &Scr->IconManagerFL;

      case kwcl_IconManagerBackground:
	GetColor (colormode, &Scr->IconManagerC.back, s);
	return &Scr->IconManagerBL;

      case kwcl_VirtualDesktopForeground:
	GetColor (colormode, &Scr->VirtualDesktopDisplayC.fore, s);
	return &Scr->VirtualDesktopColorFL;

      case kwcl_VirtualDesktopBackground:
	GetColor (colormode, &Scr->VirtualDesktopDisplayC.back, s);
	return &Scr->VirtualDesktopColorBL;

      case kwcl_VirtualDesktopBorder:
	GetColor (colormode, &Scr->VirtualDesktopDisplayBorder, s);
	return &Scr->VirtualDesktopColorBoL;

      case kwcl_DoorForeground:
	GetColor (colormode, &Scr->DoorC.fore, s);
	return &Scr->DoorForegroundL;

      case kwcl_DoorBackground:
	GetColor (colormode, &Scr->DoorC.back, s);
	return &Scr->DoorBackgroundL;

    }
    return NULL;
}

int 
do_color_keyword (int keyword, int colormode, char *s)
{
    switch (keyword) {
      case kwc_DefaultForeground:
	GetColor (colormode, &Scr->DefaultC.fore, s);
	return 1;

      case kwc_DefaultBackground:
	GetColor (colormode, &Scr->DefaultC.back, s);
	return 1;

      case kwc_MenuForeground:
	GetColor (colormode, &Scr->MenuC.fore, s);
	return 1;

      case kwc_MenuBackground:
	GetColor (colormode, &Scr->MenuC.back, s);
	return 1;

      case kwc_MenuTitleForeground:
	GetColor (colormode, &Scr->MenuTitleC.fore, s);
	return 1;

      case kwc_MenuTitleBackground:
	GetColor (colormode, &Scr->MenuTitleC.back, s);
	return 1;

      case kwc_MenuShadowColor:
	GetColor (colormode, &Scr->MenuShadowColor, s);
	return 1;

      case kwc_VirtualBackground:
	GetColor (colormode, &Scr->VirtualC.back, s);
	return 1;

      case kwc_VirtualForeground:
	GetColor (colormode, &Scr->VirtualC.fore, s);
	return 1;

      case kwc_RealScreenForeground:
	GetColor( colormode, &Scr->RealScreenC.fore, s);
	return 1;

      case kwc_RealScreenBackground:
	GetColor( colormode, &Scr->RealScreenC.back, s);
	return 1;

    }

    return 0;
}

/*
 * put_pixel_on_root() Save a pixel value in twm root window color property.
 */
void 
put_pixel_on_root (Pixel pixel)
{
  int           i, addPixel = 1;
  Atom          pixelAtom, retAtom;
  int           retFormat;
  unsigned long nPixels, retAfter;
  Pixel        *retProp;

  pixelAtom = XInternAtom(dpy, "_MIT_PRIORITY_COLORS", True);

  if (XGetWindowProperty(dpy, Scr->Root, pixelAtom, 0, 8192,
		     False, XA_CARDINAL, &retAtom,
		     &retFormat, &nPixels, &retAfter,
		     (unsigned char **)&retProp) != Success || retAtom == None)
      return;

  for (i=0; i< nPixels; i++)
      if (pixel == retProp[i]) addPixel = 0;

  if (addPixel)
      XChangeProperty (dpy, Scr->Root, _XA_MIT_PRIORITY_COLORS,
		       XA_CARDINAL, 32, PropModeAppend,
		       (unsigned char *)&pixel, 1);
}

/*
 * do_string_savecolor() save a color from a string in the twmrc file.
 */
void 
do_string_savecolor (int colormode, char *s)
{
  Pixel p = ULONG_MAX;
  GetColor(colormode, &p, s);
  if (p != ULONG_MAX)
  put_pixel_on_root(p);
}

/*
 * do_var_savecolor() save a color from a var in the twmrc file.
 */
typedef struct _cnode {int i; struct _cnode *next;} Cnode, *Cptr;
Cptr chead = NULL;

void 
do_var_savecolor (int key)
{
  Cptr cptrav, cpnew;
  if (!chead) {
    chead = (Cptr)malloc(sizeof(Cnode));
    chead->i = key; chead->next = NULL;
  }
  else {
    cptrav = chead;
    while (cptrav->next != NULL) { cptrav = cptrav->next; }
    cpnew = (Cptr)malloc(sizeof(Cnode));
    cpnew->i = key; cpnew->next = NULL; cptrav->next = cpnew;
  }
}

/*
 * assign_var_savecolor() traverse the var save color list placeing the pixels
 *                        in the root window property.
 */
void 
assign_var_savecolor (void)
{
  Cptr cp = chead;
  while (cp != NULL) {
    switch (cp->i) {
    case kwcl_BorderColor:
      put_pixel_on_root(Scr->BorderColor);
      break;
    case kwcl_IconManagerHighlight:
      put_pixel_on_root(Scr->IconManagerHighlight);
      break;
    case kwcl_BorderTileForeground:
      put_pixel_on_root(Scr->BorderTileC.fore);
      break;
    case kwcl_BorderTileBackground:
      put_pixel_on_root(Scr->BorderTileC.back);
      break;
    case kwcl_TitleForeground:
      put_pixel_on_root(Scr->TitleC.fore);
      break;
    case kwcl_TitleBackground:
      put_pixel_on_root(Scr->TitleC.back);
      break;
    case kwcl_IconForeground:
      put_pixel_on_root(Scr->IconC.fore);
      break;
    case kwcl_IconBackground:
      put_pixel_on_root(Scr->IconC.back);
      break;
    case kwcl_IconBorderColor:
      put_pixel_on_root(Scr->IconBorderColor);
      break;
    case kwcl_IconManagerForeground:
      put_pixel_on_root(Scr->IconManagerC.fore);
      break;
    case kwcl_IconManagerBackground:
      put_pixel_on_root(Scr->IconManagerC.back);
      break;
    }
    cp = cp->next;
  }
  if (chead) {
    free(chead);
    chead = NULL;
  }
}

void 
do_squeeze_entry (
    name_list **list,			/* squeeze or dont-squeeze list */
    char *name,				/* window name */
    int type,				/* match type */
    int justify,			/* left, center, right */
    int num,				/* signed pixel count or fraction num */
    int denom				/* signed 0 or indicates fraction denom */
)
{
    int absnum = (num < 0 ? -num : num);
    int absdenom = (denom < 0 ? -denom : denom);

    if (num < 0) {
	twmrc_error_prefix();
	fprintf (stderr, "SqueezeTitle numerator %d made positive\n", num);
    }
    if (denom < 0) {
	twmrc_error_prefix();
	fprintf (stderr, "SqueezeTitle denominator %d made positive\n", denom);
    }
    if (absnum > absdenom && denom != 0) {
	twmrc_error_prefix();
	fprintf (stderr, "SqueezeTitle fraction %d/%d outside window\n",
		 num, denom);
	return;
    }
    if (denom == 1) {
	twmrc_error_prefix();
	fprintf (stderr, "useless SqueezeTitle fraction %d/%d, assuming 0/0\n",
		 num, denom);
	absnum = 0;
	absdenom = 0;
    }

    if (HasShape) {
	SqueezeInfo *sinfo;
	sinfo = (SqueezeInfo *) malloc (sizeof(SqueezeInfo));

	if (!sinfo) {
	    twmrc_error_prefix();
	    fprintf (stderr, "unable to allocate %lu bytes for squeeze info\n",
		     sizeof(SqueezeInfo));
	    return;
	}
	sinfo->justify = justify;
	sinfo->num = absnum;
	sinfo->denom = absdenom;
	AddToList (list, name, type, (char *)sinfo);
    }
}

#ifndef NO_M4_SUPPORT
static char *
make_m4_cmdline (char *display_name, char *cp, char *m4_option)
{
  char *m4_lines[6] = {
      "m4 -DHOME='%s' -DWIDTH='%d' -DHEIGHT='%d' -DSOUND='%s' ",
      "-DPLANES='%d' -DBITS_PER_RGB='%d' -DCLASS='%s' -DXPM='%s' ",
      "-DCOLOR='%s' -DX_RESOLUTION='%d' -DY_RESOLUTION='%d' ",
      "-DREGEX='%s' -DUSER='%s' -DSERVERHOST='%s' -DCLIENTHOST='%s' ",
      "-DHOSTNAME='%s' -DTWM_TYPE='vtwm' -DVERSION='%d' ",
      "-DREVISION='%d' -DVENDOR='%s' -DRELEASE='%d'"
  };
  char *client, *server, *hostname;
  char *m4_cmdline, *colon, *vc, *env_username;
  char *is_sound, *is_xpm, *is_regex, *is_color;
  int i, client_len, opt_len = 0, cmd_len = 0, server_is_client = 0;
  struct hostent *hostname_ent;

  if (m4_option)
  {
    opt_len = strlen(m4_option);

    /* this isn't likely ever needed, but you just never know... */
    if (m4_option[0] == '\'' || m4_option[0] == '"')
    {
      if (m4_option[opt_len - 1] != '\'' && m4_option[opt_len - 1] != '"')
      {
	fprintf(stderr,"%s: badly formed user-defined m4 parameter\n", ProgramName);
	return (NULL);
      }
      else
      {
	m4_option++;
	opt_len -= 2;
      }
    }
  }

  /* the sourcing of various hostnames is stolen from tvtwm */
  for(client = NULL, client_len = 256; client_len < 1024; client_len *= 2){
    if((client = malloc(client_len + 1)) == NULL){
      fprintf(stderr,"%s: cannot allocate %d bytes for m4\n", ProgramName, client_len + 1);
      return (NULL);
    }

    client[client_len] = '\0';
    XmuGetHostname(client, client_len);

    if(client[client_len] == '\0')
       break;

    free(client);
    client = NULL;
  }

  if(client == NULL){
    fprintf(stderr, "%s: cannot get hostname for m4\n", ProgramName);
    return (NULL);
  }

  if((server = XDisplayName(display_name)) == NULL){
    fprintf(stderr, "%s: cannot get display name for m4\n", ProgramName);
    return (NULL);
  }

  /* we copy so we can modify it safely */
  server = strdup(server);

  if((colon = index(server, ':')) != NULL){
    *colon = '\0';
  }

  /* connected to :0 or unix:0 ? */
  if((server[0] == '\0') || (!strcmp(server, "unix"))){
    if (colon){
      *colon = ':';
      colon = NULL;
    }
    free(server);
    server = client;
    server_is_client = 1;
  }

  hostname_ent = gethostbyname(client);
  hostname = hostname_ent != NULL ? hostname_ent->h_name : client;

#ifdef NO_XPM_SUPPORT
  is_xpm = "No";
#else
  is_xpm = "Yes";
#endif
#ifdef NO_SOUND_SUPPORT
  is_sound = "No";
#else
  is_sound = "Yes";
#endif
#ifdef NO_REGEX_SUPPORT
  is_regex = "No";
#else
  is_regex = "Yes";
#endif

  /* assume colour visual */
  is_color = "Yes";
  switch(Scr->d_visual->class)
  {
    case(StaticGray):	vc = "StaticGray"; is_color = "No";	break;
    case(GrayScale):	vc = "GrayScale";  is_color = "No";	break;
    case(StaticColor):	vc = "StaticColor";	break;
    case(PseudoColor):	vc = "PseudoColor";	break;
    case(TrueColor):	vc = "TrueColor";	break;
    case(DirectColor):	vc = "DirectColor";	break;
    default:            vc = "NonStandard";	break;
  }

  if((env_username = getenv("LOGNAME")) == NULL){
    env_username = "";
  }

  /*
   * Start with slightly more than the minimal command line, add space for
   * nine numeric fields, ensure we have room for each of the strings, and
   * add some breathing room
   *
   * Then add space for any user-defined parameters - djhjr - 2/20/99
   */
  for (i = 0; i < 6; i++) cmd_len += strlen(m4_lines[i]);
  cmd_len += M4_MAXDIGITS * 9 + HomeLen + strlen(vc) + strlen(is_xpm) +
      strlen(is_color) + strlen(env_username) + strlen(server) +
      strlen(client) + strlen(hostname) + strlen(ServerVendor(dpy)) +
      strlen(is_sound) + strlen(is_regex) + strlen(cp) + 16;
  if (opt_len) cmd_len += opt_len;

  if((m4_cmdline = malloc(cmd_len)) == NULL){
    fprintf(stderr,"%s: cannot allocate %d bytes for m4\n", ProgramName, cmd_len);
    return (NULL);
  }

  /*
   * Non-SysV systems - specifically, BSD-derived systems - return a
   * pointer to the string, not its length.
   */
  sprintf(m4_cmdline, m4_lines[0], Home, Scr->MyDisplayWidth,
      Scr->MyDisplayHeight, is_sound);
  cmd_len = strlen(m4_cmdline);
  sprintf(m4_cmdline + cmd_len, m4_lines[1], Scr->d_depth,
      Scr->d_visual->bits_per_rgb, vc, is_xpm);
  cmd_len = strlen(m4_cmdline);
  sprintf(m4_cmdline + cmd_len, m4_lines[2], is_color,
	  Resolution(Scr->MyDisplayWidth, DisplayWidthMM(dpy, Scr->screen)),
	  Resolution(Scr->MyDisplayHeight, DisplayHeightMM(dpy, Scr->screen)));
  cmd_len = strlen(m4_cmdline);
  sprintf(m4_cmdline + cmd_len, m4_lines[3], is_regex, env_username,
	  server, client);
  cmd_len = strlen(m4_cmdline);
  sprintf(m4_cmdline + cmd_len, m4_lines[4], hostname, ProtocolVersion(dpy));
  cmd_len = strlen(m4_cmdline);
  sprintf(m4_cmdline + cmd_len, m4_lines[5], ProtocolRevision(dpy),
      ServerVendor(dpy), VendorRelease(dpy));

  cmd_len = strlen(m4_cmdline);
  if (opt_len)
  {
    sprintf(m4_cmdline + cmd_len, " %*.*s", opt_len, opt_len, m4_option);
    cmd_len = strlen(m4_cmdline);
  }
  sprintf(m4_cmdline + cmd_len, " < %s", cp);

  if (PrintErrorMessages)
    fprintf(stderr, "\n%s: %s\n", ProgramName, m4_cmdline);

  if (colon) *colon = ':';
  if (!server_is_client) free(server);
  free(client);

  return (m4_cmdline);
}
#endif /* NO_M4_SUPPORT */
