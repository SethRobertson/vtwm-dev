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
 * $XConsortium: gram.y,v 1.91 91/02/08 18:21:56 dave Exp $
 *
 * .twmrc command grammer
 *
 * 07-Jan-86 Thomas E. LaStrange	File created
 * 11-Nov-90 Dave Sternlicht            Adding SaveColors
 * 10-Oct-90 David M. Sternlicht        Storing saved colors on root
 *
 ***********************************************************************/

%{
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "twm.h"
#include "menus.h"
#include "list.h"
#include "image_formats.h"
#include "util.h"
#include "screen.h"
#include "parse.h"
#include "doors.h"
#include "iconmgr.h"
#include "regions.h"
#include "prototypes.h"
#include <X11/Xos.h>
#include <X11/Xmu/CharSet.h>

#ifdef __NeXT__
#undef isascii
#define isascii(c) ((0 <= (int)(c)) && ((int)(c) <= 127))
#endif

static char *Action = "";
static char *Name = "";
static MenuRoot	*root, *pull = NULL;

static MenuRoot *GetRoot(char *name, char *fore, char *back);
static char *RemoveDQuote(char *str);
static char *RemoveRESlash(char *str);
static int ParseUsePPosition(char *s);
static int ParseWarpCentered(char *s);

static MenuItem *lastmenuitem = (MenuItem*) 0;

static Bool CheckWarpScreenArg (char *s);
static Bool CheckWarpRingArg (char *s);
static Bool CheckColormapArg (char *s);

static void GotButton(int butt, int func);
static void GotKey(char *key, int func);
static void GotTitleButton (char *bitmapname, int func, Bool rightside);
static void yyerror(char *s);
static name_list **list;
static RootRegion *ARlist;
static int cont = 0;
static int color;
int mods = 0;
unsigned int mods_used = (ShiftMask | ControlMask | Mod1Mask);
static int ppos;
static int warpc;


/*
 * this used to be the definition - now making the assumption it's
 * defined in lex's skeleton file (submitted by Nelson H. F. Beebe)
 */
extern int yylineno;

%}

%union
{
    int num;
    char *ptr;
    struct
    {
	short ltype;
	char *lval;
    } match;
};

%token <num> LP RP MENUS MENU BUTTON DEFAULT_FUNCTION PLUS MINUS
%token <num> ALL OR CURSORS PIXMAPS ICONS COLOR MONOCHROME FUNCTION
%token <num> ICONMGR_SHOW ICONMGR WINDOW_FUNCTION ZOOM ICONMGRS
%token <num> ICONMGR_GEOMETRY ICONMGR_NOSHOW MAKE_TITLE
%token <num> ICONIFY_BY_UNMAPPING DONT_ICONIFY_BY_UNMAPPING
%token <num> NO_TITLE AUTO_RAISE NO_HILITE NO_ICONMGR_HILITE ICON_REGION
%token <num> WARP_CENTERED
%token <num> USE_PPOSITION
%token <num> NO_BORDER
%token <num> APPLET_REGION
%token <num> META SHIFT LOCK CONTROL WINDOW TITLE ICON ROOT FRAME VIRTUAL VIRTUAL_WIN
%token <num> COLON EQUALS TILDE SQUEEZE_TITLE DONT_SQUEEZE_TITLE
%token <num> OPAQUE_MOVE NO_OPAQUE_MOVE OPAQUE_RESIZE NO_OPAQUE_RESIZE
%token <num> START_ICONIFIED NO_TITLE_HILITE TITLE_HILITE
%token <num> MOVE RESIZE WAIT SELECT KILL LEFT_TITLEBUTTON RIGHT_TITLEBUTTON
%token <num> NUMBER KEYWORD MKEYWORD NKEYWORD CKEYWORD CLKEYWORD FKEYWORD FSKEYWORD
%token <num> SNKEYWORD SKEYWORD DKEYWORD JKEYWORD WINDOW_RING NO_WINDOW_RING WARP_CURSOR
%token <num> ERRORTOKEN NO_STACKMODE NAILEDDOWN VIRTUALDESKTOP NO_SHOW_IN_DISPLAY
%token <num> NO_SHOW_IN_TWMWINDOWS
%token DOORS DOOR
%token <num> VIRTUALMAP
%token <num> REALSCREENMAP
%token <num> ICONMGRICONMAP
%token <num> MENUICONMAP
%token SOUNDS
%token <ptr> STRING REGEXP
%token <num> IGNORE_MODS
%token SAVECOLOR
%token LB
%token RB

%type <match> matcher
%type <ptr> string regexp
%type <num> action button number signed_number full fullkey

%start twmrc

%%
twmrc		: stmts
		;

