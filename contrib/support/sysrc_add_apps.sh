#!/bin/sh
#
# Adds installed applications to the box-stock system.vtwmrc file.

syntax ()
{
	# could be a "here-doc", but it'd break at least one shell
	echo
	echo "Usage: ${0##*/} [-i in_file] [-o out_file] [-m menu_decl] \\"
	echo "       [-a app_list | -A app_file] [-f] [-h]"
	echo
	echo "Default in_file: $DEF_INPUT"
	echo "Default out_file: $DEF_OUTPUT"
	echo "Default menu_decl: $DEF_MENU_DECL"
	echo "Default app_list: $DEF_LIST"
	echo
	echo "app_file is a list, each line containing one application specification"
	echo "(or the keywords \"SEPARATOR\" or \"TITLE::string\"). The specification is"
	echo "of the form \"name::string\", where \"name\" is the menu entry and \"string\""
	echo "is the command to execute. Whitespace in either part is indicated with"
	echo "a single colon."
	echo

	exit
}

check_replace ()
{
	CWD=$PWD
	cd `dirname "$INPUT"` && INDIR=$PWD && INNAME=`basename "$INPUT"`
	cd `dirname "$OUTPUT"` && OUTDIR=$PWD && OUTNAME=`basename "$OUTPUT"`
	cd $CWD
	if [ "$INDIR" = "$OUTDIR" -a "$INNAME" = "$OUTNAME" ]; then
		if [ $NO_PROMPT -eq 1 ]; then
			echo "Replacing in_file with out_file."
		else
			echo -n "Replace in_file with out_file (y/n)? "
			read ANS
			[ "$ANS" != "y" -a "$ANS" != "Y" ] && \
			    echo "Exiting." && exit
		fi

		OUTPUT=$TMPOUT
	fi
}

check_X_path ()
{
	ANS=`which xterm 2>/dev/null`
	if [ -z "$ANS" ]; then
		if [ $NO_PROMPT -eq 1 ]; then
			echo "No path to X applications (xterm)."
		else
			echo "The path to X applications (xterm) isn't set."
			while true; do
				echo -n "Enter full X path: "
				read ANS
				[ -z "$ANS" ] && echo "Exiting." && exit
				[ -d "$ANS" -a -f "$ANS/xterm" -a \
				    -x "$ANS/xterm" ] && break
			done
			PATH=$PATH:$ANS
		fi
	fi
}

check_proceed ()
{
	echo
	echo "in_file: $INPUT"
	if [ "$OUTPUT" = "$TMPOUT" ]; then
		echo "out_file: $INPUT"
	else
		echo "out_file: $OUTPUT"
	fi
	echo "menu_decl: $MENU_DECL"
	echo "app_list: $APP_LIST"
	echo "app_file: $APP_FILE"
	echo "Search path: $PATH"
	echo
	if [ $NO_PROMPT -eq 1 ]; then
		echo "Proceeding..."
	else
		echo -n "Proceed (y/n)? "
		read ANS
		[ "$ANS" != "y" -a "$ANS" != "Y" ] && echo "Exiting." && exit
	fi
}

