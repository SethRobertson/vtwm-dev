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
 * $XConsortium: icons.c,v 1.22 91/07/12 09:58:38 dave Exp $
 *
 * Icon releated routines
 *
 * 10-Apr-89 Tom LaStrange        Initial Version.
 *
 **********************************************************************/

#include <stdio.h>
#include <string.h>
#include "twm.h"
#include "screen.h"
#include "regions.h"
#include "list.h"
#include "gram.h"
#include "parse.h"
#include "image_formats.h"
#include "util.h"

extern void splitRegionEntry();
extern int roundEntryUp();
extern RegionEntry *prevRegionEntry();
extern void mergeRegionEntries();
extern void downRegionEntry();
extern RootRegion *AddRegion();

#define iconWidth(w)	(Scr->IconBorderWidth * 2 + w->icon_w_width)
#define iconHeight(w)	(Scr->IconBorderWidth * 2 + w->icon_w_height)

void PlaceIcon(tmp_win, def_x, def_y, final_x, final_y)
TwmWindow *tmp_win;
int def_x, def_y;
int *final_x, *final_y;
{
    RootRegion	*rr;
    RegionEntry	*re;
    int		w, h;

    re = 0;
    for (rr = Scr->FirstIconRegion; rr; rr = rr->next) {
	w = roundEntryUp (iconWidth (tmp_win), rr->stepx);
	h = roundEntryUp (iconHeight (tmp_win), rr->stepy);
	for (re = rr->entries; re; re=re->next) {
	    if (re->usedby)
		continue;
	    if (re->w >= iconWidth(tmp_win) && re->h >= iconHeight(tmp_win))
		break;
	}
	if (re)
	    break;
    }
    if (re) {
	splitRegionEntry (re, rr->grav1, rr->grav2, w, h);
	re->usedby = USEDBY_TWIN;
	re->u.twm_win = tmp_win;

	*final_x = re->x;
	*final_y = re->y;

	/* adjust for region gravity - djhjr 4/26/99 */
	if (rr->grav2 == D_EAST)
		*final_x += re->w - iconWidth(tmp_win);
	if (rr->grav1 == D_SOUTH)
		*final_y += re->h - iconHeight(tmp_win);

    } else {
	*final_x = def_x;
	*final_y = def_y;
    }
    return;
}

static RegionEntry *
FindIconEntry (tmp_win, rrp)
    TwmWindow   *tmp_win;
    RootRegion	**rrp;
{
    RootRegion	*rr;
    RegionEntry	*re;

    for (rr = Scr->FirstIconRegion; rr; rr = rr->next) {
	for (re = rr->entries; re; re=re->next)
	    if (re->u.twm_win == tmp_win) {
		if (rrp)
		    *rrp = rr;
		return re;
	    }
    }
    return 0;
}

void IconUp (tmp_win)
    TwmWindow   *tmp_win;
{
    int		x, y;
    int		defx, defy;
    struct RootRegion *rr;

    /*
     * If the client specified a particular location, let's use it (this might
     * want to be an option at some point).  Otherwise, try to fit within the
     * icon region.
     */
    if (tmp_win->wmhints && (tmp_win->wmhints->flags & IconPositionHint))
      return;

    if (tmp_win->icon_moved) {
	if (!XGetGeometry (dpy, tmp_win->icon_w.win, &JunkRoot, &defx, &defy,
			   &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth))
	  return;

	x = defx;
	y = defy;

	for (rr = Scr->FirstIconRegion; rr; rr = rr->next) {
	    if (x >= rr->x && x < (rr->x + rr->w) &&
		y >= rr->y && y < (rr->y + rr->h))
	      break;
	}
	if (!rr) return;		/* outside icon regions, leave alone */
    }

    defx = -100;
    defy = -100;
    PlaceIcon(tmp_win, defx, defy, &x, &y);
    if (x != defx || y != defy) {
	XMoveWindow (dpy, tmp_win->icon_w.win, x, y);
	tmp_win->icon_moved = FALSE;	/* since we've restored it */
    }
}

