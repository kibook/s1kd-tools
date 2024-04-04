.PHONY: docs clean maintainer-clean install uninstall

common_dir = ../common

sources = s1kd-brexcheck.c $(common_dir)/s1kd_tools.c

ifeq ($(OS),Windows_NT)
  executable = s1kd-brexcheck.exe
else
  executable = s1kd-brexcheck
endif

warning_flags = -Wall -Werror -pedantic-errors

CFLAGS = $(warning_flags) -I $(common_dir) `pkg-config --cflags libxml-2.0 libxslt libexslt`

ifneq ($(MSYSTEM),)
  CFLAGS += -isystem $(common_dir)/msys
endif

ifeq ($(debug),1)
  CFLAGS += -g -DDEBUG
else
  CFLAGS += -O3
endif

LDFLAGS = `pkg-config --libs libxml-2.0 libxslt libexslt`

prefix = /usr/local
exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin

INSTALL = install -Ds

xpath2_engine = none

ifeq ($(xpath2_engine),saxon)
  ifeq ($(OS),Windows_NT)
    saxon_c_dir = /cygdrive/c/Progra~1/Saxonica/SaxonHEC1.2.1
  else
    saxon_c_dir = /usr/lib
  endif

  saxon_c_api = saxon/Saxon.C.API

  saxon_objects += saxon/saxon.o
  saxon_objects += saxon/SaxonCGlue.o
  saxon_objects += saxon/SaxonCXPath.o
  saxon_objects += saxon/SaxonProcessor.o
  saxon_objects += saxon/SchemaValidator.o
  saxon_objects += saxon/XdmAtomicValue.o
  saxon_objects += saxon/XdmItem.o
  saxon_objects += saxon/XdmNode.o
  saxon_objects += saxon/XdmValue.o
  saxon_objects += saxon/XPathProcessor.o
  saxon_objects += saxon/XQueryProcessor.o
  saxon_objects += saxon/Xslt30Processor.o
  saxon_objects += saxon/XsltProcessor.o

  sources += $(saxon_objects)
  LDFLAGS += -lstdc++ -L$(saxon_c_dir) -lsaxonhec

  saxon_cflags += `pkg-config --cflags libxml-2.0`

  ifneq ($(OSTYPE),msys)
    LDFLAGS += -ldl
  endif

  # Treat MinGW/Cygwin as Linux when compiling Saxon/C API
  ifeq ($(OSTYPE),msys)
    saxon_cflags += -D__linux__
  endif
  ifeq ($(OSTYPE),cygwin)
    saxon_cflags += -D__linux__
  endif

  ifeq ($(debug),1)
    saxon_cflags += -g -DDEBUG
  else
    saxon_cflags += -O3
  endif
endif

ifeq ($(xpath2_engine),xqilla)
  xqilla_objects += xqilla/xqilla.o

  sources += $(xqilla_objects)
  LDFLAGS += -lstdc++ `pkg-config --libs xerces-c` -lxqilla

  xqilla_cflags += `pkg-config --cflags libxml-2.0 xerces-c`

  ifeq ($(debug),1)
    xqilla_cflags += -g -DDEBUG
  else
    xqilla_cflags += -O3
  endif
endif

CFLAGS += -DXPATH2_ENGINE=$(xpath2_engine)

all: $(executable)

$(executable): $(sources) brex.h
	$(CC) $(CFLAGS) -o $(executable) $(sources) $(LDFLAGS)

saxon/%.o: $(saxon_c_api)/%.cpp
	$(CXX) $(saxon_cflags) -c -o $@ $+

saxon/SaxonC%.o: $(saxon_c_api)/SaxonC%.c
	$(CC) $(saxon_cflags) -c -o $@ $+

saxon/saxon.o: saxon/saxon.cpp
	$(CXX) $(saxon_cflags) -c -I$(saxon_c_api) -o $@ $+

xqilla/xqilla.o: xqilla/xqilla.cpp
	$(CXX) -c $(xqilla_cflags) -o $@ $+

brex.h: brex/DMC-*.XML stats.xsl
	> $@ && for f in $+; do xxd -i "$$f" >> $@; done

docs:
	$(MAKE) -C doc

clean:
	$(RM) $(executable) brex.h $(saxon_objects) $(xqilla_objects)

maintainer-clean: clean
	$(MAKE) -C doc clean

install: $(executable)
	$(INSTALL) $(executable) $(bindir)/$(executable)
	$(MAKE) -C doc install

uninstall:
	$(RM) $(bindir)/$(executable)
	$(MAKE) -C doc uninstall
