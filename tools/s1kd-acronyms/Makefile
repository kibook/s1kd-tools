SOURCE=s1kd-acronyms.c ../common/s1kd_tools.c
OUTPUT=s1kd-acronyms

WARNING_FLAGS=-Wall -Werror -pedantic-errors
CFLAGS=$(WARNING_FLAGS) -I ../common `pkg-config --cflags libxml-2.0 libxslt`

ifeq ($(DEBUG),1)
	CFLAGS+=-g
else
	CFLAGS+=-O3
endif

LDFLAGS=`pkg-config --libs libxml-2.0 libxslt`

PREFIX=/usr/local
INSTALL_PREFIX=$(PREFIX)/bin
INSTALL=install -s

all: $(OUTPUT)

stylesheets.h: stylesheets/*.xsl
	xxd -i stylesheets/acronyms.xsl >  stylesheets.h
	xxd -i stylesheets/unique.xsl   >> stylesheets.h
	xxd -i stylesheets/list.xsl     >> stylesheets.h
	xxd -i stylesheets/table.xsl    >> stylesheets.h
	xxd -i stylesheets/types.xsl    >> stylesheets.h
	xxd -i stylesheets/sort.xsl     >> stylesheets.h
	xxd -i stylesheets/term.xsl     >> stylesheets.h
	xxd -i stylesheets/id.xsl       >> stylesheets.h
	xxd -i stylesheets/delete.xsl   >> stylesheets.h
	xxd -i stylesheets/30.xsl       >> stylesheets.h
	xxd -i stylesheets/prefmt.xsl   >> stylesheets.h

$(OUTPUT): $(SOURCE) stylesheets.h
	$(CC) $(CFLAGS) -o $(OUTPUT) $(SOURCE) $(LDFLAGS)

.PHONY: docs clean maintainer-clean install uninstall

docs:
	$(MAKE) -C doc

clean:
	rm -f $(OUTPUT) stylesheets.h

maintainer-clean: clean
	$(MAKE) -C doc clean

install: $(OUTPUT)
	$(INSTALL) $(OUTPUT) $(INSTALL_PREFIX) 
	$(MAKE) -C doc install

uninstall:
	rm -f $(INSTALL_PREFIX)/$(OUTPUT)
	$(MAKE) -C doc uninstall