stmts		: /* Empty */
		| stmts stmt
		;

stmt		: error
		| noarg
		| sarg
		| narg
		| snarg
		| squeeze
		| doors
		| ICON_REGION string DKEYWORD DKEYWORD number number
					{ AddIconRegion($2, $3, $4, $5, $6); }
		| APPLET_REGION string DKEYWORD DKEYWORD number number
					{ ARlist = AddAppletRegion($2, $3, $4, $5, $6); }
		  applet_list
		| ICONMGR_GEOMETRY string number	{ if (Scr->FirstTime)
						  {
						    Scr->iconmgr.geometry=$2;
						    Scr->iconmgr.columns=$3;
						  }
						}
		| ICONMGR_GEOMETRY string	{ if (Scr->FirstTime)
						    Scr->iconmgr.geometry = $2;
						}
		| ZOOM number		{ if (Scr->FirstTime)
					  {
						Scr->DoZoom = TRUE;
						Scr->ZoomCount = $2;
					  }
					}
		| ZOOM			{ if (Scr->FirstTime)
						Scr->DoZoom = TRUE; }
		| PIXMAPS pixmap_list	{}
		| CURSORS cursor_list	{}
		| ICONIFY_BY_UNMAPPING	{ list = &Scr->IconifyByUn; }
		  win_list
		| ICONIFY_BY_UNMAPPING	{ if (Scr->FirstTime)
		    Scr->IconifyByUnmapping = TRUE; }

		| OPAQUE_MOVE { list = &Scr->OpaqueMoveL; }
		  win_list
		| OPAQUE_MOVE { if (Scr->FirstTime) Scr->OpaqueMove = TRUE; }
		| NO_OPAQUE_MOVE { list = &Scr->NoOpaqueMoveL; }
		  win_list
		| NO_OPAQUE_MOVE { if (Scr->FirstTime) Scr->OpaqueMove = FALSE; }
		| OPAQUE_RESIZE { list = &Scr->OpaqueResizeL; }
		  win_list
		| OPAQUE_RESIZE { if (Scr->FirstTime) Scr->OpaqueResize = TRUE; }
		| NO_OPAQUE_RESIZE { list = &Scr->NoOpaqueResizeL; }
		  win_list
		| NO_OPAQUE_RESIZE { if (Scr->FirstTime) Scr->OpaqueResize = FALSE; }

		| LEFT_TITLEBUTTON string EQUALS action {
					  GotTitleButton ($2, $4, False);
					}
		| RIGHT_TITLEBUTTON string EQUALS action {
					  GotTitleButton ($2, $4, True);
					}
		| button string		{ root = GetRoot($2, NULLSTR, NULLSTR);
					  if ($1 <= NumButtons) {
					    Scr->Mouse[MOUSELOC($1,C_ROOT,0)].func = F_MENU;
					    Scr->Mouse[MOUSELOC($1,C_ROOT,0)].menu = root;
					  }
					}
		| button action		{ if ($1 <= NumButtons) {
					    Scr->Mouse[MOUSELOC($1,C_ROOT,0)].func = $2;
					    if ($2 == F_MENU)
					    {
					      pull->prev = NULL;
					      Scr->Mouse[MOUSELOC($1,C_ROOT,0)].menu = pull;
					    }
					    else
					    {
					      root = GetRoot(TWM_ROOT,NULLSTR,NULLSTR);
					      Scr->Mouse[MOUSELOC($1,C_ROOT,0)].item =
						  AddToMenu(root,"x",Action,
							    NULL,$2,NULLSTR,NULLSTR);
					    }
					    Action = "";
					    pull = NULL;
					  }
					}
		| string fullkey	{ GotKey($1, $2); }
		| button full		{ GotButton($1, $2); }
		| IGNORE_MODS keys	{ Scr->IgnoreModifiers = mods;
					  mods = 0;
					}
		| DONT_ICONIFY_BY_UNMAPPING { list = &Scr->DontIconify; }
		  win_list
		| ICONMGR_NOSHOW	{ list = &Scr->IconMgrNoShow; }
		  win_list
		| ICONMGR_NOSHOW	{ Scr->IconManagerDontShow = TRUE; }
		| ICONMGRS		{ list = &Scr->IconMgrs; }
		  iconm_list
		| ICONMGR_SHOW		{ list = &Scr->IconMgrShow; }
		  win_list
		| NO_TITLE_HILITE	{ list = &Scr->NoTitleHighlight; }
		  win_list
		| NO_TITLE_HILITE	{ if (Scr->FirstTime)
						Scr->TitleHighlight = FALSE; }
		| NO_ICONMGR_HILITE		{ Scr->IconMgrHighlight = FALSE; }
		| NO_HILITE		{ list = &Scr->NoHighlight; }
		  win_list
		| NO_HILITE		{ if (Scr->FirstTime)
						Scr->Highlight = FALSE; }
		| NO_STACKMODE		{ list = &Scr->NoStackModeL; }
		  win_list
		| NO_STACKMODE		{ if (Scr->FirstTime)
						Scr->StackMode = FALSE; }
		| NO_TITLE		{ list = &Scr->NoTitle; }
		  win_list
		| NO_TITLE		{ if (Scr->FirstTime)
						Scr->NoTitlebar = TRUE; }
		| NO_BORDER		{ list = &Scr->NoBorder; }
		  win_list
		| NO_BORDER		{ if (Scr->FirstTime)
						Scr->NoBorders = TRUE; }
		| MAKE_TITLE		{ list = &Scr->MakeTitle; }
		  win_list
		| START_ICONIFIED	{ list = &Scr->StartIconified; }
		  win_list
		| AUTO_RAISE		{ list = &Scr->AutoRaise; }
		  win_list
		| AUTO_RAISE		{ Scr->AutoRaiseDefault = TRUE; }
		| WARP_CENTERED string	{ if (Scr->FirstTime) {
						if ((warpc = ParseWarpCentered($2)) != -1)
							Scr->WarpCentered = warpc;
					  }
					}
		| USE_PPOSITION string	{ if (Scr->FirstTime) {
						if ((ppos = ParseUsePPosition($2)) != -1)
							Scr->UsePPosition = ppos;
					  }
					}
		| USE_PPOSITION		{ list = &Scr->UsePPositionL; }
		  ppos_list
		| USE_PPOSITION string	{ if (Scr->FirstTime) {
						if ((ppos = ParseUsePPosition($2)) != -1)
							Scr->UsePPosition = ppos;
					  }
					  list = &Scr->UsePPositionL;
					}
		  ppos_list
		| MENU string LP string COLON string RP	{
					root = GetRoot($2, $4, $6); }
		  menu			{ root->real_menu = TRUE;}
		| MENU string		{ root = GetRoot($2, NULLSTR, NULLSTR); }
		  menu			{ root->real_menu = TRUE; }
		| FUNCTION string	{ root = GetRoot($2, NULLSTR, NULLSTR); }
		  function
		| ICONS		{ list = &Scr->IconNames; }
		  icon_list
		| SOUNDS
		  sound_list
		| COLOR		{ color = COLOR; }
		  color_list
		| SAVECOLOR
		  save_color_list
		| MONOCHROME		{ color = MONOCHROME; }
		  color_list
		| DEFAULT_FUNCTION action { Scr->DefaultFunction.func = $2;
					  if ($2 == F_MENU)
					  {
					    pull->prev = NULL;
					    Scr->DefaultFunction.menu = pull;
					  }
					  else
					  {
					    root = GetRoot(TWM_ROOT,NULLSTR,NULLSTR);
					    Scr->DefaultFunction.item =
						AddToMenu(root,"x",Action,
							  NULL,$2, NULLSTR, NULLSTR);
					  }
					  Action = "";
					  pull = NULL;
					}
		| WINDOW_FUNCTION action { Scr->WindowFunction.func = $2;
					   root = GetRoot(TWM_ROOT,NULLSTR,NULLSTR);
					   Scr->WindowFunction.item =
						AddToMenu(root,"x",Action,
							  NULL,$2, NULLSTR, NULLSTR);
					   Action = "";
					   pull = NULL;
					}
		| WARP_CURSOR		{ list = &Scr->WarpCursorL; }
		  win_list
		| WARP_CURSOR		{ if (Scr->FirstTime)
					    Scr->WarpCursor = TRUE; }
		| WINDOW_RING		{ list = &Scr->WindowRingL; }
		  win_list
		| WINDOW_RING       {	if (Scr->FirstTime)
						Scr->UseWindowRing = TRUE; }
		| NO_WINDOW_RING	{ list = &Scr->NoWindowRingL; }
		  win_list
		| NAILEDDOWN		{ list = &Scr->NailedDown; }
		  win_list
		| VIRTUALDESKTOP string number
					{ SetVirtualDesktop($2, $3); }
		| NO_SHOW_IN_DISPLAY	{ list = &Scr->DontShowInDisplay; }
		  win_list
		| NO_SHOW_IN_TWMWINDOWS	{ list = &Scr->DontShowInTWMWindows; }
		  win_list
		;


