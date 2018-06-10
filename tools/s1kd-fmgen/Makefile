SOURCE=s1kd-fmgen.c
OUTPUT=s1kd-fmgen

CFLAGS=-Wall -Werror -pedantic-errors -g -I ../common `pkg-config --cflags libxml-2.0 libxslt`
LDFLAGS=`pkg-config --libs libxml-2.0 libxslt`

PREFIX=/usr/local
INSTALL_PREFIX=$(PREFIX)/bin

all: $(OUTPUT)
	$(MAKE) -C doc

$(OUTPUT): $(SOURCE) xsl.h
	$(CC) $(CFLAGS) -o $(OUTPUT) $(SOURCE) $(LDFLAGS)

xsl.h: fmtypes.xml xsl/*
	xxd -i fmtypes.xml > xsl.h
	for f in xsl/*; do xxd -i "$$f" >> xsl.h; done

clean:
	rm -f $(OUTPUT) xsl.h
	$(MAKE) -C doc clean

install: $(OUTPUT)
	cp $(OUTPUT) $(INSTALL_PREFIX)/
	$(MAKE) -C doc install

