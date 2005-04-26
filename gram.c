
# line 42 "gram.y"
#include <stdio.h>
#include <ctype.h>
#include "twm.h"
#include "menus.h"
#include "list.h"
#include "util.h"
#include "screen.h"
#include "parse.h"
#include "doors.h"
#include <X11/Xos.h>
#include <X11/Xmu/CharSet.h>

static char *Action = "";
static char *Name = "";
static MenuRoot	*root, *pull = NULL;

static MenuRoot *GetRoot();

static Bool CheckWarpScreenArg(), CheckWarpRingArg();
static Bool CheckColormapArg();
static void GotButton(), GotKey(), GotTitleButton();
static char *ptr;
static name_list **list;
static int cont = 0;
static int color;
int mods = 0;
unsigned int mods_used = (ShiftMask | ControlMask | Mod1Mask);

extern int do_single_keyword(), do_string_keyword(), do_number_keyword();
extern name_list **do_colorlist_keyword();
extern int do_color_keyword(), do_string_savecolor();
extern int yylineno;

# line 76 "gram.y"
typedef union 
{
    int num;
    char *ptr;
} YYSTYPE;
#ifdef __cplusplus
#  include <stdio.h>
   extern "C" {
     extern void yyerror(char *);
     extern int yylex();
   }
#endif	/* __cplusplus */ 
# define LB 257
# define RB 258
# define LP 259
# define RP 260
# define MENUS 261
# define MENU 262
# define BUTTON 263
# define DEFAULT_FUNCTION 264
# define PLUS 265
# define MINUS 266
# define ALL 267
# define OR 268
# define CURSORS 269
# define PIXMAPS 270
# define ICONS 271
# define COLOR 272
# define SAVECOLOR 273
# define MONOCHROME 274
# define FUNCTION 275
# define ICONMGR_SHOW 276
# define ICONMGR 277
# define WINDOW_FUNCTION 278
# define ZOOM 279
# define ICONMGRS 280
# define ICONMGR_GEOMETRY 281
# define ICONMGR_NOSHOW 282
# define MAKE_TITLE 283
# define ICONIFY_BY_UNMAPPING 284
# define DONT_ICONIFY_BY_UNMAPPING 285
# define NO_TITLE 286
# define AUTO_RAISE 287
# define NO_HILITE 288
# define ICON_REGION 289
# define META 290
# define SHIFT 291
# define LOCK 292
# define CONTROL 293
# define WINDOW 294
# define TITLE 295
# define ICON 296
# define ROOT 297
# define FRAME 298
# define VIRTUAL 299
# define VIRTUAL_WIN 300
# define COLON 301
# define EQUALS 302
# define SQUEEZE_TITLE 303
# define DONT_SQUEEZE_TITLE 304
# define START_ICONIFIED 305
# define NO_TITLE_HILITE 306
# define TITLE_HILITE 307
# define MOVE 308
# define RESIZE 309
# define WAIT 310
# define SELECT 311
# define KILL 312
# define LEFT_TITLEBUTTON 313
# define RIGHT_TITLEBUTTON 314
# define NUMBER 315
# define KEYWORD 316
# define NKEYWORD 317
# define CKEYWORD 318
# define CLKEYWORD 319
# define FKEYWORD 320
# define FSKEYWORD 321
# define SKEYWORD 322
# define DKEYWORD 323
# define JKEYWORD 324
# define WINDOW_RING 325
# define WARP_CURSOR 326
# define ERRORTOKEN 327
# define NO_STACKMODE 328
# define NAILEDDOWN 329
# define VIRTUALDESKTOP 330
# define NO_SHOW_IN_DISPLAY 331
# define DOORS 332
# define DOOR 333
# define VIRTUALMAP 334
# define REALSCREENMAP 335
# define STRING 336
#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
extern int yychar;
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif

/* __YYSCLASS defines the scoping/storage class for global objects
 * that are NOT renamed by the -p option.  By default these names
 * are going to be 'static' so that multi-definition errors
 * will not occur with multiple parsers.
 * If you want (unsupported) access to internal names you need
 * to define this to be null so it implies 'extern' scope.
 * This should not be used in conjunction with -p.
 */
#ifndef __YYSCLASS
# define __YYSCLASS static
#endif
YYSTYPE yylval;
__YYSCLASS YYSTYPE yyval;
typedef int yytabelem;
# define YYERRCODE 256

# line 675 "gram.y"

yyerror(s) char *s;
{
    twmrc_error_prefix();
    fprintf (stderr, "error in input file:  %s\n", s ? s : "");
    ParseError = 1;
}
RemoveDQuote(str)
char *str;
{
    register char *i, *o;
    register n;
    register count;

    for (i=str+1, o=str; *i && *i != '\"'; o++)
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
		    n = (n<<3) + (*i++ - '0');
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
			n = (n<<4) + (*i - '0');
		    else if (*i >= 'a' && *i <= 'f')
			n = (n<<4) + (*i - 'a') + 10;
		    else if (*i >= 'A' && *i <= 'F')
			n = (n<<4) + (*i - 'A') + 10;
		    else
			break;
		}
		*o = n;
		break;
	    case '\n':
		i++;	/* punt */
		o--;	/* to account for o++ at end of loop */
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
    }
    *o = '\0';
}

static MenuRoot *GetRoot(name, fore, back)
char *name;
char *fore, *back;
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
	GetColor(COLOR, &tmp->hi_fore, fore);
	GetColor(COLOR, &tmp->hi_back, back);
	Scr->FirstTime = save;
    }

    return tmp;
}

static void GotButton(butt, func)
int butt, func;
{
    int i;

    for (i = 0; i < NUM_CONTEXTS; i++)
    {
	if ((cont & (1 << i)) == 0)
	    continue;

	Scr->Mouse[butt][i][mods].func = func;
	if (func == F_MENU)
	{
	    pull->prev = NULL;
	    Scr->Mouse[butt][i][mods].menu = pull;
	}
	else
	{
	    root = GetRoot(TWM_ROOT, NULLSTR, NULLSTR);
	    Scr->Mouse[butt][i][mods].item = AddToMenu(root,"x",Action,
		    NULLSTR, func, NULLSTR, NULLSTR);
	}
    }
    Action = "";
    pull = NULL;
    cont = 0;
    mods_used |= mods;
    mods = 0;
}

static void GotKey(key, func)
char *key;
int func;
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


static void GotTitleButton (bitmapname, func, rightside)
    char *bitmapname;
    int func;
    Bool rightside;
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

static Bool CheckWarpScreenArg (s)
    register char *s;
{
    XmuCopyISOLatin1Lowered (s, s);

    if (strcmp (s,  WARPSCREEN_NEXT) == 0 ||
	strcmp (s,  WARPSCREEN_PREV) == 0 ||
	strcmp (s,  WARPSCREEN_BACK) == 0)
      return True;

    for (; *s && isascii(*s) && isdigit(*s); s++) ; /* SUPPRESS 530 */
    return (*s ? False : True);
}


static Bool CheckWarpRingArg (s)
    register char *s;
{
    XmuCopyISOLatin1Lowered (s, s);

    if (strcmp (s,  WARPSCREEN_NEXT) == 0 ||
	strcmp (s,  WARPSCREEN_PREV) == 0)
      return True;

    return False;
}


static Bool CheckColormapArg (s)
    register char *s;
{
    XmuCopyISOLatin1Lowered (s, s);

    if (strcmp (s, COLORMAP_NEXT) == 0 ||
	strcmp (s, COLORMAP_PREV) == 0 ||
	strcmp (s, COLORMAP_DEFAULT) == 0)
      return True;

    return False;
}


twmrc_error_prefix ()
{
    fprintf (stderr, "%s:  line %d:  ", ProgramName, yylineno);
}
__YYSCLASS yytabelem yyexca[] ={
-1, 1,
	0, -1,
	-2, 0,
-1, 2,
	0, 1,
	-2, 0,
-1, 15,
	257, 17,
	-2, 19,
-1, 21,
	257, 28,
	-2, 30,
-1, 24,
	257, 35,
	-2, 37,
-1, 25,
	257, 38,
	-2, 40,
-1, 26,
	257, 41,
	-2, 43,
-1, 27,
	257, 44,
	-2, 46,
-1, 30,
	257, 51,
	-2, 53,
-1, 39,
	257, 69,
	-2, 71,
-1, 40,
	257, 72,
	-2, 74,
-1, 47,
	257, 176,
	-2, 175,
-1, 48,
	257, 179,
	-2, 178,
-1, 284,
	257, 163,
	-2, 162,
	};
