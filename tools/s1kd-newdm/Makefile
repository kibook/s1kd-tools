CFLAGS=-Wall -Werror -pedantic-errors -O3 `pkg-config --cflags libxml-2.0 libxslt`
LDFLAGS=`pkg-config --libs libxml-2.0 libxslt`

SOURCE=s1kd-newdm.c
OUTPUT=s1kd-newdm

PREFIX=/usr/local
INSTALL_PREFIX=$(PREFIX)/bin

all: $(OUTPUT)
	$(MAKE) -C doc

templates.h: templates/42/*.xml ../common/42to*.xsl
	> templates.h
	for t in templates/42/*.xml; do xxd -i $$t >> templates.h; done
	xxd -i ../common/42to20.xsl >> templates.h
	xxd -i ../common/42to21.xsl >> templates.h
	xxd -i ../common/42to22.xsl >> templates.h
	xxd -i ../common/42to23.xsl >> templates.h
	xxd -i ../common/42to30.xsl >> templates.h
	xxd -i ../common/42to40.xsl >> templates.h
	xxd -i ../common/42to41.xsl >> templates.h

dmtypes.h: dmtypes.xml dmtypes.txt
	xxd -i dmtypes.xml > dmtypes.h
	xxd -i dmtypes.txt >> dmtypes.h

sns.h: sns/*.XML
	> sns.h
	for s in sns/*.XML; do xxd -i $$s >> sns.h; done

$(OUTPUT): $(SOURCE) templates.h dmtypes.h sns.h
	$(CC) $(CFLAGS) $(SOURCE) -o $(OUTPUT) $(LDFLAGS)

clean:
	rm -f $(OUTPUT) templates.h dmtypes.h sns.h
	$(MAKE) -C doc clean

install: $(OUTPUT)
	cp $(OUTPUT) $(INSTALL_PREFIX)/
	$(MAKE) -C doc install
