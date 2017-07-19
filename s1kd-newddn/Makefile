SOURCE=s1kd-newddn.c
OUTPUT=s1kd-newddn

CFLAGS=-Wall -Werror -pedantic-errors -O3 `pkg-config --cflags libxml-2.0`
LDFLAGS=`pkg-config --libs libxml-2.0`

INSTALL_PREFIX=/usr/bin

all: $(OUTPUT)
	$(MAKE) -C doc

templates.h:
	xxd -i templates/ddn.xml > templates.h

$(OUTPUT): $(SOURCE) templates.h
	$(CC) $(CFLAGS) -o $(OUTPUT) $(SOURCE) $(LDFLAGS)

clean:
	rm -f $(OUTPUT)
	$(MAKE) -C doc clean

install:
	cp $(OUTPUT) $(INSTALL_PREFIX)/
	$(MAKE) -C doc install
