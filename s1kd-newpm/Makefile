CFLAGS=-Wall -Werror -pedantic-errors -O3 `pkg-config --cflags libxml-2.0 libxslt`
LDFLAGS=`pkg-config --libs libxml-2.0 libxslt`

SOURCE=s1kd-newpm.c
OUTPUT=s1kd-newpm

INSTALL_PREFIX=/usr/local/bin

all: $(OUTPUT)
	$(MAKE) -C doc

template.h: pm.xml ../common/42to*.xsl
	xxd -i pm.xml > template.h
	xxd -i ../common/42to23.xsl >> template.h
	xxd -i ../common/42to30.xsl >> template.h
	xxd -i ../common/42to40.xsl >> template.h
	xxd -i ../common/42to41.xsl >> template.h

$(OUTPUT): $(SOURCE) template.h
	$(CC) $(CFLAGS) $(SOURCE) -o $(OUTPUT) $(LDFLAGS)

clean:
	rm -f $(OUTPUT) template.h
	$(MAKE) -C doc clean

install: $(OUTPUT)
	cp $(OUTPUT) $(INSTALL_PREFIX)/
	$(MAKE) -C doc install
