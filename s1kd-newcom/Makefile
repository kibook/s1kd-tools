SOURCE=s1kd-newcom.c
OUTPUT=s1kd-newcom

CFLAGS=-Wall -Werror -pedantic-errors -O3 `pkg-config --cflags libxml-2.0`
LDFLAGS=`pkg-config --libs libxml-2.0`

INSTALL_PREFIX=/usr/bin

all: $(OUTPUT)
	$(MAKE) -C doc

template.h: comment.xml
	xxd -i comment.xml > template.h

$(OUTPUT): $(SOURCE) template.h
	$(CC) $(CFLAGS) -o $(OUTPUT) $(SOURCE) $(LDFLAGS)

clean:
	rm -f $(OUTPUT)
	rm -f template.h
	$(MAKE) -C doc clean

install: $(OUTPUT)
	cp $(OUTPUT) $(INSTALL_PREFIX)/
	$(MAKE) -C doc install

