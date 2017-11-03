CFLAGS=-Wall -Werror -pedantic-errors -O3 `pkg-config --cflags libxml-2.0`
LDFLAGS=`pkg-config --libs libxml-2.0`

SOURCE=s1kd-newdm.c
OUTPUT=s1kd-newdm

all: $(OUTPUT)
	$(MAKE) -C doc

templates.h: templates/*/*.xml
	> templates.h
	for t in templates/*/*.xml; do xxd -i $$t >> templates.h; done

$(OUTPUT): $(SOURCE) templates.h
	$(CC) $(CFLAGS) $(SOURCE) -o $(OUTPUT) $(LDFLAGS)

clean:
	rm -f $(OUTPUT) templates.h
	$(MAKE) -C doc clean

install: $(OUTPUT)
	cp $(OUTPUT) /usr/bin/
	$(MAKE) -C doc install
