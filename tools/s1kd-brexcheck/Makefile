SOURCE=s1kd-brexcheck.c
OUTPUT=s1kd-brexcheck

CFLAGS=-Wall -Werror -pedantic-errors -O3 `pkg-config --cflags libxml-2.0`
LDFLAGS=`pkg-config --libs libxml-2.0`

PREFIX=/usr/local
INSTALL_PREFIX=$(PREFIX)/bin

all: $(OUTPUT)
	$(MAKE) -C doc

$(OUTPUT): $(SOURCE) brex.h
	$(CC) $(CFLAGS) -o $(OUTPUT) $(SOURCE) $(LDFLAGS)

brex.h: brex/DMC-*.XML
	> brex.h
	for f in brex/DMC-*.XML; do xxd -i "$$f" >> brex.h; done

clean:
	rm -f $(OUTPUT) brex.h
	$(MAKE) -C doc clean

install: $(OUTPUT)
	cp $(OUTPUT) $(INSTALL_PREFIX)/
	$(MAKE) -C doc install

