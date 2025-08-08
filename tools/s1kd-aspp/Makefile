.PHONY: docs clean maintainer-clean install uninstall

common_dir = ../common

sources = s1kd-aspp.c $(common_dir)/s1kd_tools.c

ifeq ($(OS),Windows_NT)
  executable = s1kd-aspp.exe
else
  executable = s1kd-aspp
endif

warning_flags = -Wall -Werror -pedantic-errors

CFLAGS = $(warning_flags) -I $(common_dir) `pkg-config --cflags libxml-2.0 libxslt libexslt`

ifneq ($(MSYSTEM),)
ifneq ($(MSYSTEM),MSYS)
  CFLAGS += -isystem $(common_dir)/msys
endif
endif

ifeq ($(DEBUG),1)
  CFLAGS += -g
else
  CFLAGS += -O3
endif

LDFLAGS = `pkg-config --libs libxml-2.0 libxslt libexslt`

prefix = /usr/local
exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin

INSTALL = install -Ds

all: $(executable)

$(executable): $(sources) resources.h
	$(CC) $(CFLAGS) -o $(executable) $(sources) $(LDFLAGS)

resources.h: elements.list disptext.xsl disptext.xml addTags.xsl
	> $@ && for f in $+; do xxd -i $$f >> $@; done

docs:
	$(MAKE) -C doc

clean:
	$(RM) $(executable) resources.h

maintainer-clean: clean
	$(MAKE) -C doc clean

install: $(executable)
	$(INSTALL) $(executable) $(bindir)/$(executable)
	$(MAKE) -C doc install

uninstall:
	$(RM) $(bindir)/$(executable)
	$(MAKE) -C doc uninstall
