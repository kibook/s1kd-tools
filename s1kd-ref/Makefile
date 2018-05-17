CFLAGS=-Wall -Werror -pedantic-errors -O3 `pkg-config --cflags libxml-2.0 libxslt`
LDFLAGS=`pkg-config --libs libxml-2.0 libxslt`

SOURCE=s1kd-ref.c
OUTPUT=s1kd-ref

PREFIX=/usr/local
INSTALL_PREFIX=$(PREFIX)/bin

all: $(OUTPUT)
	$(MAKE) -C doc

$(OUTPUT): $(SOURCE) xslt.h
	$(CC) $(CFLAGS) $(SOURCE) -o $(OUTPUT) $(LDFLAGS)

xslt.h: ../common/42to*.xsl
	>xslt.h
	for f in ../common/42to*.xsl; do xxd -i "$$f" >> xslt.h; done

install: $(OUTPUT)
	cp $(OUTPUT) $(INSTALL_PREFIX)/
	$(MAKE) -C doc install

clean:
	rm -f $(OUTPUT) xslt.h
	$(MAKE) -C doc clean
