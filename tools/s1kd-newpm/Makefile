CFLAGS=-Wall -Werror -pedantic-errors -O3 -I ../common `pkg-config --cflags libxml-2.0 libxslt`
LDFLAGS=`pkg-config --libs libxml-2.0 libxslt`

SOURCE=s1kd-newpm.c
OUTPUT=s1kd-newpm

PREFIX=/usr/local
INSTALL_PREFIX=$(PREFIX)/bin
INSTALL=install -s

all: $(OUTPUT)
	$(MAKE) -C doc

template.h: pm.xml ../common/42to*.xsl
	xxd -i pm.xml > template.h
	xxd -i ../common/42to20.xsl >> template.h
	xxd -i ../common/42to21.xsl >> template.h
	xxd -i ../common/42to22.xsl >> template.h
	xxd -i ../common/42to23.xsl >> template.h
	xxd -i ../common/42to30.xsl >> template.h
	xxd -i ../common/42to40.xsl >> template.h
	xxd -i ../common/42to41.xsl >> template.h

$(OUTPUT): $(SOURCE) template.h
	$(CC) $(CFLAGS) $(SOURCE) -o $(OUTPUT) $(LDFLAGS)

.PHONY: clean install uninstall

clean:
	rm -f $(OUTPUT) template.h
	$(MAKE) -C doc clean

install: $(OUTPUT)
	$(INSTALL) $(OUTPUT) $(INSTALL_PREFIX)
	$(MAKE) -C doc install

uninstall:
	rm -f $(INSTALL_PREFIX)/$(OUTPUT)
	$(MAKE) -C doc uninstall