void
IconDown (tmp_win)
    TwmWindow   *tmp_win;
{
    RegionEntry	*re;
    RootRegion	*rr;

    re = FindIconEntry (tmp_win, &rr);
    if (re)
	downRegionEntry(rr, re);
}

void
AddIconRegion(geom, grav1, grav2, stepx, stepy)
char *geom;
int grav1, grav2, stepx, stepy;
{
    RootRegion *rr;

    rr = AddRegion(geom, grav1, grav2, stepx, stepy);

    if (Scr->LastIconRegion)
	Scr->LastIconRegion->next = rr;
    Scr->LastIconRegion = rr;
    if (!Scr->FirstIconRegion)
	Scr->FirstIconRegion = rr;
}

Image *
GetIconImage(name, background, numcolors)
char *name;
Pixel background;
unsigned int *numcolors;
{
    Image *iconimage;
	GC gc;
    Pixmap bm;
    int bitmap_height, bitmap_width;

	iconimage = (Image *)LookInNameList(Scr->Icons, name);
	if (iconimage == NULL)
	{
		bm = FindBitmap(name, &bitmap_width, &bitmap_height);
		if (bm != None)
		{
			iconimage = (Image *)malloc(sizeof(Image));
			iconimage->mask = None;
			iconimage->height = bitmap_height;
			iconimage->width = bitmap_width;
			iconimage->pixmap = XCreatePixmap(dpy, Scr->Root, bitmap_width,
					bitmap_height, Scr->d_depth);

			XGetGeometry(dpy, bm,
					&JunkRoot, &JunkX, &JunkY,
					&JunkWidth, &JunkHeight, &JunkBW, &JunkDepth);

			/*
			 * XCopyArea() seems to be necessary for some apps that change
			 * their icons - djhjr - rem'd 8/23/98, re-instated 11/15/98
			 */
			if (JunkDepth == Scr->d_depth)
				XCopyArea(dpy, bm, iconimage->pixmap,
						Scr->NormalGC, 0, 0, iconimage->width, iconimage->height,
						0, 0);
			else
				XCopyPlane(dpy, bm, iconimage->pixmap,
						Scr->NormalGC, 0, 0, iconimage->width, iconimage->height,
						0, 0, 1);

			iconimage->mask = XCreatePixmap(dpy, Scr->Root,
					iconimage->width, iconimage->height, 1);
			if (iconimage->mask)
			{
				gc = XCreateGC(dpy, iconimage->mask, 0, NULL);
				if (gc)
				{
					XCopyArea(dpy, bm, iconimage->mask,
							gc, 0, 0, iconimage->width, iconimage->height, 0, 0);
					XFreeGC (dpy, gc);
				}
			}

			XFreePixmap(dpy, bm);
		}
		else
		{
			iconimage = FindImage(name, background);
		}

		if (iconimage != NULL)
			AddToList(&Scr->Icons, name, LTYPE_EXACT_NAME,
					(char *)iconimage);
	}

	*numcolors = 0;
#ifndef NO_XPM_SUPPORT
	if (iconimage != NULL)
		*numcolors = SetPixmapsBackground(iconimage, Scr->Root, background);
#endif

	return (iconimage);
}

