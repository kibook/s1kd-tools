SOURCE=s1kd-aspp.c
OUTPUT=s1kd-aspp

CFLAGS=-Wall -pedantic-errors -O3 `pkg-config --cflags libxml-2.0 libxslt libexslt`
LDFLAGS=`pkg-config --libs libxml-2.0 libxslt libexslt`

INSTALL_PREFIX=/usr/bin

all: $(OUTPUT)
	$(MAKE) -C doc

$(OUTPUT): $(SOURCE) elements_list.h generateDisplayText.h identity.h
	$(CC) $(CFLAGS) -o $(OUTPUT) $(SOURCE) $(LDFLAGS)

elements_list.h: elements.list
	xxd -i elements.list > elements_list.h

generateDisplayText.h: generateDisplayText.xsl
	xxd -i generateDisplayText.xsl > generateDisplayText.h

identity.h: ../common/identity.xsl
	xxd -i ../common/identity.xsl > identity.h

install: $(OUTPUT)
	cp $(OUTPUT) $(INSTALL_PREFIX)/
	$(MAKE) -C doc install

clean:
	rm -f $(OUTPUT)
	rm -f elements_list.h
	rm -f generateDisplayText.h
	rm -f identity.h
	$(MAKE) -C doc clean
