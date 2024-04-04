source = DMC-S1KDTOOLS-A-23-00-00-00A-040A-D_EN-CA.XML
manpage = s1kd-flatten.1
readme = ../README.md

prefix = /usr/local
datarootdir = $(prefix)/share
mandir = $(datarootdir)/man
man1dir = $(mandir)/man1

INSTALL = install -D

all: $(manpage) $(readme)

$(readme): $(source)
	s1kd2db $^ | pandoc -f docbook -t gfm -s -o $@

$(manpage): $(source)
	s1kd2db $^ | pandoc -f docbook -t man -s -o $@

.PHONY: clean install uninstall

clean:
	$(RM) $(manpage) $(readme)

install: $(manpage)
	$(INSTALL) $(manpage) $(man1dir)/$(manpage)

uninstall:
	$(RM) $(man1dir)/$(manpage)
