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


extern void	Zoom();
extern void	MoveOutline();
extern void	GetUnknownIcon();
extern char 	*ExpandFilename();
extern void		GetColor();
extern Cursor	NoCursor();

extern Image *GetImage ();
extern void Draw3DBorder (Drawable w, int x, int y, int width, int height,
			int bw, ColorPair cp, int state, int fill, int forcebw);
extern void GetShadeColors();
extern void PaintBorders();
extern void PaintIcon();
extern void PaintTitle();
extern void PaintTitleButton();
extern void InsertRGBColormap();
extern void RemoveRGBColormap();
extern void SetFocus();
extern void LocateStandardColormaps();

extern void GetFont (MyFont *font);

extern int  MyFont_TextWidth (MyFont *font, char *string, int len);

extern void MyFont_DrawImageString (Display *dpy, MyWindow *win, MyFont *font,
					ColorPair *col,
					int x, int y, char * string, int len);

extern void MyFont_DrawString (Display *dpy, MyWindow *win, MyFont *font,
					ColorPair *col,
					int x, int y, char * string, int len);

extern Status I18N_FetchName (Display *dpy, Window w, char **winname);
extern Status I18N_GetIconName (Display *dpy, Window w, char **iconname);


void setBorderGC();
#ifdef USE_ORIGINAL_CORNERS
void Draw3DCorner();
#else
GC setBevelGC();
void Draw3DBevel();
#endif

void PaintTitleHighlight();
int ComputeHighlightWindowWidth();
extern Image *SetPixmapsPixmap();

#ifndef NO_XPM_SUPPORT
extern int SetPixmapsBackground();
#endif

#ifdef TWM_USE_XFT
extern XftDraw * MyXftDrawCreate (Window win);
extern void MyXftDrawDestroy (XftDraw *draw);
extern void CopyPixelToXftColor (unsigned long pixel, XftColor *col);
#endif
#ifdef TWM_USE_OPACITY	 /*opacity: 0 = transparent ... 255 = opaque*/
extern void SetWindowOpacity (Window win, unsigned int opacity);
extern void PropagateWindowOpacity (TwmWindow *tmp);
#endif

extern int HotX, HotY;

#endif /* _UTIL_ */
