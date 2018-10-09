SOURCE=s1kd-newupf.c ../common/s1kd_tools.c
OUTPUT=s1kd-newupf

CFLAGS=-Wall -Werror -pedantic-errors -I ../common `pkg-config --cflags libxml-2.0 libxslt`

ifeq ($(DEBUG),1)
	CFLAGS+=-g
else
	CFLAGS+=-O3
endif

LDFLAGS=`pkg-config --libs libxml-2.0 libxslt`

PREFIX=/usr/local
INSTALL_PREFIX=$(PREFIX)/bin
INSTALL=install -s

all: $(OUTPUT)

templates.h: update.xml ../common/42to41.xsl
	xxd -i update.xml > templates.h
	xxd -i ../common/42to41.xsl >> templates.h

$(OUTPUT): $(SOURCE) templates.h
	$(CC) $(CFLAGS) -o $(OUTPUT) $(SOURCE) $(LDFLAGS)

.PHONY: docs clean maintainer-clean install uninstall

docs:
	$(MAKE) -C doc

clean:
	rm -f $(OUTPUT) templates.h

maintainer-clean: clean
	$(MAKE) -C doc clean

install: $(OUTPUT)
	cp $(OUTPUT) $(INSTALL_PREFIX)/
	$(MAKE) -C doc install

uninstall:
	rm -f $(INSTALL_PREFIX)/$(OUTPUT)
	$(MAKE) -C doc uninstall