# define YYNPROD 217
# define YYLAST 448
__YYSCLASS yytabelem yyact[]={

     4,    51,   286,   183,   312,   277,    31,    50,    37,   142,
   102,    55,   228,    14,    13,    33,    34,    35,    36,    32,
    23,    68,    38,    12,    22,    11,    21,    28,    15,    20,
    27,    30,    25,    10,   221,    55,   215,    66,    67,    66,
    67,   224,   109,   294,   108,   209,   280,    47,    48,    29,
    24,    70,   177,   309,   143,    51,    51,    16,    17,   218,
    44,    46,   315,   292,   186,   125,    45,    19,   188,    40,
    39,   178,    26,    41,    42,    43,    49,   304,    52,    53,
    51,    51,    51,    51,    61,    62,    63,   148,   272,   273,
    51,   158,   154,   180,    64,   298,   299,   107,   267,    82,
    83,   227,   226,   145,    66,    67,   153,   139,    66,    67,
    94,   268,    51,    96,    51,   262,   263,   264,   265,   266,
   269,   270,   260,    51,   151,   152,   173,   150,   162,   163,
   146,   147,    89,    90,   131,   110,   129,   155,   156,   157,
   159,   160,   258,   259,   130,    55,    51,    54,   169,   170,
   171,   172,   253,   116,   271,   173,   100,    51,    87,   174,
    59,    57,   161,   111,   278,   254,   219,   281,   220,   248,
   249,   250,   251,   252,   255,   256,   246,   169,   170,   171,
   172,   222,   181,   210,   164,   216,   175,   189,   167,   141,
    99,   187,    98,   176,    97,   313,   310,   184,   101,   132,
   185,   103,   303,   165,   166,   295,   225,   182,   257,   190,
   149,   105,   144,   192,   193,   194,   104,   261,   195,   196,
   197,   198,   199,   200,   201,   202,   203,   204,   205,   206,
   207,   208,   211,   133,   112,   247,   168,   214,   212,    95,
    93,    92,   137,   217,    91,    88,    86,    85,   128,   223,
    84,   179,   106,   127,   126,   229,   301,    81,   230,    80,
    79,    78,    77,   232,   233,   234,   235,   236,   237,   238,
   239,   240,   241,   242,   243,   244,   245,    76,    75,    74,
   115,    73,   274,    72,    71,   275,   276,   279,    60,    58,
   191,   283,    56,     9,   284,   285,     8,     7,   287,     6,
     5,     3,     2,     1,    69,    65,   296,    18,     0,     0,
     0,     0,     0,     0,     0,   282,     0,   213,     0,     0,
     0,     0,     0,     0,   113,   114,     0,   117,   118,   119,
   120,   121,   122,   123,   124,     0,     0,     0,     0,   231,
     0,   288,     0,   291,   134,   135,   136,     0,   138,     0,
     0,     0,   140,     0,   308,   289,     0,     0,     0,     0,
     0,     0,   302,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,   293,     0,     0,   311,   314,     0,
     0,     0,   316,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
   317,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,   290,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,   297,     0,     0,     0,     0,   300,
     0,     0,     0,     0,   305,     0,   306,   307 };
__YYSCLASS yytabelem yypact[]={

 -3000, -3000,  -256, -3000, -3000, -3000, -3000, -3000, -3000, -3000,
  -335,  -335,  -304,   -96,   -97, -3000,  -335,  -335,  -281,  -251,
 -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000,
 -3000,  -335,  -335, -3000, -3000,   -99, -3000,  -283,  -283, -3000,
 -3000, -3000,  -335, -3000, -3000,  -335,  -304, -3000, -3000,  -101,
  -304, -3000,  -313,  -304, -3000, -3000, -3000, -3000, -3000, -3000,
  -160,  -258,  -260, -3000, -3000, -3000, -3000,  -335, -3000, -3000,
 -3000,  -160,  -160,  -104,  -160,  -160,  -160,  -160,  -160,  -160,
  -160,  -160,  -194, -3000,  -121,  -123, -3000, -3000,  -123, -3000,
 -3000,  -160,  -160,  -160,  -304,  -160, -3000, -3000,  -150,  -160,
 -3000, -3000,  -314, -3000,  -204,  -171, -3000, -3000,  -283,  -283,
 -3000,  -113,  -142, -3000, -3000, -3000, -3000, -3000, -3000, -3000,
 -3000, -3000, -3000, -3000, -3000,  -335,  -186,  -164, -3000, -3000,
 -3000, -3000,  -255, -3000, -3000, -3000, -3000, -3000, -3000, -3000,
 -3000,  -190,  -304, -3000, -3000,  -335,  -335,  -335, -3000, -3000,
  -335,  -335,  -335,  -335,  -335,  -335,  -335,  -335,  -335,  -335,
  -335,  -335,  -335,  -335,  -213, -3000, -3000, -3000, -3000,  -304,
 -3000, -3000, -3000, -3000, -3000,  -222,  -242, -3000, -3000, -3000,
 -3000,  -224,  -217, -3000, -3000, -3000, -3000,  -246, -3000, -3000,
  -335,  -304, -3000, -3000, -3000,  -335,  -335,  -335,  -335,  -335,
  -335,  -335,  -335,  -335,  -335,  -335,  -335,  -335,  -335, -3000,
 -3000, -3000,  -125, -3000,  -179, -3000, -3000,  -335,  -335,  -253,
  -212, -3000, -3000,  -335, -3000, -3000,  -335,  -335, -3000,  -322,
  -335, -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000,
 -3000, -3000, -3000, -3000, -3000, -3000,  -283, -3000, -3000, -3000,
 -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000,
  -283, -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000,
 -3000, -3000, -3000, -3000, -3000,  -280,  -197, -3000, -3000,  -216,
 -3000, -3000, -3000, -3000, -3000, -3000,  -170, -3000, -3000, -3000,
 -3000,  -304, -3000, -3000,  -335,  -180,  -304, -3000,  -304,  -304,
 -3000,  -186,  -248, -3000, -3000, -3000, -3000, -3000, -3000,  -335,
  -254,  -198, -3000, -3000,  -335,  -283, -3000, -3000 };
__YYSCLASS yytabelem yypgo[]={

     0,    67,    94,   307,   147,   306,   305,   304,   303,   302,
   301,   300,   299,   297,   296,   293,   292,   289,   288,   252,
   284,   283,   281,   280,   279,   278,   277,   262,   261,   260,
   259,   257,   256,    52,   254,   253,   251,   250,   248,   247,
   144,   246,   245,   244,   241,   240,   239,   163,   238,   237,
   236,   235,   217,   216,   212,   211,   210,   207,   206,   205,
   202,   199,   197,   196,   195,   192,   191,   190,   189,   187,
   186,   185,   184,   183,   182,   181,   168,   167,   166,   164 };
__YYSCLASS yytabelem yyr1[]={

     0,     8,     9,     9,    10,    10,    10,    10,    10,    10,
    10,    10,    10,    10,    10,    10,    10,    18,    10,    10,
    10,    10,    10,    10,    10,    10,    20,    10,    21,    10,
    10,    22,    10,    24,    10,    25,    10,    10,    26,    10,
    10,    27,    10,    10,    28,    10,    10,    29,    10,    30,
    10,    31,    10,    10,    32,    10,    34,    10,    35,    10,
    37,    10,    39,    10,    10,    42,    10,    10,    10,    43,
    10,    10,    44,    10,    10,    45,    10,    10,    46,    10,
    11,    12,    13,     6,     7,    47,    47,    50,    50,    50,
    50,    50,    50,    48,    48,    51,    51,    51,    51,    51,
    51,    51,    51,    51,    51,    51,    51,    49,    49,    52,
    52,    52,    52,    52,    52,    52,    52,    52,    52,    52,
    52,    52,    16,    53,    53,    54,    54,    54,    17,    55,
    55,    56,    56,    56,    56,    56,    56,    56,    56,    56,
    56,    56,    56,    56,    56,    56,    56,    56,    56,    56,
    56,    56,    56,    56,    56,    56,    56,    56,    56,    40,
    57,    57,    58,    59,    58,    58,    41,    61,    61,    62,
    62,    60,    63,    63,    64,    14,    65,    14,    14,    67,
    14,    66,    66,    15,    68,    68,    69,    23,    70,    70,
    71,    71,    19,    72,    72,    73,    38,    74,    74,    75,
    36,    76,    76,    77,    33,    78,    78,    79,    79,     2,
     2,     5,     5,     5,     3,     1,     4 };
__YYSCLASS yytabelem yyr2[]={

     0,     2,     0,     4,     2,     2,     2,     2,     2,     2,
    13,     7,     5,     5,     3,     5,     5,     1,     6,     3,
     9,     9,     5,     5,     5,     5,     1,     6,     1,     6,
     3,     1,     6,     1,     6,     1,     6,     3,     1,     6,
     3,     1,     6,     3,     1,     6,     3,     1,     6,     1,
     6,     1,     6,     3,     1,    19,     1,     9,     1,     8,
     1,     6,     1,     6,     4,     1,     6,     5,     5,     1,
     6,     3,     1,     6,     3,     1,     6,     7,     1,     6,
     3,     5,     5,    13,    13,     0,     4,     3,     3,     3,
     3,     5,     3,     0,     4,     3,     3,     3,     3,     3,
     3,     3,     3,     3,     3,     3,     3,     0,     4,     3,
     3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
     3,     3,     6,     0,     4,     5,     5,     5,     6,     0,
     4,     7,     5,     7,     5,     7,     5,     7,     5,     7,
     5,     7,     5,     7,     5,     7,     5,     7,     5,     7,
     5,     7,     5,     7,     5,     7,     5,     7,     5,     6,
     0,     4,     5,     1,     8,     5,     6,     0,     4,     3,
     3,     6,     0,     4,     5,     3,     1,    10,     3,     1,
     6,     0,    11,     8,     0,     4,     7,     6,     0,     4,
     7,     9,     6,     0,     4,     3,     6,     0,     4,     5,
     6,     0,     4,     3,     6,     0,     4,     5,    15,     3,
     5,     3,     5,     5,     5,     3,     3 };
