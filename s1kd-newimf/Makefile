CFLAGS=-Wall -Werror -pedantic-errors -O3 `pkg-config --cflags libxml-2.0`
LDFLAGS=`pkg-config --libs libxml-2.0`

SOURCE=s1kd-newimf.c
OUTPUT=s1kd-newimf

all: $(OUTPUT)
	$(MAKE) -C doc

template.h: icnmetadata.xml
	xxd -i icnmetadata.xml > template.h

$(OUTPUT): $(SOURCE) template.h
	$(CC) $(CFLAGS) $(SOURCE) -o $(OUTPUT) $(LDFLAGS)

clean:
	rm -f $(OUTPUT) $(OUTPUT).exe
	$(MAKE) -C doc clean

install: $(OUTPUT)
	cp $(OUTPUT) /usr/bin/
	$(MAKE) -C doc install
