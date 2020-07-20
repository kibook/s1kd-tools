COMMON=../common

SOURCE=s1kd-repcheck.c $(COMMON)/s1kd_tools.c
OUTPUT=s1kd-repcheck

WARNING_FLAGS=-Wall -Werror -pedantic-errors
CFLAGS=$(WARNING_FLAGS) -I $(COMMON) `pkg-config --cflags libxml-2.0 libxslt`

ifeq ($(DEBUG),1)
	CFLAGS+=-g
else
	CFLAGS+=-O3
endif

LDFLAGS=`pkg-config --libs libxml-2.0 libxslt`

PREFIX=/usr/local
INSTALL_PREFIX=$(PREFIX)/bin
INSTALL=install -Ds

all: $(OUTPUT)

$(OUTPUT): $(SOURCE) xsl.h
	$(CC) $(CFLAGS) -o $(OUTPUT) $(SOURCE) $(LDFLAGS)

xsl.h: xsl/cirrefs.xsl xsl/cirrefsall.xsl xsl/stats.xsl
	>xsl.h
	for f in $+; do xxd -i "$$f" >>xsl.h; done

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
