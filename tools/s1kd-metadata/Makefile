.PHONY: docs clean maintainer-clean install uninstall

COMMON=../common

SOURCE=s1kd-metadata.c $(COMMON)/s1kd_tools.c

ifeq ($(OS),Windows_NT)
  OUTPUT=s1kd-metadata.exe
else
  OUTPUT=s1kd-metadata
endif

WARNING_FLAGS=-Wall -Werror -pedantic-errors
CFLAGS=$(WARNING_FLAGS) -I $(COMMON) `pkg-config --cflags libxml-2.0`

ifeq ($(DEBUG),1)
  CFLAGS+=-g
else
  CFLAGS+=-O3
endif

LDFLAGS=`pkg-config --libs libxml-2.0`

ifeq ($(OSTYPE),msys)
  LDFLAGS+=-lregex2
endif

PREFIX=/usr/local
INSTALL_PREFIX=$(PREFIX)/bin
INSTALL=install -Ds

all: $(OUTPUT)

$(OUTPUT): $(SOURCE)
	$(CC) $(CFLAGS) -o $(OUTPUT) $(SOURCE) $(LDFLAGS)

docs:
	$(MAKE) -C doc

clean:
	rm -f $(OUTPUT)

maintainer-clean: clean
	$(MAKE) -C doc clean

install:
	$(INSTALL) $(OUTPUT) $(INSTALL_PREFIX)/$(OUTPUT)
	$(MAKE) -C doc install

uninstall:
	rm -f $(INSTALL_PREFIX)/$(OUTPUT)
	$(MAKE) -C doc uninstall
