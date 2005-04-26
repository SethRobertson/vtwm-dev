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
#include <X11/Xos.h>
#include <X11/Xmu/CharSet.h>
#include "twm.h"
#include "screen.h"
#include "menus.h"
#include "util.h"
#include "gram.h"
#include "parse.h"
#include <X11/Xatom.h>

#ifndef SYSTEM_INIT_FILE
#define SYSTEM_INIT_FILE "/usr/lib/X11/twm/system.vtwmrc"
#endif
#define BUF_LEN 300

static FILE *twmrc;
static int ptr = 0;
static int len = 0;
static char buff[BUF_LEN+1];
static char overflowbuff[20];		/* really only need one */
static int overflowlen;
static char **stringListSource, *currentString;
static int ParseUsePPosition();
int RaiseDelay = 0;           /* msec, for AutoRaise *//*RAISEDELAY*/
extern int yylineno;
extern int mods;

int ConstrainedMoveTime = 400;		/* milliseconds, event times */

static int twmFileInput(), twmStringListInput();
void twmUnput();
int (*twmInputFunc)();

extern char *defTwmrc[];		/* default bindings */


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

static int doparse (ifunc, srctypename, srcname)
    int (*ifunc)();
    char *srctypename;
    char *srcname;
{
    mods = 0;
    ptr = 0;
    len = 0;
    yylineno = 1;
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


int ParseTwmrc (filename)
    char *filename;
{
    int i;
    char *home = NULL;
    int homelen = 0;
    char *cp = NULL;
    char tmpfilename[257];
    int fd;
    int to_m4[2];

    /*
     * If filename given, try it, else try ~/.vtwmrc.#, else try ~/.vtwmrc,
     * else try ~/.twmrc.#, else ~/.twmrc, else system.twmrc; finally using
     * built-in defaults.
     */
    for (twmrc = NULL, i = 0; !twmrc && i < 6; i++) {
	switch (i) {
	  case 0:			/* -f filename */
	    cp = filename;
	    break;

	  case 1:			/* ~/.vtwmrc.screennum */
	    if (!filename) {
		home = getenv ("HOME");
		if (home) {
		    homelen = strlen (home);
		    cp = tmpfilename;
		    (void) sprintf (tmpfilename, "%s/.vtwmrc.%d",
				    home, Scr->screen);
		    break;
		}
	    }
	    continue;

	  case 2:			/* ~/.vtwmrc */
	    if (home) {
		tmpfilename[homelen + 8] = '\0';
	    }
	    break;

	  case 3:			/* ~/.twmrc.screennum */
	    if (!filename) {
		home = getenv ("HOME");
		if (home) {
		    homelen = strlen (home);
		    cp = tmpfilename;
		    (void) sprintf (tmpfilename, "%s/.twmrc.%d",
				    home, Scr->screen);
		    break;
		}
	    }
	    continue;

	  case 4:			/* ~/.twmrc */
	    if (home) {
		tmpfilename[homelen + 7] = '\0';
	    }
	    break;

	  case 5:			/* system.twmrc */
	    cp = SYSTEM_INIT_FILE;
	    break;
	}

	if ((fd = open(cp, O_RDONLY)) < 0) {
	    continue;
	}
 
	/* start m4 */
	if (pipe(to_m4) < 0) {
	    perror("pipe");
	    Done();
	}
 
	switch(fork()) {
	case -1:
	    perror("fork");
	    break;
	case 0:
	    /* child */
	    {
		char **argv;
		int i = 0;
		
		argv = buildSymbolList();
		
		(void)dup2(fd, 0);
		(void)dup2(to_m4[1], 1);
		
		close(fd);
		close(to_m4[0]);
		close(to_m4[1]);
 
		execv("/usr/bin/m4", argv);
		execv("/bin/m4", argv);
                       
		perror("exec");
		exit(-1);
	    }
	default:
	    /* parent */
	    twmrc = fdopen(to_m4[0], "r");
	    
	    close(to_m4[1]);
	    close(fd);
	    break;
	}
    }

    if (twmrc) {
	int status;

	if (filename && cp != filename) {
	    fprintf (stderr,
		     "%s:  unable to open twmrc file %s, using %s instead\n",
		     ProgramName, filename, cp);
	}
	status = doparse (twmFileInput, "file", cp);
	fclose (twmrc);
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

int ParseStringList (sl)
    char **sl;
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

static int twmFileInput()
{
    if (overflowlen) return (int) overflowbuff[--overflowlen];

    while (ptr == len)
    {
	if (fgets(buff, BUF_LEN, twmrc) == NULL)
	    return 0;

	yylineno++;

	ptr = 0;
	len = strlen(buff);
    }
    return ((int)buff[ptr++]);
}

static int twmStringListInput()
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

void twmUnput (c)
    int c;
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
TwmOutput(c)
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
#define kw0_NoHighlightDesktop          30
#define kw0_Use3DMenus                  33
#define kw0_Use3DTitles                 34
#define kw0_Use3DIconManagers           35
#define kw0_SunkFocusWindowTitle        36
#define kw0_BeNiceToColormap            37
#define kw0_CenteredInfoBox             38
#define kw0_NoRaiseDesktop              39

#define kws_UsePPosition		1
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
#define kws_PixmapDirectory             12
#define kws_DefaultFont                 13
#define kws_InfoFont                    14

#define kwn_ConstrainedMoveTime		1
#define kwn_MoveDelta			2
#define kwn_XorValue			3
#define kwn_FramePadding		4
#define kwn_TitlePadding		5
#define kwn_ButtonIndent		6
#define kwn_BorderWidth			7
#define kwn_IconBorderWidth		8
#define kwn_TitleButtonBorderWidth	9
#define kwn_PanDistanceX                10
#define kwn_PanDistanceY                11
#define kwn_AutoPan			12
#define kwn_RaiseDelay                  13/*RAISEDELAY*/
#define kwn_TransientOnTop              14
#define kwn_OpaqueMoveThreshold         15
#define kwn_OpaqueResizeThreshold       16
#define kwn_ClearShadowContrast         17
#define kwn_DarkShadowContrast          18
#define kwn_VirtualDesktopBorderWidth   20

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
#define kwc_VirtualForeground	8 /*RFB VCOLOR*/
#define kwc_VirtualBackground	9 /*RFB VCOLOR*/
#define kwc_RealScreenBackground		10	/* RFB 4/92 */
#define kwc_RealScreenForeground		11	/* RFB 4/92 */


/*
 * The following is sorted alphabetically according to name (which must be
 * in lowercase and only contain the letters a-z).  It is fed to a binary
 * search to parse keywords.
 */
static TwmKeyword keytable[] = {
    { "all",			ALL, 0 },
    { "autopan",		NKEYWORD, kwn_AutoPan },
    { "autoraise",		AUTO_RAISE, 0 },
	{ "autoraisedelay",       NKEYWORD, kwn_RaiseDelay },/*RAISEDELAY*/
    { "autorelativeresize",	KEYWORD, kw0_AutoRelativeResize },
    { "benicetocolormap",       KEYWORD, kw0_BeNiceToColormap },
    { "bordercolor",		CLKEYWORD, kwcl_BorderColor },
    { "bordertilebackground",	CLKEYWORD, kwcl_BorderTileBackground },
    { "bordertileforeground",	CLKEYWORD, kwcl_BorderTileForeground },
    { "borderwidth",		NKEYWORD, kwn_BorderWidth },
    { "button",			BUTTON, 0 },
    { "buttonindent",		NKEYWORD, kwn_ButtonIndent },
    { "c",			CONTROL, 0 },
    { "center",			JKEYWORD, J_CENTER },
    { "centeredinfobox",        KEYWORD, kw0_CenteredInfoBox },
    { "clearshadowcontrast",    NKEYWORD, kwn_ClearShadowContrast },
    { "clientborderwidth",	KEYWORD, kw0_ClientBorderWidth },
    { "color",			COLOR, 0 },
    { "constrainedmovetime",	NKEYWORD, kwn_ConstrainedMoveTime },
    { "control",		CONTROL, 0 },
    { "cursors",		CURSORS, 0 },
    { "d",			VIRTUAL_WIN, 0 },
    { "darkshadowcontrast",     NKEYWORD, kwn_DarkShadowContrast },
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
    { "donticonifybyunmapping",	DONT_ICONIFY_BY_UNMAPPING, 0 },
    { "dontmoveoff",		KEYWORD, kw0_DontMoveOff },
    { "dontshowindisplay",	NO_SHOW_IN_DISPLAY, 0  },
    { "dontsqueezetitle",	DONT_SQUEEZE_TITLE, 0 },
    { "door",			DOOR, 0 },
    { "doorbackground",		CLKEYWORD, kwcl_DoorBackground },
    { "doorfont",		SKEYWORD, kws_DoorFont },
    { "doorforeground",		CLKEYWORD, kwcl_DoorForeground },
    { "doors",			DOORS, 0 },
    { "east",			DKEYWORD, D_EAST },
    { "f",			FRAME, 0 },
    { "f.autopan",		FKEYWORD, F_AUTOPAN },/*RFB F_AUTOPAN*/
    { "f.autoraise",		FKEYWORD, F_AUTORAISE },
    { "f.backiconmgr",		FKEYWORD, F_BACKICONMGR },
    { "f.beep",			FKEYWORD, F_BEEP },
    { "f.bottomzoom",		FKEYWORD, F_BOTTOMZOOM },
    { "f.circledown",		FKEYWORD, F_CIRCLEDOWN },
    { "f.circleup",		FKEYWORD, F_CIRCLEUP },
    { "f.colormap",		FSKEYWORD, F_COLORMAP },
    { "f.cut",			FSKEYWORD, F_CUT },
    { "f.cutfile",		FKEYWORD, F_CUTFILE },
    { "f.deiconify",		FKEYWORD, F_DEICONIFY },
    { "f.delete",		FKEYWORD, F_DELETE },
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
    { "f.namedoor",             FKEYWORD, F_NAME_DOOR },
    { "f.newdoor",		FKEYWORD, F_NEWDOOR },
    { "f.nexticonmgr",		FKEYWORD, F_NEXTICONMGR },
    { "f.nop",			FKEYWORD, F_NOP },
    { "f.pandown",		FSKEYWORD, F_PANDOWN },
    { "f.panleft",		FSKEYWORD, F_PANLEFT },
    { "f.panright",		FSKEYWORD, F_PANRIGHT },
    { "f.panup",		FSKEYWORD, F_PANUP },
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
    { "f.ring",		        FKEYWORD, F_RING },
    { "f.saveyourself",		FKEYWORD, F_SAVEYOURSELF },
    { "f.seperator",            FKEYWORD, F_SEPERATOR },
    { "f.setrealscreen",	FSKEYWORD, F_SETREALSCREEN },
    { "f.showdesktopdisplay",	FKEYWORD, F_SHOWDESKTOP },
    { "f.showiconmgr",		FKEYWORD, F_SHOWLIST },
    { "f.snap",			FKEYWORD, F_SNAP },
    { "f.snugdesktop",          FKEYWORD, F_SNUGDESKTOP },
    { "f.snugwindow",           FKEYWORD, F_SNUGWINDOW },
    { "f.sorticonmgr",		FKEYWORD, F_SORTICONMGR },
    { "f.source",		FSKEYWORD, F_BEEP },  /* XXX - don't work */
    { "f.squeezecenter",		FKEYWORD, F_SQUEEZECENTER },/*RFB SQUEEZE*/
    { "f.squeezeleft",		FKEYWORD, F_SQUEEZELEFT },/*RFB SQUEEZE*/
    { "f.squeezeright",		FKEYWORD, F_SQUEEZERIGHT },/*RFB SQUEEZE*/
    { "f.title",		FKEYWORD, F_TITLE },
    { "f.topzoom",		FKEYWORD, F_TOPZOOM },
    { "f.twmrc",		FKEYWORD, F_RESTART },
    { "f.unfocus",		FKEYWORD, F_UNFOCUS },
    { "f.upiconmgr",		FKEYWORD, F_UPICONMGR },
    { "f.version",		FKEYWORD, F_VERSION },
    { "f.vlzoom",		FKEYWORD, F_LEFTZOOM },
    { "f.vrzoom",		FKEYWORD, F_RIGHTZOOM },
    { "f.warpring",		FSKEYWORD, F_WARPRING },
    { "f.warpto",		FSKEYWORD, F_WARPTO },
    { "f.warptoiconmgr",	FSKEYWORD, F_WARPTOICONMGR },
    { "f.warptoscreen",		FSKEYWORD, F_WARPTOSCREEN },
    { "f.winrefresh",		FKEYWORD, F_WINREFRESH },
    { "f.zoom",			FKEYWORD, F_ZOOM },
    { "forceicons",		KEYWORD, kw0_ForceIcons },
    { "frame",			FRAME, 0 },
    { "framepadding",		NKEYWORD, kwn_FramePadding },
    { "function",		FUNCTION, 0 },
    { "i",			ICON, 0 },
    { "icon",			ICON, 0 },
    { "iconbackground",		CLKEYWORD, kwcl_IconBackground },
    { "iconbordercolor",	CLKEYWORD, kwcl_IconBorderColor },
    { "iconborderwidth",	NKEYWORD, kwn_IconBorderWidth },
    { "icondirectory",		SKEYWORD, kws_IconDirectory },
    { "iconfont",		SKEYWORD, kws_IconFont },
    { "iconforeground",		CLKEYWORD, kwcl_IconForeground },
    { "iconifybyunmapping",	ICONIFY_BY_UNMAPPING, 0 },
    { "iconmanagerbackground",	CLKEYWORD, kwcl_IconManagerBackground },
    { "iconmanagerdontshow",	ICONMGR_NOSHOW, 0 },
    { "iconmanagerfont",	SKEYWORD, kws_IconManagerFont },
    { "iconmanagerforeground",	CLKEYWORD, kwcl_IconManagerForeground },
    { "iconmanagergeometry",	ICONMGR_GEOMETRY, 0 },
    { "iconmanagerhighlight",	CLKEYWORD, kwcl_IconManagerHighlight },
    { "iconmanagers",		ICONMGRS, 0 },
    { "iconmanagershow",	ICONMGR_SHOW, 0 },
    { "iconmgr",		ICONMGR, 0 },
    { "iconregion",		ICON_REGION, 0 },
    { "icons",			ICONS, 0 },
    { "infofont",               SKEYWORD, kws_InfoFont },
    { "interpolatemenucolors",	KEYWORD, kw0_InterpolateMenuColors },
    { "l",			LOCK, 0 },
    { "left",			JKEYWORD, J_LEFT },
    { "lefttitlebutton",	LEFT_TITLEBUTTON, 0 },
    { "lock",			LOCK, 0 },
    { "m",			META, 0 },
    { "maketitle",		MAKE_TITLE, 0 },
    { "maxwindowsize",		SKEYWORD, kws_MaxWindowSize },
    { "menu",			MENU, 0 },
    { "menubackground",		CKEYWORD, kwc_MenuBackground },
    { "menufont",		SKEYWORD, kws_MenuFont },
    { "menuforeground",		CKEYWORD, kwc_MenuForeground },
    { "menushadowcolor",	CKEYWORD, kwc_MenuShadowColor },
    { "menutitlebackground",	CKEYWORD, kwc_MenuTitleBackground },
    { "menutitleforeground",	CKEYWORD, kwc_MenuTitleForeground },
    { "meta",			META, 0 },
    { "mod",			META, 0 },  /* fake it */
    { "monochrome",		MONOCHROME, 0 },
    { "move",			MOVE, 0 },
    { "movedelta",		NKEYWORD, kwn_MoveDelta },
    { "naileddown",		NAILEDDOWN, 0},
    { "nobackingstore",		KEYWORD, kw0_NoBackingStore },
    { "nocasesensitive",	KEYWORD, kw0_NoCaseSensitive },
    { "nodefaults",		KEYWORD, kw0_NoDefaults },
    { "nograbserver",		KEYWORD, kw0_NoGrabServer },
    { "nohighlight",		NO_HILITE, 0 },
    { "nohighlightindesktop",   KEYWORD, kw0_NoHighlightDesktop },
    { "noiconmanagers",		KEYWORD, kw0_NoIconManagers },
    { "noicontitle",            NO_ICON_TITLE, 0  },
    { "nomenushadows",		KEYWORD, kw0_NoMenuShadows },
    { "noopaquemove",           NOOPAQUEMOVE, 0 },
    { "noopaqueresize",         NOOPAQUERESIZE, 0 },
    { "noraisedesktoponmove",   KEYWORD, kw0_NoRaiseDesktop },
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
    { "opaquemove",             OPAQUEMOVE, 0 },
    { "opaquemovethreshold",    NKEYWORD, kwn_OpaqueMoveThreshold },
    { "opaqueresize",           OPAQUERESIZE, 0 },
    { "opaqueresizethreshold",  NKEYWORD, kwn_OpaqueResizeThreshold },
    { "pandistancex",		NKEYWORD, kwn_PanDistanceX },
    { "pandistancey",		NKEYWORD, kwn_PanDistanceY },
    { "pixmapdirectory",        SKEYWORD, kws_PixmapDirectory },
    { "pixmaps",		PIXMAPS, 0 },
    { "r",			ROOT, 0 },
    { "raisedelay",       NKEYWORD, kwn_RaiseDelay },/*RAISEDELAY*/
    { "randomplacement",	KEYWORD, kw0_RandomPlacement },
    { "realscreenbackground", CKEYWORD, kwc_RealScreenBackground },/*RFB 4/92*/
    { "realscreenforeground", CKEYWORD, kwc_RealScreenForeground },/*RFB 4/92*/
    { "realscreenpixmap",		REALSCREENMAP, 0 },/*RFB PIXMAP*/
    { "resize",			RESIZE, 0 },
    { "resizefont",		SKEYWORD, kws_ResizeFont },
    { "restartpreviousstate",	KEYWORD, kw0_RestartPreviousState },
    { "right",			JKEYWORD, J_RIGHT },
    { "righttitlebutton",	RIGHT_TITLEBUTTON, 0 },
    { "root",			ROOT, 0 },
    { "s",			SHIFT, 0 },
    { "savecolor",              SAVECOLOR, 0},
    { "select",			SELECT, 0 },
    { "shift",			SHIFT, 0 },
    { "showiconmanager",	KEYWORD, kw0_ShowIconManager },
    { "snaprealscreen",		KEYWORD, kw0_SnapRealScreen },
    { "sorticonmanager",	KEYWORD, kw0_SortIconManager },
    { "south",			DKEYWORD, D_SOUTH },
    { "squeezetitle",		SQUEEZE_TITLE, 0 },
    { "starticonified",		START_ICONIFIED, 0 },
    { "sticky",                 NAILEDDOWN, 0 },/*RFB*/
    { "sunkfocuswindowtitle",   KEYWORD, kw0_SunkFocusWindowTitle },
    { "t",			TITLE, 0 },
    { "title",			TITLE, 0 },
    { "titlebackground",	CLKEYWORD, kwcl_TitleBackground },
    { "titlebuttonborderwidth",	NKEYWORD, kwn_TitleButtonBorderWidth },
    { "titlefont",		SKEYWORD, kws_TitleFont },
    { "titleforeground",	CLKEYWORD, kwcl_TitleForeground },
    { "titlehighlight",		TITLE_HILITE, 0 },
    { "titlepadding",		NKEYWORD, kwn_TitlePadding },
    { "transientontop",         NKEYWORD, kwn_TransientOnTop },
    { "unknownicon",		SKEYWORD, kws_UnknownIcon },
    { "usepposition",		SKEYWORD, kws_UsePPosition },
    { "usethreediconmanagers",  KEYWORD, kw0_Use3DIconManagers },
    { "usethreedmenus",         KEYWORD, kw0_Use3DMenus },
    { "usethreedtitles",        KEYWORD, kw0_Use3DTitles },
    { "v",			VIRTUAL, 0 },
    { "virtual",		VIRTUAL, 0 },
    { "virtualbackground",	CKEYWORD, kwc_VirtualBackground },/*RFB VCOLOR*/
    { "virtualbackgroundpixmap",		VIRTUALMAP, 0 },/*RFB PIXMAP*/
    { "virtualdesktop",		VIRTUALDESKTOP, 0 },
    { "virtualdesktopborderwidth", NKEYWORD, kwn_VirtualDesktopBorderWidth },
    { "virtualdesktopfont",	SKEYWORD, kws_VirtualFont },
    { "virtualforeground",	CKEYWORD, kwc_VirtualForeground },/*RFB VCOLOR*/
    { "w",			WINDOW, 0 },
    { "wait",			WAIT, 0 },
    { "warpcursor",		WARP_CURSOR, 0 },
    { "warpunmapped",		KEYWORD, kw0_WarpUnmapped },
    { "warpwindows",		KEYWORD, kw0_WarpWindows },
    { "west",			DKEYWORD, D_WEST },
    { "window",			WINDOW, 0 },
    { "windowfunction",		WINDOW_FUNCTION, 0 },
    { "windowring",		WINDOW_RING, 0 },
    { "xorvalue",		NKEYWORD, kwn_XorValue },
    { "xpmicondirectory",       SKEYWORD, kws_PixmapDirectory },
    { "zoom",			ZOOM, 0 },
};

static int numkeywords = (sizeof(keytable)/sizeof(keytable[0]));

int parse_keyword (s, nump)
    char *s;
    int *nump;
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

int do_single_keyword (keyword)
    int keyword;
{
    switch (keyword) {
      case kw0_NoDefaults:
	Scr->NoDefaults = TRUE;
	return 1;

      case kw0_AutoRelativeResize:
	Scr->AutoRelativeResize = TRUE;
	return 1;

      case kw0_ForceIcons:
	if (Scr->FirstTime) Scr->ForceIcon = TRUE;
	return 1;

      case kw0_NoHighlightDesktop:
	Scr->HighlightDesktopFocus = FALSE;
	return 1;

      case kw0_NoIconManagers:
	Scr->NoIconManagers = TRUE;
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

      case kw0_CenteredInfoBox:
	Scr->CenteredInfoBox = TRUE;
	return 1;

    case kw0_NoRaiseDesktop:
	if (Scr->FirstTime) {
	    Scr->NoRaiseDesktop = TRUE;
	}
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

      case kw0_DecorateTransients:
	Scr->DecorateTransients = TRUE;
	return 1;

      case kw0_ShowIconManager:
	Scr->ShowIconManager = TRUE;
	return 1;

    case kw0_Use3DIconManagers:
	Scr->use3Diconmanagers = TRUE;
	return 1;

    case kw0_Use3DMenus:
	Scr->use3Dmenus = TRUE;
	return 1;

    case kw0_Use3DTitles:
	Scr->use3Dtitles = TRUE;
	return 1;
	
    case kw0_SunkFocusWindowTitle:
	Scr->SunkFocusWindowTitle = TRUE;
	return 1;
	
    case kw0_BeNiceToColormap:
	Scr->BeNiceToColormap = TRUE;
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
	Scr->SnapRealScreen = TRUE;
	return 1;

      case kw0_NotVirtualGeometries:
	Scr->GeometriesAreVirtual = FALSE;
	break;
    }

    return 0;
}


int do_string_keyword (keyword, s)
    int keyword;
    char *s;
{
    switch (keyword) {
      case kws_UsePPosition:
	{
	    int ppos = ParseUsePPosition (s);
	    if (ppos < 0) {
		twmrc_error_prefix();
		fprintf (stderr,
			 "ignoring invalid UsePPosition argument \"%s\"\n", s);
	    } else {
		Scr->UsePPosition = ppos;
	    }
	    return 1;
	}

    case kws_DefaultFont:
	if (!Scr->HaveFonts) Scr->DefaultFont.name = s;
	return 1;

    case kws_InfoFont:
	if (!Scr->HaveFonts) Scr->InfoFont.name = s;
	return 1;

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

      case kws_UnknownIcon:
	if (Scr->FirstTime) GetUnknownIcon (s);
	return 1;

      case kws_IconDirectory:
	if (Scr->FirstTime) Scr->IconDirectory = ExpandFilename (s);
	return 1;

    case kws_PixmapDirectory:
	if (Scr->FirstTime) Scr->PixmapDirectory = ExpandFilename (s);
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
    }

    return 0;
}


int do_number_keyword (keyword, num)
    int keyword;
    int num;
{
    switch (keyword) {
    case kwn_VirtualDesktopBorderWidth:
	Scr->VirtualDesktopDisplayBorderWidth = num;
	return 1;

      case kwn_ConstrainedMoveTime:
	ConstrainedMoveTime = num;
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
 		Scr->VirtualDesktopPanDistanceX = (num * Scr->MyDisplayWidth) / 100;
 	return 1;

      case kwn_PanDistanceY:
 	if (Scr->FirstTime)
 		Scr->VirtualDesktopPanDistanceY = (num * Scr->MyDisplayHeight) / 100;
 	return 1;

    case kwn_RaiseDelay: RaiseDelay = num; return 1;/*RAISEDELAY*/
	
      case kwn_AutoPan:
	if (Scr->FirstTime) {
		Scr->AutoPan = (num * Scr->MyDisplayWidth) / 100;
		if (Scr->AutoPan <= 0)
			Scr->AutoPan = 1;
	}
	return 1;

    case kwn_TransientOnTop:
	if (Scr->FirstTime) Scr->TransientOnTop = num;
	return 1;
  
    case kwn_OpaqueMoveThreshold:
	if (Scr->FirstTime) Scr->OpaqueMoveThreshold = num;
	return 1;
 
    case kwn_OpaqueResizeThreshold:
	if (Scr->FirstTime) Scr->OpaqueResizeThreshold = num;
	return 1;
 
    case kwn_ClearShadowContrast:
	if (Scr->FirstTime) Scr->ClearShadowContrast = num;
	if (Scr->ClearShadowContrast <   0) Scr->ClearShadowContrast =   0;
	if (Scr->ClearShadowContrast > 100) Scr->ClearShadowContrast = 100;
        return 1;
  
    case kwn_DarkShadowContrast:
	if (Scr->FirstTime) Scr->DarkShadowContrast = num;
	if (Scr->DarkShadowContrast <   0) Scr->DarkShadowContrast =   0;
	if (Scr->DarkShadowContrast > 100) Scr->DarkShadowContrast = 100;
	return 1;
    }

    return 0;
}

name_list **do_colorlist_keyword (keyword, colormode, s)
    int keyword;
    int colormode;
    char *s;
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

int do_color_keyword (keyword, colormode, s)
    int keyword;
    int colormode;
    char *s;
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

      case kwc_VirtualBackground:/*RFB VCOLOR*/
	GetColor (colormode, &Scr->VirtualC.back, s);/*RFB VCOLOR*/
	return 1;/*RFB VCOLOR*/

      case kwc_VirtualForeground:/*RFB VCOLOR*/
	GetColor (colormode, &Scr->VirtualC.fore, s);/*RFB VCOLOR*/
	return 1;/*RFB VCOLOR*/

      case kwc_RealScreenForeground:
	GetColor( colormode, &Scr->RealScreenC.fore, s);/*RFB 4/92 */
	return 1;

      case kwc_RealScreenBackground:
	GetColor( colormode, &Scr->RealScreenC.back, s);/*RFB 4/92 */
	return 1;

    }

    return 0;
}

/*
 * put_pixel_on_root() Save a pixel value in twm root window color property.
 */
put_pixel_on_root(pixel)
    Pixel pixel;
{
  int           i, addPixel = 1;
  Atom          pixelAtom, retAtom;
  int           retFormat;
  unsigned long nPixels, retAfter;
  Pixel        *retProp;
  pixelAtom = XInternAtom(dpy, "_MIT_PRIORITY_COLORS", True);
  XGetWindowProperty(dpy, Scr->Root, pixelAtom, 0, 8192,
		     False, XA_CARDINAL, &retAtom,
		     &retFormat, &nPixels, &retAfter,
		     (unsigned char **)&retProp);

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
int do_string_savecolor(colormode, s)
     int colormode;
     char *s;
{
  Pixel p;
  GetColor(colormode, &p, s);
  put_pixel_on_root(p);
}

/*
 * do_var_savecolor() save a color from a var in the twmrc file.
 */
typedef struct _cnode {int i; struct _cnode *next;} Cnode, *Cptr;
Cptr chead = NULL;

int do_var_savecolor(key)
int key;
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
void assign_var_savecolor()
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

static int ParseUsePPosition (s)
    register char *s;
{
    XmuCopyISOLatin1Lowered (s, s);

    if (strcmp (s, "off") == 0) {
	return PPOS_OFF;
    } else if (strcmp (s, "on") == 0) {
	return PPOS_ON;
    } else if (strcmp (s, "non-zero") == 0 ||
	       strcmp (s, "nonzero") == 0) {
	return PPOS_NON_ZERO;
    }

    return -1;
}


do_squeeze_entry (list, name, justify, num, denom)
    name_list **list;			/* squeeze or dont-squeeze list */
    char *name;				/* window name */
    int justify;			/* left, center, right */
    int num;				/* signed num */
    int denom;				/* 0 or indicates fraction denom */
{
    int absnum = (num < 0 ? -num : num);

    if (denom < 0) {
	twmrc_error_prefix();
	fprintf (stderr, "negative SqueezeTitle denominator %d\n", denom);
	return;
    }
    if (absnum > denom && denom != 0) {
	twmrc_error_prefix();
	fprintf (stderr, "SqueezeTitle fraction %d/%d outside window\n",
		 num, denom);
	return;
    }
    if (denom == 1) {
	twmrc_error_prefix();
	fprintf (stderr, "useless SqueezeTitle faction %d/%d, assuming 0/0\n",
		 num, denom);
	num = 0;
	denom = 0;
    }

    if (HasShape) {
	SqueezeInfo *sinfo;
	sinfo = (SqueezeInfo *) malloc (sizeof(SqueezeInfo));

	if (!sinfo) {
	    twmrc_error_prefix();
	    fprintf (stderr, "unable to allocate %d bytes for squeeze info\n",
		     sizeof(SqueezeInfo));
	    return;
	}
	sinfo->justify = justify;
	sinfo->num = num;
	sinfo->denom = denom;
	AddToList (list, name, (char *) sinfo);
    }
}
