SOURCE=s1kd-dmls.c
OUTPUT=s1kd-dmls

CFLAGS=-Wall -Werror -O3 `pkg-config --cflags libxml-2.0`
LDFLAGS=`pkg-config --libs libxml-2.0`

INSTALL_PREFIX=/usr/bin

all: $(OUTPUT)
	$(MAKE) -C doc

$(OUTPUT): $(SOURCE)
	$(CC) $(CFLAGS) -o $(OUTPUT) $(SOURCE) $(LDFLAGS)

clean:
	rm -f $(OUTPUT)
	$(MAKE) -C doc clean

install: $(OUTPUT)
	cp $(OUTPUT) $(INSTALL_PREFIX)/
	$(MAKE) -C doc install
