.PHONY: docs clean maintainer-clean install uninstall

common_dir = ../common

sources = s1kd-instance.c $(common_dir)/s1kd_tools.c

ifeq ($(OS),Windows_NT)
  executable = s1kd-instance.exe
else
  executable = s1kd-instance
endif

warning_flags = -Wall -Werror -pedantic-errors

CFLAGS = $(warning_flags) -I $(common_dir) `pkg-config --cflags libxml-2.0 libxslt libexslt`

ifneq ($(MSYSTEM),)
  CFLAGS += -isystem $(common_dir)/msys
endif

ifeq ($(debug),1)
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

xsl.h: $(common_dir)/remove-empty-pmentries.xsl xsl/*.xsl cirxsl/*.xsl
	> $@ && for f in $+; do xxd -i "$$f" >> $@; done

$(executable): $(sources) xsl.h
	$(CC) $(CFLAGS) -o $(executable) $(sources) $(LDFLAGS)

docs:
	$(MAKE) -C doc

clean:
	$(RM) $(executable) xsl.h

maintainer-clean: clean
	$(MAKE) -C doc clean

install: $(executable)
	$(INSTALL) $(executable) $(bindir)/$(executable)
	$(MAKE) -C doc install

uninstall:
	$(RM) $(bindir)/$(executable)
	$(MAKE) -C doc uninstall