noarg		: KEYWORD		{ if (!do_single_keyword ($1)) {
					    twmrc_error_prefix();
					    fprintf (stderr,
					"unknown singleton keyword %d\n",
						     $1);
					    ParseError = 1;
					  }
					}
		;

sarg		: SKEYWORD string	{ if (!do_string_keyword ($1, $2)) {
					    twmrc_error_prefix();
					    fprintf (stderr,
				"unknown string keyword %d (value \"%s\")\n",
						     $1, $2);
					    ParseError = 1;
					  }
					}
		;

narg		: NKEYWORD number	{ if (!do_number_keyword ($1, $2)) {
					    twmrc_error_prefix();
					    fprintf (stderr,
				"unknown numeric keyword %d (value %d)\n",
						     $1, $2);
					    ParseError = 1;
					  }
					}
		;

snarg		: SNKEYWORD signed_number	{ if (!do_number_keyword ($1, $2)) {
					    twmrc_error_prefix();
					    fprintf (stderr,
				"unknown signed keyword %d (value %d)\n",
						     $1, $2);
					    ParseError = 1;
					  }
					}
		;



full		: EQUALS keys COLON contexts COLON action  { $$ = $6; }
		;

fullkey		: EQUALS keys COLON contextkeys COLON action  { $$ = $6; }
		;

