SOURCE=s1kd-newcom.c
OUTPUT=s1kd-newcom

CFLAGS=-Wall -Werror -pedantic-errors -O3 `pkg-config --cflags libxml-2.0 libxslt`
LDFLAGS=`pkg-config --libs libxml-2.0 libxslt`

PREFIX=/usr/local
INSTALL_PREFIX=$(PREFIX)/bin

all: $(OUTPUT)
	$(MAKE) -C doc

template.h: comment.xml ../common/42to*.xsl
	xxd -i comment.xml > template.h
	xxd -i ../common/42to20.xsl >> template.h
	xxd -i ../common/42to21.xsl >> template.h
	xxd -i ../common/42to22.xsl >> template.h
	xxd -i ../common/42to23.xsl >> template.h
	xxd -i ../common/42to30.xsl >> template.h
	xxd -i ../common/42to40.xsl >> template.h
	xxd -i ../common/42to41.xsl >> template.h

$(OUTPUT): $(SOURCE) template.h
	$(CC) $(CFLAGS) -o $(OUTPUT) $(SOURCE) $(LDFLAGS)

clean:
	rm -f $(OUTPUT)
	rm -f template.h
	$(MAKE) -C doc clean

install: $(OUTPUT)
	cp $(OUTPUT) $(INSTALL_PREFIX)/
	$(MAKE) -C doc install