check_app ()
{
	if [ "$1" = "SEPARATOR" ]; then
		printf "\t\"\"\t\tf.separator\n" >>$2
	else
		CMD=${1%%::*}
		STR=${1##*::}

		if [ "$CMD" = "TITLE" ]; then
			printf "\t\"  %s  \"\t\tf.title\n" $STR \
			    |sed -e 's/:/ /g' >>$2
		else
			[ -z "`which $CMD 2>/dev/null`" ] && return
			printf "\t\"%s\"\t\tf.exec \"%s &\"\n" $CMD $STR \
			    |sed -e 's/:/ /g' >>$2
		fi
	fi
}

DEF_INPUT=./system.vtwmrc
DEF_OUTPUT=./custom.vtwmrc

DEF_LIST="TITLE::Shells emu eterm mxterm rxvt xterm xvt"
DEF_LIST="$DEF_LIST midc::xterm:-e:midc top::xterm:-e:top"
DEF_LIST="$DEF_LIST TITLE::Editors nedit xcoral xvile"
DEF_LIST="$DEF_LIST vi::xterm:-e:vi vile::xterm:-e:vile vim::xterm:-e:vim"
DEF_LIST="$DEF_LIST SEPARATOR gimp xbmbrowser xfig xpaint xv bitmap pixmap"
DEF_LIST="$DEF_LIST TITLE::Desktop applix soffice abiword lyx gnumeric"
DEF_LIST="$DEF_LIST ghostview xcal xcalendar"
DEF_LIST="$DEF_LIST SEPARATOR tkman xman"
DEF_LIST="$DEF_LIST SEPARATOR mplayer::mplayer:-gui xine xmcd xmmix xmms"
DEF_LIST="$DEF_LIST SEPARATOR hexcalc xcalc editres"
DEF_LIST="$DEF_LIST xbiff xcb xev xeyes xload xmag"
DEF_LIST="$DEF_LIST SEPARATOR moonclock mouseclock oclock"
DEF_LIST="$DEF_LIST rclock sunclock t3d xarclock xclock xdaliclock"
DEF_LIST="$DEF_LIST TITLE::Network chimera ie mozilla netscape opera"
DEF_LIST="$DEF_LIST links::xterm:-e:links lynx::xterm:-e:lynx"
DEF_LIST="$DEF_LIST SEPARATOR exmh knews xdir"
DEF_LIST="$DEF_LIST ftp::xterm:-e:ftp telnet::xterm:-e:telnet"
DEF_LIST="$DEF_LIST elm::xterm:-e:elm mutt::xterm:-e:mutt"
DEF_LIST="$DEF_LIST pine::xterm:-e:pine tin::xterm:-e:tin"

DEF_MENU_DECL="menu \"apps\""

NO_PROMPT=0

TMPOUT=out.$$
LISTOUT=list.$$

while getopts "i:o:m:a:A:fh" OPT; do
        case $OPT in
                i) INPUT=$OPTARG;;
                o) OUTPUT=$OPTARG;;
		m) MENU_DECL=$OPTARG;;
                a) APP_LIST=$OPTARG;;
                A) APP_FILE=$OPTARG;;
		f) NO_PROMPT=1;;
                *) syntax;;
        esac
done

[ -z "$INPUT" ] && INPUT=$DEF_INPUT
[ -z "$OUTPUT" ] && OUTPUT=$DEF_OUTPUT
[ -z "$APP_LIST" ] && APP_LIST=$DEF_LIST
[ -n "$APP_FILE" ] && APP_LIST=
[ -z "$MENU_DECL" ] && MENU_DECL=$DEF_MENU_DECL

[ ! -f "$INPUT" ] && echo "$INPUT: File not found" && syntax

check_replace
check_X_path
check_proceed

if [ -n "$APP_LIST" ]; then
	for APP in $APP_LIST; do
		check_app $APP $LISTOUT
	done
else
	if [ -f "$APP_FILE" ]; then
		while read APP; do
			check_app $APP $LISTOUT
		done <"$APP_FILE"
	else
		echo "$APP_FILE: File not found"
		exit
	fi
fi

[ ! -s $LISTOUT ] && echo "No applications added." && exit

uniq $LISTOUT $$.$$
mv $$.$$ $LISTOUT

cat "$INPUT" \
    |awk -v list="$LISTOUT" -v menu="$MENU_DECL" \
    'BEGIN { \
	split(menu, menu); \
    } { \
	if ($1 == menu[1] && $2 == menu[2]) { \
	    print $0; \
	    do { \
		if ($1 == "{") print "{"; \
		a = getline; \
	    } while (a && $1 != "}"); \
	    if (a && $1 == "}") { \
		while (getline app <list) \
		    print app; \
	        print $0; \
	    } \
	} else \
	    print $0; \
    }' \
    >"$OUTPUT"

[ -s $TMPOUT ] && mv $TMPOUT "$INPUT"
rm -f *.$$
echo "Done."
