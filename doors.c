/*
 * $Id: doors.c,v 3.0 90/11/20 16:13:17 dme Exp Locker: dme $
 *
 * Copyright (c) 1990 Dave Edmondson.
 * Copyright (c) 1990 Imperial College of Science, Technoology & Medicine
 * All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of Dave Edmondson or Imperial College
 * not be used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission. Dave Edmondson and
 * Imperial College make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 */

#include <stdio.h>
#include <string.h>
#include "doors.h"
#include "screen.h"
#include "desktop.h"
#include "add_window.h"

#if (defined ibm || defined ultrix)
char *strdup(s1)
char * s1;
{
	char *s2;

	s2 = malloc((unsigned) strlen(s1)+1);
	return (s2 == NULL ? NULL : strcpy(s2,s1));
}
#endif /* ibm */

extern TwmDoor *door_add_internal();

TwmDoor *door_add(name, position, destination)
char *name, *position, *destination;
{
	int px, py, pw, ph, dx, dy;

	JunkMask = XParseGeometry (position, &JunkX, &JunkY,
				   &JunkWidth, &JunkHeight);

	/* we have some checking for negative (x,y) to do 
	   sorta taken from desktop.c by DSE */
	if ((JunkMask & XNegative) == XNegative) {
		JunkX += Scr->MyDisplayWidth - JunkWidth -
			(2 * Scr->BorderWidth);
		}
	if ((JunkMask & YNegative) == YNegative) {
		JunkY += Scr->MyDisplayHeight - JunkHeight -
			(2 * Scr->BorderWidth);
		}

	if ((JunkMask & (XValue | YValue)) !=
	    (XValue | YValue)) {
		twmrc_error_prefix();
		fprintf (stderr, "bad Door position \"%s\"\n", position);
		return NULL;
	}

/*	if (JunkX <= 0 || JunkY <= 0) {  */

	if (JunkX < 0 || JunkY < 0) { /* 0,0 accepted now -- DSE */
		twmrc_error_prefix();
		fprintf (stderr, "silly Door position \"%s\"\n", position);
		return NULL;
	}

	/* they seemed ok */
	px = JunkX;
	py = JunkY;

	if (JunkMask & WidthValue)
		pw = JunkWidth;
	else
		/* means figure it out when you create the window */
		pw = -1;
	if (JunkMask & HeightValue)
		ph = JunkHeight;
	else
		ph = -1;

	JunkMask = XParseGeometry (destination, &JunkX, &JunkY,
				   &JunkWidth, &JunkHeight);
	if ((JunkMask & (XValue | YValue)) !=
	    (XValue | YValue)) {
		twmrc_error_prefix();
		fprintf (stderr, "bad Door destination \"%s\"\n", destination);
		return NULL;
	}
	if (JunkX < 0 || JunkY < 0) {
		twmrc_error_prefix();
		fprintf (stderr, "silly Door destination \"%s\"\n",
			 destination);
		return NULL;
	}
	dx = JunkX;
	dy = JunkY;

	return (door_add_internal(name, px, py, pw, ph, dx, dy));
}

TwmDoor *door_add_internal(name, px, py, pw, ph, dx, dy)
char *name;
int px, py, pw, ph, dx, dy;
{
	TwmDoor *new;

	new = (TwmDoor *)malloc(sizeof(TwmDoor));
	new->name = strdup(name);

	/* this for getting colors */
	new->class = XAllocClassHint();
	new->class->res_name = new->name;
	new->class->res_class = strdup(TWM_DOOR_CLASS);

	new->x = px;
	new->y = py;
	new->width = pw;
	new->height = ph;
	new->goto_x = dx;
	new->goto_y = dy;

	/* link into the list */
	new->prev = NULL;
	new->next = Scr->Doors;
	if (Scr->Doors)
		Scr->Doors->prev = new;
	Scr->Doors = new;

	return (new);
}

