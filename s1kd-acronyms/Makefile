SOURCE=s1kd-acronyms.c
OUTPUT=s1kd-acronyms

CFLAGS=-Wall -Werror -pedantic-errors -g `pkg-config --cflags libxml-2.0 libxslt`
LDFLAGS=`pkg-config --libs libxml-2.0 libxslt`

INSTALL_PREFIX=/usr/bin

all: $(OUTPUT)
	$(MAKE) -C doc

stylesheets.h: stylesheets/*.xsl
	xxd -i stylesheets/acronyms.xsl >  stylesheets.h
	xxd -i stylesheets/unique.xsl   >> stylesheets.h
	xxd -i stylesheets/list.xsl     >> stylesheets.h
	xxd -i stylesheets/table.xsl    >> stylesheets.h
	xxd -i stylesheets/types.xsl    >> stylesheets.h
	xxd -i stylesheets/sort.xsl     >> stylesheets.h

$(OUTPUT): $(SOURCE) stylesheets.h
	$(CC) $(CFLAGS) -o $(OUTPUT) $(SOURCE) $(LDFLAGS)

clean:
	rm -f $(OUTPUT) stylesheets.h
	$(MAKE) -C doc clean

install: $(OUTPUT)
	cp $(OUTPUT) $(INSTALL_PREFIX)/
	$(MAKE) -C doc install