keys		: /* Empty */
		| keys key
		;

key		: META			{ mods |= Mod1Mask; }
		| SHIFT			{ mods |= ShiftMask; }
		| LOCK			{ mods |= LockMask; }
		| CONTROL		{ mods |= ControlMask; }
		| META number		{ if ($2 < 1 || $2 > 5) {
					     twmrc_error_prefix();
					     fprintf (stderr,
				"bad modifier number (%d), must be 1-5\n",
						      $2);
					     ParseError = 1;
					  } else {
					     mods |= (Mod1Mask << ($2 - 1));
					  }
					}
		| OR			{ }
		;

contexts	: /* Empty */
		| contexts context
		;

context		: WINDOW		{ cont |= C_WINDOW_BIT; }
		| TITLE			{ cont |= C_TITLE_BIT; }
		| ICON			{ cont |= C_ICON_BIT; }
		| ROOT			{ cont |= C_ROOT_BIT; }
		| FRAME			{ cont |= C_FRAME_BIT; }
		| ICONMGR		{ cont |= C_ICONMGR_BIT; }
		| META			{ cont |= C_ICONMGR_BIT; }
		| VIRTUAL               { cont |= C_VIRTUAL_BIT; }
		| VIRTUAL_WIN           { cont |= C_VIRTUAL_WIN_BIT; }
		| DOOR			{ cont |= C_DOOR_BIT; }
		| ALL			{ cont |= C_ALL_BITS; }
		| OR			{  }
		;

contextkeys	: /* Empty */
		| contextkeys contextkey
		;

contextkey	: WINDOW		{ cont |= C_WINDOW_BIT; }
		| TITLE			{ cont |= C_TITLE_BIT; }
		| ICON			{ cont |= C_ICON_BIT; }
		| ROOT			{ cont |= C_ROOT_BIT; }
		| FRAME			{ cont |= C_FRAME_BIT; }
		| ICONMGR		{ cont |= C_ICONMGR_BIT; }
		| META			{ cont |= C_ICONMGR_BIT; }
		| VIRTUAL               { cont |= C_VIRTUAL_BIT; }
		| VIRTUAL_WIN           { cont |= C_VIRTUAL_WIN_BIT; }
		| DOOR                  { cont |= C_DOOR_BIT; }
		| ALL			{ cont |= C_ALL_BITS; }
		| OR			{ }
		| string		{ Name = $1; cont |= C_NAME_BIT; }
		;


pixmap_list	: LB pixmap_entries RB
		;

pixmap_entries	: /* Empty */
		| pixmap_entries pixmap_entry
		;

pixmap_entry	: TITLE_HILITE string { SetHighlightPixmap ($2); }
		| VIRTUALMAP string { SetVirtualPixmap ($2); }
		| REALSCREENMAP string { SetRealScreenPixmap ($2); }
		| ICONMGRICONMAP string { SetIconMgrPixmap($2); }
		| MENUICONMAP string { SetMenuIconPixmap($2); }
		;


cursor_list	: LB cursor_entries RB
		;

cursor_entries	: /* Empty */
		| cursor_entries cursor_entry
		;

