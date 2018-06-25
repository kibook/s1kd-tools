SOURCE=s1kd-neutralize.c
OUTPUT=s1kd-neutralize

CFLAGS=-Wall -Werror -pedantic-errors -O3 `pkg-config --cflags libxml-2.0 libxslt`
LDFLAGS=`pkg-config --libs libxml-2.0 libxslt`

PREFIX=/usr/local
INSTALL_PREFIX=$(PREFIX)/bin
INSTALL=install -s

all: $(OUTPUT)
	$(MAKE) -C doc

stylesheets.h: stylesheets/*.xsl
	xxd -i stylesheets/rdf.xsl > stylesheets.h
	xxd -i stylesheets/xlink.xsl >> stylesheets.h

$(OUTPUT): $(SOURCE) stylesheets.h
	$(CC) $(CFLAGS) -o $(OUTPUT) $(SOURCE) $(LDFLAGS)

.PHONY: clean install uninstall

clean:
	rm -f stylesheets.h
	rm -f $(OUTPUT)
	$(MAKE) -C doc clean

install: $(OUTPUT)
	$(INSTALL) $(OUTPUT) $(INSTALL_PREFIX)
	$(MAKE) -C doc install

uninstall:
	rm -f $(INSTALL_PREFIX)/$(OUTPUT)
	$(MAKE) -C doc uninstall
