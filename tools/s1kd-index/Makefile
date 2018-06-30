SOURCE=s1kd-index.c
OUTPUT=s1kd-index

CFLAGS=-Wall -pedantic-errors -O3 -I ../common `pkg-config --cflags libxml-2.0 libxslt`
LDFLAGS=`pkg-config --libs libxml-2.0 libxslt`

PREFIX=/usr/local
INSTALL_PREFIX=$(PREFIX)/bin
INSTALL=install -s

all: $(OUTPUT)
	$(MAKE) -C doc

$(OUTPUT): $(SOURCE) xslt.h
	$(CC) $(CFLAGS) -o $(OUTPUT) $(SOURCE) $(LDFLAGS)

xslt.h: iss30.xsl delete.xsl
	xxd -i iss30.xsl > xslt.h
	xxd -i delete.xsl >> xslt.h

.PHONY: clean install uninstall

clean:
	rm -f $(OUTPUT) xslt.h
	$(MAKE) -C doc clean

install: $(OUTPUT)
	$(INSTALL) $(OUTPUT) $(INSTALL_PREFIX)
	$(MAKE) -C doc install

uninstall:
	rm -f $(INSTALL_PREFIX)/$(OUTPUT)
	$(MAKE) -C doc uninstall