cursor_entry	: FRAME string string {
			NewBitmapCursor(&Scr->FrameCursor, $2, $3); }
		| FRAME string	{
			NewFontCursor(&Scr->FrameCursor, $2); }
		| TITLE string string {
			NewBitmapCursor(&Scr->TitleCursor, $2, $3); }
		| TITLE string {
			NewFontCursor(&Scr->TitleCursor, $2); }
		| ICON string string {
			NewBitmapCursor(&Scr->IconCursor, $2, $3); }
		| ICON string {
			NewFontCursor(&Scr->IconCursor, $2); }
		| ICONMGR string string {
			NewBitmapCursor(&Scr->IconMgrCursor, $2, $3); }
		| ICONMGR string {
			NewFontCursor(&Scr->IconMgrCursor, $2); }
		| BUTTON string string {
			NewBitmapCursor(&Scr->ButtonCursor, $2, $3); }
		| BUTTON string {
			NewFontCursor(&Scr->ButtonCursor, $2); }
		| MOVE string string {
			NewBitmapCursor(&Scr->MoveCursor, $2, $3); }
		| MOVE string {
			NewFontCursor(&Scr->MoveCursor, $2); }
		| RESIZE string string {
			NewBitmapCursor(&Scr->ResizeCursor, $2, $3); }
		| RESIZE string {
			NewFontCursor(&Scr->ResizeCursor, $2); }
		| WAIT string string {
			NewBitmapCursor(&Scr->WaitCursor, $2, $3); }
		| WAIT string {
			NewFontCursor(&Scr->WaitCursor, $2); }
		| MENU string string {
			NewBitmapCursor(&Scr->MenuCursor, $2, $3); }
		| MENU string {
			NewFontCursor(&Scr->MenuCursor, $2); }
		| SELECT string string {
			NewBitmapCursor(&Scr->SelectCursor, $2, $3); }
		| SELECT string {
			NewFontCursor(&Scr->SelectCursor, $2); }
		| KILL string string {
			NewBitmapCursor(&Scr->DestroyCursor, $2, $3); }
		| KILL string {
			NewFontCursor(&Scr->DestroyCursor, $2); }
		| DOOR string string {
			NewBitmapCursor(&Scr->DoorCursor, $2, $3); }
		| DOOR string {
			NewFontCursor(&Scr->DoorCursor, $2); }
		| VIRTUAL string string {
			NewBitmapCursor(&Scr->VirtualCursor, $2, $3); }
		| VIRTUAL string {
			NewFontCursor(&Scr->VirtualCursor, $2); }
		| VIRTUAL_WIN string string {
			NewBitmapCursor(&Scr->DesktopCursor, $2, $3); }
		| VIRTUAL_WIN string {
			NewFontCursor(&Scr->DesktopCursor, $2); }
		;

color_list	: LB color_entries RB
		;


color_entries	: /* Empty */
		| color_entries color_entry
		;

color_entry	: CLKEYWORD string	{ if (!do_colorlist_keyword ($1, color,
								     $2)) {
					    twmrc_error_prefix();
					    fprintf (stderr,
			"unhandled list color keyword %d (string \"%s\")\n",
						     $1, $2);
					    ParseError = 1;
					  }
					}
		| CLKEYWORD string	{ list = do_colorlist_keyword($1,color,
								      $2);
					  if (!list) {
					    twmrc_error_prefix();
					    fprintf (stderr,
			"unhandled color list keyword %d (string \"%s\")\n",
						     $1, $2);
					    ParseError = 1;
					  }
					}
		  win_color_list
		| CKEYWORD string	{ if (!do_color_keyword ($1, color,
								 $2)) {
					    twmrc_error_prefix();
					    fprintf (stderr,
			"unhandled color keyword %d (string \"%s\")\n",
						     $1, $2);
					    ParseError = 1;
					  }
					}
		;

save_color_list : LB s_color_entries RB
		;

s_color_entries : /* Empty */
		| s_color_entries s_color_entry
		;

s_color_entry   : string            { do_string_savecolor(color, $1); }
		| CLKEYWORD         { do_var_savecolor($1); }
		;

win_color_list	: LB win_color_entries RB
		;

win_color_entries	: /* Empty */
		| win_color_entries win_color_entry
		;

win_color_entry	: matcher string	{ if (Scr->FirstTime &&
					      color == Scr->Monochrome)
					    AddToList(list, $1.lval, $1.ltype,
							$2); }
		;

squeeze		: SQUEEZE_TITLE {
				    if (HasShape) Scr->SqueezeTitle = TRUE;
				}
		| SQUEEZE_TITLE { list = &Scr->SqueezeTitleL;
				  if (HasShape && Scr->SqueezeTitle == -1)
				    Scr->SqueezeTitle = TRUE;
				}
		  LB win_sqz_entries RB
		| DONT_SQUEEZE_TITLE { Scr->SqueezeTitle = FALSE; }
		| DONT_SQUEEZE_TITLE { list = &Scr->DontSqueezeTitleL; }
		  win_list
		;

win_sqz_entries	: /* Empty */
		| win_sqz_entries matcher JKEYWORD signed_number signed_number	{
				if (Scr->FirstTime) {
				   do_squeeze_entry (list, $2.lval, $2.ltype,
							$3, $4, $5);
				}
			}
		;

