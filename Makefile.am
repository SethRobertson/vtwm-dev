## Process this file with automake to produce Makefile.in
## aclocal && automake --foreign && autoconf && ./configure

AM_YFLAGS = -d


if ENABLE_NEXPM
bin_PROGRAMS = vtwm nexpm
else
bin_PROGRAMS = vtwm
endif

rcdir = ${datadir}/X11/twm
dist_rc_DATA = system.vtwmrc

vtwm_CFLAGS = $(VWM_CFLAGS) -D_BSD_SOURCE -fno-strict-aliasing
vtwm_CFLAGS += -D_POSIX_C_SOURCE=199309L -D_POSIX_SOURCE -D_XOPEN_SOURCE -D_SVID_SOURCE 
vtwm_CFLAGS += -DSYSTEM_VTWMRC=\"${datadir}/X11/twm/system.vtwmrc\" -DSYSTEM_TWMRC=\"${datadir}/X11/twm/system.twmrc\"
vtwm_CFLAGS += @SOUND_SUPPORT@ @xpm_CFLAGS@ @regex_CFLAGS@ @xft_CFLAGS@ @xft_AUX_CFLAGS@ @xrandr_CFLAGS@ @xrandr_AUX_CFLAGS@
vtwm_CFLAGS += @TWM_USE_SPACING@ @TWM_USE_OPACITY@ @TWM_USE_SLOPPYFOCUS@ @xinerama_CFLAGS@ @xinerama_AUX_CFLAGS@
vtwm_CFLAGS += @NO_BUILD_INFO@ @NEED_SELECT_H@ @NEED_PROCESS_H@ @NEED_PUTENV_F@ @NO_M4_SUPPORT@ @png_CFLAGS@
if MAINTAINER_MODE
vtwm_CFLAGS += -Wall -Werror -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes
endif

vtwm_LDADD = $(VTWM_LIBS) $(xpm_LIBS) $(png_LIBS) $(regex_LIBS) $(xft_LIBS) $(xrandr_LIBS) $(xinerama_LIBS)

nexpm_CFLAGS = $(vtwm_CFLAGS) -Icontrib/nexpm
nexpm_LDADD = $(vtwm_LDADD)

vtwm_SOURCES =   \
	add_window.c \
	add_window.h \
	applets.c \
	config.h \
	cursor.c \
	deftwmrc.c \
	desktop.c \
	desktop.h \
	doors.c \
	doors.h \
	events.c \
	events.h \
	gc.c \
	gc.h \
	iconmgr.c \
	iconmgr.h \
	icons.c \
	image_formats.c \
	image_formats.h \
	lastmake.c \
	list.c \
	list.h \
	menus.c \
	menus.h \
	parse.c \
	parse.h \
	regions.c \
	regions.h \
	resize.c \
	resize.h \
	screen.h \
	sound.c \
	sound.h \
	twm.c \
	twm.h \
	util.c \
	util.h \
	version-tmp.c \
	version.h \
	gram.y \
	lex.l

nexpm_SOURCES = contrib/nexpm/nexpm.c

BUILT_SOURCES = gram.h gram.c lex.c deftwmrc.c lastmake.c version-tmp.c
MAINTAINERCLEANFILES =
DISTCLEANFILES=$(BUILT_SOURCES) system.vtwmrc vtwm.man vtwm.1 nexpm
DISTCLEANDIRS=autom4te.cache


deftwmrc.c: system.vtwmrc
	$(RM) $@
	echo '/* ' >>$@
	echo ' * This file is generated automatically from the default' >>$@
	echo ' * VTWM bindings file system.vtwmrc.'$(SYS_VTWMRC_LOOK)' by the VTWM Makefile.' >>$@
	echo ' */' >>$@
	echo '' >>$@
	echo 'char *defTwmrc[] = {' >>deftwmrc.c
	sed -e '/^$$/d' -e '/^#/d' -e 's/"/\\"/g' -e 's/^/"/' -e 's/$$/",/' -e 's/[	]\{1,\}/ /g' -e 's/^" /"/' system.vtwmrc >>$@
	echo '(char *)0 };' >>$@

.INTERMEDIATE: version-tmp.c
version-tmp.c: version.c
	if [ -d .git ]; then sed "s/SUBST/`git describe --tags`/" < version.c > version-tmp.c; else cp version.c version-tmp.c; fi

releasetar: maintainer-clean
	if [ ! -d .git ]; then echo Cannot create release tar without git information; exit 2; fi
	sed -i "s/SUBST/`git describe --tags`/" version.c
	P=`pwd | sed 's:.*/::'`; (cd ..; tar --exclude .git -czf $$P.tar.gz  $$P)
	git checkout version.c

FORCE:
lastmake.c: FORCE
	$(RM) $@
	echo '/* ' >>$@
	echo ' * This file is generated automatically by the VTWM Makefile.' >>$@
	echo ' */' >>$@
	echo '' >>$@
	echo 'char *lastmake[] = {' >>lastmake.c
	echo '    "Platform:  '`uname -r -s`'",' >>$@
	echo '    "Build:  '`date`'",' >>$@
	echo '    "" };' >>$@

gram.h: gram.c

#nexpm: contrib/nexpm/nexpm.c
#	$(vtwm_LINK) -o $@ -Icontrib/nexpm $^ $(vtwm_LDADD) $(LIBS)

system.vtwmrc:
	$(RM) $@
	$(CP) $@.$(SYS_VTWMRC_LOOK) $@

man1_MANS = vtwm.man
EXRTA_DIST = $(man1_MANS)

vtwm.man: doc/vtwm.man
	$(RM) $@
	$(LN) doc/$@ $@
