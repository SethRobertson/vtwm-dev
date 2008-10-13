#ifndef _PROTOTYPES_H
#define _PROTOTYPES_H

#ifndef NO_REGEX_SUPPORT
#include <sys/types.h>
#include <regex.h>
#endif
#include "regions.h"

extern void Zoom(Window wf, IconMgr *ipf, Window wt, IconMgr *ipt);
extern void MoveOutline(Window root, int x, int y, int width, int height, int bw, int th);
extern void GetUnknownIcon(char *name);
extern char *ExpandFilename(char *name);
extern void GetColor(int kind, Pixel *what, char *name);
extern Cursor NoCursor(void);
extern Image *GetImage(char *name, int w, int h, int pad, ColorPair cp);
extern void Draw3DBorder (Drawable w, int x, int y, int width, int height, int bw, ColorPair cp, int state, int fill, int forcebw);
extern void GetShadeColors(ColorPair *cp);
extern void PaintBorders(TwmWindow *tmp_win, int focus);
extern void PaintIcon(TwmWindow *tmp_win);
extern void PaintTitle(TwmWindow *tmp_win);
extern void PaintTitleButton(TwmWindow *tmp_win, TBWindow *tbw, int onoroff);
extern void InsertRGBColormap(Atom a, XStandardColormap *maps, int nmaps, int replace);
extern void RemoveRGBColormap(Atom a);
extern void SetFocus(TwmWindow *tmp_win, Time time);
extern void LocateStandardColormaps(void);
extern void GetFont (MyFont *font);
extern int  MyFont_TextWidth (MyFont *font, char *string, int len);
extern void MyFont_DrawImageString (Display *dpy, MyWindow *win, MyFont *font, ColorPair *col, int x, int y, char * string, int len);
extern void MyFont_DrawString (Display *dpy, MyWindow *win, MyFont *font, ColorPair *col, int x, int y, char * string, int len);
extern Status I18N_FetchName (Display *dpy, Window w, char **winname);
extern Status I18N_GetIconName (Display *dpy, Window w, char **iconname);
extern void PaintTitleHighlight(TwmWindow *tmp_win, int onoroff);
extern int ComputeHighlightWindowWidth(TwmWindow *tmp_win);
extern Image *SetPixmapsPixmap(char *filename);

#ifndef NO_XPM_SUPPORT
extern int SetPixmapsBackground(Image *image, Drawable drawable, Pixel color);
#endif

#ifdef TWM_USE_XFT
extern XftDraw * MyXftDrawCreate (Window win);
extern void MyXftDrawDestroy (XftDraw *draw);
extern void CopyPixelToXftColor (unsigned long pixel, XftColor *col);
#endif
#ifdef TWM_USE_OPACITY	 /*opacity: 0 = transparent ... 255 = opaque*/
extern void SetWindowOpacity (Window win, unsigned int opacity);
extern void PropagateWindowOpacity (TwmWindow *tmp);
#endif