doors		: DOORS LB door_list RB
		;

door_list	: /* Empty */
		| door_list door_entry
		;

door_entry	: string string string
			{
				(void) door_add($1, $2, $3);
			}
		;

iconm_list	: LB iconm_entries RB
		;

iconm_entries	: /* Empty */
		| iconm_entries iconm_entry
		;

iconm_entry	: matcher string number	{ if (Scr->FirstTime)
					    AddToList(list, $1.lval, $1.ltype, (char *)
						AllocateIconManager($1.lval,
							NULLSTR, $2, $3));
					}
		| matcher string string number
					{ if (Scr->FirstTime)
					    AddToList(list, $1.lval, $1.ltype, (char *)
						AllocateIconManager($1.lval,
							$2, $3, $4));
					}
		;

win_list	: LB win_entries RB
		;

win_entries	: /* Empty */
		| win_entries win_entry
		;

win_entry	: matcher		{ if (Scr->FirstTime)
					    AddToList(list, $1.lval, $1.ltype, 0);
					}
		;

icon_list	: LB icon_entries RB
		;

icon_entries	: /* Empty */
		| icon_entries icon_entry
		;

icon_entry	: matcher string	{ if (Scr->FirstTime)
					    AddToList(list, $1.lval, $1.ltype, $2);
					}
		;

ppos_list	: LB ppos_entries RB
		;

ppos_entries	: /* Empty */
		| ppos_entries ppos_entry
		;

ppos_entry	: matcher string	{ if (Scr->FirstTime) {
					    if ((ppos = ParseUsePPosition($2)) != -1)
					      AddToList(list, $1.lval, $1.ltype, (char *)&ppos);
					  }
					}
		;

sound_list	: LB sound_entries RB
		;

sound_entries	: /* Empty */
		| sound_entries sound_entry
		;

sound_entry	: string string		{ if (Scr->FirstTime) SetSound($1, $2, -1); }
		| string string number	{ if (Scr->FirstTime) SetSound($1, $2, $3); }
		;

applet_list	: LB applet_entries RB
		;

applet_entries	: /* Empty */
		| applet_entries applet_entry
		;

applet_entry	: matcher		{ if (Scr->FirstTime)
					      AddToAppletList(ARlist,
							$1.lval, $1.ltype);
					}
		;

function	: LB function_entries RB
		;

function_entries: /* Empty */
		| function_entries function_entry
		;

function_entry	: action		{ AddToMenu(root, "", Action, NULL, $1,
						NULLSTR, NULLSTR);
					  Action = "";
					}
		;

menu		: LB menu_entries RB {lastmenuitem = (MenuItem*) 0;}
		;

menu_entries	: /* Empty */
		| menu_entries menu_entry
		;

menu_entry	: string action		{
			if ($2 == F_SEPARATOR) {
			    if (lastmenuitem) lastmenuitem->separated = 1;
			}
			else {
			    lastmenuitem = AddToMenu(root, $1, Action, pull, $2, NULLSTR, NULLSTR);
			    Action = "";
			    pull = NULL;
			}
		}
		| string LP string COLON string RP action {
			if ($7 == F_SEPARATOR) {
			    if (lastmenuitem) lastmenuitem->separated = 1;
			}
			else {
			    lastmenuitem = AddToMenu(root, $1, Action, pull, $7, $3, $5);
			    Action = "";
			    pull = NULL;
			}
		}
		;

action		: FKEYWORD	{ $$ = $1; }
		| FSKEYWORD string {
				$$ = $1;
				Action = $2;
				switch ($1) {
				  case F_MENU:
				    pull = GetRoot ($2, NULLSTR,NULLSTR);
				    pull->prev = root;
				    break;
				  case F_WARPRING:
				    if (!CheckWarpRingArg (Action)) {
					twmrc_error_prefix();
					fprintf (stderr,
			"ignoring invalid f.warptoring argument \"%s\"\n",
						 Action);
					$$ = F_NOP;
				    }
				  case F_WARPTOSCREEN:
				    if (!CheckWarpScreenArg (Action)) {
					twmrc_error_prefix();
					fprintf (stderr,
			"ignoring invalid f.warptoscreen argument \"%s\"\n",
						 Action);
					$$ = F_NOP;
				    }
				    break;
				  case F_COLORMAP:
				    if (CheckColormapArg (Action)) {
					$$ = F_COLORMAP;
				    } else {
					twmrc_error_prefix();
					fprintf (stderr,
			"ignoring invalid f.colormap argument \"%s\"\n",
						 Action);
					$$ = F_NOP;
				    }
				    break;
				} /* end switch */
				   }
		;


