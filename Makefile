# Makefile generated by imake - do not edit!
# $XConsortium: imake.c,v 1.65 91/07/25 17:50:17 rws Exp $
#
# The cpp used on this machine replaces all newlines and multiple tabs and
# spaces in a macro expansion with a single space.  Imake tries to compensate
# for this, but is not always successful.
#

# -------------------------------------------------------------------------
# Makefile generated from "Imake.tmpl" and </tmp/IIf.a00979>
# $XConsortium: Imake.tmpl,v 1.139 91/09/16 08:52:48 rws Exp $
#
# Platform-specific parameters may be set in the appropriate <vendor>.cf
# configuration files.  Site-specific parameters should be set in the file
# site.def.  Full rebuilds are recommended if any parameters are changed.
#
# If your C preprocessor does not define any unique symbols, you will need
# to set BOOTSTRAPCFLAGS when rebuilding imake (usually when doing
# "make World" the first time).
#

# -------------------------------------------------------------------------
# site-specific configuration parameters that need to come before
# the platform-specific parameters - edit site.def to change

# site:  $XConsortium: site.def,v 1.2 91/07/30 20:26:44 rws Exp $

# -------------------------------------------------------------------------
# platform-specific configuration parameters - edit sun.cf to change

# platform:  $XConsortium: sun.cf,v 1.72.1.1 92/03/18 13:13:37 rws Exp

# operating system:  SunOS 4.1

# $XConsortium: sunLib.rules,v 1.7 91/12/20 11:19:47 rws Exp $

# -------------------------------------------------------------------------
# site-specific configuration parameters that go after
# the platform-specific parameters - edit site.def to change

# site:  $XConsortium: site.def,v 1.2 91/07/30 20:26:44 rws Exp $

            SHELL = /bin/sh

              TOP = .
      CURRENT_DIR = .

               AR = ar clq
  BOOTSTRAPCFLAGS =
               CC = cc
               AS = as

         COMPRESS = compress
              CPP = /lib/cpp $(STD_CPP_DEFINES)
    PREPROCESSCMD = cc -E $(STD_CPP_DEFINES)
          INSTALL = install
               LD = ld
             LINT = lint
      LINTLIBFLAG = -C
         LINTOPTS = -axz
               LN = ln -s
             MAKE = make
               MV = mv
               CP = cp

           RANLIB = ranlib
  RANLIBINSTFLAGS =

               RM = rm -f
            TROFF = psroff
         MSMACROS = -ms
              TBL = tbl
              EQN = eqn
     STD_INCLUDES =
  STD_CPP_DEFINES =
      STD_DEFINES =
 EXTRA_LOAD_FLAGS =
  EXTRA_LIBRARIES =
             TAGS = ctags

    SHAREDCODEDEF = -DSHAREDCODE
         SHLIBDEF = -DSUNSHLIB

    PROTO_DEFINES =

     INSTPGMFLAGS =

     INSTBINFLAGS = -m 0755
     INSTUIDFLAGS = -m 4755
     INSTLIBFLAGS = -m 0644
     INSTINCFLAGS = -m 0444
     INSTMANFLAGS = -m 0444
     INSTDATFLAGS = -m 0444
    INSTKMEMFLAGS = -g kmem -m 2755

      PROJECTROOT = /vol/X11R5

     TOP_INCLUDES = -I$(INCROOT)

      CDEBUGFLAGS = -O
        CCOPTIONS = -pipe

      ALLINCLUDES = $(INCLUDES) $(EXTRA_INCLUDES) $(TOP_INCLUDES) $(STD_INCLUDES)
       ALLDEFINES = $(ALLINCLUDES) $(STD_DEFINES) $(EXTRA_DEFINES) $(PROTO_DEFINES) $(DEFINES)
           CFLAGS = $(CDEBUGFLAGS) $(CCOPTIONS) $(ALLDEFINES)
        LINTFLAGS = $(LINTOPTS) -DLINT $(ALLDEFINES)

           LDLIBS = $(SYS_LIBRARIES) $(EXTRA_LIBRARIES)

        LDOPTIONS = $(CDEBUGFLAGS) $(CCOPTIONS) $(LOCAL_LDFLAGS) -L$(USRLIBDIR)

   LDCOMBINEFLAGS = -X -r
      DEPENDFLAGS =

        MACROFILE = sun.cf
           RM_CMD = $(RM) *.CKP *.ln *.BAK *.bak *.o core errs ,* *~ *.a .emacs_* tags TAGS make.log MakeOut

    IMAKE_DEFINES =

         IRULESRC = $(CONFIGDIR)
        IMAKE_CMD = $(IMAKE) -DUseInstalled -I$(IRULESRC) $(IMAKE_DEFINES)

     ICONFIGFILES = $(IRULESRC)/Imake.tmpl $(IRULESRC)/Imake.rules \
			$(IRULESRC)/Project.tmpl $(IRULESRC)/site.def \
			$(IRULESRC)/$(MACROFILE) $(EXTRA_ICONFIGFILES)

# -------------------------------------------------------------------------
# X Window System Build Parameters
# $XConsortium: Project.tmpl,v 1.138.1.1 92/11/11 09:49:19 rws Exp $

