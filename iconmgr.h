
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
 * $XConsortium: iconmgr.h,v 1.11 89/12/10 17:47:02 jim Exp $
 *
 * Icon Manager includes
 *
 * 09-Mar-89 Tom LaStrange		File Created
 *
 ***********************************************************************/

#ifndef _ICONMGR_
#define _ICONMGR_

typedef struct WList
{
  struct WList *next;
  struct WList *prev;
  struct TwmWindow *twm;
  struct IconMgr *iconmgr;
  MyWindow w;
  Window icon;
  int x, y, width, height;
  int row, col;
  int me;

  ColorPair cp;
  Image *iconifypm;

  Pixel fore, back, highlight;
  unsigned top, bottom;
  short active;
  short down;
} WList;

typedef struct IconMgr
{
  struct IconMgr *next;		/* pointer to the next icon manager */
  struct IconMgr *prev;		/* pointer to the previous icon mgr */
  struct IconMgr *lasti;	/* pointer to the last icon mgr */
  struct WList *first;		/* first window in the list */
  struct WList *last;		/* last window in the list */
  struct WList *active;		/* the active entry */
  TwmWindow *twm_win;		/* back pointer to the new parent */
  struct ScreenInfo *scr;	/* the screen this thing is on */
  Window w;			/* this icon manager window */
  char *geometry;		/* geometry string */
  char *name;
  char *icon_name;
  int x, y, width, height;
  int columns, cur_rows, cur_columns;
  int count;
} IconMgr;


#define VTWM_ICONMGR_CLASS "VTWM Icon Manager"

#endif /* _ICONMGR_ */


/*
  Local Variables:
  mode:c
  c-file-style:"GNU"
  c-file-offsets:((substatement-open 0)(brace-list-open 0)(c-hanging-comment-ender-p . nil)(c-hanging-comment-beginner-p . nil)(comment-start . "// ")(comment-end . "")(comment-column . 48))
  End:
*/
/* vim: sw=2
*/