extern void Reborder(Time time);
extern void ComputeCommonTitleOffsets(void);
extern void ComputeWindowTitleOffsets(TwmWindow *tmp_win, int width, int squeeze);
extern void ComputeTitleLocation(TwmWindow *tmp);
extern void NewFontCursor(Cursor *cp, char *str);
extern void RestoreWithdrawnLocation(TwmWindow *tmp);
extern void CreateFonts(void);
extern void Draw3DBorder(Drawable w, int x, int y, int width, int height, int bw, ColorPair cp, int state, int fill, int forcebw);
extern int GetWMState(Window w, int *statep, Window *iwp);
extern ScreenInfo * FindPointerScreenInfo (void);
extern ScreenInfo * FindDrawableScreenInfo (Drawable d);
extern ScreenInfo * FindWindowScreenInfo (XWindowAttributes *attr);
extern ScreenInfo * FindScreenInfo (Window w);
extern int  FindNearestTileToArea (int x0y0x1y1[4]);
extern int  FindNearestTileToPoint (int x, int y);
extern int  FindNearestTileToClient (TwmWindow *tmp);
extern int  FindNearestTileToMouse (void);
extern void EnsureRectangleOnTile (int tile, int *x0, int *y0, int w, int h);
extern void EnsureGeometryVisibility (int mask, int *x0, int *y0, int w, int h);
extern void TilesFullZoom (int x0y0x1y1[4]);
extern void StartResize(XEvent *evp, TwmWindow *tmp_win, int fromtitlebar, int context);
extern void AddStartResize(TwmWindow *tmp_win, int x, int y, int w, int h);
extern void DoResize(int x_root, int y_root, TwmWindow *tmp_win);
extern void DisplaySize(TwmWindow *tmp_win, int width, int height);
extern void EndResize(void);
extern void AddEndResize(TwmWindow *tmp_win);
extern void SetupWindow(TwmWindow *tmp_win, int x, int y, int w, int h, int bw);
extern void SetupFrame(TwmWindow *tmp_win, int x, int y, int w, int h, int bw, int sendEvent);
extern void SetFrameShape(TwmWindow *tmp);
extern void ConstrainSize(TwmWindow *tmp_win, int *widthp, int *heightp);
extern void MenuStartResize(TwmWindow *tmp_win, int x, int y, int w, int h, int context);
extern void MenuEndResize(TwmWindow *tmp_win, int context);
extern void PaintBorderAndTitlebar(TwmWindow *tmp_win);
extern void fullzoom(TwmWindow *tmp_win, int flag);
extern int ParseTwmrc(char *filename, char *display_name, int m4_preprocess, char *m4_option);
extern int ParseStringList(char **sl);
extern int (*twmInputFunc)(void);
extern void twmUnput(int c);
extern void TwmOutput(int c);
extern int yyparse(void);
extern int yywrap(void);
extern int InitTitlebarButtons(void);
extern void InitMenus(void);
extern MenuRoot *NewMenuRoot(char *name);
extern void SetMenuIconPixmap(char *filename);
extern MenuItem *AddToMenu(MenuRoot *menu, char *item, char *action, MenuRoot *sub, int func, char *fore, char *back);
extern int PopUpMenu(MenuRoot *menu, int x, int y, int center);
extern MenuRoot *FindMenuRoot(char *name);
extern int AddFuncKey(char *name, int cont, int mods, int func, char *win_name, char *action);
extern int ExecuteFunction(int func, char *action, Window w, TwmWindow *tmp_win, XEvent *eventp, int context, int pulldown);
extern int DeferExecution(int context, int func, Cursor cursor);
extern void Execute(ScreenInfo *scr, char *s);
extern void FocusOnRoot(void);
extern void FocusOnClient(TwmWindow *tmp_win);
extern void SetBorder(TwmWindow *tmp, int onoroff);
extern void ReGrab(void);
extern void WarpToWindow(TwmWindow *t);
extern void PaintEntry(MenuRoot *mr, MenuItem *mi, int exposure);
extern void DeIconify(TwmWindow *tmp_win);
extern void SetMapStateProp(TwmWindow *tmp_win, int state);
extern void Iconify(TwmWindow *tmp_win, int def_x, int def_y);
extern void PopDownMenu(void);
extern void UpdateMenu(void);
extern void SendTakeFocusMessage(TwmWindow *tmp, Time timestamp);
extern void PaintEntry(MenuRoot *mr, MenuItem *mi, int exposure);
extern void SetBorder(TwmWindow *tmp, int onoroff);
extern int CreateTitleButton(char *name, int func, char *action, MenuRoot *menuroot, int rightside, int append);
extern void MakeMenus(void);
extern void PaintMenu(MenuRoot *mr, XEvent *e);
extern void DisplayPosition(int x, int y);
extern void WarpWindowOrScreen(TwmWindow *t);
extern void WarpInIconMgr(WList *w, TwmWindow *t);
extern void AddWindowToRing(TwmWindow *tmp_win);
extern void RemoveWindowFromRing(TwmWindow *tmp_win);
extern void DoAudible(void);
extern void AddToList(name_list **list_head, char *name, int type, char *ptr);
extern char *LookInList(name_list *list_head, char *name, XClassHint *class);
extern char *LookInNameList(name_list *list_head, char *name);
extern int GetColorFromList(name_list *list_head, char *name, XClassHint *class, Pixel *ptr);
extern void FreeList(name_list **list);
extern name_list *next_entry(name_list *list);
extern char *contents_of_entry(name_list *list);
extern Pixmap GetBitmap(char *name);
extern Pixmap FindBitmap(char *name, unsigned int *widthp, unsigned int *heightp);
#ifndef NO_XPM_SUPPORT
extern Image *FindImage(char *name, Pixel color);
#endif
extern int iconmgr_textx;
extern WList *DownIconManager;
extern void SetIconMgrPixmap(char *filename);
extern void CreateIconManagers(void);
extern IconMgr *AllocateIconManager(char *name, char *icon_name, char *geom, int columns);
extern void MoveIconManager(int dir);
extern void JumpIconManager(register int dir);
extern WList *AddIconManager(TwmWindow *tmp_win);
extern void InsertInIconManager(IconMgr *ip, WList *tmp, TwmWindow *tmp_win);
extern void RemoveFromIconManager(IconMgr *ip, WList *tmp);
extern void RemoveIconManager(TwmWindow *tmp_win);
extern void ActiveIconManager(WList *active);
extern void NotActiveIconManager(WList *active);
extern void DrawIconManagerBorder(WList *tmp, int fill);
extern void SortIconManager(IconMgr *ip);
extern void PackIconManager(IconMgr *ip);
extern void CreateGCs(void);
extern void InitEvents(void);
extern int StashEventTime(register XEvent *ev);
extern Time lastTimestamp;
extern void SimulateMapRequest(Window w);
extern void AutoRaiseWindow(TwmWindow *tmp);
extern int DispatchEvent(void);
extern void HandleEvents(void);
extern void HandleExpose(void);
extern void HandleDestroyNotify(void);
extern void HandleMapRequest(void);
extern void HandleMapNotify(void);
extern void HandleUnmapNotify(void);
extern void HandleMotionNotify(void);
extern void HandleButtonRelease(void);
extern void HandleButtonPress(void);
extern void HandleEnterNotify(void);
extern void HandleLeaveNotify(void);
extern void HandleConfigureRequest(void);
extern void HandleClientMessage(void);
extern void HandlePropertyNotify(void);
extern void HandleKeyPress(void);
extern void HandleColormapNotify(void);
extern void HandleVisibilityNotify(void);
extern void HandleUnknown(void);
extern void SendConfigureNotify(TwmWindow *tmp_win, int x, int y);
extern void InstallRootColormap(void);
extern int Transient(Window w, Window *propw);
extern void UninstallRootColormap(void);
extern void InstallWindowColormaps(int type, TwmWindow *tmp);
extern void RedoDoorName(TwmWindow *twin, TwmDoor *door);
extern void RedoListWindow(TwmWindow *twin);
extern TwmDoor *door_add(char *name, char *position, char *destination);
extern void door_open(TwmDoor *tmp_door);
extern void door_open_all(void);
extern void door_enter(Window w, TwmDoor *d);
extern void door_new(void);
extern void door_delete(Window w, TwmDoor *d);
extern void door_paste_name(Window w, TwmDoor *d);
extern void CreateDesktopDisplay(void);
extern void UpdateDesktop(TwmWindow *tmp_win);
extern void MoveResizeDesktop(TwmWindow *tmp_win, int noraise);
extern void NailDesktop(TwmWindow *tmp_win);
extern void DisplayScreenOnDesktop(void);
extern void StartMoveWindowInDesktop(XMotionEvent ev);
extern void EndMoveWindowOnDesktop(void);
extern void DoMoveWindowOnDesktop(int x, int y);
extern void ResizeDesktopDisplay(int w, int h);
extern void VirtualMoveWindow(TwmWindow *t, int x, int y);
extern void SnapRealScreen(void);
extern void SetRealScreen(int x, int y);
extern void PanRealScreen(int xoff, int yoff, int *dx, int *dy);
extern void RaiseStickyAbove(void);
extern void LowerSticky(void);
extern void RaiseAutoPan(void);
extern TwmWindow *AddWindow(Window w, int iconm, IconMgr *iconp);
extern int MappedNotOverride(Window w);
extern void GrabButtons(TwmWindow *tmp_win);
extern void GrabKeys(TwmWindow *tmp_win);
extern void GrabModKeys(Window w, FuncKey *k);
extern void UngrabModKeys(Window w, FuncKey *k);
extern void GetWindowSizeHints(TwmWindow *tmp);
extern void FetchWmProtocols(TwmWindow *tmp);
extern void FetchWmColormapWindows(TwmWindow *tmp);
extern void GetGravityOffsets(TwmWindow *tmp, int *xp, int *yp);
extern void AddDefaultBindings(void);
extern void SetVirtualDesktop (char *geom, int scale);
extern void SetRaiseWindow (TwmWindow *tmp);
extern TwmColormap *CreateTwmColormap(Colormap c);
extern void free_cwins(TwmWindow *tmp);
extern void CreateIconWindow (TwmWindow *tmp_win, int def_x, int def_y);
extern void IconUp (TwmWindow *tmp_win);
extern void IconDown (TwmWindow *tmp_win);
extern void AppletDown (TwmWindow *tmp_win);
extern void delete_pidfile(void);
extern int MatchName(
	  char *name,
	  char *pattern,
#ifndef NO_REGEX_SUPPORT
	  regex_t *compiled,
#else
	  char *compiled,
#endif
	  short type);
