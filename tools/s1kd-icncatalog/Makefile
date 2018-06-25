SOURCE=s1kd-icncatalog.c
OUTPUT=s1kd-icncatalog

CFLAGS=-Wall -Werror -pedantic-errors -O3 `pkg-config --cflags libxml-2.0`
LDFLAGS=`pkg-config --libs libxml-2.0`

PREFIX=/usr/local
INSTALL_PREFIX=$(PREFIX)/bin
INSTALL=install -s

all: $(OUTPUT)
	$(MAKE) -C doc

$(OUTPUT): $(SOURCE) templates.h
	$(CC) $(CFLAGS) -o $(OUTPUT) $(SOURCE) $(LDFLAGS)

templates.h: icncatalog.xml
	xxd -i icncatalog.xml > templates.h

.PHONY: clean install uninstall

clean:
	rm -f $(OUTPUT) templates.h
	$(MAKE) -C doc clean

install: $(OUTPUT)
	$(INSTALL) $(OUTPUT) $(INSTALL_PREFIX)
	$(MAKE) -C doc install

uninstall:
	rm -f $(INSTALL_PREFIX)/$(OUTPUT)
	$(MAKE) -C doc uninstall
