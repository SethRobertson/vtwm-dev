
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

/**********************************************************************
 *
 * regions.h (was icons.h)
 *
 * Region related definitions
 *
 * 4/26/99 D. J. Hawkey Jr.
 *
 **********************************************************************/

#ifndef REGIONS_H
#define REGIONS_H

#define USEDBY_TWIN 1
#define USEDBY_NAME 2

#ifndef NO_REGEX_SUPPORT
#include <sys/types.h>
#include <regex.h>
#endif

typedef struct RootRegion
{
  struct RootRegion *next;
  int x, y, w, h;
  int grav1, grav2;
  int stepx, stepy;
  struct RegionEntry *entries;
} RootRegion;

typedef struct RegionEntry
{
  struct RegionEntry *next;
  int x, y, w, h;

  /* icons use twm_win, applets use both - djhjr - 4/26/99 */
  union
  {
    TwmWindow *twm_win;
    char *name;
  } u;

#ifndef NO_REGEX_SUPPORT
  regex_t re;
#else
  char re;
#endif
  short type;

  short usedby;
} RegionEntry;

#endif /* REGIONS_H */


/*
  Local Variables:
  mode:c
  c-file-style:"GNU"
  c-file-offsets:((substatement-open 0)(brace-list-open 0)(c-hanging-comment-ender-p . nil)(c-hanging-comment-beginner-p . nil)(comment-start . "// ")(comment-end . "")(comment-column . 48))
  End:
*/
/* vim: sw=2
*/