__YYSCLASS yytabelem yychk[]={

 -3000,    -8,    -9,   -10,   256,   -11,   -12,   -13,   -14,   -15,
   289,   281,   279,   270,   269,   284,   313,   314,    -3,    -1,
   285,   282,   280,   276,   306,   288,   328,   286,   283,   305,
   287,   262,   275,   271,   272,   273,   274,   264,   278,   326,
   325,   329,   330,   331,   316,   322,   317,   303,   304,   332,
   263,   336,    -1,    -1,    -4,   315,   -16,   257,   -17,   257,
   -18,    -1,    -1,    -1,    -2,    -6,   320,   321,   302,    -7,
   302,   -20,   -21,   -22,   -24,   -25,   -26,   -27,   -28,   -29,
   -30,   -31,    -1,    -1,   -37,   -39,   -41,   257,   -42,    -2,
    -2,   -43,   -44,   -45,    -1,   -46,    -1,    -4,   -65,   -67,
   257,    -4,   323,    -4,   -53,   -55,   -19,   257,   302,   302,
    -1,   -47,   -47,   -19,   -19,   -23,   257,   -19,   -19,   -19,
   -19,   -19,   -19,   -19,   -19,   259,   -34,   -35,   -38,   257,
   -40,   257,   -61,   -40,   -19,   -19,   -19,    -4,   -19,   257,
   -19,   -68,   323,   258,   -54,   307,   334,   335,   258,   -56,
   298,   295,   296,   277,   263,   308,   309,   310,   262,   311,
   312,   333,   299,   300,   -72,    -2,    -2,   301,   -50,   290,
   291,   292,   293,   268,   301,   -70,    -1,   -33,   257,   -36,
   257,   -74,   -57,   258,   -62,    -1,   319,   -66,   258,   -69,
    -1,    -4,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   258,
   -73,    -1,   -48,    -4,   -49,   258,   -71,    -1,   301,   -78,
   -76,   258,   -75,    -1,   258,   -58,   319,   318,   258,    -1,
    -1,    -4,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,   301,   -51,   294,   295,
   296,   297,   298,   277,   290,   299,   300,   333,   267,   268,
   301,   -52,   294,   295,   296,   297,   298,   277,   290,   299,
   300,   333,   267,   268,    -1,    -1,    -1,   258,   -79,    -1,
   258,   -77,    -2,    -1,    -1,    -1,   324,    -1,    -2,    -2,
    -4,    -1,   260,    -2,   259,   -59,    -5,    -4,   265,   266,
    -4,   -32,    -1,   -60,   257,    -4,    -4,    -4,   -33,   301,
   -63,    -1,   258,   -64,    -1,   260,    -1,    -2 };
__YYSCLASS yytabelem yydef[]={

     2,    -2,    -2,     3,     4,     5,     6,     7,     8,     9,
     0,     0,    14,     0,     0,    -2,     0,     0,     0,     0,
    26,    -2,    31,    33,    -2,    -2,    -2,    -2,    47,    49,
    -2,     0,     0,    60,    62,     0,    65,     0,     0,    -2,
    -2,    75,     0,    78,    80,     0,     0,    -2,    -2,     0,
     0,   215,     0,    12,    13,   216,    15,   123,    16,   129,
     0,     0,     0,    22,    23,    25,   209,     0,    85,    24,
    85,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,    56,    58,     0,     0,    64,   167,     0,    67,
    68,     0,     0,     0,     0,     0,    81,    82,     0,     0,
   184,   214,     0,    11,     0,     0,    18,   193,     0,     0,
   210,     0,     0,    27,    29,    32,   188,    34,    36,    39,
    42,    45,    48,    50,    52,     0,     0,     0,    61,   197,
    63,   160,     0,    66,    70,    73,    76,    77,    79,   181,
   180,     0,     0,   122,   124,     0,     0,     0,   128,   130,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,    20,    21,    93,    86,    87,
    88,    89,    90,    92,   107,     0,     0,    57,   205,    59,
   201,     0,     0,   166,   168,   169,   170,     0,   183,   185,
     0,     0,   125,   126,   127,   132,   134,   136,   138,   140,
   142,   144,   146,   148,   150,   152,   154,   156,   158,   192,
   194,   195,     0,    91,     0,   187,   189,     0,     0,     0,
     0,   196,   198,     0,   159,   161,     0,     0,   177,     0,
     0,    10,   131,   133,   135,   137,   139,   141,   143,   145,
   147,   149,   151,   153,   155,   157,     0,    94,    95,    96,
    97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     0,   108,   109,   110,   111,   112,   113,   114,   115,   116,
   117,   118,   119,   120,   121,     0,     0,   204,   206,     0,
   200,   202,   203,   199,    -2,   165,     0,   186,    83,    84,
   190,     0,    54,   207,     0,     0,     0,   211,     0,     0,
   191,     0,     0,   164,   172,   182,   212,   213,    55,     0,
     0,     0,   171,   173,     0,     0,   174,   208 };
typedef struct { char *t_name; int t_val; } yytoktype;
#ifndef YYDEBUG
#	define YYDEBUG	0	/* don't allow debugging */
#endif

#if YYDEBUG

__YYSCLASS yytoktype yytoks[] =
{
	"LB",	257,
	"RB",	258,
	"LP",	259,
	"RP",	260,
	"MENUS",	261,
	"MENU",	262,
	"BUTTON",	263,
	"DEFAULT_FUNCTION",	264,
	"PLUS",	265,
	"MINUS",	266,
	"ALL",	267,
	"OR",	268,
	"CURSORS",	269,
	"PIXMAPS",	270,
	"ICONS",	271,
	"COLOR",	272,
	"SAVECOLOR",	273,
	"MONOCHROME",	274,
	"FUNCTION",	275,
	"ICONMGR_SHOW",	276,
	"ICONMGR",	277,
	"WINDOW_FUNCTION",	278,
	"ZOOM",	279,
	"ICONMGRS",	280,
	"ICONMGR_GEOMETRY",	281,
	"ICONMGR_NOSHOW",	282,
	"MAKE_TITLE",	283,
	"ICONIFY_BY_UNMAPPING",	284,
	"DONT_ICONIFY_BY_UNMAPPING",	285,
	"NO_TITLE",	286,
	"AUTO_RAISE",	287,
	"NO_HILITE",	288,
	"ICON_REGION",	289,
	"META",	290,
	"SHIFT",	291,
	"LOCK",	292,
	"CONTROL",	293,
	"WINDOW",	294,
	"TITLE",	295,
	"ICON",	296,
	"ROOT",	297,
	"FRAME",	298,
	"VIRTUAL",	299,
	"VIRTUAL_WIN",	300,
	"COLON",	301,
	"EQUALS",	302,
	"SQUEEZE_TITLE",	303,
	"DONT_SQUEEZE_TITLE",	304,
	"START_ICONIFIED",	305,
	"NO_TITLE_HILITE",	306,
	"TITLE_HILITE",	307,
	"MOVE",	308,
	"RESIZE",	309,
	"WAIT",	310,
	"SELECT",	311,
	"KILL",	312,
	"LEFT_TITLEBUTTON",	313,
	"RIGHT_TITLEBUTTON",	314,
	"NUMBER",	315,
	"KEYWORD",	316,
	"NKEYWORD",	317,
	"CKEYWORD",	318,
	"CLKEYWORD",	319,
	"FKEYWORD",	320,
	"FSKEYWORD",	321,
	"SKEYWORD",	322,
	"DKEYWORD",	323,
	"JKEYWORD",	324,
	"WINDOW_RING",	325,
	"WARP_CURSOR",	326,
	"ERRORTOKEN",	327,
	"NO_STACKMODE",	328,
	"NAILEDDOWN",	329,
	"VIRTUALDESKTOP",	330,
	"NO_SHOW_IN_DISPLAY",	331,
	"DOORS",	332,
	"DOOR",	333,
	"VIRTUALMAP",	334,
	"REALSCREENMAP",	335,
	"STRING",	336,
	"-unknown-",	-1	/* ends search */
};

