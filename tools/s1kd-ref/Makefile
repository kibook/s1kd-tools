CFLAGS=-Wall -Werror -pedantic-errors -O3 `pkg-config --cflags libxml-2.0 libxslt`
LDFLAGS=`pkg-config --libs libxml-2.0 libxslt`

SOURCE=s1kd-ref.c
OUTPUT=s1kd-ref

PREFIX=/usr/local
INSTALL_PREFIX=$(PREFIX)/bin
INSTALL=install -s

all: $(OUTPUT)

$(OUTPUT): $(SOURCE) xslt.h
	$(CC) $(CFLAGS) $(SOURCE) -o $(OUTPUT) $(LDFLAGS)

xslt.h: ../common/42to*.xsl
	>xslt.h
	for f in ../common/42to*.xsl; do xxd -i "$$f" >> xslt.h; done

.PHONY: docs clean maintainer-clean install uninstall

docs:
	$(MAKE) -C doc

clean:
	rm -f $(OUTPUT) xslt.h

maintainer-clean: clean
	$(MAKE) -C doc clean

install: $(OUTPUT)
	$(INSTALL) $(OUTPUT) $(INSTALL_PREFIX)
	$(MAKE) -C doc install

uninstall:
	rm -f $(INSTALL_PREFIX)/$(OUTPUT)
	$(MAKE) -C doc uninstall
