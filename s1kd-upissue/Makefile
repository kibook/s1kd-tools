SOURCE=s1kd-upissue.c
OUTPUT=s1kd-upissue

CFLAGS=-Wall -Werror -O3 `pkg-config --cflags libxml-2.0`
LDFLAGS=`pkg-config --libs libxml-2.0`

INSTALL_PREFIX=/usr/bin

$(OUTPUT): $(SOURCE)
	$(CC) $(CFLAGS) -o $(OUTPUT) $(SOURCE) $(LDFLAGS)

clean:
	rm -f $(OUTPUT)

install: $(OUTPUT)
	cp $(OUTPUT) $(INSTALL_PREFIX)/