__YYSCLASS char * yyreds[] =
{
	"-no such reduction-",
	"twmrc : stmts",
	"stmts : /* empty */",
	"stmts : stmts stmt",
	"stmt : error",
	"stmt : noarg",
	"stmt : sarg",
	"stmt : narg",
	"stmt : squeeze",
	"stmt : doors",
	"stmt : ICON_REGION string DKEYWORD DKEYWORD number number",
	"stmt : ICONMGR_GEOMETRY string number",
	"stmt : ICONMGR_GEOMETRY string",
	"stmt : ZOOM number",
	"stmt : ZOOM",
	"stmt : PIXMAPS pixmap_list",
	"stmt : CURSORS cursor_list",
	"stmt : ICONIFY_BY_UNMAPPING",
	"stmt : ICONIFY_BY_UNMAPPING win_list",
	"stmt : ICONIFY_BY_UNMAPPING",
	"stmt : LEFT_TITLEBUTTON string EQUALS action",
	"stmt : RIGHT_TITLEBUTTON string EQUALS action",
	"stmt : button string",
	"stmt : button action",
	"stmt : string fullkey",
	"stmt : button full",
	"stmt : DONT_ICONIFY_BY_UNMAPPING",
	"stmt : DONT_ICONIFY_BY_UNMAPPING win_list",
	"stmt : ICONMGR_NOSHOW",
	"stmt : ICONMGR_NOSHOW win_list",
	"stmt : ICONMGR_NOSHOW",
	"stmt : ICONMGRS",
	"stmt : ICONMGRS iconm_list",
	"stmt : ICONMGR_SHOW",
	"stmt : ICONMGR_SHOW win_list",
	"stmt : NO_TITLE_HILITE",
	"stmt : NO_TITLE_HILITE win_list",
	"stmt : NO_TITLE_HILITE",
	"stmt : NO_HILITE",
	"stmt : NO_HILITE win_list",
	"stmt : NO_HILITE",
	"stmt : NO_STACKMODE",
	"stmt : NO_STACKMODE win_list",
	"stmt : NO_STACKMODE",
	"stmt : NO_TITLE",
	"stmt : NO_TITLE win_list",
	"stmt : NO_TITLE",
	"stmt : MAKE_TITLE",
	"stmt : MAKE_TITLE win_list",
	"stmt : START_ICONIFIED",
	"stmt : START_ICONIFIED win_list",
	"stmt : AUTO_RAISE",
	"stmt : AUTO_RAISE win_list",
	"stmt : AUTO_RAISE",
	"stmt : MENU string LP string COLON string RP",
	"stmt : MENU string LP string COLON string RP menu",
	"stmt : MENU string",
	"stmt : MENU string menu",
	"stmt : FUNCTION string",
	"stmt : FUNCTION string function",
	"stmt : ICONS",
	"stmt : ICONS icon_list",
	"stmt : COLOR",
	"stmt : COLOR color_list",
	"stmt : SAVECOLOR save_color_list",
	"stmt : MONOCHROME",
	"stmt : MONOCHROME color_list",
	"stmt : DEFAULT_FUNCTION action",
	"stmt : WINDOW_FUNCTION action",
	"stmt : WARP_CURSOR",
	"stmt : WARP_CURSOR win_list",
	"stmt : WARP_CURSOR",
	"stmt : WINDOW_RING",
	"stmt : WINDOW_RING win_list",
	"stmt : WINDOW_RING",
	"stmt : NAILEDDOWN",
	"stmt : NAILEDDOWN win_list",
	"stmt : VIRTUALDESKTOP string number",
	"stmt : NO_SHOW_IN_DISPLAY",
	"stmt : NO_SHOW_IN_DISPLAY win_list",
	"noarg : KEYWORD",
	"sarg : SKEYWORD string",
	"narg : NKEYWORD number",
	"full : EQUALS keys COLON contexts COLON action",
	"fullkey : EQUALS keys COLON contextkeys COLON action",
	"keys : /* empty */",
	"keys : keys key",
	"key : META",
	"key : SHIFT",
	"key : LOCK",
	"key : CONTROL",
	"key : META number",
	"key : OR",
	"contexts : /* empty */",
	"contexts : contexts context",
	"context : WINDOW",
	"context : TITLE",
	"context : ICON",
	"context : ROOT",
	"context : FRAME",
	"context : ICONMGR",
	"context : META",
	"context : VIRTUAL",
	"context : VIRTUAL_WIN",
	"context : DOOR",
	"context : ALL",
	"context : OR",
	"contextkeys : /* empty */",
	"contextkeys : contextkeys contextkey",
	"contextkey : WINDOW",
	"contextkey : TITLE",
	"contextkey : ICON",
	"contextkey : ROOT",
	"contextkey : FRAME",
	"contextkey : ICONMGR",
	"contextkey : META",
	"contextkey : VIRTUAL",
	"contextkey : VIRTUAL_WIN",
	"contextkey : DOOR",
	"contextkey : ALL",
	"contextkey : OR",
	"contextkey : string",
	"pixmap_list : LB pixmap_entries RB",
	"pixmap_entries : /* empty */",
	"pixmap_entries : pixmap_entries pixmap_entry",
	"pixmap_entry : TITLE_HILITE string",
	"pixmap_entry : VIRTUALMAP string",
	"pixmap_entry : REALSCREENMAP string",
	"cursor_list : LB cursor_entries RB",
	"cursor_entries : /* empty */",
	"cursor_entries : cursor_entries cursor_entry",
	"cursor_entry : FRAME string string",
	"cursor_entry : FRAME string",
	"cursor_entry : TITLE string string",
	"cursor_entry : TITLE string",
	"cursor_entry : ICON string string",
	"cursor_entry : ICON string",
	"cursor_entry : ICONMGR string string",
	"cursor_entry : ICONMGR string",
	"cursor_entry : BUTTON string string",
	"cursor_entry : BUTTON string",
	"cursor_entry : MOVE string string",
	"cursor_entry : MOVE string",
	"cursor_entry : RESIZE string string",
	"cursor_entry : RESIZE string",
	"cursor_entry : WAIT string string",
	"cursor_entry : WAIT string",
	"cursor_entry : MENU string string",
	"cursor_entry : MENU string",
	"cursor_entry : SELECT string string",
	"cursor_entry : SELECT string",
	"cursor_entry : KILL string string",
	"cursor_entry : KILL string",
	"cursor_entry : DOOR string string",
	"cursor_entry : DOOR string",
	"cursor_entry : VIRTUAL string string",
	"cursor_entry : VIRTUAL string",
	"cursor_entry : VIRTUAL_WIN string string",
	"cursor_entry : VIRTUAL_WIN string",
	"color_list : LB color_entries RB",
	"color_entries : /* empty */",
	"color_entries : color_entries color_entry",
	"color_entry : CLKEYWORD string",
	"color_entry : CLKEYWORD string",
	"color_entry : CLKEYWORD string win_color_list",
	"color_entry : CKEYWORD string",
	"save_color_list : LB s_color_entries RB",
	"s_color_entries : /* empty */",
	"s_color_entries : s_color_entries s_color_entry",
	"s_color_entry : string",
	"s_color_entry : CLKEYWORD",
	"win_color_list : LB win_color_entries RB",
	"win_color_entries : /* empty */",
	"win_color_entries : win_color_entries win_color_entry",
	"win_color_entry : string string",
	"squeeze : SQUEEZE_TITLE",
	"squeeze : SQUEEZE_TITLE",
	"squeeze : SQUEEZE_TITLE LB win_sqz_entries RB",
	"squeeze : DONT_SQUEEZE_TITLE",
	"squeeze : DONT_SQUEEZE_TITLE",
	"squeeze : DONT_SQUEEZE_TITLE win_list",
	"win_sqz_entries : /* empty */",
	"win_sqz_entries : win_sqz_entries string JKEYWORD signed_number number",
	"doors : DOORS LB door_list RB",
	"door_list : /* empty */",
	"door_list : door_list door_entry",
	"door_entry : string string string",
	"iconm_list : LB iconm_entries RB",
	"iconm_entries : /* empty */",
	"iconm_entries : iconm_entries iconm_entry",
	"iconm_entry : string string number",
	"iconm_entry : string string string number",
	"win_list : LB win_entries RB",
	"win_entries : /* empty */",
	"win_entries : win_entries win_entry",
	"win_entry : string",
	"icon_list : LB icon_entries RB",
	"icon_entries : /* empty */",
	"icon_entries : icon_entries icon_entry",
	"icon_entry : string string",
	"function : LB function_entries RB",
	"function_entries : /* empty */",
	"function_entries : function_entries function_entry",
	"function_entry : action",
	"menu : LB menu_entries RB",
	"menu_entries : /* empty */",
	"menu_entries : menu_entries menu_entry",
	"menu_entry : string action",
	"menu_entry : string LP string COLON string RP action",
	"action : FKEYWORD",
	"action : FSKEYWORD string",
	"signed_number : number",
	"signed_number : PLUS number",
	"signed_number : MINUS number",
	"button : BUTTON number",
	"string : STRING",
	"number : NUMBER",
};
#endif /* YYDEBUG */
#define YYFLAG  (-3000)
/* @(#) $Revision: 70.7 $ */    

/*
** Skeleton parser driver for yacc output
*/

#if defined(NLS) && !defined(NL_SETN)
#include <msgbuf.h>
#endif

#ifndef nl_msg
#define nl_msg(i,s) (s)
#endif

/*
** yacc user known macros and defines
*/
#define YYERROR		goto yyerrlab

#ifndef __RUNTIME_YYMAXDEPTH
#define YYACCEPT	return(0)
#define YYABORT		return(1)
#else
#define YYACCEPT	{free_stacks(); return(0);}
#define YYABORT		{free_stacks(); return(1);}
#endif

