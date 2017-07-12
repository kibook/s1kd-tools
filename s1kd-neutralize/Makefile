SOURCE=s1kd-neutralize.c
OUTPUT=s1kd-neutralize

CFLAGS=-Wall -Werror -pedantic-errors -g `pkg-config --cflags libxml-2.0 libxslt`
LDFLAGS=`pkg-config --libs libxml-2.0 libxslt`

INSTALL_PREFIX=/usr/bin

all: $(OUTPUT)
	$(MAKE) -C doc

stylesheets.h: stylesheets/*.xsl
	xxd -i stylesheets/rdf.xsl > stylesheets.h
	xxd -i stylesheets/syncrefs.xsl >> stylesheets.h
	xxd -i stylesheets/xlink.xsl >> stylesheets.h

$(OUTPUT): $(SOURCE) stylesheets.h
	$(CC) $(CFLAGS) -o $(OUTPUT) $(SOURCE) $(LDFLAGS)

clean:
	rm -f stylesheets.h
	rm -f $(OUTPUT)
	$(MAKE) -C doc clean

install: $(OUTPUT)
	cp $(OUTPUT) $(INSTALL_PREFIX)/
	$(MAKE) -C doc install
