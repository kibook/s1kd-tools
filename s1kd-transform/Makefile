SOURCE=s1kd-transform.c
OUTPUT=s1kd-transform

CFLAGS=-Wall -Werror -pedantic-errors -O3 `pkg-config --cflags libxml-2.0 libxslt libexslt`
LDFLAGS=`pkg-config --libs libxml-2.0 libxslt libexslt`

INSTALL_PREFIX=/usr/local/bin

all: $(OUTPUT)
	$(MAKE) -C doc

identity.h: ../common/identity.xsl
	xxd -i ../common/identity.xsl > identity.h

$(OUTPUT): $(SOURCE) identity.h
	$(CC) $(CFLAGS) -o $(OUTPUT) $(SOURCE) $(LDFLAGS)

clean:
	rm -f identity.h
	rm -f $(OUTPUT)
	$(MAKE) -C doc clean

install: $(OUTPUT)
	cp $(OUTPUT) $(INSTALL_PREFIX)/
	$(MAKE) -C doc install

