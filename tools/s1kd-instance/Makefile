SOURCE=s1kd-instance.c
OUTPUT=s1kd-instance

CFLAGS=-Wall -Werror -pedantic-errors -O3 `pkg-config --cflags libxml-2.0 libxslt libexslt`
LDFLAGS=`pkg-config --libs libxml-2.0 libxslt libexslt`

PREFIX=/usr/local
INSTALL_PREFIX=$(PREFIX)/bin
INSTALL=install -s

all: $(OUTPUT)
	$(MAKE) -C doc

strings.h: help.msg
	xxd -i help.msg > strings.h

xsl.h: xsl/*.xsl cirxsl/*.xsl
	> xsl.h
	for f in xsl/*.xsl cirxsl/*.xsl ../common/identity.xsl; do xxd -i "$$f" >> xsl.h; done

$(OUTPUT): $(SOURCE) strings.h xsl.h
	$(CC) $(CFLAGS) $(SOURCE) -o $(OUTPUT) $(LDFLAGS)

.PHONY: clean install uninstall

clean:
	rm -f $(OUTPUT) strings.h xsl.h
	$(MAKE) -C doc clean

install: $(OUTPUT)
	$(INSTALL) $(OUTPUT) $(INSTALL_PREFIX)
	$(MAKE) -C doc install

uninstall:
	rm -f $(INSTALL_PREFIX)/$(OUTPUT)
	$(MAKE) -C doc uninstall