void door_open(tmp_door)
TwmDoor *tmp_door;
{
	Window w;

	/* look up colours */
	if (!GetColorFromList(Scr->DoorForegroundL,
			      tmp_door->name,
			      tmp_door->class, &tmp_door->colors.fore))
		tmp_door->colors.fore = Scr->DoorC.fore;
	if (!GetColorFromList(Scr->DoorBackgroundL,
			      tmp_door->name,
			      tmp_door->class, &tmp_door->colors.back))
		tmp_door->colors.back = Scr->DoorC.back;

	if (tmp_door->width < 0)
		tmp_door->width = XTextWidth(Scr->DoorFont.font,
					     tmp_door->name,
					     strlen(tmp_door->name))
			+ SIZE_HINDENT;
	if (tmp_door->height < 0)
		tmp_door->height = Scr->DoorFont.height + SIZE_VINDENT;

	/* create the window */
	w = XCreateSimpleWindow(dpy, Scr->Root,
				tmp_door->x, tmp_door->y,
				tmp_door->width, tmp_door->height,
				Scr->BorderWidth,
				tmp_door->colors.fore,
				tmp_door->colors.back);
	tmp_door->w = XCreateSimpleWindow(dpy, w,
					  0, 0,
					  tmp_door->width, tmp_door->height,
					  0,
					  tmp_door->colors.fore,
					  tmp_door->colors.back);

	if ((tmp_door->x < 0) || (tmp_door->y < 0)) {
		XSizeHints *hints = NULL;
		long ret;

		/* set the wmhints so that addwindow will allow
		 * the user to place the window */
		if (XGetWMNormalHints(dpy, w, hints, &ret) > 0) {
			hints->flags = hints->flags &
				(!USPosition & !PPosition);
			XSetStandardProperties(dpy, w,
					       tmp_door->class->res_name,
					       tmp_door->class->res_name,
					       None, NULL, 0, hints);
		}
	} else {
		XSetStandardProperties(dpy, w,
				       tmp_door->class->res_name,
				       tmp_door->class->res_name,
				       None, NULL, 0, NULL);
	}

	XSetClassHint(dpy, w, tmp_door->class);

	/* set the name on both */
	XStoreName(dpy, tmp_door->w, tmp_door->name);
	XStoreName(dpy, w, tmp_door->name);

	XDefineCursor( dpy, w, Scr->FrameCursor );/*RFB*/
	XDefineCursor( dpy, tmp_door->w, Scr->DoorCursor );/*RFBCURSOR*/

	/* give to twm */
	tmp_door->twin = AddWindow(w, FALSE, NULL);

	SetMapStateProp(tmp_door->twin, NormalState);

	/* interested in... */
	XSelectInput(dpy, tmp_door->w, ExposureMask |
		     ButtonPressMask | ButtonReleaseMask);

	/* store the address of the door on the window */
	XSaveContext(dpy,
		     tmp_door->w, DoorContext, (caddr_t) tmp_door);
	XSaveContext(dpy,
		     tmp_door->w,
		     TwmContext, (caddr_t) tmp_door->twin);
	XSaveContext(dpy,
		     w, DoorContext, (caddr_t) tmp_door);

	/* map it */
	XMapWindow(dpy, tmp_door->w);
	XMapWindow(dpy, w);
}

void door_open_all()
{
	TwmDoor *tmp_door;
	Window w;

	for (tmp_door = Scr->Doors; tmp_door; tmp_door = tmp_door->next)
		door_open(tmp_door);
}

/*
 * go into a door
 */
void door_enter(w, d)
Window w;
TwmDoor *d;
{
	if (!d)
		/* find the door */
		if (XFindContext(dpy, w, DoorContext, (caddr_t *)&d)
		    == XCNOENT)
			/* not a door ! */
			return;

	/* go to it */
	SetRealScreen(d->goto_x, d->goto_y);
}

/*
 * delete a door
 */
void door_delete(w, d)
Window w;
TwmDoor *d;
{	/*marcel@duteca.et.tudelft.nl*/
	if (!d)
		/* find the door */
		if (XFindContext(dpy, w, DoorContext, (caddr_t *)&d)
		    == XCNOENT)
			/* not a door ! */
			return;

	/* unlink it: */
	if (Scr->Doors == d)
		Scr->Doors = d->next;
	if (d->prev != NULL)
		d->prev->next = d->next;
	if (d->next != NULL)
		d->next->prev = d->prev;
/* Must this be done here ? Is it do by XDestroyWindow, or by
	HandleDestroyNotify() in events.c, or should it be done there...?

	XDeleteContext(dpy, d->w, DoorContext);
	XDeleteContext(dpy, d->w,  TwmContext);
	XDeleteContext(dpy, d->twin, DoorContext);
	XUnmapWindow(dpy, d->w);
	XUnmapWindow(dpy, w);
*/
	XDestroyWindow(dpy, w);
	XFree(d->class);
	free(d);
	/*
	 * Did I release all allocated memory ??? M.J.E. Mol.
	 */
}

/*
 * create a new door on the fly
 */
void door_new()
{
	TwmDoor *d;
	char name[256];
	XSizeHints *hints;
	long ret;

	sprintf(name, "+%d+%d", Scr->VirtualDesktopX, Scr->VirtualDesktopY);

	d = door_add_internal(name, -1, -1, -1, -1,
			      Scr->VirtualDesktopX, Scr->VirtualDesktopY);

	door_open(d);
}
