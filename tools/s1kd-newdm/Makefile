.PHONY: docs clean maintainer-clean install uninstall

COMMON=../common

SOURCE=s1kd-newdm.c $(COMMON)/s1kd_tools.c

ifeq ($(OS),Windows_NT)
  OUTPUT=s1kd-newdm.exe
else
  OUTPUT=s1kd-newdm
endif

WARNING_FLAGS=-Wall -Werror -pedantic-errors
CFLAGS=$(WARNING_FLAGS) -I $(COMMON) `pkg-config --cflags libxml-2.0 libxslt`

ifeq ($(MSYSTEM),MINGW64)
  CFLAGS+=-isystem $(COMMON)/mingw64
endif

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

templates.h: templates/*.xml $(COMMON)/to*.xsl $(COMMON)/brexmap.xml
	> $@ && for f in $+; do xxd -i $$f >> $@; done

dmtypes.h: dmtypes.xml dmtypes.txt
	> $@ && for f in $+; do xxd -i $$f >> $@; done

sns.h: sns/*.XML
	> $@ && for f in $+; do xxd -i $$f >> $@; done

$(OUTPUT): $(SOURCE) templates.h dmtypes.h sns.h
	$(CC) $(CFLAGS) $(SOURCE) -o $(OUTPUT) $(LDFLAGS)

docs:
	$(MAKE) -C doc

clean:
	rm -f $(OUTPUT) templates.h dmtypes.h sns.h

maintainer-clean: clean
	$(MAKE) -C doc clean

install: $(OUTPUT)
	$(INSTALL) $(OUTPUT) $(INSTALL_PREFIX)/$(OUTPUT)
	$(MAKE) -C doc install

uninstall:
	rm -f $(INSTALL_PREFIX)/$(OUTPUT)
	$(MAKE) -C doc uninstall
