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

/**********************************************************************
 *
 * $XConsortium: gc.c,v 1.22 91/01/09 17:13:12 rws Exp $
 *
 * Open the fonts and create the GCs
 *
 * 31-Mar-88 Tom LaStrange        Initial Version.
 *
 * Do the necessary modification to be integrated in ctwm.
 * Can no longer be used for the standard twm.
 *
 * 22-April-92 Claude Lecommandeur.
 *
 *
 **********************************************************************/

#include <stdio.h>
#include "twm.h"
#include "image_formats.h"
#include "util.h"
#include "screen.h"
#include "prototypes.h"

/***********************************************************************
 *
 *  Procedure:
 *	CreateGCs - open fonts and create all the needed GC's.  I only
 *		    want to do this once, hence the first_time flag.
 *
 ***********************************************************************
 */

void
CreateGCs(void)
{
  static ScreenInfo *prevScr = NULL;
  XGCValues gcv;
  unsigned long gcm;
  static unsigned char greypattern[] = { 0x0f, 0x05, 0x0f, 0x0a };
  Pixmap greypixmap;
  static char dashlist[2] = { 1, 1 };

  if (!Scr->FirstTime || prevScr == Scr)
    return;

  prevScr = Scr;

  /* create GC's */

  gcm = 0;
  gcm |= GCFunction;
  gcv.function = GXxor;
  gcm |= GCLineWidth;
  gcv.line_width = 0;
  gcm |= GCForeground;
  gcv.foreground = Scr->XORvalue;
  gcm |= GCSubwindowMode;
  gcv.subwindow_mode = IncludeInferiors;

  Scr->DrawGC = XCreateGC(dpy, Scr->Root, gcm, &gcv);

  gcm = 0;
  gcm |= GCForeground;
  gcv.foreground = Scr->MenuC.fore;
  gcm |= GCBackground;
  gcv.background = Scr->MenuC.back;
  gcm |= GCGraphicsExposures;
  gcv.graphics_exposures = False;

#ifdef TWM_USE_XFT
  if (!(Scr->use_xft > 0))
#endif
    if (use_fontset != True)
    {
      gcm |= GCFont;
      gcv.font = Scr->MenuFont.font->fid;
    }

  Scr->MenuGC = XCreateGC(dpy, Scr->Root, gcm, &gcv);

  gcm = 0;
  gcm |= GCPlaneMask;
  gcv.plane_mask = AllPlanes;
  /*
   * Prevent GraphicsExpose and NoExpose events.  We'd only get NoExpose
   * events anyway;  they cause BadWindow errors from XGetWindowAttributes
   * call in FindScreenInfo (events.c) (since drawable is a pixmap).
   */
  gcm |= GCGraphicsExposures;
  gcv.graphics_exposures = False;
  gcm |= GCLineWidth;
  gcv.line_width = 0;

  Scr->NormalGC = XCreateGC(dpy, Scr->Root, gcm, &gcv);

  greypixmap = XCreatePixmapFromBitmapData(dpy, Scr->Root, (char *)greypattern, 4, 4, 1, 0, 1);

  gcm = 0;
  gcm |= GCStipple;
  gcv.stipple = greypixmap;
  gcm |= GCFillStyle;
  gcv.fill_style = FillOpaqueStippled;
  gcm |= GCForeground;
  gcv.foreground = Scr->Black;
  gcm |= GCBackground;
  gcv.background = Scr->White;
  Scr->GreyGC = XCreateGC(dpy, Scr->Root, gcm, &gcv);
  XSetDashes(dpy, Scr->GreyGC, 1, dashlist, 2);

  if (Scr->BeNiceToColormap)
  {
    gcm = 0;
    gcm |= GCLineStyle;
    gcv.line_style = LineDoubleDash;
    Scr->ShadGC = XCreateGC(dpy, Scr->Root, gcm, &gcv);
    XSetDashes(dpy, Scr->ShadGC, 0, dashlist, 2);
  }

  gcm = 0;
  Scr->RootGC = XCreateGC(dpy, Scr->Root, gcm, &gcv);

  /* depth-1 pixmap-GC for shape mask copying: */
  gcm = 0;
  gcv.graphics_exposures = False; /* suppress 'NoExpose' events */
  gcm |= GCGraphicsExposures;
  gcv.foreground = WhitePixel (dpy, Scr->screen); /* '1' */
  gcv.background = BlackPixel (dpy, Scr->screen); /* '0' */
  gcm |= GCForeground;
  gcm |= GCBackground;
  Scr->BitGC = XCreateGC(dpy, greypixmap, gcm, &gcv);
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