extern void ResizeTwmWindowContents(TwmWindow *tmp_win, int width, int height);
extern void SetRaiseWindow(TwmWindow *tmp);
extern void RestartVtwm(Time time);
extern int parse_keyword(char *s, int *nump);
extern int do_single_keyword(int keyword);
extern int do_string_keyword(int keyword, char *s);
extern int do_number_keyword(int keyword, int num);
extern name_list **do_colorlist_keyword(int keyword, int colormode, char *s);
extern int do_color_keyword(int keyword, int colormode, char *s);
extern void do_string_savecolor(int colormode, char *s);
extern void do_var_savecolor(int key);
extern void assign_var_savecolor(void);
extern void do_squeeze_entry(name_list **list, char *name, int type, int justify, int num, int denom);
extern void splitRegionEntry(RegionEntry *re, int grav1, int grav2, int w, int h);
extern int roundEntryUp(int v, int multiple);
extern RegionEntry *prevRegionEntry(RegionEntry *re, RootRegion *rr);
extern void mergeRegionEntries(RegionEntry *old, RegionEntry *re);
extern void downRegionEntry(RootRegion *rr, RegionEntry *re);
extern RootRegion *AddRegion(char *geom, int grav1, int grav2, int stepx, int stepy);
extern void FreeRegionEntries(RootRegion *rr);
extern void FreeRegions(RootRegion *first, RootRegion *last);
extern int SetSound(char *function, char *filename, int volume);
extern void SetSoundHost(char *host);
extern void SetSoundVolume(int volume);
extern ColormapWindow *CreateColormapWindow(Window w, int creating_parent, int property_window);
extern void twmrc_error_prefix(void);
extern void SetHighlightPixmap(char *filename);
extern void SetVirtualPixmap(char *filename);
extern void SetRealScreenPixmap(char *filename);
extern void NewBitmapCursor(Cursor *cp, char *source, char *mask);
extern void AddIconRegion (char *geom, int grav1, int grav2, int stepx, int stepy);
extern RootRegion *AddAppletRegion (char *geom, int grav1, int grav2, int stepx, int stepy);
extern void AddToAppletList (RootRegion *list_head, char *name, int type);
extern int PlaceApplet(TwmWindow *tmp_win, int def_x, int def_y, int *final_x, int *final_y);
extern int yylex (void);

#endif /* _PROTOTYPES_H */
