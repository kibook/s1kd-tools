.PHONY: docs clean maintainer-clean install uninstall

common_dir = ../common

sources = s1kd-dmrl.c $(common_dir)/s1kd_tools.c

ifeq ($(OS),Windows_NT)
  executable = s1kd-dmrl.exe
else
  executable = s1kd-dmrl
endif

warning_flags = -Wall -Werror -pedantic-errors

CFLAGS = $(warning_flags) -I $(common_dir) `pkg-config --cflags libxml-2.0 libxslt`

ifneq ($(MSYSTEM),)
ifneq ($(MSYSTEM),MSYS)
  CFLAGS += -isystem $(common_dir)/msys
endif
endif

ifeq ($(debug),1)
  CFLAGS += -g
else
  CFLAGS += -O3
endif

LDFLAGS = `pkg-config --libs libxml-2.0 libxslt`

prefix = /usr/local
exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin

INSTALL = install -Ds

all: $(executable)

$(executable): $(sources) dmrl.h
	$(CC) $(CFLAGS) -o $(executable) $(sources) $(LDFLAGS)

dmrl.h: dmrl.xsl
	xxd -i $+ > $@

docs:
	$(MAKE) -C doc

clean:
	$(RM) $(executable) dmrl.h

maintainer-clean: clean
	$(MAKE) -C doc clean

install: $(executable)
	$(INSTALL) $(executable) $(bindir)/$(executable)
	$(MAKE) -C doc install

uninstall:
	$(RM) $(bindir)/$(executable)
	$(MAKE) -C doc uninstall
