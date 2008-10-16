
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
 * regions.c
 *
 * Region related routines
 *
 * 4/26/99 D. J. Hawkey Jr.
 *
 **********************************************************************/

#include <stdio.h>
#include <string.h>
#include "twm.h"
#include "screen.h"
#include "list.h"
#include "regions.h"
#include "gram.h"
#include "parse.h"
#include "image_formats.h"
#include "util.h"
#include "prototypes.h"

void
splitRegionEntry(RegionEntry * re, int grav1, int grav2, int w, int h)
{
  RegionEntry *new;

  switch (grav1)
  {
  case D_NORTH:
  case D_SOUTH:
    if (w != re->w)
      splitRegionEntry(re, grav2, grav1, w, re->h);
    if (h != re->h)
    {
      new = (RegionEntry *) malloc(sizeof(RegionEntry));
      new->u.twm_win = 0;
      new->type = LTYPE_EXACT_NAME;
      new->usedby = 0;
      new->next = re->next;
      re->next = new;
      new->x = re->x;
      new->h = (re->h - h);
      new->w = re->w;
      re->h = h;
      if (grav1 == D_SOUTH)
      {
	new->y = re->y;
	re->y = new->y + new->h;
      }
      else
	new->y = re->y + re->h;
    }
    break;
  case D_EAST:
  case D_WEST:
    if (h != re->h)
      splitRegionEntry(re, grav2, grav1, re->w, h);
    if (w != re->w)
    {
      new = (RegionEntry *) malloc(sizeof(RegionEntry));
      new->u.twm_win = 0;
      new->type = LTYPE_EXACT_NAME;
      new->usedby = 0;
      new->next = re->next;
      re->next = new;
      new->y = re->y;
      new->w = (re->w - w);
      new->h = re->h;
      re->w = w;
      if (grav1 == D_EAST)
      {
	new->x = re->x;
	re->x = new->x + new->w;
      }
      else
	new->x = re->x + re->w;
    }
    break;
  }
}

int
roundEntryUp(int v, int multiple)
{
  return ((v + multiple - 1) / multiple) * multiple;
}

RegionEntry *
prevRegionEntry(RegionEntry * re, RootRegion * rr)
{
  RegionEntry *ep;

  if (re == rr->entries)
    return 0;
  for (ep = rr->entries; ep->next != re; ep = ep->next)
    ;
  return ep;
}

/*
 * old is being freed; and is adjacent to re.  Merge regions together.
 */
void
mergeRegionEntries(RegionEntry * old, RegionEntry * re)
{
  if (old->y == re->y)
  {
    re->w = old->w + re->w;
    if (old->x < re->x)
      re->x = old->x;
  }
  else
  {
    re->h = old->h + re->h;
    if (old->y < re->y)
      re->y = old->y;
  }
}

void
downRegionEntry(RootRegion * rr, RegionEntry * re)
{
  RegionEntry *ep, *en;

  re->u.twm_win = 0;
  re->usedby = 0;
  ep = prevRegionEntry(re, rr);
  en = re->next;
  for (;;)
  {
    if (ep && ep->usedby == 0 && ((ep->x == re->x && ep->w == re->w) || (ep->y == re->y && ep->h == re->h)))
    {
      ep->next = re->next;
      mergeRegionEntries(re, ep);
      if (re->usedby == USEDBY_NAME)
	free(re->u.name);
#ifndef NO_REGEX_SUPPORT
      if (re->type & LTYPE_C_REGEXP)
	regfree(&re->re);
#endif
      free((char *)re);
      re = ep;
      ep = prevRegionEntry(ep, rr);
    }
    else if (en && en->usedby == 0 && ((en->x == re->x && en->w == re->w) || (en->y == re->y && en->h == re->h)))
    {
      re->next = en->next;
      mergeRegionEntries(en, re);
      if (en->usedby == USEDBY_NAME)
	free(en->u.name);
#ifndef NO_REGEX_SUPPORT
      if (en->type & LTYPE_C_REGEXP)
	regfree(&en->re);
#endif
      free((char *)en);
      en = re->next;
    }
    else
      break;
  }
}

RootRegion *
AddRegion(char *geom, int grav1, int grav2, int stepx, int stepy)
{
  RootRegion *rr;
  int mask;

  rr = (RootRegion *) malloc(sizeof(RootRegion));
  rr->next = NULL;
  rr->grav1 = grav1;
  rr->grav2 = grav2;
  rr->stepx = (stepx <= 0) ? 2 : stepx;	/* hard-coded value was '1' - djhjr - 9/26/99 */
  rr->stepy = (stepy <= 0) ? 1 : stepy;
  rr->x = rr->y = rr->w = rr->h = 0;

  mask = XParseGeometry(geom, &rr->x, &rr->y, (unsigned int *)&rr->w, (unsigned int *)&rr->h);

#ifdef TILED_SCREEN
  if (Scr->use_tiles == TRUE)
    EnsureGeometryVisibility(mask, &rr->x, &rr->y, rr->w, rr->h);
  else
#endif
  {
    if (mask & XNegative)
      rr->x += Scr->MyDisplayWidth - rr->w;
    if (mask & YNegative)
      rr->y += Scr->MyDisplayHeight - rr->h;
  }

  rr->entries = (RegionEntry *) malloc(sizeof(RegionEntry));
  rr->entries->next = 0;
  rr->entries->x = rr->x;
  rr->entries->y = rr->y;
  rr->entries->w = rr->w;
  rr->entries->h = rr->h;
  rr->entries->u.twm_win = 0;
  rr->entries->type = LTYPE_EXACT_NAME;
  rr->entries->usedby = 0;

  return rr;
}

void
FreeRegionEntries(RootRegion * rr)
{
  RegionEntry *re, *tmp;

  for (re = rr->entries; re; re = tmp)
  {
    tmp = re->next;
    if (re->usedby == USEDBY_NAME)
      free(re->u.name);
#ifndef NO_REGEX_SUPPORT
    if (re->type & LTYPE_C_REGEXP)
      regfree(&re->re);
#endif
    free((char *)re);
  }
}

void
FreeRegions(RootRegion * first, RootRegion * last)
{
  RootRegion *rr, *tmp;

  for (rr = first; rr != NULL;)
  {
    tmp = rr;
    FreeRegionEntries(rr);
    rr = rr->next;
    free((char *)tmp);
  }
  first = NULL;
  last = NULL;
}


/*
  Local Variables:
  mode:c
  c-file-style:"GNU"
  c-file-offsets:((substatement-open 0)(brace-list-open 0)(c-hanging-comment-ender-p . nil)(c-hanging-comment-beginner-p . nil)(comment-start . "// ")(comment-end . "")(comment-column . 48))
  End:
*/
/* vim: sw=2: */
