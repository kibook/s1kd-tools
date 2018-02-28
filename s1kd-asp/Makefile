SOURCE=s1kd-asp.c
OUTPUT=s1kd-asp

CFLAGS=-Wall -pedantic-errors -g `pkg-config --cflags libxml-2.0`
LDFLAGS=`pkg-config --libs libxml-2.0`

INSTALL_PREFIX=/usr/bin

all: $(OUTPUT)
	$(MAKE) -C doc

$(OUTPUT): $(SOURCE) elements_list.h
	$(CC) $(CFLAGS) -o $(OUTPUT) $(SOURCE) $(LDFLAGS)

elements_list.h: elements.list
	xxd -i elements.list > elements_list.h

install: $(OUTPUT)
	cp $(OUTPUT) $(INSTALL_PREFIX)/
	$(MAKE) -C doc install

clean:
	rm -f $(OUTPUT)
	rm -f elements_list.h
	$(MAKE) -C doc clean
