SOURCE=s1kd-instance.c
OUTPUT=s1kd-instance

CFLAGS=-Wall -Werror -pedantic-errors -O3 `pkg-config --cflags libxml-2.0 libxslt libexslt`
LDFLAGS=`pkg-config --libs libxml-2.0 libxslt libexslt`

INSTALL_PREFIX=/usr/bin

all: $(OUTPUT)
	$(MAKE) -C doc

strings.h: help.msg
	xxd -i help.msg > strings.h

identity.h: ../common/identity.xsl
	xxd -i ../common/identity.xsl > identity.h

cirxsl.h: cirxsl/*.xsl
	> cirxsl.h
	for f in cirxsl/*.xsl; do xxd -i "$$f" >> cirxsl.h; done

$(OUTPUT): $(SOURCE) strings.h identity.h cirxsl.h
	$(CC) $(CFLAGS) $(SOURCE) -o $(OUTPUT) $(LDFLAGS)

clean:
	rm -f $(OUTPUT) strings.h identity.h cirxsl.h
	$(MAKE) -C doc clean

install: $(OUTPUT)
	cp $(OUTPUT) $(INSTALL_PREFIX)/
	$(MAKE) -C doc install