void
CreateIconWindow(tmp_win, def_x, def_y)
TwmWindow *tmp_win;
int def_x, def_y;
{
    unsigned long event_mask;
    unsigned long valuemask;		/* mask for create windows */
    XSetWindowAttributes attributes;	/* attributes for create windows */
    Pixmap pm;				/* tmp pixmap variable */
    Image *iconimage;
    char *icon_name;
    int x, final_x, final_y;
	unsigned int pm_numcolors = 0;

    GetColorFromList(Scr->IconBorderColorL, tmp_win->full_name, &tmp_win->class,
	&tmp_win->icon_border);
    GetColorFromList(Scr->IconForegroundL, tmp_win->full_name, &tmp_win->class,
	&tmp_win->iconc.fore);
    GetColorFromList(Scr->IconBackgroundL, tmp_win->full_name, &tmp_win->class,
	&tmp_win->iconc.back);

	if (Scr->IconBevelWidth > 0 && !Scr->BeNiceToColormap) GetShadeColors(&tmp_win->iconc);

    FB(Scr, tmp_win->iconc.fore, tmp_win->iconc.back);

    tmp_win->forced = FALSE;
    tmp_win->icon_not_ours = FALSE;
    iconimage = NULL;
    pm = None;

    /*
     * now go through the steps to get an icon window,  if ForceIcon is
     * set, then no matter what else is defined, the bitmap from the
     * .vtwmrc file is used
     */
	if (Scr->ForceIcon)
	{
		icon_name = LookInNameList(Scr->IconNames, tmp_win->full_name);
		if (icon_name == NULL)
			icon_name = LookInList(Scr->IconNames, tmp_win->full_name,
					&tmp_win->class);

		if (icon_name != NULL)
			iconimage = GetIconImage(icon_name, tmp_win->iconc.back,
					&pm_numcolors);

		if (iconimage != NULL)
		{
			tmp_win->icon_width = iconimage->width;
			tmp_win->icon_height = iconimage->height;

			pm = iconimage->pixmap;
			tmp_win->forced = TRUE;
		}
	}

	/*
	 * if the pixmap is still NULL, we didn't get one from the above code,
	 * that could mean that ForceIcon was not set, or that the window
	 * was not in the Icons list, now check the WM hints for an icon
	 */

	if (pm == None && tmp_win->wmhints &&
			tmp_win->wmhints->flags & IconPixmapHint)
	{
		XGetGeometry(dpy, tmp_win->wmhints->icon_pixmap,
				&JunkRoot, &JunkX, &JunkY,
				&JunkWidth, &JunkHeight, &JunkBW, &JunkDepth);

		pm_numcolors = 3;

		iconimage = (Image*)malloc(sizeof(Image));
		iconimage->mask = None;
		iconimage->width = JunkWidth;
		iconimage->height = JunkHeight;
		iconimage->pixmap = XCreatePixmap(dpy, Scr->Root, iconimage->width,
				iconimage->height, Scr->d_depth);

		if (JunkDepth == Scr->d_depth)
			XCopyArea(dpy, tmp_win->wmhints->icon_pixmap, iconimage->pixmap,
					Scr->NormalGC, 0, 0, iconimage->width, iconimage->height,
					0, 0);
		else
			XCopyPlane(dpy, tmp_win->wmhints->icon_pixmap, iconimage->pixmap,
					Scr->NormalGC, 0, 0, iconimage->width, iconimage->height,
					0, 0, 1);

		if ((tmp_win->wmhints->flags & IconMaskHint) &&
				XGetGeometry(dpy, tmp_win->wmhints->icon_mask,
						&JunkRoot, &JunkX, &JunkY, &JunkWidth, &JunkHeight,
						&JunkBW, &JunkDepth) &&
				JunkDepth == 1)
		{
			GC gc;

			iconimage->mask = XCreatePixmap(dpy, Scr->Root,
					JunkWidth, JunkHeight, 1);
			if (iconimage->mask)
			{
				gc = XCreateGC(dpy, iconimage->mask, 0, NULL);
				if (gc)
				{
					XCopyArea(dpy, tmp_win->wmhints->icon_mask, iconimage->mask,
							gc, 0, 0, JunkWidth, JunkHeight, 0, 0);
					XFreeGC (dpy, gc);
				}
			}
		}

		if (iconimage != NULL)
		{
			tmp_win->icon_width = iconimage->width;
			tmp_win->icon_height = iconimage->height;

			pm = iconimage->pixmap;
		}
	}

	/*
	 * if we still haven't got an icon, let's look in the Icon list
	 * if ForceIcon is not set
	 */
	if ((pm == None) && (!Scr->ForceIcon))
	{
		icon_name = LookInNameList(Scr->IconNames, tmp_win->full_name);
		if (icon_name == NULL)
			icon_name = LookInList(Scr->IconNames, tmp_win->full_name,
					&tmp_win->class);

		if (icon_name != NULL)
			iconimage = GetIconImage(icon_name, tmp_win->iconc.back,
					&pm_numcolors);

		if (iconimage != NULL)
		{
			tmp_win->icon_width = iconimage->width;
			tmp_win->icon_height = iconimage->height;

			pm = iconimage->pixmap;
		}
	}

	if (pm == None && Scr->unknownName != NULL)
	{
		iconimage = GetIconImage(Scr->unknownName, tmp_win->iconc.back,
				&pm_numcolors);




		if (iconimage != NULL)
		{
			tmp_win->icon_width = iconimage->width;
			tmp_win->icon_height = iconimage->height;

			pm = iconimage->pixmap;
		}
	}

    if (pm == None)
    {
	tmp_win->icon_height = 0;
	tmp_win->icon_width = 0;
	valuemask = 0;
    }
    else
    {
	valuemask = CWBackPixmap;
	attributes.background_pixmap = pm;

		if ((iconimage->type==IMAGE_TYPE_XPM) && (pm_numcolors <= 2)) /* not a pixmap */
		{
			valuemask |= CWBackPixel;
			attributes.background_pixel = tmp_win->iconc.fore;
		}
    }

    tmp_win->icon_w_width = MyFont_TextWidth(&Scr->IconFont,
				       tmp_win->icon_name, strlen(tmp_win->icon_name));

#ifdef TWM_USE_SPACING
    tmp_win->icon_w_width += Scr->IconFont.height; /*approx. '1ex' on both sides*/
#else
    tmp_win->icon_w_width += 8;
#endif
    if (tmp_win->icon_w_width < tmp_win->icon_width + 8)
    {
		tmp_win->icon_x = (((tmp_win->icon_width + 8) - tmp_win->icon_w_width)/2) + 4;
		tmp_win->icon_w_width = tmp_win->icon_width + 8;
    }
    else
#ifdef TWM_USE_SPACING
		tmp_win->icon_x = Scr->IconFont.height/2;
#else
		tmp_win->icon_x = 4;
#endif

#ifdef TWM_USE_SPACING
    /* icon label height := 1.44 times font height: */
    tmp_win->icon_w_height = tmp_win->icon_height + 144*Scr->IconFont.height/100;
    tmp_win->icon_y = tmp_win->icon_height + Scr->IconFont.y + 44*Scr->IconFont.height/200;
#else
    tmp_win->icon_w_height = tmp_win->icon_height + Scr->IconFont.height + 8;
    tmp_win->icon_y = tmp_win->icon_height + Scr->IconFont.height + 2;
#endif

    event_mask = 0;
    if (tmp_win->wmhints && tmp_win->wmhints->flags & IconWindowHint)
    {
	tmp_win->icon_w.win = tmp_win->wmhints->icon_window;
	if (tmp_win->forced ||
	    XGetGeometry(dpy, tmp_win->icon_w.win, &JunkRoot, &JunkX, &JunkY,
		     (unsigned int *)&tmp_win->icon_w_width, (unsigned int *)&tmp_win->icon_w_height,
		     &JunkBW, &JunkDepth) == 0)
	{
	    tmp_win->icon_w.win = None;
	    tmp_win->wmhints->flags &= ~IconWindowHint;
	}
	else
	{
	    tmp_win->icon_not_ours = TRUE;
	    event_mask = EnterWindowMask | LeaveWindowMask;
	}
    }
    else
    {
	tmp_win->icon_w.win = None;
    }

	if (Scr->IconBevelWidth > 0)
	{
		tmp_win->icon_w_width += 2 * Scr->IconBevelWidth;
		tmp_win->icon_w_height += 2 * Scr->IconBevelWidth;

		tmp_win->icon_x += Scr->IconBevelWidth;
		tmp_win->icon_y += Scr->IconBevelWidth;
	}

    if (tmp_win->icon_w.win == None)
    {
	tmp_win->icon_w.win = XCreateSimpleWindow(dpy, Scr->Root,
	    0,0,
	    tmp_win->icon_w_width, tmp_win->icon_w_height,
	    Scr->IconBorderWidth, tmp_win->icon_border, tmp_win->iconc.back);
	event_mask = ExposureMask;
#ifdef TWM_USE_XFT
	if (Scr->use_xft > 0)
	    tmp_win->icon_w.xft = MyXftDrawCreate (tmp_win->icon_w.win);
#endif
#ifdef TWM_USE_OPACITY
	SetWindowOpacity (tmp_win->icon_w.win, Scr->IconOpacity);
#endif
    }

    XSelectInput (dpy, tmp_win->icon_w.win,
		  KeyPressMask | ButtonPressMask | ButtonReleaseMask |
		  event_mask);

    tmp_win->icon_bm_w = None;
    if (pm != None &&
	(! (tmp_win->wmhints && tmp_win->wmhints->flags & IconWindowHint)))
    {
	int y;

	y = 4;

#ifdef TWM_USE_SPACING
	tmp_win->icon_w_height += y;
	tmp_win->icon_y += y;
	XResizeWindow (dpy, tmp_win->icon_w.win, tmp_win->icon_w_width, tmp_win->icon_w_height);
#endif

	if (tmp_win->icon_w_width == tmp_win->icon_width)
	    x = 0;
	else
	    x = (tmp_win->icon_w_width - tmp_win->icon_width)/2;

	if (Scr->IconBevelWidth > 0)
		y += Scr->IconBevelWidth;

	tmp_win->icon_bm_w = XCreateWindow (dpy, tmp_win->icon_w.win, x, y,
					    (unsigned int)tmp_win->icon_width,
					    (unsigned int)tmp_win->icon_height,
					    (unsigned int) 0, Scr->d_depth,
					    (unsigned int) CopyFromParent,
					    Scr->d_visual, valuemask,
					    &attributes);


    if (HasShape)
		if (iconimage != NULL && iconimage->mask != None)
			XShapeCombineMask(dpy,tmp_win->icon_bm_w , ShapeBounding, 0, 0,
					iconimage->mask, ShapeSet);
    }

    /* I need to figure out where to put the icon window now, because
     * getting here means that I am going to make the icon visible
     */
    if (tmp_win->wmhints &&
	tmp_win->wmhints->flags & IconPositionHint)
    {
	final_x = tmp_win->wmhints->icon_x;
	final_y = tmp_win->wmhints->icon_y;
    }
    else
    {
	PlaceIcon(tmp_win, def_x, def_y, &final_x, &final_y);
    }

    if (final_x > Scr->MyDisplayWidth)
	final_x = Scr->MyDisplayWidth - tmp_win->icon_w_width -
	    (2 * Scr->IconBorderWidth);

    if (final_y > Scr->MyDisplayHeight)
	final_y = Scr->MyDisplayHeight - tmp_win->icon_height -
	    Scr->IconFont.height - 4 - (2 * Scr->IconBorderWidth);

    XMoveWindow(dpy, tmp_win->icon_w.win, final_x, final_y);
    tmp_win->iconified = TRUE;

    XMapSubwindows(dpy, tmp_win->icon_w.win);
    XSaveContext(dpy, tmp_win->icon_w.win, TwmContext, (caddr_t)tmp_win);
    XSaveContext(dpy, tmp_win->icon_w.win, ScreenContext, (caddr_t)Scr);
    XDefineCursor(dpy, tmp_win->icon_w.win, Scr->IconCursor);

    return;
}