button		: BUTTON number		{ $$ = $2;
					  if ($2 == 0)
						yyerror("bad button 0");

					  if ($2 > NumButtons)
					  {
						$$ = 0;
						yyerror("button number too large");
					  }
					}
		;

matcher		: string		{ $$.ltype = LTYPE_ANY_STRING;
					  $$.lval = $1;
					}
		| regexp		{ $$.ltype = LTYPE_ANY_REGEXP;
					  $$.lval = $1;
					}
		| MKEYWORD EQUALS string { $$.ltype = $1 | LTYPE_STRING;
					   $$.lval = $3;
					 }
		| MKEYWORD TILDE regexp { $$.ltype = $1 | LTYPE_REGEXP;
					  $$.lval = $3;
					}
		;

string		: STRING		{ $$ = RemoveDQuote($1); }
		;

regexp		: REGEXP		{ $$ = RemoveRESlash($1); }
		;

signed_number	: number		{ $$ = $1; }
		| PLUS number		{ $$ = $2; }
		| MINUS number		{ $$ = -($2); }
		;

number		: NUMBER		{ $$ = $1; }
		;

%%
static void
yyerror(char *s)
{
  twmrc_error_prefix();
  fprintf (stderr, "error in input file:  %s\n", s ? s : "");
  ParseError = 1;
}

static char *RemoveDQuote(char *str)
{
  register char *i, *o;
  register int n, count;
  int length = 0;
  char *ptr = "";

  for (i = str + 1, o = str; *i && *i != '\"'; o++)
  {
    if (*i == '\\')
    {
      switch (*++i)
      {
      case 'n':
	*o = '\n';
	i++;
	break;
      case 'b':
	*o = '\b';
	i++;
	break;
      case 'r':
	*o = '\r';
	i++;
	break;
      case 't':
	*o = '\t';
	i++;
	break;
      case 'f':
	*o = '\f';
	i++;
	break;
      case '0':
	if (*++i == 'x')
	  goto hex;
	else
	  --i;
      case '1': case '2': case '3':
      case '4': case '5': case '6': case '7':
	n = 0;
	count = 0;
	while (*i >= '0' && *i <= '7' && count < 3)
	{
	  n = (n << 3) + (*i++ - '0');
	  count++;
	}
	*o = n;
	break;
      hex:
      case 'x':
	n = 0;
	count = 0;
	while (i++, count++ < 2)
	{
	  if (*i >= '0' && *i <= '9')
	    n = (n << 4) + (*i - '0');
	  else if (*i >= 'a' && *i <= 'f')
	    n = (n << 4) + (*i - 'a') + 10;
	  else if (*i >= 'A' && *i <= 'F')
	    n = (n << 4) + (*i - 'A') + 10;
	  else
	  {
	    length--; /* account for length++ at loop end */
	    break;
	  }
	}
	*o = n;
	break;
      case '\n':
	i++;	/* punt */
	o--;	/* to account for o++ at end of loop */
	length--; /* account for length++ at loop end */
	break;
      case '\"':
      case '\'':
      case '\\':
      default:
	*o = *i++;
	break;
      }
    }
    else
      *o = *i++;

    length++;
  }
  *o = '\0';

  if (length > 0)
  {
    ptr = (char *)malloc(length + 1);
    memcpy(ptr, str, length);
    ptr[length] = '\0';

#ifdef DEBUG
    fprintf(stderr, "RemoveDQuote(): '");
    for (n = 0; n < length; n++)
      fprintf(stderr, "%c", ptr[n]);
    fprintf(stderr, "'\n", ptr);
#endif
  }

  return (ptr);
}

static char *RemoveRESlash(char *str)
{
  char *ptr = "";
#ifndef NO_REGEX_SUPPORT
  int length = strlen(str);

  if (length > 2)
  {
    ptr = (char *)malloc(length - 1);
    memcpy(ptr, str + 1, length - 2);
    ptr[length - 2] = '\0';

#ifdef DEBUG
    fprintf(stderr, "RemoveRESlash(): '%s'\n", ptr);
#endif
  }
#else
  twmrc_error_prefix();
  fprintf(stderr, "no regex support for %s\n", str);
  ParseError = 1;
#endif

  return (ptr);
}

static int ParseUsePPosition(char *s)
{
  XmuCopyISOLatin1Lowered (s, s);

  if (strcmp(s, "off") == 0)
    return PPOS_OFF;
  else if (strcmp(s, "on") == 0)
    return PPOS_ON;
  else if (strcmp(s, "non-zero") == 0 || strcmp(s, "nonzero") == 0)
    return PPOS_NON_ZERO;
  else if (strcmp(s, "on-screen") == 0 || strcmp(s, "onscreen") == 0)
    return PPOS_ON_SCREEN;

  twmrc_error_prefix();
  fprintf(stderr, "ignoring invalid UsePPosition argument \"%s\"\n", s);
  return -1;
}

