SOURCE=s1kd-newdm.c ../common/s1kd_tools.c
OUTPUT=s1kd-newdm

CFLAGS=-Wall -Werror -pedantic-errors -I ../common `pkg-config --cflags libxml-2.0 libxslt`

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

templates.h: templates/42/*.xml ../common/42to*.xsl ../common/brexmap.xml
	> templates.h
	for t in templates/42/*.xml; do xxd -i $$t >> templates.h; done
	xxd -i ../common/42to20.xsl >> templates.h
	xxd -i ../common/42to21.xsl >> templates.h
	xxd -i ../common/42to22.xsl >> templates.h
	xxd -i ../common/42to23.xsl >> templates.h
	xxd -i ../common/42to30.xsl >> templates.h
	xxd -i ../common/42to40.xsl >> templates.h
	xxd -i ../common/42to41.xsl >> templates.h
	xxd -i ../common/brexmap.xml >> templates.h

dmtypes.h: dmtypes.xml dmtypes.txt
	xxd -i dmtypes.xml > dmtypes.h
	xxd -i dmtypes.txt >> dmtypes.h

sns.h: sns/*.XML
	> sns.h
	for s in sns/*.XML; do xxd -i $$s >> sns.h; done

$(OUTPUT): $(SOURCE) templates.h dmtypes.h sns.h
	$(CC) $(CFLAGS) $(SOURCE) -o $(OUTPUT) $(LDFLAGS)

.PHONY: docs clean maintainer-clean install uninstall

docs:
	$(MAKE) -C doc

clean:
	rm -f $(OUTPUT) templates.h dmtypes.h sns.h

maintainer-clean: clean
	$(MAKE) -C doc clean

install: $(OUTPUT)
	$(INSTALL) $(OUTPUT) $(INSTALL_PREFIX)
	$(MAKE) -C doc install

uninstall:
	rm -f $(INSTALL_PREFIX)/$(OUTPUT)
	$(MAKE) -C doc uninstall
