############################################################
##                                                        ##
##  Here is my .kshrc, with uninteresting things          ##
##  expunged, showing how I associate different window    ##
##  names with different history files...                 ##
##                                                        ##
############################################################

# [...]

if [ "" = "$SHL" ]
then	# this is the top-level shell.
	if [ "" = "$LAYER" ]
	then	# this is the login shell.
		PS1=";$sysname.odin (!): "
		# $sysname was set in .profile to the machine name.
	else	# top-level shell from xterm.
		SHL=1; export SHL	# ptyshl
	fi
fi

if [ "1" = "$SHL" ]
then	# Need to process $LAYER
	SHL=2;export SHL
	# save subshells from disaster.

	PS1=";$sysname.$LAYER(!): "; export PS1
	HISTFILE=$HOME/.adm/.$LAYER;export HISTFILE
	# a different history file for each window.

	case "$LAYER" in
		MakeErrors)
			echo 
			if [ -f makeall.out ]
			then
				exec dired makeall.out
			else
				exec dired t3270.out
			fi
			;;
		Mercury)
			exec rlogin mercury
			;;
		#othername)
		# and many other rlogins for machine names...
		#	;;
		Rambo|rambO|rAmbo|raMbo|ramBo)
			TERM=$LAYER; export TERM
			exec rlogin rambo -l gnohmon
			# multiple windows to same machine.
			# The .kshrc on that machine recognizes these
			# $TERM values.
			;;
		Seveso|Rialto|Chioggia|Murano|Lido|Laguna|Zara|Accademia|SanMarco|Mestre)
			SHELL=zsh
			export SHELL
			exec $SHELL
			;;
	esac
	exec ksh	# use the new $HISTFILE
fi
