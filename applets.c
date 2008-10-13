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
 * applets.c
 *
 * Applet region related routines
 *
 * 4/26/99 D. J. Hawkey Jr.
 *
 **********************************************************************/

#include <stdio.h>
#include <string.h>
#include "twm.h"
#include "screen.h"
#include "regions.h"
#include "gram.h"
#include "parse.h"
#include "image_formats.h"
#include "util.h"

extern void twmrc_error_prefix();
extern int MatchName();

extern void splitRegionEntry();
extern int roundEntryUp();
extern RegionEntry *prevRegionEntry();
extern void mergeRegionEntries();
extern void downRegionEntry();
extern RootRegion *AddRegion();

int appletWidth(tmp_win)
TwmWindow *tmp_win;
{
	if (Scr->NoBorders || LookInList(Scr->NoBorder, tmp_win->full_name, &tmp_win->class))
		return tmp_win->attr.width;
	else
		return Scr->BorderWidth * 2 + tmp_win->attr.width;
}

int appletHeight(tmp_win)
TwmWindow *tmp_win;
{
	if (Scr->NoBorders || LookInList(Scr->NoBorder, tmp_win->full_name, &tmp_win->class))
		return tmp_win->title_height + tmp_win->attr.height;
	else
		return Scr->BorderWidth * 2 + tmp_win->title_height + tmp_win->attr.height;
}

int PlaceApplet(tmp_win, def_x, def_y, final_x, final_y)
TwmWindow *tmp_win;
int def_x, def_y;
int *final_x, *final_y;
{
    RootRegion	*rr;
    RegionEntry	*re;
    int			matched, w, h;

	for (rr = Scr->FirstAppletRegion; rr; rr = rr->next)
	{
		matched = 0;
		w = roundEntryUp (appletWidth (tmp_win), rr->stepx);
		h = roundEntryUp (appletHeight (tmp_win), rr->stepy);

		for (re = rr->entries; re; re = re->next)
		{
			if (!matched)
			{
				if (MatchName(tmp_win->full_name, re->u.name, &re->re, re->type))
					if (MatchName(tmp_win->class.res_name, re->u.name, &re->re, re->type))
						if (MatchName(tmp_win->class.res_class, re->u.name, &re->re, re->type))
							continue;

				matched = 1;
			}

			if (re->usedby) continue;
			if (re->w < appletWidth(tmp_win) || re->h < appletHeight(tmp_win))
				continue;

			splitRegionEntry (re, rr->grav1, rr->grav2, w, h);
			re->usedby = USEDBY_TWIN;
			re->u.twm_win = tmp_win;

			*final_x = re->x;
			*final_y = re->y;

			if (rr->grav2 == D_EAST)
				*final_x += re->w - appletWidth(tmp_win);
			if (rr->grav1 == D_SOUTH)
				*final_y += re->h - appletHeight(tmp_win);

			return 1;
		}
	}

	*final_x = def_x;
	*final_y = def_y;

	return 0;
}

static RegionEntry *
FindAppletEntry (tmp_win, rrp)
    TwmWindow   *tmp_win;
    RootRegion	**rrp;
{
    RootRegion	*rr;
    RegionEntry	*re;

    for (rr = Scr->FirstAppletRegion; rr; rr = rr->next) {
	for (re = rr->entries; re; re=re->next)
	    if (re->u.twm_win == tmp_win) {
		if (rrp)
		    *rrp = rr;
		return re;
	    }
    }
    return 0;
}

void
AppletDown (tmp_win)
    TwmWindow   *tmp_win;
{
    RegionEntry	*re;
    RootRegion	*rr;

    re = FindAppletEntry (tmp_win, &rr);
    if (re)
	downRegionEntry(rr, re);
}

RootRegion *
AddAppletRegion(geom, grav1, grav2, stepx, stepy)
char *geom;
int grav1, grav2, stepx, stepy;
{
    RootRegion *rr;

    rr = AddRegion(geom, grav1, grav2, stepx, stepy);

    if (Scr->LastAppletRegion)
	Scr->LastAppletRegion->next = rr;
    Scr->LastAppletRegion = rr;
    if (!Scr->FirstAppletRegion)
	Scr->FirstAppletRegion = rr;

	return rr;
}

void
AddToAppletList(list_head, name, type)
RootRegion *list_head;
char *name;
short type;
{
    RegionEntry *nptr;

    if (!list_head) return;	/* ignore empty inserts */

    nptr = (RegionEntry *)malloc(sizeof(RegionEntry));
    if (nptr == NULL)
    {
	twmrc_error_prefix();
	fprintf (stderr, "unable to allocate %lu bytes for RegionEntry\n",
		 sizeof(RegionEntry));
	Done();
    }

    nptr->next = list_head->entries;
    nptr->type = type;
    nptr->usedby = USEDBY_NAME;
    nptr->u.name = (char*)strdup(name);
    list_head->entries = nptr;
}
