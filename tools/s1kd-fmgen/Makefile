SOURCE=s1kd-fmgen.c
OUTPUT=s1kd-fmgen

CFLAGS=-Wall -Werror -pedantic-errors -g -I ../common `pkg-config --cflags libxml-2.0 libxslt`
LDFLAGS=`pkg-config --libs libxml-2.0 libxslt`

PREFIX=/usr/local
INSTALL_PREFIX=$(PREFIX)/bin
INSTALL=install -s

all: $(OUTPUT)

$(OUTPUT): $(SOURCE) xsl.h
	$(CC) $(CFLAGS) -o $(OUTPUT) $(SOURCE) $(LDFLAGS)

xsl.h: fmtypes.xml fmtypes.txt xsl/*
	xxd -i fmtypes.xml > xsl.h
	xxd -i fmtypes.txt >> xsl.h
	for f in xsl/*; do xxd -i "$$f" >> xsl.h; done

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
