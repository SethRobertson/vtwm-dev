${HOME}/.vtwmrc: vtwmrc.new
	mv ${HOME}/.vtwmrc vtwmrc.bak
	cp vtwmrc.new ${HOME}/.vtwmrc

vtwmrc.new: vtwmrc.proto bg.h fg.h makewin.h
	sed \
		-e '/#include "bg.h"/r bg.h' \
		-e '/#include "fg.h"/r fg.h' \
		-e '/#include "makewin.h"/r makewin.h' \
		< vtwmrc.proto > vtwmrc.new

makewin.h: makewin.proto \
		color.bg \
		color.fg \
		Rxterm.list
# "sort -f" ignores capitalization
	sort -f makewin.proto | while read name; \
		do entry_make $$name "makewin"; \
		done > makewin.h

fg.h:	color.fg
		awk '$$1 != "#" { printf "\t\"%s\"\t\"%s\"\n",$$1,$$2}' \
		color.fg \
		| sed -e 's/%/ /g' \
		> fg.h

bg.h:	color.bg
		awk '$$1 != "#" { printf "\t\"%s\"\t\"%s\"\n",$$1,$$2}' \
		color.bg \
		| sed -e 's/%/ /g' \
		> bg.h