# -------------------------------------------------------------------------
# X Window System make variables; this need to be coordinated with rules

          PATHSEP = /
        USRLIBDIR = /vol/X11R5/lib
           BINDIR = /vol/X11R5/bin
          INCROOT = /vol/X11R5/include
     BUILDINCROOT = $(TOP)
      BUILDINCDIR = $(BUILDINCROOT)/X11
      BUILDINCTOP = ..
           INCDIR = $(INCROOT)/X11
           ADMDIR = /usr/adm
           LIBDIR = $(USRLIBDIR)/X11
        CONFIGDIR = $(LIBDIR)/config
       LINTLIBDIR = $(USRLIBDIR)/lint

          FONTDIR = $(LIBDIR)/fonts
         XINITDIR = $(LIBDIR)/xinit
           XDMDIR = $(LIBDIR)/xdm
           TWMDIR = $(LIBDIR)/twm
          MANPATH = /vol/X11R5/man
    MANSOURCEPATH = $(MANPATH)/man
        MANSUFFIX = 1
     LIBMANSUFFIX = 3
           MANDIR = $(MANSOURCEPATH)$(MANSUFFIX)
        LIBMANDIR = $(MANSOURCEPATH)$(LIBMANSUFFIX)
           NLSDIR = $(LIBDIR)/nls
        PEXAPIDIR = $(LIBDIR)/PEX
      XAPPLOADDIR = $(LIBDIR)/app-defaults
       FONTCFLAGS = -t

     INSTAPPFLAGS = $(INSTDATFLAGS)

            IMAKE = imake
           DEPEND = makedepend
              RGB = rgb

            FONTC = bdftopcf

        MKFONTDIR = mkfontdir
        MKDIRHIER = /bin/sh $(BINDIR)/mkdirhier

        CONFIGSRC = $(TOP)/config
       DOCUTILSRC = $(TOP)/doc/util
        CLIENTSRC = $(TOP)/clients
          DEMOSRC = $(TOP)/demos
           LIBSRC = $(TOP)/lib
          FONTSRC = $(TOP)/fonts
       INCLUDESRC = $(TOP)/X11
        SERVERSRC = $(TOP)/server
          UTILSRC = $(TOP)/util
        SCRIPTSRC = $(UTILSRC)/scripts
       EXAMPLESRC = $(TOP)/examples
       CONTRIBSRC = $(TOP)/../contrib
           DOCSRC = $(TOP)/doc
           RGBSRC = $(TOP)/rgb
        DEPENDSRC = $(UTILSRC)/makedepend
         IMAKESRC = $(CONFIGSRC)
         XAUTHSRC = $(LIBSRC)/Xau
          XLIBSRC = $(LIBSRC)/X
           XMUSRC = $(LIBSRC)/Xmu
       TOOLKITSRC = $(LIBSRC)/Xt
       AWIDGETSRC = $(LIBSRC)/Xaw
       OLDXLIBSRC = $(LIBSRC)/oldX
      XDMCPLIBSRC = $(LIBSRC)/Xdmcp
      BDFTOSNFSRC = $(FONTSRC)/bdftosnf
      BDFTOSNFSRC = $(FONTSRC)/clients/bdftosnf
      BDFTOPCFSRC = $(FONTSRC)/clients/bdftopcf
     MKFONTDIRSRC = $(FONTSRC)/clients/mkfontdir
         FSLIBSRC = $(FONTSRC)/lib/fs
    FONTSERVERSRC = $(FONTSRC)/server
     EXTENSIONSRC = $(TOP)/extensions
         XILIBSRC = $(EXTENSIONSRC)/lib/xinput
        PEXLIBSRC = $(EXTENSIONSRC)/lib/PEXlib
      PHIGSLIBSRC = $(EXTENSIONSRC)/lib/PEX

# $XConsortium: sunLib.tmpl,v 1.14.1.2 92/11/11 09:55:02 rws Exp $

SHLIBLDFLAGS = -assert pure-text
PICFLAGS = -pic

  DEPEXTENSIONLIB =
     EXTENSIONLIB = -lXext

          DEPXLIB = $(DEPEXTENSIONLIB)
             XLIB = $(EXTENSIONLIB) -lX11

        DEPXMULIB = $(USRLIBDIR)/libXmu.sa.$(SOXMUREV)
       XMULIBONLY = -lXmu
           XMULIB = -lXmu

       DEPOLDXLIB =
          OLDXLIB = -loldX

      DEPXTOOLLIB = $(USRLIBDIR)/libXt.sa.$(SOXTREV)
         XTOOLLIB = -lXt

        DEPXAWLIB = $(USRLIBDIR)/libXaw.sa.$(SOXAWREV)
           XAWLIB = -lXaw

        DEPXILIB =
           XILIB = -lXi

        DEPPEXLIB =
           PEXLIB = -lPEX5

        SOXLIBREV = 4.10
          SOXTREV = 4.10
         SOXAWREV = 5.0
        SOOLDXREV = 4.10
         SOXMUREV = 4.10
        SOXEXTREV = 4.10
      SOXINPUTREV = 4.10
         SOPEXREV = 1.0

      DEPXAUTHLIB = $(USRLIBDIR)/libXau.a
         XAUTHLIB =  -lXau
      DEPXDMCPLIB = $(USRLIBDIR)/libXdmcp.a
         XDMCPLIB =  -lXdmcp

        DEPPHIGSLIB = $(USRLIBDIR)/libphigs.a
           PHIGSLIB =  -lphigs

       DEPXBSDLIB = $(USRLIBDIR)/libXbsd.a
          XBSDLIB =  -lXbsd

 LINTEXTENSIONLIB = $(LINTLIBDIR)/llib-lXext.ln
         LINTXLIB = $(LINTLIBDIR)/llib-lX11.ln
          LINTXMU = $(LINTLIBDIR)/llib-lXmu.ln
        LINTXTOOL = $(LINTLIBDIR)/llib-lXt.ln
          LINTXAW = $(LINTLIBDIR)/llib-lXaw.ln
           LINTXI = $(LINTLIBDIR)/llib-lXi.ln
          LINTPEX = $(LINTLIBDIR)/llib-lPEX5.ln
        LINTPHIGS = $(LINTLIBDIR)/llib-lphigs.ln

          DEPLIBS = $(DEPXAWLIB) $(DEPXMULIB) $(DEPXTOOLLIB) $(DEPXLIB)

         DEPLIBS1 = $(DEPLIBS)
         DEPLIBS2 = $(DEPLIBS)
         DEPLIBS3 = $(DEPLIBS)