static int ParseWarpCentered(char *s)
{
  XmuCopyISOLatin1Lowered (s, s);

  if (strcmp(s, "off") == 0)
    return WARPC_OFF;
  else if (strcmp(s, "on") == 0)
    return WARPC_ON;
  else if (strcmp(s, "titled") == 0)
    return WARPC_TITLED;
  else if (strcmp(s, "untitled") == 0)
    return WARPC_UNTITLED;

  twmrc_error_prefix();
  fprintf(stderr, "ignoring invalid WarpCentered argument \"%s\"\n", s);
  return -1;
}

static MenuRoot *GetRoot(char *name, char *fore, char *back)
{
  MenuRoot *tmp;

  tmp = FindMenuRoot(name);
  if (tmp == NULL)
    tmp = NewMenuRoot(name);

  if (fore)
  {
    int save;

    save = Scr->FirstTime;
    Scr->FirstTime = TRUE;

    GetColor(COLOR, &tmp->highlight.fore, fore);
    GetColor(COLOR, &tmp->highlight.back, back);

    Scr->FirstTime = save;
  }

  return tmp;
}

static void GotButton(int butt, int func)
{
  int i;

  if (butt > NumButtons)
    return;

  for (i = 0; i < NUM_CONTEXTS; i++)
  {
    if ((cont & (1 << i)) == 0)
      continue;

    Scr->Mouse[MOUSELOC(butt,i,mods)].func = func;

    if (func == F_MENU)
    {
      pull->prev = NULL;

      Scr->Mouse[MOUSELOC(butt,i,mods)].menu = pull;
    }
    else
    {
      root = GetRoot(TWM_ROOT, NULLSTR, NULLSTR);

      Scr->Mouse[MOUSELOC(butt,i,mods)].item = AddToMenu(root,"x",Action,
							 NULL, func, NULLSTR, NULLSTR);

    }
  }
  Action = "";
  pull = NULL;
  cont = 0;
  mods_used |= mods;
  mods = 0;
}

static void GotKey(char *key, int func)
{
  int i;

  for (i = 0; i < NUM_CONTEXTS; i++)
  {
    if ((cont & (1 << i)) == 0)
      continue;
    if (!AddFuncKey(key, i, mods, func, Name, Action))
      break;
  }

  Action = "";
  pull = NULL;
  cont = 0;
  mods_used |= mods;
  mods = 0;
}


static void GotTitleButton (char *bitmapname, int func, Bool rightside)
{
  if (!CreateTitleButton (bitmapname, func, Action, pull, rightside, True)) {
    twmrc_error_prefix();
    fprintf (stderr,
	     "unable to create %s titlebutton \"%s\"\n",
	     rightside ? "right" : "left", bitmapname);
  }
  Action = "";
  pull = NULL;
}

static Bool CheckWarpScreenArg (char *s)
{
  XmuCopyISOLatin1Lowered (s, s);

  if (strcmp (s,  WARPSCREEN_NEXT) == 0 ||
      strcmp (s,  WARPSCREEN_PREV) == 0 ||
      strcmp (s,  WARPSCREEN_BACK) == 0)
    return True;

  for (; *s && isascii(*s) && isdigit(*s); s++) ; /* SUPPRESS 530 */
  return (*s ? False : True);
}


static Bool CheckWarpRingArg (char *s)
{
  XmuCopyISOLatin1Lowered (s, s);

  if (strcmp (s,  WARPSCREEN_NEXT) == 0 ||
      strcmp (s,  WARPSCREEN_PREV) == 0)
    return True;

  return False;
}


static Bool CheckColormapArg (char *s)
{
  XmuCopyISOLatin1Lowered (s, s);

  if (strcmp (s, COLORMAP_NEXT) == 0 ||
      strcmp (s, COLORMAP_PREV) == 0 ||
      strcmp (s, COLORMAP_DEFAULT) == 0)
    return True;

  return False;
}


void twmrc_error_prefix (void)
{
  fprintf (stderr, "%s:  line %d:  ", ProgramName, yylineno);
}


/*
  Local Variables:
  mode:c
  c-file-style:"GNU"
  c-file-offsets:((substatement-open 0)(brace-list-open 0)(c-hanging-comment-ender-p . nil)(c-hanging-comment-beginner-p . nil)(comment-start . "// ")(comment-end . "")(comment-column . 48))
  End:
*/
/* vim: sw=2
*/