#define YYBACKUP( newtoken, newvalue )\
{\
	if ( yychar >= 0 || ( yyr2[ yytmp ] >> 1 ) != 1 )\
	{\
		yyerror( (nl_msg(30001,"syntax error - cannot backup")) );\
		goto yyerrlab;\
	}\
	yychar = newtoken;\
	yystate = *yyps;\
	yylval = newvalue;\
	goto yynewstate;\
}
#define YYRECOVERING()	(!!yyerrflag)
#ifndef YYDEBUG
#	define YYDEBUG	1	/* make debugging available */
#endif

/*
** user known globals
*/
int yydebug;			/* set to 1 to get debugging */

/*
** driver internal defines
*/
/* define for YYFLAG now generated by yacc program. */
/*#define YYFLAG		(FLAGVAL)*/

/*
** global variables used by the parser
*/
# ifndef __RUNTIME_YYMAXDEPTH
__YYSCLASS YYSTYPE yyv[ YYMAXDEPTH ];	/* value stack */
__YYSCLASS int yys[ YYMAXDEPTH ];		/* state stack */
# else
__YYSCLASS YYSTYPE *yyv;			/* pointer to malloc'ed value stack */
__YYSCLASS int *yys;			/* pointer to malloc'ed stack stack */

#if defined(__STDC__) || defined (__cplusplus)
#include <stdlib.h>
#else
	extern char *malloc();
	extern char *realloc();
	extern void free();
#endif /* __STDC__ or __cplusplus */


static int allocate_stacks(); 
static void free_stacks();
# ifndef YYINCREMENT
# define YYINCREMENT (YYMAXDEPTH/2) + 10
# endif
# endif	/* __RUNTIME_YYMAXDEPTH */
long  yymaxdepth = YYMAXDEPTH;

__YYSCLASS YYSTYPE *yypv;			/* top of value stack */
__YYSCLASS int *yyps;			/* top of state stack */

__YYSCLASS int yystate;			/* current state */
__YYSCLASS int yytmp;			/* extra var (lasts between blocks) */

int yynerrs;			/* number of errors */
__YYSCLASS int yyerrflag;			/* error recovery flag */
int yychar;			/* current input token number */



