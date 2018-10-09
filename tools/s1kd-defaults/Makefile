SOURCE=s1kd-defaults.c ../common/s1kd_tools.c
OUTPUT=s1kd-defaults

CFLAGS=-Wall -pedantic-errors -I ../common `pkg-config --cflags libxml-2.0 libxslt`

ifeq ($(DEBUG),1)
	CFLAGS+=-g
else
	CFLAGS+=-O3
endif

LDFLAGS=`pkg-config --libs libxml-2.0 libxslt libexslt`

PREFIX=/usr/local
INSTALL_PREFIX=$(PREFIX)/bin
INSTALL=install -s

all: $(OUTPUT)

$(OUTPUT): $(SOURCE) xsl.h defaults.h
	$(CC) $(CFLAGS) -o $(OUTPUT) $(SOURCE) $(LDFLAGS)

defaults.h: defaults.xml
	xxd -i defaults.xml > defaults.h

xsl.h: xsl/* ../common/brexmap.xml
	>xsl.h
	for f in xsl/*; do xxd -i "$$f" >> xsl.h; done
	xxd -i ../common/brexmap.xml >> xsl.h

.PHONY: docs clean maintainer-clean install uninstall

docs:
	$(MAKE) -C doc

clean:
	rm -f $(OUTPUT) xsl.h

maintainer-clean: clean
	$(MAKE) -C doc clean

install: $(OUTPUT)
	$(INSTALL) $(OUTPUT) $(INSTALL_PREFIX)
	$(MAKE) -C doc install

uninstall:
	rm -f $(INSTALL_PREFIX)/$(OUTPUT)
	$(MAKE) -C doc uninstall
