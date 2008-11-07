
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
#include "prototypes.h"

extern void SetMapStateProp(TwmWindow * tmp_win, int state);
extern TwmDoor *door_add_internal(char *name, int px, int py, int pw, int ph, int dx, int dy);
extern void HandleExpose(void);
extern void SetupWindow(TwmWindow * tmp_win, int x, int y, int w, int h, int bw);

TwmDoor *
door_add(char *name, char *position, char *destination)
{
  int px, py, pw, ph, dx, dy, bw;

  /*
   * panel name is encoded into geometry string as "1200x20+10-10@1"
   * where "@1" is panel "1"
   */
  int tile;
  char *tile_name = strchr (position, '@');
  if (tile_name != NULL)
    *tile_name++ = '\0';
  tile = ParsePanelIndex (tile_name);

  bw = Scr->BorderWidth;

  JunkMask = XParseGeometry(position, &JunkX, &JunkY, &JunkWidth, &JunkHeight);

#ifdef TILED_SCREEN
  if ((Scr->use_tiles == TRUE) && (JunkMask & XValue) && (JunkMask & YValue))
    EnsureGeometryVisibility(tile, JunkMask, &JunkX, &JunkY, JunkWidth + 2 * bw, JunkHeight + 2 * bw);
  else
#endif
  {
    /* we have some checking for negative (x,y) to do
     * sorta taken from desktop.c by DSE */
    if ((JunkMask & XNegative) == XNegative)
    {
      JunkX += Scr->MyDisplayWidth - JunkWidth - (2 * bw);
    }
    if ((JunkMask & YNegative) == YNegative)
    {
      JunkY += Scr->MyDisplayHeight - JunkHeight - (2 * bw);
    }
  }

  if ((JunkMask & (XValue | YValue)) != (XValue | YValue))
  {
    /* allow AddWindow() to position it - djhjr - 5/10/99 */
    JunkX = JunkY = -1;
  }
  else
  {
    if (JunkX < 0 || JunkY < 0)
    {
      twmrc_error_prefix();
      fprintf(stderr, "silly Door position \"%s\"\n", position);
      return NULL;
    }
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

  JunkMask = XParseGeometry(destination, &JunkX, &JunkY, &JunkWidth, &JunkHeight);
  if ((JunkMask & (XValue | YValue)) != (XValue | YValue))
  {
    twmrc_error_prefix();
    fprintf(stderr, "bad Door destination \"%s\"\n", destination);
    return NULL;
  }
  if (JunkX < 0 || JunkY < 0)
  {
    twmrc_error_prefix();
    fprintf(stderr, "silly Door destination \"%s\"\n", destination);
    return NULL;
  }
  dx = JunkX;
  dy = JunkY;

  return (door_add_internal(name, px, py, pw, ph, dx, dy));
}

TwmDoor *
door_add_internal(char *name, int px, int py, int pw, int ph, int dx, int dy)
{
  TwmDoor *new;

  new = (TwmDoor *) malloc(sizeof(TwmDoor));
  new->name = strdup(name);

  /* this for getting colors */
  new->class = XAllocClassHint();
  new->class->res_name = new->name;
  new->class->res_class = strdup(VTWM_DOOR_CLASS);

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

void
door_open(TwmDoor * tmp_door)
{
  Window w;

  int bw = Scr->BorderWidth;

  /* look up colours */
  if (!GetColorFromList(Scr->DoorForegroundL, tmp_door->name, tmp_door->class, &tmp_door->colors.fore))
    tmp_door->colors.fore = Scr->DoorC.fore;
  if (!GetColorFromList(Scr->DoorBackgroundL, tmp_door->name, tmp_door->class, &tmp_door->colors.back))
    tmp_door->colors.back = Scr->DoorC.back;

  if (tmp_door->width < 0)
    tmp_door->width = MyFont_TextWidth(&Scr->DoorFont,
				       tmp_door->name, strlen(tmp_door->name)) + (Scr->DoorBevelWidth * 2) + SIZE_HINDENT;

  if (tmp_door->height < 0)
    tmp_door->height = Scr->DoorFont.height + (Scr->DoorBevelWidth * 2) + SIZE_VINDENT;

  /* create the window */
  w = XCreateSimpleWindow(dpy, Scr->Root,
			  tmp_door->x, tmp_door->y,
			  tmp_door->width, tmp_door->height, bw, tmp_door->colors.fore, tmp_door->colors.back);
  tmp_door->w.win = XCreateSimpleWindow(dpy, w,
					0, 0, tmp_door->width, tmp_door->height, 0, tmp_door->colors.fore, tmp_door->colors.back);

#ifdef TWM_USE_XFT
  if (Scr->use_xft > 0)
  {
    tmp_door->w.xft = MyXftDrawCreate(tmp_door->w.win);
    CopyPixelToXftColor(tmp_door->colors.fore, &tmp_door->colors.xft);
  }
#endif
#if defined TWM_USE_OPACITY  &&  1	/* "door" windows get MenuOpacity */
  SetWindowOpacity(w, Scr->MenuOpacity);
#endif

  {
    XSizeHints *hints = NULL;

    if (tmp_door->x < 0 || tmp_door->y < 0)
    {
      long ret;

      /*
       * set the wmhints so that add_window() will allow
       * the user to place the window
       */
      if (XGetWMNormalHints(dpy, w, hints, &ret) != 0)
	hints->flags &= (!USPosition & !PPosition);
    }

    if (!hints)
      hints = XAllocSizeHints();

    hints->flags |= PMinSize;
    hints->min_width = tmp_door->width;
    hints->min_height = tmp_door->height;

    XSetStandardProperties(dpy, w, tmp_door->class->res_name, tmp_door->class->res_name, None, NULL, 0, hints);
  }

  XSetClassHint(dpy, w, tmp_door->class);

  /* set the name on both */
  XStoreName(dpy, tmp_door->w.win, tmp_door->name);
  XStoreName(dpy, w, tmp_door->name);

  XDefineCursor(dpy, w, Scr->FrameCursor);
  XDefineCursor(dpy, tmp_door->w.win, Scr->DoorCursor);

  /* store the address of the door on the window */
  XSaveContext(dpy, tmp_door->w.win, DoorContext, (caddr_t) tmp_door);
  XSaveContext(dpy, w, DoorContext, (caddr_t) tmp_door);

  /* give to twm */
  tmp_door->twin = AddWindow(w, FALSE, NULL);

  SetMapStateProp(tmp_door->twin, NormalState);

  /* interested in... */
  XSelectInput(dpy, tmp_door->w.win, ExposureMask | ButtonPressMask | ButtonReleaseMask);

  /* store the address of the door on the window */
  XSaveContext(dpy, tmp_door->w.win, TwmContext, (caddr_t) tmp_door->twin);

  /* map it */
  XMapWindow(dpy, tmp_door->w.win);
  XMapWindow(dpy, w);
}

void
door_open_all(void)
{
  TwmDoor *tmp_door;

  for (tmp_door = Scr->Doors; tmp_door; tmp_door = tmp_door->next)
    door_open(tmp_door);
}

/*
 * go into a door
 */
void
door_enter(Window w, TwmDoor * d)
{
  int snapon;			/* doors override real screen snapping - djhjr - 2/5/99 */

  if (!d)
    /* find the door */
    if (XFindContext(dpy, w, DoorContext, (caddr_t *) & d) == XCNOENT)
      /* not a door ! */
      return;

  /* go to it */
  snapon = (int)Scr->snapRealScreen;
  Scr->snapRealScreen = FALSE;
  SetRealScreen(d->goto_x, d->goto_y);
  Scr->snapRealScreen = (snapon) ? TRUE : FALSE;
}

/*
 * delete a door
 */
void
door_delete(Window w, TwmDoor * d)
{
  if (!d)
    /* find the door */
    if (XFindContext(dpy, w, DoorContext, (caddr_t *) & d) == XCNOENT)
      /* not a door ! */
      return;

  /* unlink it: */
  if (Scr->Doors == d)
    Scr->Doors = d->next;
  if (d->prev != NULL)
    d->prev->next = d->next;
  if (d->next != NULL)
    d->next->prev = d->prev;

  AppletDown(d->twin);

/*
 * Must this be done here ? Is it do by XDestroyWindow(),
 * or by HandleDestroyNotify() in events.c, or should it
 * it be done there ? M.J.E. Mol.
 *
 * It looks as though the contexts, at least, should be
 * deleted here, maybe more, I dunno. - djhjr 2/25/99
 */
  XDeleteContext(dpy, d->w.win, DoorContext);
  XDeleteContext(dpy, d->w.win, TwmContext);
  XDeleteContext(dpy, d->twin->w, DoorContext);	/* ??? */
  XUnmapWindow(dpy, d->w.win);
  XUnmapWindow(dpy, w);
#ifdef TWM_USE_XFT
  if (Scr->use_xft > 0)
    MyXftDrawDestroy(d->w.xft);
#endif
  XDestroyWindow(dpy, w);
  free(d->class->res_class);
  XFree(d->class);
  free(d->name);
  free(d);
}

/*
 * create a new door on the fly
 */
void
door_new(void)
{
  TwmDoor *d;
  char name[256];

  sprintf(name, "+%d+%d", Scr->VirtualDesktopX, Scr->VirtualDesktopY);

  d = door_add_internal(name, -1, -1, -1, -1, Scr->VirtualDesktopX, Scr->VirtualDesktopY);

  door_open(d);
}

/*
 * rename a door from cut buffer 0
 *
 * adapted from VTWM-5.2b - djhjr - 4/20/98
 */
void
door_paste_name(Window w, TwmDoor * d)
{
  int width, height, count;
  char *ptr;
  int bw = 0;

  if (Scr->BorderBevelWidth)
    bw = Scr->BorderWidth;

  if (!d)
    if (XFindContext(dpy, w, DoorContext, (caddr_t *) & d) == XCNOENT)
      return;

  if (!(ptr = XFetchBytes(dpy, &count)) || count == 0)
    return;
  if (count > 128)
    count = 128;

  if (d->name)
    d->name = realloc(d->name, count + 1);
  else
    d->name = malloc(count + 1);

  sprintf(d->name, "%*s", count, ptr);
  XFree(ptr);

  XClearWindow(dpy, d->w.win);

  width = MyFont_TextWidth(&Scr->DoorFont, d->name, count) + SIZE_HINDENT + (Scr->DoorBevelWidth * 2);
  height = Scr->DoorFont.height + SIZE_VINDENT + (Scr->DoorBevelWidth * 2);

  /* limit the size of a door - djhjr - 3/1/99 */
  d->twin->hints.flags |= PMinSize;
  d->twin->hints.min_width = width;
  d->twin->hints.min_height = height;

  SetupWindow(d->twin, d->twin->frame_x, d->twin->frame_y, width + 2 * bw, height + d->twin->title_height + 2 * bw, -1);

  HandleExpose();
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
