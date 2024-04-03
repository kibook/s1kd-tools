.PHONY: docs clean maintainer-clean install uninstall

COMMON=../common

SOURCE=s1kd-appcheck.c $(COMMON)/s1kd_tools.c

ifeq ($(OS),Windows_NT)
  OUTPUT=s1kd-appcheck.exe
else
  OUTPUT=s1kd-appcheck
endif

WARNING_FLAGS=-Wall -Werror -pedantic-errors
CFLAGS=$(WARNING_FLAGS) -I $(COMMON) `pkg-config --cflags libxml-2.0 libxslt libexslt`

ifneq ($(MSYSTEM),)
  CFLAGS+=-isystem $(COMMON)/msys
endif

ifeq ($(DEBUG),1)
  CFLAGS+=-g
else
  CFLAGS+=-O3
endif

LDFLAGS=`pkg-config --libs libxml-2.0 libxslt libexslt`

PREFIX=/usr/local
INSTALL_PREFIX=$(PREFIX)/bin
INSTALL=install -Ds

all: $(OUTPUT)

$(OUTPUT): $(SOURCE) stylesheets.h
	$(CC) $(CFLAGS) -o $(OUTPUT) $(SOURCE) $(LDFLAGS)

stylesheets.h: combos.xsl stats.xsl duplicate.xsl clean-duplicate.xsl
	> $@ && for f in $+; do xxd -i $$f >> $@; done

docs:
	$(MAKE) -C doc

clean:
	rm -f $(OUTPUT) stylesheets.h

maintainer-clean: clean
	$(MAKE) -C doc clean

install: $(OUTPUT)
	$(INSTALL) $(OUTPUT) $(INSTALL_PREFIX)/$(OUTPUT)
	$(MAKE) -C doc install

uninstall:
	rm -f $(INSTALL_PREFIX)/$(OUTPUT)
	$(MAKE) -C doc uninstall
