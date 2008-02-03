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
 * $XConsortium: util.h,v 1.10 89/12/10 17:47:04 jim Exp $
 *
 * utility routines header file
 *
 * 28-Oct-87 Thomas E. LaStrange		File created
 *
 ***********************************************************************/

#ifndef _UTIL_
#define _UTIL_

typedef struct _Image {
    Pixmap pixmap;
    Pixmap mask;
    int    width;
    int    height;
    struct _Image *next;
} Image;

extern void	Zoom();
extern void	MoveOutline();
extern Pixmap	GetBitmap(), FindBitmap();
#ifndef NO_XPM_SUPPORT
extern Image *FindImage();
#endif
extern void	GetUnknownIcon();
extern char 	*ExpandFilename();
extern void		GetColor();
extern Cursor	NoCursor();

extern Image *GetImage ();
extern void Draw3DBorder();
extern void GetShadeColors();
extern void PaintBorders();
extern void PaintIcon();
extern void PaintTitle();
extern void PaintTitleButton();
extern void InsertRGBColormap();
extern void RemoveRGBColormap();
extern void SetFocus();
extern void LocateStandardColormaps();
extern void GetFont();

extern int MyFont_TextWidth();

extern void MyFont_DrawImageString (Display *dpy, Drawable d, MyFont *font,
					ColorPair *col,
					int x, int y, char * string, int len);

extern void MyFont_DrawString (Display *dpy, Drawable d, MyFont *font,
					ColorPair *col,
					int x, int y, char * string, int len);

extern Status I18N_FetchName (Display *dpy, Window w, char **winname);
extern Status I18N_GetIconName (Display *dpy, Window w, char **iconname);

/* djhjr - 1/13/98 */
void setBorderGC();
#ifdef USE_ORIGINAL_CORNERS
void Draw3DCorner();
#else
GC setBevelGC();
void Draw3DBevel();
#endif

/* djhjr - 4/25/96 */
void PaintTitleHighlight();

/* djhjr - 4/2/98 */
int ComputeHighlightWindowWidth();

/* djhjr - 5/17/98 */
extern Image *SetPixmapsPixmap();

/* djhjr - 5/23/98 */
#ifndef NO_XPM_SUPPORT
extern int SetPixmapsBackground();
#endif

extern int HotX, HotY;

#endif /* _UTIL_ */
