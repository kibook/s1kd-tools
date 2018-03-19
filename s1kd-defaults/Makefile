SOURCE=s1kd-defaults.c
OUTPUT=s1kd-defaults

CFLAGS=-Wall -pedantic-errors -g `pkg-config --cflags libxml-2.0 libxslt libexslt`
LDFLAGS=`pkg-config --libs libxml-2.0 libxslt libexslt`

INSTALL_PREFIX=/usr/bin

all: $(OUTPUT)
	$(MAKE) -C doc

$(OUTPUT): $(SOURCE) xsl.h defaults.h
	$(CC) $(CFLAGS) -o $(OUTPUT) $(SOURCE) $(LDFLAGS)

defaults.h: defaults.xml
	xxd -i defaults.xml > defaults.h

xsl.h: xsl/*
	>xsl.h
	for f in xsl/*; do xxd -i "$$f" >> xsl.h; done

install: $(OUTPUT)
	cp $(OUTPUT) $(INSTALL_PREFIX)/
	$(MAKE) -C doc install

clean:
	rm -f $(OUTPUT) xsl.h
	$(MAKE) -C doc clean
