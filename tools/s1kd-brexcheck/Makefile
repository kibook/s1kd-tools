.PHONY: docs clean maintainer-clean install uninstall

COMMON=../common

SOURCE=s1kd-brexcheck.c $(COMMON)/s1kd_tools.c

ifeq ($(OS),Windows_NT)
  OUTPUT=s1kd-brexcheck.exe
else
  OUTPUT=s1kd-brexcheck
endif

WARNING_FLAGS=-Wall -Werror -pedantic-errors
CFLAGS=$(WARNING_FLAGS) -I $(COMMON) `pkg-config --cflags libxml-2.0 libxslt libexslt`

ifeq ($(DEBUG),1)
  CFLAGS+=-g
else
  CFLAGS+=-O3
endif

LDFLAGS=`pkg-config --libs libxml-2.0 libxslt libexslt`

PREFIX=/usr/local
INSTALL_PREFIX=$(PREFIX)/bin
INSTALL=install -Ds

ifeq ($(BREXCHECK_XPATH_ENGINE),saxon)
  SAXON_C_API=saxon/Saxon.C.API

  SAXON_OBJECTS+=saxon/saxon.o
  SAXON_OBJECTS+=saxon/SaxonCGlue.o
  SAXON_OBJECTS+=saxon/SaxonCXPath.o
  SAXON_OBJECTS+=saxon/SaxonProcessor.o
  SAXON_OBJECTS+=saxon/SchemaValidator.o
  SAXON_OBJECTS+=saxon/XdmAtomicValue.o
  SAXON_OBJECTS+=saxon/XdmItem.o
  SAXON_OBJECTS+=saxon/XdmNode.o
  SAXON_OBJECTS+=saxon/XdmValue.o
  SAXON_OBJECTS+=saxon/XPathProcessor.o
  SAXON_OBJECTS+=saxon/XQueryProcessor.o
  SAXON_OBJECTS+=saxon/Xslt30Processor.o
  SAXON_OBJECTS+=saxon/XsltProcessor.o

  SOURCE+=$(SAXON_OBJECTS)
  CFLAGS+=-DUSE_SAXON
  LDFLAGS+=-lstdc++

  ifneq ($(OSTYPE),msys)
    LDFLAGS+=-ldl
  endif

  # Treat MinGW/Cygwin as Linux when compiling Saxon/C API
  ifeq ($(OSTYPE),msys)
    SAXON_CFLAGS+=-D__linux__
  endif
  ifeq ($(OSTYPE),cygwin)
    SAXON_CFLAGS+=-D__linux__
  endif

  ifeq ($(DEBUG),1)
    SAXON_CFLAGS+=-g
    SAXON_GLUE_CFLAGS+=-g
  else
    SAXON_CFLAGS+=-O3
    SAXON_GLUE_CFLAGS+=-O3
  endif
endif

all: $(OUTPUT)

$(OUTPUT): $(SOURCE) brex.h
	$(CC) $(CFLAGS) -o $(OUTPUT) $(SOURCE) $(LDFLAGS)

saxon/%.o: $(SAXON_C_API)/%.cpp
	$(CXX) $(SAXON_CFLAGS) -c -o $@ $+

saxon/SaxonC%.o: $(SAXON_C_API)/SaxonC%.c
	$(CC) $(SAXON_CFLAGS) -c -o $@ $+

saxon/SaxonCGlue.o: $(SAXON_C_API)/SaxonCGlue.c
	$(CC) $(SAXON_GLUE_CFLAGS) -c -o $@ $+

saxon/saxon.o: saxon/saxon.cpp
	$(CXX) $(SAXON_CFLAGS) -c -I$(SAXON_C_API) -o $@ `pkg-config --cflags libxml-2.0` $+

brex.h: brex/DMC-*.XML stats.xsl
	> $@ && for f in $+; do xxd -i "$$f" >> $@; done

docs:
	$(MAKE) -C doc

clean:
ifeq ($(BREXCHECK_XPATH_ENGINE),saxon)
	rm -f $(SAXON_OBJECTS)
endif
	rm -f $(OUTPUT) brex.h

maintainer-clean: clean
	$(MAKE) -C doc clean

install: $(OUTPUT)
	$(INSTALL) $(OUTPUT) $(INSTALL_PREFIX)
	$(MAKE) -C doc install

uninstall:
	rm -f $(INSTALL_PREFIX)/$(OUTPUT)
	$(MAKE) -C doc uninstall
