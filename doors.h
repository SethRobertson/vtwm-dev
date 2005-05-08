/*
 * $Id: doors.h,v 3.0 90/11/20 16:13:19 dme Exp Locker: dme $
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

#ifndef DOORS_H_INCLUDED
#define DOORS_H_INCLUDED

#include "twm.h"

/* the class of twm doors */
/* djhjr - 4/27/96
#define TWM_DOOR_CLASS "Twm Door"
*/
#define TWM_DOOR_CLASS "VTWM Door"

/*
 * the door structure
 */
typedef struct TwmDoor {
	struct TwmDoor *next; /* next in the linked list */
	struct TwmDoor *prev; /* prev in the linked list */

	char *name;           /* name of this door */
	int x, y;             /* position */
	int width, height;    /* size */
	
	int goto_x, goto_y;   /* destination */

	XClassHint *class;     /* name and class of this door */

	ColorPair colors;     /* fore and back */

	Window w;             /* the x window for this */
	TwmWindow *twin;      /* the twmwindow for this */
} TwmDoor;

extern TwmDoor *door_add();
extern void door_open();
extern void door_open_all();
extern void door_enter();
extern void door_new();

#endif /* DOORS_H_INCLUDED */
