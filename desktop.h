/*
 * $Id: desktop.h,v 3.0 90/11/20 16:13:13 dme Exp Locker: dme $
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

extern Window moving_window; /* indicates that we are doing a move in the vd display */

extern void CreateDesktopDisplay();
extern void UpdateDesktop();
extern void RemoveFromDesktop();
extern void DisplayScreenOnDesktop();
extern void StartMoveWindowOnDesktop();
extern void EndMoveWindowOnDesktop();
extern void DoMoveWindowOnDesktop();
extern void ResizeDesktopDisplay();
extern void VirtualMoveWindow();
extern void SnapRealScreen();
extern void SetRealScreen();
extern void PanRealScreen();
extern void RaiseAutoPan();

extern void desktopCreateWindow();
extern void desktopNailWindow();
extern void MapFrame();
extern void RaiseFrame();
extern void LowerFrame();

/* convert real space to virtual space */
#define R_TO_V_X(x) ((x) + Scr->VirtualDesktopX)
#define R_TO_V_Y(y) ((y) + Scr->VirtualDesktopY)

/* convert virtual space to real space */
#define V_TO_R_X(x) (-(Scr->VirtualDesktopX - (x)))
#define V_TO_R_Y(y) (-(Scr->VirtualDesktopY - (y)))

/* scale up and down from desktop display to real sizes */
/* #define SCALE_D(x) (((x)/Scr->VirtualDesktopDScale)+1) */
/* don't pass me something like `x++' - please */
#define SCALE_D(x) ((((x)/Scr->VirtualDesktopDScale) < 1) ? 1 : (x)/Scr->VirtualDesktopDScale)
#define SCALE_U(x) ((x)*Scr->VirtualDesktopDScale)

/* how wide/high the autopan windows are */
#define AP_SIZE 5
