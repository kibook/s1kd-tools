CFLAGS=-Wall -Werror -pedantic-errors -O3 `pkg-config --cflags libxml-2.0`
LDFLAGS=`pkg-config --libs libxml-2.0`

SOURCE=s1kd-newdm.c
OUTPUT=s1kd-newdm

all: $(OUTPUT)
	$(MAKE) -C doc

templates.h: templates/*
	xxd -i templates/descript.xml > templates.h
	xxd -i templates/proced.xml >> templates.h
	xxd -i templates/frontmatter.xml >> templates.h
	xxd -i templates/brex.xml >> templates.h
	xxd -i templates/brdoc.xml >> templates.h
	xxd -i templates/appliccrossreftable.xml >> templates.h
	xxd -i templates/comrep.xml >> templates.h
	xxd -i templates/prdcrossreftable.xml >> templates.h
	xxd -i templates/condcrossreftable.xml >> templates.h
	xxd -i templates/process.xml >> templates.h
	xxd -i templates/ipd.xml >> templates.h

$(OUTPUT): $(SOURCE) templates.h
	$(CC) $(CFLAGS) $(SOURCE) -o $(OUTPUT) $(LDFLAGS)

clean:
	rm -f $(OUTPUT) templates.h
	$(MAKE) -C doc clean

install: $(OUTPUT)
	cp $(OUTPUT) /usr/bin/
	$(MAKE) -C doc install
