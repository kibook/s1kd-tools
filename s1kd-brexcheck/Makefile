SOURCE=s1kd-brexcheck.c
OUTPUT=s1kd-brexcheck

CFLAGS=-Wall -Werror -pedantic-errors -O3 `pkg-config --cflags libxml-2.0`
LDFLAGS=`pkg-config --libs libxml-2.0`

INSTALL_PREFIX=/usr/bin

$(OUTPUT): $(SOURCE)
	$(CC) $(CFLAGS) -o $(OUTPUT) $(SOURCE) $(LDFLAGS)

clean:
	rm -f $(OUTPUT)

install: $(OUTPUT)
	cp $(OUTPUT) $(INSTALL_PREFIX)/
