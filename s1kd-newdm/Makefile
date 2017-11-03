CFLAGS=-Wall -Werror -pedantic-errors -O3 `pkg-config --cflags libxml-2.0 libxslt`
LDFLAGS=`pkg-config --libs libxml-2.0 libxslt`

SOURCE=s1kd-newdm.c
OUTPUT=s1kd-newdm

all: $(OUTPUT)
	$(MAKE) -C doc

templates.h: templates/*/*.xml templates/30/42to30.xsl
	> templates.h
	for t in templates/42/*.xml; do xxd -i $$t >> templates.h; done
	xxd -i templates/41/42to41.xsl >> templates.h
	xxd -i templates/40/42to40.xsl >> templates.h
	xxd -i templates/30/42to30.xsl >> templates.h

$(OUTPUT): $(SOURCE) templates.h
	$(CC) $(CFLAGS) $(SOURCE) -o $(OUTPUT) $(LDFLAGS)

clean:
	rm -f $(OUTPUT) templates.h
	$(MAKE) -C doc clean

install: $(OUTPUT)
	cp $(OUTPUT) /usr/bin/
	$(MAKE) -C doc install
