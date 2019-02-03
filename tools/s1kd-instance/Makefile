SOURCE=s1kd-instance.c ../common/s1kd_tools.c
OUTPUT=s1kd-instance

CFLAGS=-Wall -Werror -pedantic-errors -I ../common `pkg-config --cflags libxml-2.0 libxslt libexslt`

ifeq ($(DEBUG),1)
	CFLAGS+=-g
else
	CFLAGS+=-O3
endif

LDFLAGS=`pkg-config --libs libxml-2.0 libxslt libexslt`

PREFIX=/usr/local
INSTALL_PREFIX=$(PREFIX)/bin
INSTALL=install -s

all: $(OUTPUT)

xsl.h: xsl/*.xsl cirxsl/*.xsl
	> xsl.h
	for f in xsl/*.xsl cirxsl/*.xsl ../common/identity.xsl; do xxd -i "$$f" >> xsl.h; done

$(OUTPUT): $(SOURCE) xsl.h
	$(CC) $(CFLAGS) $(SOURCE) -o $(OUTPUT) $(LDFLAGS)

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