# -------------------------------------------------------------------------
# Imake rules for building libraries, programs, scripts, and data files
# rules:  $XConsortium: Imake.rules,v 1.123 91/09/16 20:12:16 rws Exp $

# -------------------------------------------------------------------------
# start of Imakefile

# $XConsortium: Imakefile,v 1.33 91/07/17 00:48:06 gildea Exp $
#
# Here is an Imakefile for vtwm.  It depends on having TWMDIR defined
# in Imake.tmpl.  I like to use Imakefiles for everything, and I am sure
# other people do also, so perhaps you could do us all a favor and
# distribute this one.
#

#MAINTAINER=`whoami`
MAINTAINER="Nick Williams (njw@cs.city.ac.uk)"

         YFLAGS = -d
       LINTLIBS = $(LINTXMU) $(LINTEXTENSIONLIB) $(LINTXLIB)

      XPMLIBDIR =
      XPMINCDIR =

     XPMDEFINES = -DXPM
         XPMLIB = $(XPMLIBDIR) -lXpm

        DEPLIBS = $(DEPXMULIB) $(DEPEXTENSIONLIB) $(DEPXLIB)
LOCAL_LIBRARIES = $(XPMLIB) $(XMULIB) $(XTOOLLIB) $(EXTENSIONLIB) $(XLIB)
        DEFINES = $(SIGNAL_DEFINES) $(XPMDEFINES)

           SRCS = gram.c lex.c deftwmrc.c add_window.c gc.c list.c twm.c \
		parse.c menus.c events.c resize.c util.c version.c iconmgr.c \
		cursor.c icons.c desktop.c doors.c move.c

           OBJS = gram.o lex.o deftwmrc.o add_window.o gc.o list.o twm.o \
		parse.o menus.o events.o resize.o util.o version.o iconmgr.o \
		cursor.o icons.o desktop.o doors.o move.o

  PIXMAPFILES =	xpm/IslandD.xpm        xpm/mail1.xpm          xpm/xgopher.xpm \
		xpm/IslandW.xpm        xpm/nothing.xpm        xpm/xgrab.xpm \
		xpm/LRom.xpm           xpm/pixmap.xpm         xpm/xhpcalc.xpm \
		xpm/LRom1.xpm          xpm/postit.xpm         xpm/xmail.xpm \
		xpm/arthur.xpm         xpm/term.xpm           xpm/xman.xpm \
		xpm/cdrom1.xpm         xpm/unknown.xpm        xpm/xnomail.xpm \
		xpm/claude.xpm         xpm/unread.xpm         xpm/xrn.goodnews.xpm \
		xpm/datebook.xpm       xpm/xarchie.xpm        xpm/xrn.nonews.xpm \
		xpm/emacs.xpm          xpm/xcalc.xpm          xpm/xrn.xpm \
		xpm/hpterm.xpm         xpm/xcalc2.xpm         xpm/xterm.xpm \
		xpm/mail0.xpm          xpm/xedit.xpm          xpm/welcome.xpm \
		xpm/audio_editor.xpm   xpm/clipboard.xpm      xpm/ghostview.xpm \
		xpm/xirc.xpm           xpm/xmosaic.xpm        xpm/unknown1.xpm \
		xpm/xrn-compose.xpm \
		xpm/3D_Expand15.xpm    xpm/3D_Iconify15.xpm   xpm/3D_Lightning15.xpm \
		xpm/3D_Menu15.xpm      xpm/3D_Resize15.xpm    xpm/3D_Zoom15.xpm \
		xpm/background1.xpm    xpm/background2.xpm    xpm/background3.xpm \
		xpm/background4.xpm    xpm/background5.xpm    xpm/background6.xpm \
		xpm/background7.xpm    xpm/background8.xpm    xpm/background9.xpm

all:: vtwm

parse.o: parse.c
	$(RM) $@
	$(CC) -c $(CFLAGS) '-DSYSTEM_INIT_FILE="'$(TWMDIR)'/system.vtwmrc"' $*.c

menus.o: menus.c
	$(RM) $@
	$(CC) -c $(CFLAGS) '-DMAINTAINER="'$(MAINTAINER)'"' $*.c

twm.o: twm.c
	$(RM) $@
	$(CC) -c $(CFLAGS) '-DPIXMAP_DIRECTORY="'$(TWMDIR)'"' $*.c

depend:: lex.c gram.c deftwmrc.c

 PROGRAM = vtwm

all:: vtwm

vtwm: $(OBJS) $(DEPLIBS)
	$(RM) $@
	$(CC) -o $@ $(OBJS) $(LDOPTIONS) $(LOCAL_LIBRARIES) $(LDLIBS) $(EXTRA_LOAD_FLAGS)

