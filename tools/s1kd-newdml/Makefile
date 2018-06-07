SOURCE=s1kd-newdml.c
OUTPUT=s1kd-newdml

CFLAGS=-Wall -Werror -pedantic-errors -O3 -I ../common `pkg-config --cflags libxml-2.0 libxslt`
LDFLAGS=`pkg-config --libs libxml-2.0 libxslt`

PREFIX=/usr/local
INSTALL_PREFIX=$(PREFIX)/bin

all: $(OUTPUT)
	$(MAKE) -C doc

$(OUTPUT): $(SOURCE) templates.h
	$(CC) $(CFLAGS) -o $(OUTPUT) $(SOURCE) $(LDFLAGS)

templates.h: dml.xml ../common/42to*.xsl sns2dmrl.xsl sort.xsl
	xxd -i dml.xml > templates.h
	xxd -i sns2dmrl.xsl >> templates.h
	xxd -i sort.xsl >> templates.h
	xxd -i ../common/42to20.xsl >> templates.h
	xxd -i ../common/42to21.xsl >> templates.h
	xxd -i ../common/42to22.xsl >> templates.h
	xxd -i ../common/42to23.xsl >> templates.h
	xxd -i ../common/42to30.xsl >> templates.h
	xxd -i ../common/42to40.xsl >> templates.h
	xxd -i ../common/42to41.xsl >> templates.h

clean:
	rm -f $(OUTPUT) templates.h
	$(MAKE) -C doc clean

install: $(OUTPUT)
	cp $(OUTPUT) $(INSTALL_PREFIX)/
	$(MAKE) -C doc install