/*
** yyparse - return 0 if worked, 1 if syntax error not recovered from
*/
int
yyparse()
{
	register YYSTYPE *yypvt;	/* top of value stack for $vars */

	/*
	** Initialize externals - yyparse may be called more than once
	*/
# ifdef __RUNTIME_YYMAXDEPTH
	if (allocate_stacks()) YYABORT;
# endif
	yypv = &yyv[-1];
	yyps = &yys[-1];
	yystate = 0;
	yytmp = 0;
	yynerrs = 0;
	yyerrflag = 0;
	yychar = -1;

	goto yystack;
	{
		register YYSTYPE *yy_pv;	/* top of value stack */
		register int *yy_ps;		/* top of state stack */
		register int yy_state;		/* current state */
		register int  yy_n;		/* internal state number info */

		/*
		** get globals into registers.
		** branch to here only if YYBACKUP was called.
		*/
	yynewstate:
		yy_pv = yypv;
		yy_ps = yyps;
		yy_state = yystate;
		goto yy_newstate;

		/*
		** get globals into registers.
		** either we just started, or we just finished a reduction
		*/
	yystack:
		yy_pv = yypv;
		yy_ps = yyps;
		yy_state = yystate;

		/*
		** top of for (;;) loop while no reductions done
		*/
	yy_stack:
		/*
		** put a state and value onto the stacks
		*/
#if YYDEBUG
		/*
		** if debugging, look up token value in list of value vs.
		** name pairs.  0 and negative (-1) are special values.
		** Note: linear search is used since time is not a real
		** consideration while debugging.
		*/
		if ( yydebug )
		{
			register int yy_i;

			printf( "State %d, token ", yy_state );
			if ( yychar == 0 )
				printf( "end-of-file\n" );
			else if ( yychar < 0 )
				printf( "-none-\n" );
			else
			{
				for ( yy_i = 0; yytoks[yy_i].t_val >= 0;
					yy_i++ )
				{
					if ( yytoks[yy_i].t_val == yychar )
						break;
				}
				printf( "%s\n", yytoks[yy_i].t_name );
			}
		}
#endif /* YYDEBUG */
		if ( ++yy_ps >= &yys[ yymaxdepth ] )	/* room on stack? */
		{
# ifndef __RUNTIME_YYMAXDEPTH
			yyerror( (nl_msg(30002,"yacc stack overflow")) );
			YYABORT;
# else
			/* save old stack bases to recalculate pointers */
			YYSTYPE * yyv_old = yyv;
			int * yys_old = yys;
			yymaxdepth += YYINCREMENT;
			yys = (int *) realloc(yys, yymaxdepth * sizeof(int));
			yyv = (YYSTYPE *) realloc(yyv, yymaxdepth * sizeof(YYSTYPE));
			if (yys==0 || yyv==0) {
			    yyerror( (nl_msg(30002,"yacc stack overflow")) );
			    YYABORT;
			    }
			/* Reset pointers into stack */
			yy_ps = (yy_ps - yys_old) + yys;
			yyps = (yyps - yys_old) + yys;
			yy_pv = (yy_pv - yyv_old) + yyv;
			yypv = (yypv - yyv_old) + yyv;
# endif

		}
		*yy_ps = yy_state;
		*++yy_pv = yyval;

		/*
		** we have a new state - find out what to do
		*/
	yy_newstate:
		if ( ( yy_n = yypact[ yy_state ] ) <= YYFLAG )
			goto yydefault;		/* simple state */
#if YYDEBUG
		/*
		** if debugging, need to mark whether new token grabbed
		*/
		yytmp = yychar < 0;
#endif
		if ( ( yychar < 0 ) && ( ( yychar = yylex() ) < 0 ) )
			yychar = 0;		/* reached EOF */
#if YYDEBUG
		if ( yydebug && yytmp )
		{
			register int yy_i;

			printf( "Received token " );
			if ( yychar == 0 )
				printf( "end-of-file\n" );
			else if ( yychar < 0 )
				printf( "-none-\n" );
			else
			{
				for ( yy_i = 0; yytoks[yy_i].t_val >= 0;
					yy_i++ )
				{
					if ( yytoks[yy_i].t_val == yychar )
						break;
				}
				printf( "%s\n", yytoks[yy_i].t_name );
			}
		}
#endif /* YYDEBUG */
		if ( ( ( yy_n += yychar ) < 0 ) || ( yy_n >= YYLAST ) )
			goto yydefault;
		if ( yychk[ yy_n = yyact[ yy_n ] ] == yychar )	/*valid shift*/
		{
			yychar = -1;
			yyval = yylval;
			yy_state = yy_n;
			if ( yyerrflag > 0 )
				yyerrflag--;
			goto yy_stack;
		}

	yydefault:
		if ( ( yy_n = yydef[ yy_state ] ) == -2 )
		{
#if YYDEBUG
			yytmp = yychar < 0;
#endif
			if ( ( yychar < 0 ) && ( ( yychar = yylex() ) < 0 ) )
				yychar = 0;		/* reached EOF */
#if YYDEBUG
			if ( yydebug && yytmp )
			{
				register int yy_i;

				printf( "Received token " );
				if ( yychar == 0 )
					printf( "end-of-file\n" );
				else if ( yychar < 0 )
					printf( "-none-\n" );
				else
				{
					for ( yy_i = 0;
						yytoks[yy_i].t_val >= 0;
						yy_i++ )
					{
						if ( yytoks[yy_i].t_val
							== yychar )
						{
							break;
						}
					}
					printf( "%s\n", yytoks[yy_i].t_name );
				}
			}
#endif /* YYDEBUG */
			/*
			** look through exception table
			*/
			{
				register int *yyxi = yyexca;

				while ( ( *yyxi != -1 ) ||
					( yyxi[1] != yy_state ) )
				{
					yyxi += 2;
				}
				while ( ( *(yyxi += 2) >= 0 ) &&
					( *yyxi != yychar ) )
					;
				if ( ( yy_n = yyxi[1] ) < 0 )
					YYACCEPT;
			}
		}

		/*
		** check for syntax error
		*/
		if ( yy_n == 0 )	/* have an error */
		{
			/* no worry about speed here! */
			switch ( yyerrflag )
			{
			case 0:		/* new error */
				yyerror( (nl_msg(30003,"syntax error")) );
				yynerrs++;
				goto skip_init;
			yyerrlab:
				/*
				** get globals into registers.
				** we have a user generated syntax type error
				*/
				yy_pv = yypv;
				yy_ps = yyps;
				yy_state = yystate;
				yynerrs++;
			skip_init:
			case 1:
			case 2:		/* incompletely recovered error */
					/* try again... */
				yyerrflag = 3;
				/*
				** find state where "error" is a legal
				** shift action
				*/
				while ( yy_ps >= yys )
				{
					yy_n = yypact[ *yy_ps ] + YYERRCODE;
					if ( yy_n >= 0 && yy_n < YYLAST &&
						yychk[yyact[yy_n]] == YYERRCODE)					{
						/*
						** simulate shift of "error"
						*/
						yy_state = yyact[ yy_n ];
						goto yy_stack;
					}
					/*
					** current state has no shift on
					** "error", pop stack
					*/
#if YYDEBUG
#	define _POP_ "Error recovery pops state %d, uncovers state %d\n"
					if ( yydebug )
						printf( _POP_, *yy_ps,
							yy_ps[-1] );
#	undef _POP_
#endif
					yy_ps--;
					yy_pv--;
				}
				/*
				** there is no state on stack with "error" as
				** a valid shift.  give up.
				*/
				YYABORT;
			case 3:		/* no shift yet; eat a token */
#if YYDEBUG
				/*
				** if debugging, look up token in list of
				** pairs.  0 and negative shouldn't occur,
				** but since timing doesn't matter when
				** debugging, it doesn't hurt to leave the
				** tests here.
				*/
				if ( yydebug )
				{
					register int yy_i;

					printf( "Error recovery discards " );
					if ( yychar == 0 )
						printf( "token end-of-file\n" );
					else if ( yychar < 0 )
						printf( "token -none-\n" );
					else
					{
						for ( yy_i = 0;
							yytoks[yy_i].t_val >= 0;
							yy_i++ )
						{
							if ( yytoks[yy_i].t_val
								== yychar )
							{
								break;
							}
						}
						printf( "token %s\n",
							yytoks[yy_i].t_name );
					}
				}
#endif /* YYDEBUG */
				if ( yychar == 0 )	/* reached EOF. quit */
					YYABORT;
				yychar = -1;
				goto yy_newstate;
			}
		}/* end if ( yy_n == 0 ) */
		/*
		** reduction by production yy_n
		** put stack tops, etc. so things right after switch
		*/
#if YYDEBUG
		/*
		** if debugging, print the string that is the user's
		** specification of the reduction which is just about
		** to be done.
		*/
		if ( yydebug )
			printf( "Reduce by (%d) \"%s\"\n",
				yy_n, yyreds[ yy_n ] );
#endif
		yytmp = yy_n;			/* value to switch over */
		yypvt = yy_pv;			/* $vars top of value stack */
		/*
		** Look in goto table for next state
		** Sorry about using yy_state here as temporary
		** register variable, but why not, if it works...
		** If yyr2[ yy_n ] doesn't have the low order bit
		** set, then there is no action to be done for
		** this reduction.  So, no saving & unsaving of
		** registers done.  The only difference between the
		** code just after the if and the body of the if is
		** the goto yy_stack in the body.  This way the test
		** can be made before the choice of what to do is needed.
		*/
		{
			/* length of production doubled with extra bit */
			register int yy_len = yyr2[ yy_n ];

			if ( !( yy_len & 01 ) )
			{
				yy_len >>= 1;
				yyval = ( yy_pv -= yy_len )[1];	/* $$ = $1 */
				yy_state = yypgo[ yy_n = yyr1[ yy_n ] ] +
					*( yy_ps -= yy_len ) + 1;
				if ( yy_state >= YYLAST ||
					yychk[ yy_state =
					yyact[ yy_state ] ] != -yy_n )
				{
					yy_state = yyact[ yypgo[ yy_n ] ];
				}
				goto yy_stack;
			}
			yy_len >>= 1;
			yyval = ( yy_pv -= yy_len )[1];	/* $$ = $1 */
			yy_state = yypgo[ yy_n = yyr1[ yy_n ] ] +
				*( yy_ps -= yy_len ) + 1;
			if ( yy_state >= YYLAST ||
				yychk[ yy_state = yyact[ yy_state ] ] != -yy_n )
			{
				yy_state = yyact[ yypgo[ yy_n ] ];
			}
		}
					/* save until reenter driver code */
		yystate = yy_state;
		yyps = yy_ps;
		yypv = yy_pv;
	}
	/*
	** code supplied by user is placed in this switch
	*/
	switch( yytmp )
	{
		
case 10:
# line 122 "gram.y"
{ AddIconRegion(yypvt[-4].ptr, yypvt[-3].num, yypvt[-2].num, yypvt[-1].num, yypvt[-0].num); } break;
case 11:
# line 123 "gram.y"
{ if (Scr->FirstTime)
						  {
						    Scr->iconmgr.geometry=yypvt[-1].ptr;
						    Scr->iconmgr.columns=yypvt[-0].num;
						  }
						} break;
case 12:
# line 129 "gram.y"
{ if (Scr->FirstTime)
						    Scr->iconmgr.geometry = yypvt[-0].ptr;
						} break;
case 13:
# line 132 "gram.y"
{ if (Scr->FirstTime)
					  {
						Scr->DoZoom = TRUE;
						Scr->ZoomCount = yypvt[-0].num;
					  }
					} break;
case 14:
# line 138 "gram.y"
{ if (Scr->FirstTime)
						Scr->DoZoom = TRUE; } break;
case 15:
# line 140 "gram.y"
{} break;
case 16:
# line 141 "gram.y"
{} break;
case 17:
# line 142 "gram.y"
{ list = &Scr->IconifyByUn; } break;
case 19:
# line 144 "gram.y"
{ if (Scr->FirstTime)
		    Scr->IconifyByUnmapping = TRUE; } break;
case 20:
# line 146 "gram.y"
{
					  GotTitleButton (yypvt[-2].ptr, yypvt[-0].num, False);
					} break;
case 21:
# line 149 "gram.y"
{
					  GotTitleButton (yypvt[-2].ptr, yypvt[-0].num, True);
					} break;
case 22:
# line 152 "gram.y"
{ root = GetRoot(yypvt[-0].ptr, NULLSTR, NULLSTR);
					  Scr->Mouse[yypvt[-1].num][C_ROOT][0].func = F_MENU;
					  Scr->Mouse[yypvt[-1].num][C_ROOT][0].menu = root;
					} break;
case 23:
# line 156 "gram.y"
{ Scr->Mouse[yypvt[-1].num][C_ROOT][0].func = yypvt[-0].num;
					  if (yypvt[-0].num == F_MENU)
					  {
					    pull->prev = NULL;
					    Scr->Mouse[yypvt[-1].num][C_ROOT][0].menu = pull;
					  }
					  else
					  {
					    root = GetRoot(TWM_ROOT,NULLSTR,NULLSTR);
					    Scr->Mouse[yypvt[-1].num][C_ROOT][0].item =
						AddToMenu(root,"x",Action,
							  NULLSTR,yypvt[-0].num,NULLSTR,NULLSTR);
					  }
					  Action = "";
					  pull = NULL;
					} break;
case 24:
# line 172 "gram.y"
{ GotKey(yypvt[-1].ptr, yypvt[-0].num); } break;
case 25:
# line 173 "gram.y"
{ GotButton(yypvt[-1].num, yypvt[-0].num); } break;
case 26:
# line 174 "gram.y"
{ list = &Scr->DontIconify; } break;
case 28:
# line 176 "gram.y"
{ list = &Scr->IconMgrNoShow; } break;
case 30:
# line 178 "gram.y"
{ Scr->IconManagerDontShow = TRUE; } break;
case 31:
# line 179 "gram.y"
{ list = &Scr->IconMgrs; } break;
case 33:
# line 181 "gram.y"
{ list = &Scr->IconMgrShow; } break;
case 35:
# line 183 "gram.y"
{ list = &Scr->NoTitleHighlight; } break;
case 37:
# line 185 "gram.y"
{ if (Scr->FirstTime)
						Scr->TitleHighlight = FALSE; } break;
case 38:
# line 187 "gram.y"
{ list = &Scr->NoHighlight; } break;
case 40:
# line 189 "gram.y"
{ if (Scr->FirstTime)
						Scr->Highlight = FALSE; } break;
case 41:
# line 191 "gram.y"
{ list = &Scr->NoStackModeL; } break;
case 43:
# line 193 "gram.y"
{ if (Scr->FirstTime)
						Scr->StackMode = FALSE; } break;
case 44:
# line 195 "gram.y"
{ list = &Scr->NoTitle; } break;
case 46:
# line 197 "gram.y"
{ if (Scr->FirstTime)
						Scr->NoTitlebar = TRUE; } break;
case 47:
# line 199 "gram.y"
{ list = &Scr->MakeTitle; } break;
case 49:
# line 201 "gram.y"
{ list = &Scr->StartIconified; } break;
case 51:
# line 203 "gram.y"
{ list = &Scr->AutoRaise; } break;
case 53:
# line 205 "gram.y"
{ Scr->AutoRaiseDefault = TRUE; } break;
case 54:
# line 206 "gram.y"
{
					root = GetRoot(yypvt[-5].ptr, yypvt[-3].ptr, yypvt[-1].ptr); } break;
case 55:
# line 208 "gram.y"
{ root->real_menu = TRUE;} break;
case 56:
# line 209 "gram.y"
{ root = GetRoot(yypvt[-0].ptr, NULLSTR, NULLSTR); } break;
case 57:
# line 210 "gram.y"
{ root->real_menu = TRUE; } break;
case 58:
# line 211 "gram.y"
{ root = GetRoot(yypvt[-0].ptr, NULLSTR, NULLSTR); } break;
case 60:
# line 213 "gram.y"
{ list = &Scr->IconNames; } break;
case 62:
# line 215 "gram.y"
{ color = COLOR; } break;
case 65:
# line 219 "gram.y"
{ color = MONOCHROME; } break;
case 67:
# line 221 "gram.y"
{ Scr->DefaultFunction.func = yypvt[-0].num;
					  if (yypvt[-0].num == F_MENU)
					  {
					    pull->prev = NULL;
					    Scr->DefaultFunction.menu = pull;
					  }
					  else
					  {
					    root = GetRoot(TWM_ROOT,NULLSTR,NULLSTR);
					    Scr->DefaultFunction.item =
						AddToMenu(root,"x",Action,
							  NULLSTR,yypvt[-0].num, NULLSTR, NULLSTR);
					  }
					  Action = "";
					  pull = NULL;
					} break;
case 68:
# line 237 "gram.y"
{ Scr->WindowFunction.func = yypvt[-0].num;
					   root = GetRoot(TWM_ROOT,NULLSTR,NULLSTR);
					   Scr->WindowFunction.item =
						AddToMenu(root,"x",Action,
							  NULLSTR,yypvt[-0].num, NULLSTR, NULLSTR);
					   Action = "";
					   pull = NULL;
					} break;
case 69:
# line 245 "gram.y"
{ list = &Scr->WarpCursorL; } break;
case 71:
# line 247 "gram.y"
{ if (Scr->FirstTime)
					    Scr->WarpCursor = TRUE; } break;
case 72:
# line 249 "gram.y"
{ list = &Scr->WindowRingL; } break;
case 74:
# line 251 "gram.y"
{	if (Scr->FirstTime)
						Scr->UseWindowRing = TRUE; } break;
case 75:
# line 253 "gram.y"
{ list = &Scr->NailedDown; } break;
case 77:
# line 256 "gram.y"
{ SetVirtualDesktop(yypvt[-1].ptr, yypvt[-0].num); } break;
case 78:
# line 257 "gram.y"
{ list = &Scr->DontShowInDisplay; } break;
case 80:
# line 262 "gram.y"
{ if (!do_single_keyword (yypvt[-0].num)) {
					    twmrc_error_prefix();
					    fprintf (stderr,
					"unknown singleton keyword %d\n",
						     yypvt[-0].num);
					    ParseError = 1;
					  }
					} break;
case 81:
# line 272 "gram.y"
{ if (!do_string_keyword (yypvt[-1].num, yypvt[-0].ptr)) {
					    twmrc_error_prefix();
					    fprintf (stderr,
				"unknown string keyword %d (value \"%s\")\n",
						     yypvt[-1].num, yypvt[-0].ptr);
					    ParseError = 1;
					  }
					} break;
case 82:
# line 282 "gram.y"
{ if (!do_number_keyword (yypvt[-1].num, yypvt[-0].num)) {
					    twmrc_error_prefix();
					    fprintf (stderr,
				"unknown numeric keyword %d (value %d)\n",
						     yypvt[-1].num, yypvt[-0].num);
					    ParseError = 1;
					  }
					} break;
case 83:
# line 294 "gram.y"
{ yyval.num = yypvt[-0].num; } break;
case 84:
# line 297 "gram.y"
{ yyval.num = yypvt[-0].num; } break;
case 87:
# line 304 "gram.y"
{ mods |= Mod1Mask; } break;
case 88:
# line 305 "gram.y"
{ mods |= ShiftMask; } break;
case 89:
# line 306 "gram.y"
{ mods |= LockMask; } break;
case 90:
# line 307 "gram.y"
{ mods |= ControlMask; } break;
case 91:
# line 308 "gram.y"
{ if (yypvt[-0].num < 1 || yypvt[-0].num > 5) {
					     twmrc_error_prefix();
					     fprintf (stderr,
				"bad modifier number (%d), must be 1-5\n",
						      yypvt[-0].num);
					     ParseError = 1;
					  } else {
					     mods |= (Mod1Mask << (yypvt[-0].num - 1));
					  }
					} break;
case 92:
# line 318 "gram.y"
{ } break;
case 95:
# line 325 "gram.y"
{ cont |= C_WINDOW_BIT; } break;
case 96:
# line 326 "gram.y"
{ cont |= C_TITLE_BIT; } break;
case 97:
# line 327 "gram.y"
{ cont |= C_ICON_BIT; } break;
case 98:
# line 328 "gram.y"
{ cont |= C_ROOT_BIT; } break;
case 99:
# line 329 "gram.y"
{ cont |= C_FRAME_BIT; } break;
case 100:
# line 330 "gram.y"
{ cont |= C_ICONMGR_BIT; } break;
case 101:
# line 331 "gram.y"
{ cont |= C_ICONMGR_BIT; } break;
case 102:
# line 332 "gram.y"
{ cont |= C_VIRTUAL_BIT; } break;
case 103:
# line 333 "gram.y"
{ cont |= C_VIRTUAL_WIN_BIT; } break;
case 104:
# line 334 "gram.y"
{ cont |= C_DOOR_BIT; } break;
case 105:
# line 335 "gram.y"
{ cont |= C_ALL_BITS; } break;
case 106:
# line 336 "gram.y"
{  } break;
case 109:
# line 343 "gram.y"
{ cont |= C_WINDOW_BIT; } break;
case 110:
# line 344 "gram.y"
{ cont |= C_TITLE_BIT; } break;
case 111:
# line 345 "gram.y"
{ cont |= C_ICON_BIT; } break;
case 112:
# line 346 "gram.y"
{ cont |= C_ROOT_BIT; } break;
case 113:
# line 347 "gram.y"
{ cont |= C_FRAME_BIT; } break;
case 114:
# line 348 "gram.y"
{ cont |= C_ICONMGR_BIT; } break;
case 115:
# line 349 "gram.y"
{ cont |= C_ICONMGR_BIT; } break;
case 116:
# line 350 "gram.y"
{ cont |= C_VIRTUAL_BIT; } break;
case 117:
# line 351 "gram.y"
{ cont |= C_VIRTUAL_WIN_BIT; } break;
case 118:
# line 352 "gram.y"
{ cont |= C_DOOR_BIT; } break;
case 119:
# line 353 "gram.y"
{ cont |= C_ALL_BITS; } break;
case 120:
# line 354 "gram.y"
{ } break;
case 121:
# line 355 "gram.y"
{ Name = yypvt[-0].ptr; cont |= C_NAME_BIT; } break;
case 125:
# line 366 "gram.y"
{ SetHighlightPixmap (yypvt[-0].ptr); } break;
case 126:
# line 367 "gram.y"
{ SetVirtualPixmap (yypvt[-0].ptr); } break;
case 127:
# line 368 "gram.y"
{ SetRealScreenPixmap (yypvt[-0].ptr); } break;
case 131:
# line 379 "gram.y"
{
			NewBitmapCursor(&Scr->FrameCursor, yypvt[-1].ptr, yypvt[-0].ptr); } break;
case 132:
# line 381 "gram.y"
{
			NewFontCursor(&Scr->FrameCursor, yypvt[-0].ptr); } break;
case 133:
# line 383 "gram.y"
{
			NewBitmapCursor(&Scr->TitleCursor, yypvt[-1].ptr, yypvt[-0].ptr); } break;
case 134:
# line 385 "gram.y"
{
			NewFontCursor(&Scr->TitleCursor, yypvt[-0].ptr); } break;
case 135:
# line 387 "gram.y"
{
			NewBitmapCursor(&Scr->IconCursor, yypvt[-1].ptr, yypvt[-0].ptr); } break;
case 136:
# line 389 "gram.y"
{
			NewFontCursor(&Scr->IconCursor, yypvt[-0].ptr); } break;
case 137:
# line 391 "gram.y"
{
			NewBitmapCursor(&Scr->IconMgrCursor, yypvt[-1].ptr, yypvt[-0].ptr); } break;
case 138:
# line 393 "gram.y"
{
			NewFontCursor(&Scr->IconMgrCursor, yypvt[-0].ptr); } break;
case 139:
# line 395 "gram.y"
{
			NewBitmapCursor(&Scr->ButtonCursor, yypvt[-1].ptr, yypvt[-0].ptr); } break;
case 140:
# line 397 "gram.y"
{
			NewFontCursor(&Scr->ButtonCursor, yypvt[-0].ptr); } break;
case 141:
# line 399 "gram.y"
{
			NewBitmapCursor(&Scr->MoveCursor, yypvt[-1].ptr, yypvt[-0].ptr); } break;
case 142:
# line 401 "gram.y"
{
			NewFontCursor(&Scr->MoveCursor, yypvt[-0].ptr); } break;
case 143:
# line 403 "gram.y"
{
			NewBitmapCursor(&Scr->ResizeCursor, yypvt[-1].ptr, yypvt[-0].ptr); } break;
case 144:
# line 405 "gram.y"
{
			NewFontCursor(&Scr->ResizeCursor, yypvt[-0].ptr); } break;
case 145:
# line 407 "gram.y"
{
			NewBitmapCursor(&Scr->WaitCursor, yypvt[-1].ptr, yypvt[-0].ptr); } break;
case 146:
# line 409 "gram.y"
{
			NewFontCursor(&Scr->WaitCursor, yypvt[-0].ptr); } break;
case 147:
# line 411 "gram.y"
{
			NewBitmapCursor(&Scr->MenuCursor, yypvt[-1].ptr, yypvt[-0].ptr); } break;
case 148:
# line 413 "gram.y"
{
			NewFontCursor(&Scr->MenuCursor, yypvt[-0].ptr); } break;
case 149:
# line 415 "gram.y"
{
			NewBitmapCursor(&Scr->SelectCursor, yypvt[-1].ptr, yypvt[-0].ptr); } break;
case 150:
# line 417 "gram.y"
{
			NewFontCursor(&Scr->SelectCursor, yypvt[-0].ptr); } break;
case 151:
# line 419 "gram.y"
{
			NewBitmapCursor(&Scr->DestroyCursor, yypvt[-1].ptr, yypvt[-0].ptr); } break;
case 152:
# line 421 "gram.y"
{
			NewFontCursor(&Scr->DestroyCursor, yypvt[-0].ptr); } break;
case 153:
# line 423 "gram.y"
{/*RFBCURSOR*/
			NewBitmapCursor(&Scr->DoorCursor, yypvt[-1].ptr, yypvt[-0].ptr); } break;
case 154:
# line 425 "gram.y"
{/*RFBCURSOR*/
			NewFontCursor(&Scr->DoorCursor, yypvt[-0].ptr); } break;
case 155:
# line 427 "gram.y"
{/*RFBCURSOR*/
			NewBitmapCursor(&Scr->VirtualCursor, yypvt[-1].ptr, yypvt[-0].ptr); } break;
case 156:
# line 429 "gram.y"
{/*RFBCURSOR*/
			NewFontCursor(&Scr->VirtualCursor, yypvt[-0].ptr); } break;
case 157:
# line 431 "gram.y"
{/*RFBCURSOR*/
			NewBitmapCursor(&Scr->DesktopCursor, yypvt[-1].ptr, yypvt[-0].ptr); } break;
case 158:
# line 433 "gram.y"
{/*RFBCURSOR*/
			NewFontCursor(&Scr->DesktopCursor, yypvt[-0].ptr); } break;
case 162:
# line 445 "gram.y"
{ if (!do_colorlist_keyword (yypvt[-1].num, color,
								     yypvt[-0].ptr)) {
					    twmrc_error_prefix();
					    fprintf (stderr,
			"unhandled list color keyword %d (string \"%s\")\n",
						     yypvt[-1].num, yypvt[-0].ptr);
					    ParseError = 1;
					  }
					} break;
case 163:
# line 454 "gram.y"
{ list = do_colorlist_keyword(yypvt[-1].num,color,
								      yypvt[-0].ptr);
					  if (!list) {
					    twmrc_error_prefix();
					    fprintf (stderr,
			"unhandled color list keyword %d (string \"%s\")\n",
						     yypvt[-1].num, yypvt[-0].ptr);
					    ParseError = 1;
					  }
					} break;
case 165:
# line 465 "gram.y"
{ if (!do_color_keyword (yypvt[-1].num, color,
								 yypvt[-0].ptr)) {
					    twmrc_error_prefix();
					    fprintf (stderr,
			"unhandled color keyword %d (string \"%s\")\n",
						     yypvt[-1].num, yypvt[-0].ptr);
					    ParseError = 1;
					  }
					} break;
case 169:
# line 483 "gram.y"
{ do_string_savecolor(color, yypvt[-0].ptr); } break;
case 170:
# line 484 "gram.y"
{ do_var_savecolor(yypvt[-0].num); } break;
case 174:
# line 494 "gram.y"
{ if (Scr->FirstTime &&
					      color == Scr->Monochrome)
					    AddToList(list, yypvt[-1].ptr, yypvt[-0].ptr); } break;
case 175:
# line 499 "gram.y"
{
				    if (HasShape) Scr->SqueezeTitle = TRUE;
				} break;
case 176:
# line 502 "gram.y"
{ list = &Scr->SqueezeTitleL;
				  if (HasShape && Scr->SqueezeTitle == -1)
				    Scr->SqueezeTitle = TRUE;
				} break;
case 178:
# line 507 "gram.y"
{ Scr->SqueezeTitle = FALSE; } break;
case 179:
# line 508 "gram.y"
{ list = &Scr->DontSqueezeTitleL; } break;
case 182:
# line 513 "gram.y"
{
				if (Scr->FirstTime) {
				   do_squeeze_entry (list, yypvt[-3].ptr, yypvt[-2].num, yypvt[-1].num, yypvt[-0].num);
				}
			} break;
case 186:
# line 528 "gram.y"
{
				(void) door_add(yypvt[-2].ptr, yypvt[-1].ptr, yypvt[-0].ptr);
			} break;
case 190:
# line 540 "gram.y"
{ if (Scr->FirstTime)
					    AddToList(list, yypvt[-2].ptr, (char *)
						AllocateIconManager(yypvt[-2].ptr, NULLSTR,
							yypvt[-1].ptr,yypvt[-0].num));
					} break;
case 191:
# line 546 "gram.y"
{ if (Scr->FirstTime)
					    AddToList(list, yypvt[-3].ptr, (char *)
						AllocateIconManager(yypvt[-3].ptr,yypvt[-2].ptr,
						yypvt[-1].ptr, yypvt[-0].num));
					} break;
case 195:
# line 560 "gram.y"
{ if (Scr->FirstTime)
					    AddToList(list, yypvt[-0].ptr, 0);
					} break;
case 199:
# line 572 "gram.y"
{ if (Scr->FirstTime) AddToList(list, yypvt[-1].ptr, yypvt[-0].ptr); } break;
case 203:
# line 582 "gram.y"
{ AddToMenu(root, "", Action, NULLSTR, yypvt[-0].num,
						NULLSTR, NULLSTR);
					  Action = "";
					} break;
case 207:
# line 595 "gram.y"
{ AddToMenu(root, yypvt[-1].ptr, Action, pull, yypvt[-0].num,
						NULLSTR, NULLSTR);
					  Action = "";
					  pull = NULL;
					} break;
case 208:
# line 600 "gram.y"
{
					  AddToMenu(root, yypvt[-6].ptr, Action, pull, yypvt[-0].num,
						yypvt[-4].ptr, yypvt[-2].ptr);
					  Action = "";
					  pull = NULL;
					} break;
case 209:
# line 608 "gram.y"
{ yyval.num = yypvt[-0].num; } break;
case 210:
# line 609 "gram.y"
{
				yyval.num = yypvt[-1].num;
				Action = yypvt[-0].ptr;
				switch (yypvt[-1].num) {
				  case F_MENU:
				    pull = GetRoot (yypvt[-0].ptr, NULLSTR,NULLSTR);
				    pull->prev = root;
				    break;
				  case F_WARPRING:
				    if (!CheckWarpRingArg (Action)) {
					twmrc_error_prefix();
					fprintf (stderr,
			"ignoring invalid f.warptoring argument \"%s\"\n",
						 Action);
					yyval.num = F_NOP;
				    }
				  case F_WARPTOSCREEN:
				    if (!CheckWarpScreenArg (Action)) {
					twmrc_error_prefix();
					fprintf (stderr,
			"ignoring invalid f.warptoscreen argument \"%s\"\n",
					         Action);
					yyval.num = F_NOP;
				    }
				    break;
				  case F_COLORMAP:
				    if (CheckColormapArg (Action)) {
					yyval.num = F_COLORMAP;
				    } else {
					twmrc_error_prefix();
					fprintf (stderr,
			"ignoring invalid f.colormap argument \"%s\"\n",
						 Action);
					yyval.num = F_NOP;
				    }
				    break;
				} /* end switch */
				   } break;
case 211:
# line 650 "gram.y"
{ yyval.num = yypvt[-0].num; } break;
case 212:
# line 651 "gram.y"
{ yyval.num = yypvt[-0].num; } break;
case 213:
# line 652 "gram.y"
{ yyval.num = -(yypvt[-0].num); } break;
case 214:
# line 655 "gram.y"
{ yyval.num = yypvt[-0].num;
					  if (yypvt[-0].num == 0)
						yyerror("bad button 0");

					  if (yypvt[-0].num > MAX_BUTTONS)
					  {
						yyval.num = 0;
						yyerror("button number too large");
					  }
					} break;
case 215:
# line 667 "gram.y"
{ ptr = (char *)malloc(strlen(yypvt[-0].ptr)+1);
					  strcpy(ptr, yypvt[-0].ptr);
					  RemoveDQuote(ptr);
					  yyval.ptr = ptr;
					} break;
case 216:
# line 672 "gram.y"
{ yyval.num = yypvt[-0].num; } break;
	}
	goto yystack;		/* reset registers in driver code */
}

# ifdef __RUNTIME_YYMAXDEPTH

static int allocate_stacks() {
	/* allocate the yys and yyv stacks */
	yys = (int *) malloc(yymaxdepth * sizeof(int));
	yyv = (YYSTYPE *) malloc(yymaxdepth * sizeof(YYSTYPE));

	if (yys==0 || yyv==0) {
	   yyerror( (nl_msg(30004,"unable to allocate space for yacc stacks")) );
	   return(1);
	   }
	else return(0);

}


static void free_stacks() {
	if (yys!=0) free((char *) yys);
	if (yyv!=0) free((char *) yyv);
}

# endif  /* defined(__RUNTIME_YYMAXDEPTH) */