install:: vtwm
	@if [ -d $(DESTDIR)$(BINDIR) -o -h $(DESTDIR)$(BINDIR) ]; then set +x; \
	else (set -x; $(MKDIRHIER) $(DESTDIR)$(BINDIR)); fi
	$(INSTALL) -c $(INSTPGMFLAGS)  vtwm $(DESTDIR)$(BINDIR)

install.man:: vtwm.man
	@if [ -d $(DESTDIR)$(MANDIR) -o -h $(DESTDIR)$(MANDIR) ]; then set +x; \
	else (set -x; $(MKDIRHIER) $(DESTDIR)$(MANDIR)); fi
	$(INSTALL) -c $(INSTMANFLAGS) vtwm.man $(DESTDIR)$(MANDIR)/vtwm.$(MANSUFFIX)

depend::
	$(DEPEND) $(DEPENDFLAGS) -s "# DO NOT DELETE" -- $(ALLDEFINES) -- $(SRCS)

lint:
	$(LINT) $(LINTFLAGS) $(SRCS) $(LINTLIBS)
lint1:
	$(LINT) $(LINTFLAGS) $(FILE) $(LINTLIBS)

clean::
	$(RM) $(PROGRAM)

install:: system.vtwmrc
	@if [ -d $(DESTDIR)$(TWMDIR) -o -h $(DESTDIR)$(TWMDIR) ]; then set +x; \
	else (set -x; $(MKDIRHIER) $(DESTDIR)$(TWMDIR)); fi
	$(INSTALL) -c $(INSTDATFLAGS) system.vtwmrc $(DESTDIR)$(TWMDIR)

install:: $(PIXMAPFILES)
	@if [ -d $(DESTDIR)$(TWMDIR) -o -h $(DESTDIR)$(TWMDIR) ]; then set +x; \
	else (set -x; $(MKDIRHIER) $(DESTDIR)$(TWMDIR)); fi
	@case '${MFLAGS}' in *[i]*) set +e;; esac; \
	for i in $(PIXMAPFILES); do \
	(set -x; $(INSTALL) -c $(INSTLIBFLAGS) $$i $(DESTDIR)$(TWMDIR)); \
	done

gram.h gram.c: gram.y
	yacc $(YFLAGS) gram.y
	$(MV) y.tab.c gram.c
	$(MV) y.tab.h gram.h

clean::
	$(RM) y.tab.h y.tab.c lex.yy.c gram.h gram.c lex.c deftwmrc.c

deftwmrc.c:  system.vtwmrc
	$(RM) $@
	echo '/* ' >>$@
	echo ' * This file is generated automatically from the default' >>$@
	echo ' * vtwm bindings file system.vtwmrc by the vtwm Imakefile.' >>$@
	echo ' */' >>$@
	echo '' >>$@
	echo 'char *defTwmrc[] = {' >>$@
	sed -e '/^#/d' -e 's/"/\\"/g' -e 's/^/    "/' -e 's/$$/",/' \
		system.vtwmrc >>$@
	echo '    (char *) 0 };' >>$@

# -------------------------------------------------------------------------
# common rules for all Makefiles - do not edit

emptyrule::

clean::
	$(RM_CMD) "#"*

Makefile::
	-@if [ -f Makefile ]; then set -x; \
	$(RM) Makefile.bak; $(MV) Makefile Makefile.bak; \
	else exit 0; fi
	$(IMAKE_CMD) -DTOPDIR=$(TOP) -DCURDIR=$(CURRENT_DIR)

tags::
	$(TAGS) -w *.[ch]
	$(TAGS) -xw *.[ch] > TAGS

# -------------------------------------------------------------------------
# empty rules for directories that do not have SUBDIRS - do not edit

install::
	@echo "install in $(CURRENT_DIR) done"

install.man::
	@echo "install.man in $(CURRENT_DIR) done"

Makefiles::

includes::

# -------------------------------------------------------------------------
# dependencies generated by makedepend

# DO NOT DELETE

