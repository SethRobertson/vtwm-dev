# Copyright 1999-2008 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: /var/cvsroot/gentoo-x86/x11-wm/twm/twm-1.0.4.ebuild,v 1.1 2008/03/10 02:46:29 dberkholz Exp $

# Must be before x-modular eclass is inherited
#SNAPSHOT="yes"

inherit x-modular

DESCRIPTION="One of many TWM descendants and implements a Virtual Desktop"
HOMEPAGE="http://www.vtwm.org/"
SRC_URI="http://www.vtwm.org/downloads/${P}.tar.gz"

LICENSE="MIT"
SLOT="0"
KEYWORDS="~alpha ~ppc ~sparc ~x86 ~amd64"
IUSE="rplay xpm png xft xrandr xinerama"

RDEPEND="x11-libs/libX11
	x11-libs/libXmu
	x11-libs/libXt
	x11-libs/libXext
	xpm? ( x11-libs/libXpm )
	png? ( media-libs/libpng )
	xft? ( virtual/xft )
	xrandr? ( x11-libs/libXrandr )
	xinerama? ( x11-libs/libXinerama )
	rplay? ( media-sound/rplay )"
DEPEND="${RDEPEND}"