gram.o: /usr/include/stdio.h /usr/include/ctype.h twm.h
gram.o: /vol/X11R5/include/X11/Xlib.h /usr/include/sys/types.h
gram.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
gram.o: /vol/X11R5/include/X11/X.h /vol/X11R5/include/X11/Xfuncproto.h
gram.o: /vol/X11R5/include/X11/Xosdefs.h /usr/include/stddef.h
gram.o: /vol/X11R5/include/X11/Xutil.h /vol/X11R5/include/X11/cursorfont.h
gram.o: /vol/X11R5/include/X11/extensions/shape.h
gram.o: /vol/X11R5/include/X11/Xfuncs.h list.h
gram.o: /vol/X11R5/include/X11/Intrinsic.h /vol/X11R5/include/X11/Xresource.h
gram.o: /usr/include/string.h /vol/X11R5/include/X11/Core.h
gram.o: /vol/X11R5/include/X11/Composite.h
gram.o: /vol/X11R5/include/X11/Constraint.h /vol/X11R5/include/X11/Object.h
gram.o: /vol/X11R5/include/X11/RectObj.h /usr/include/stdlib.h menus.h util.h
gram.o: screen.h iconmgr.h doors.h parse.h /vol/X11R5/include/X11/Xos.h
gram.o: /usr/include/fcntl.h /usr/include/sys/fcntlcom.h
gram.o: /usr/include/sys/stat.h /usr/include/unistd.h /usr/include/sys/time.h
gram.o: /usr/include/sys/time.h /vol/X11R5/include/X11/Xmu/CharSet.h
lex.o: /usr/include/stdio.h gram.h parse.h
add_window.o: /usr/include/stdio.h twm.h /vol/X11R5/include/X11/Xlib.h
add_window.o: /usr/include/sys/types.h /usr/include/sys/stdtypes.h
add_window.o: /usr/include/sys/sysmacros.h /vol/X11R5/include/X11/X.h
add_window.o: /vol/X11R5/include/X11/Xfuncproto.h
add_window.o: /vol/X11R5/include/X11/Xosdefs.h /usr/include/stddef.h
add_window.o: /vol/X11R5/include/X11/Xutil.h
add_window.o: /vol/X11R5/include/X11/cursorfont.h
add_window.o: /vol/X11R5/include/X11/extensions/shape.h
add_window.o: /vol/X11R5/include/X11/Xfuncs.h list.h
add_window.o: /vol/X11R5/include/X11/Intrinsic.h
add_window.o: /vol/X11R5/include/X11/Xresource.h /usr/include/string.h
add_window.o: /vol/X11R5/include/X11/Core.h
add_window.o: /vol/X11R5/include/X11/Composite.h
add_window.o: /vol/X11R5/include/X11/Constraint.h
add_window.o: /vol/X11R5/include/X11/Object.h
add_window.o: /vol/X11R5/include/X11/RectObj.h /usr/include/stdlib.h
add_window.o: /vol/X11R5/include/X11/Xatom.h add_window.h util.h resize.h
add_window.o: move.h parse.h gram.h events.h menus.h screen.h iconmgr.h
add_window.o: doors.h desktop.h
gc.o: /usr/include/stdio.h twm.h /vol/X11R5/include/X11/Xlib.h
gc.o: /usr/include/sys/types.h /usr/include/sys/stdtypes.h
gc.o: /usr/include/sys/sysmacros.h /vol/X11R5/include/X11/X.h
gc.o: /vol/X11R5/include/X11/Xfuncproto.h /vol/X11R5/include/X11/Xosdefs.h
gc.o: /usr/include/stddef.h /vol/X11R5/include/X11/Xutil.h
gc.o: /vol/X11R5/include/X11/cursorfont.h
gc.o: /vol/X11R5/include/X11/extensions/shape.h
gc.o: /vol/X11R5/include/X11/Xfuncs.h list.h
gc.o: /vol/X11R5/include/X11/Intrinsic.h /vol/X11R5/include/X11/Xresource.h
gc.o: /usr/include/string.h /vol/X11R5/include/X11/Core.h
gc.o: /vol/X11R5/include/X11/Composite.h /vol/X11R5/include/X11/Constraint.h
gc.o: /vol/X11R5/include/X11/Object.h /vol/X11R5/include/X11/RectObj.h
gc.o: /usr/include/stdlib.h util.h screen.h menus.h iconmgr.h doors.h
list.o: /usr/include/stdio.h twm.h /vol/X11R5/include/X11/Xlib.h
list.o: /usr/include/sys/types.h /usr/include/sys/stdtypes.h
list.o: /usr/include/sys/sysmacros.h /vol/X11R5/include/X11/X.h
list.o: /vol/X11R5/include/X11/Xfuncproto.h /vol/X11R5/include/X11/Xosdefs.h
list.o: /usr/include/stddef.h /vol/X11R5/include/X11/Xutil.h
list.o: /vol/X11R5/include/X11/cursorfont.h
list.o: /vol/X11R5/include/X11/extensions/shape.h
list.o: /vol/X11R5/include/X11/Xfuncs.h list.h
list.o: /vol/X11R5/include/X11/Intrinsic.h /vol/X11R5/include/X11/Xresource.h
list.o: /usr/include/string.h /vol/X11R5/include/X11/Core.h
list.o: /vol/X11R5/include/X11/Composite.h
list.o: /vol/X11R5/include/X11/Constraint.h /vol/X11R5/include/X11/Object.h
list.o: /vol/X11R5/include/X11/RectObj.h /usr/include/stdlib.h screen.h
list.o: menus.h iconmgr.h doors.h gram.h
twm.o: /usr/include/stdio.h /usr/include/signal.h /usr/include/sys/signal.h
twm.o: /usr/include/vm/faultcode.h /usr/include/fcntl.h
twm.o: /usr/include/sys/fcntlcom.h /usr/include/sys/stdtypes.h
twm.o: /usr/include/sys/stat.h /usr/include/sys/types.h
twm.o: /usr/include/sys/sysmacros.h twm.h /vol/X11R5/include/X11/Xlib.h
twm.o: /vol/X11R5/include/X11/X.h /vol/X11R5/include/X11/Xfuncproto.h
twm.o: /vol/X11R5/include/X11/Xosdefs.h /usr/include/stddef.h
twm.o: /vol/X11R5/include/X11/Xutil.h /vol/X11R5/include/X11/cursorfont.h
twm.o: /vol/X11R5/include/X11/extensions/shape.h
twm.o: /vol/X11R5/include/X11/Xfuncs.h list.h
twm.o: /vol/X11R5/include/X11/Intrinsic.h /vol/X11R5/include/X11/Xresource.h
twm.o: /usr/include/string.h /vol/X11R5/include/X11/Core.h
twm.o: /vol/X11R5/include/X11/Composite.h /vol/X11R5/include/X11/Constraint.h
twm.o: /vol/X11R5/include/X11/Object.h /vol/X11R5/include/X11/RectObj.h
twm.o: /usr/include/stdlib.h add_window.h gc.h parse.h version.h menus.h
twm.o: events.h util.h gram.h screen.h iconmgr.h doors.h desktop.h
twm.o: /vol/X11R5/include/X11/Xproto.h /vol/X11R5/include/X11/Xmd.h
twm.o: /vol/X11R5/include/X11/Xprotostr.h /vol/X11R5/include/X11/Xatom.h
parse.o: /usr/include/stdio.h /vol/X11R5/include/X11/Xos.h
parse.o: /vol/X11R5/include/X11/Xosdefs.h /usr/include/sys/types.h
parse.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
parse.o: /usr/include/string.h /usr/include/fcntl.h
parse.o: /usr/include/sys/fcntlcom.h /usr/include/sys/stat.h
parse.o: /usr/include/unistd.h /usr/include/sys/time.h
parse.o: /usr/include/sys/time.h /vol/X11R5/include/X11/Xmu/CharSet.h
parse.o: /vol/X11R5/include/X11/Xfuncproto.h twm.h
parse.o: /vol/X11R5/include/X11/Xlib.h /vol/X11R5/include/X11/X.h
parse.o: /usr/include/stddef.h /vol/X11R5/include/X11/Xutil.h
parse.o: /vol/X11R5/include/X11/cursorfont.h
parse.o: /vol/X11R5/include/X11/extensions/shape.h
parse.o: /vol/X11R5/include/X11/Xfuncs.h list.h
parse.o: /vol/X11R5/include/X11/Intrinsic.h
parse.o: /vol/X11R5/include/X11/Xresource.h /vol/X11R5/include/X11/Core.h
parse.o: /vol/X11R5/include/X11/Composite.h
parse.o: /vol/X11R5/include/X11/Constraint.h /vol/X11R5/include/X11/Object.h
parse.o: /vol/X11R5/include/X11/RectObj.h /usr/include/stdlib.h screen.h
parse.o: menus.h iconmgr.h doors.h util.h gram.h parse.h
parse.o: /vol/X11R5/include/X11/Xatom.h
menus.o: /usr/include/stdio.h /usr/include/signal.h /usr/include/sys/signal.h
menus.o: /usr/include/vm/faultcode.h /vol/X11R5/include/X11/Xos.h
menus.o: /vol/X11R5/include/X11/Xosdefs.h /usr/include/sys/types.h
menus.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
menus.o: /usr/include/string.h /usr/include/fcntl.h
menus.o: /usr/include/sys/fcntlcom.h /usr/include/sys/stat.h
menus.o: /usr/include/unistd.h /usr/include/sys/time.h
menus.o: /usr/include/sys/time.h twm.h /vol/X11R5/include/X11/Xlib.h
menus.o: /vol/X11R5/include/X11/X.h /vol/X11R5/include/X11/Xfuncproto.h
menus.o: /usr/include/stddef.h /vol/X11R5/include/X11/Xutil.h
menus.o: /vol/X11R5/include/X11/cursorfont.h
menus.o: /vol/X11R5/include/X11/extensions/shape.h
menus.o: /vol/X11R5/include/X11/Xfuncs.h list.h
menus.o: /vol/X11R5/include/X11/Intrinsic.h
menus.o: /vol/X11R5/include/X11/Xresource.h /vol/X11R5/include/X11/Core.h
menus.o: /vol/X11R5/include/X11/Composite.h
menus.o: /vol/X11R5/include/X11/Constraint.h /vol/X11R5/include/X11/Object.h
menus.o: /vol/X11R5/include/X11/RectObj.h /usr/include/stdlib.h gc.h menus.h
menus.o: resize.h events.h util.h parse.h gram.h screen.h iconmgr.h doors.h
menus.o: desktop.h move.h /vol/X11R5/include/X11/Xmu/CharSet.h
menus.o: /vol/X11R5/include/X11/bitmaps/menu12 version.h
events.o: /usr/include/stdio.h twm.h /vol/X11R5/include/X11/Xlib.h
events.o: /usr/include/sys/types.h /usr/include/sys/stdtypes.h
events.o: /usr/include/sys/sysmacros.h /vol/X11R5/include/X11/X.h
events.o: /vol/X11R5/include/X11/Xfuncproto.h
events.o: /vol/X11R5/include/X11/Xosdefs.h /usr/include/stddef.h
events.o: /vol/X11R5/include/X11/Xutil.h /vol/X11R5/include/X11/cursorfont.h
events.o: /vol/X11R5/include/X11/extensions/shape.h
events.o: /vol/X11R5/include/X11/Xfuncs.h list.h
events.o: /vol/X11R5/include/X11/Intrinsic.h
events.o: /vol/X11R5/include/X11/Xresource.h /usr/include/string.h
events.o: /vol/X11R5/include/X11/Core.h /vol/X11R5/include/X11/Composite.h
events.o: /vol/X11R5/include/X11/Constraint.h /vol/X11R5/include/X11/Object.h
events.o: /vol/X11R5/include/X11/RectObj.h /usr/include/stdlib.h
events.o: /vol/X11R5/include/X11/Xatom.h add_window.h menus.h events.h
events.o: resize.h parse.h gram.h util.h screen.h iconmgr.h doors.h version.h
events.o: desktop.h /usr/include/sys/time.h /usr/include/sys/time.h
resize.o: /usr/include/stdio.h twm.h /vol/X11R5/include/X11/Xlib.h
resize.o: /usr/include/sys/types.h /usr/include/sys/stdtypes.h
resize.o: /usr/include/sys/sysmacros.h /vol/X11R5/include/X11/X.h
resize.o: /vol/X11R5/include/X11/Xfuncproto.h
resize.o: /vol/X11R5/include/X11/Xosdefs.h /usr/include/stddef.h
resize.o: /vol/X11R5/include/X11/Xutil.h /vol/X11R5/include/X11/cursorfont.h
resize.o: /vol/X11R5/include/X11/extensions/shape.h
resize.o: /vol/X11R5/include/X11/Xfuncs.h list.h
resize.o: /vol/X11R5/include/X11/Intrinsic.h
resize.o: /vol/X11R5/include/X11/Xresource.h /usr/include/string.h
resize.o: /vol/X11R5/include/X11/Core.h /vol/X11R5/include/X11/Composite.h
resize.o: /vol/X11R5/include/X11/Constraint.h /vol/X11R5/include/X11/Object.h
resize.o: /vol/X11R5/include/X11/RectObj.h /usr/include/stdlib.h parse.h
resize.o: util.h resize.h add_window.h screen.h menus.h iconmgr.h doors.h
resize.o: desktop.h
util.o: twm.h /vol/X11R5/include/X11/Xlib.h /usr/include/sys/types.h
util.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
util.o: /vol/X11R5/include/X11/X.h /vol/X11R5/include/X11/Xfuncproto.h
util.o: /vol/X11R5/include/X11/Xosdefs.h /usr/include/stddef.h
util.o: /vol/X11R5/include/X11/Xutil.h /vol/X11R5/include/X11/cursorfont.h
util.o: /vol/X11R5/include/X11/extensions/shape.h
util.o: /vol/X11R5/include/X11/Xfuncs.h list.h
util.o: /vol/X11R5/include/X11/Intrinsic.h /vol/X11R5/include/X11/Xresource.h
util.o: /usr/include/string.h /vol/X11R5/include/X11/Core.h
util.o: /vol/X11R5/include/X11/Composite.h
util.o: /vol/X11R5/include/X11/Constraint.h /vol/X11R5/include/X11/Object.h
util.o: /vol/X11R5/include/X11/RectObj.h /usr/include/stdlib.h util.h gram.h
util.o: screen.h menus.h iconmgr.h doors.h /vol/X11R5/include/X11/Xos.h
util.o: /usr/include/fcntl.h /usr/include/sys/fcntlcom.h
util.o: /usr/include/sys/stat.h /usr/include/unistd.h /usr/include/sys/time.h
util.o: /usr/include/sys/time.h /vol/X11R5/include/X11/Xatom.h
util.o: /usr/include/stdio.h /vol/X11R5/include/X11/Xmu/Drawing.h
util.o: /vol/X11R5/include/X11/Xmu/CharSet.h siconify.bm
iconmgr.o: /usr/include/stdio.h twm.h /vol/X11R5/include/X11/Xlib.h
iconmgr.o: /usr/include/sys/types.h /usr/include/sys/stdtypes.h
iconmgr.o: /usr/include/sys/sysmacros.h /vol/X11R5/include/X11/X.h
iconmgr.o: /vol/X11R5/include/X11/Xfuncproto.h
iconmgr.o: /vol/X11R5/include/X11/Xosdefs.h /usr/include/stddef.h
iconmgr.o: /vol/X11R5/include/X11/Xutil.h /vol/X11R5/include/X11/cursorfont.h
iconmgr.o: /vol/X11R5/include/X11/extensions/shape.h
iconmgr.o: /vol/X11R5/include/X11/Xfuncs.h list.h
iconmgr.o: /vol/X11R5/include/X11/Intrinsic.h
iconmgr.o: /vol/X11R5/include/X11/Xresource.h /usr/include/string.h
iconmgr.o: /vol/X11R5/include/X11/Core.h /vol/X11R5/include/X11/Composite.h
iconmgr.o: /vol/X11R5/include/X11/Constraint.h
iconmgr.o: /vol/X11R5/include/X11/Object.h /vol/X11R5/include/X11/RectObj.h
iconmgr.o: /usr/include/stdlib.h util.h parse.h screen.h menus.h iconmgr.h
iconmgr.o: doors.h resize.h add_window.h siconify.bm
iconmgr.o: /vol/X11R5/include/X11/Xos.h /usr/include/fcntl.h
iconmgr.o: /usr/include/sys/fcntlcom.h /usr/include/sys/stat.h
iconmgr.o: /usr/include/unistd.h /usr/include/sys/time.h
iconmgr.o: /usr/include/sys/time.h /vol/X11R5/include/X11/Xmu/CharSet.h
cursor.o: /usr/include/stdio.h twm.h /vol/X11R5/include/X11/Xlib.h
cursor.o: /usr/include/sys/types.h /usr/include/sys/stdtypes.h
cursor.o: /usr/include/sys/sysmacros.h /vol/X11R5/include/X11/X.h
cursor.o: /vol/X11R5/include/X11/Xfuncproto.h
cursor.o: /vol/X11R5/include/X11/Xosdefs.h /usr/include/stddef.h
cursor.o: /vol/X11R5/include/X11/Xutil.h /vol/X11R5/include/X11/cursorfont.h
cursor.o: /vol/X11R5/include/X11/extensions/shape.h
cursor.o: /vol/X11R5/include/X11/Xfuncs.h list.h
cursor.o: /vol/X11R5/include/X11/Intrinsic.h
cursor.o: /vol/X11R5/include/X11/Xresource.h /usr/include/string.h
cursor.o: /vol/X11R5/include/X11/Core.h /vol/X11R5/include/X11/Composite.h
cursor.o: /vol/X11R5/include/X11/Constraint.h /vol/X11R5/include/X11/Object.h
cursor.o: /vol/X11R5/include/X11/RectObj.h /usr/include/stdlib.h
cursor.o: /vol/X11R5/include/X11/Xos.h /usr/include/fcntl.h
cursor.o: /usr/include/sys/fcntlcom.h /usr/include/sys/stat.h
cursor.o: /usr/include/unistd.h /usr/include/sys/time.h
cursor.o: /usr/include/sys/time.h screen.h menus.h iconmgr.h doors.h util.h
icons.o: /usr/include/stdio.h twm.h /vol/X11R5/include/X11/Xlib.h
icons.o: /usr/include/sys/types.h /usr/include/sys/stdtypes.h
icons.o: /usr/include/sys/sysmacros.h /vol/X11R5/include/X11/X.h
icons.o: /vol/X11R5/include/X11/Xfuncproto.h /vol/X11R5/include/X11/Xosdefs.h
icons.o: /usr/include/stddef.h /vol/X11R5/include/X11/Xutil.h
icons.o: /vol/X11R5/include/X11/cursorfont.h
icons.o: /vol/X11R5/include/X11/extensions/shape.h
icons.o: /vol/X11R5/include/X11/Xfuncs.h list.h
icons.o: /vol/X11R5/include/X11/Intrinsic.h
icons.o: /vol/X11R5/include/X11/Xresource.h /usr/include/string.h
icons.o: /vol/X11R5/include/X11/Core.h /vol/X11R5/include/X11/Composite.h
icons.o: /vol/X11R5/include/X11/Constraint.h /vol/X11R5/include/X11/Object.h
icons.o: /vol/X11R5/include/X11/RectObj.h /usr/include/stdlib.h screen.h
icons.o: menus.h iconmgr.h doors.h icons.h gram.h parse.h util.h
desktop.o: /usr/include/stdio.h twm.h /vol/X11R5/include/X11/Xlib.h
desktop.o: /usr/include/sys/types.h /usr/include/sys/stdtypes.h
desktop.o: /usr/include/sys/sysmacros.h /vol/X11R5/include/X11/X.h
desktop.o: /vol/X11R5/include/X11/Xfuncproto.h
desktop.o: /vol/X11R5/include/X11/Xosdefs.h /usr/include/stddef.h
desktop.o: /vol/X11R5/include/X11/Xutil.h /vol/X11R5/include/X11/cursorfont.h
desktop.o: /vol/X11R5/include/X11/extensions/shape.h
desktop.o: /vol/X11R5/include/X11/Xfuncs.h list.h
desktop.o: /vol/X11R5/include/X11/Intrinsic.h
desktop.o: /vol/X11R5/include/X11/Xresource.h /usr/include/string.h
desktop.o: /vol/X11R5/include/X11/Core.h /vol/X11R5/include/X11/Composite.h
desktop.o: /vol/X11R5/include/X11/Constraint.h
desktop.o: /vol/X11R5/include/X11/Object.h /vol/X11R5/include/X11/RectObj.h
desktop.o: /usr/include/stdlib.h screen.h menus.h iconmgr.h doors.h
desktop.o: add_window.h parse.h events.h desktop.h move.h
doors.o: /usr/include/stdio.h /usr/include/string.h
doors.o: /usr/include/sys/stdtypes.h doors.h twm.h
doors.o: /vol/X11R5/include/X11/Xlib.h /usr/include/sys/types.h
doors.o: /usr/include/sys/sysmacros.h /vol/X11R5/include/X11/X.h
doors.o: /vol/X11R5/include/X11/Xfuncproto.h /vol/X11R5/include/X11/Xosdefs.h
doors.o: /usr/include/stddef.h /vol/X11R5/include/X11/Xutil.h
doors.o: /vol/X11R5/include/X11/cursorfont.h
doors.o: /vol/X11R5/include/X11/extensions/shape.h
doors.o: /vol/X11R5/include/X11/Xfuncs.h list.h
doors.o: /vol/X11R5/include/X11/Intrinsic.h
doors.o: /vol/X11R5/include/X11/Xresource.h /vol/X11R5/include/X11/Core.h
doors.o: /vol/X11R5/include/X11/Composite.h
doors.o: /vol/X11R5/include/X11/Constraint.h /vol/X11R5/include/X11/Object.h
doors.o: /vol/X11R5/include/X11/RectObj.h /usr/include/stdlib.h screen.h
doors.o: menus.h iconmgr.h desktop.h add_window.h
move.o: /usr/include/stdio.h /vol/X11R5/include/X11/X.h
move.o: /vol/X11R5/include/X11/Xatom.h twm.h /vol/X11R5/include/X11/Xlib.h
move.o: /usr/include/sys/types.h /usr/include/sys/stdtypes.h
move.o: /usr/include/sys/sysmacros.h /vol/X11R5/include/X11/Xfuncproto.h
move.o: /vol/X11R5/include/X11/Xosdefs.h /usr/include/stddef.h
move.o: /vol/X11R5/include/X11/Xutil.h /vol/X11R5/include/X11/cursorfont.h
move.o: /vol/X11R5/include/X11/extensions/shape.h
move.o: /vol/X11R5/include/X11/Xfuncs.h list.h
move.o: /vol/X11R5/include/X11/Intrinsic.h /vol/X11R5/include/X11/Xresource.h
move.o: /usr/include/string.h /vol/X11R5/include/X11/Core.h
move.o: /vol/X11R5/include/X11/Composite.h
move.o: /vol/X11R5/include/X11/Constraint.h /vol/X11R5/include/X11/Object.h
move.o: /vol/X11R5/include/X11/RectObj.h /usr/include/stdlib.h screen.h
move.o: menus.h iconmgr.h doors.h desktop.h move.h events.h parse.h
