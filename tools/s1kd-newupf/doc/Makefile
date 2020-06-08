SOURCE=DMC-S1KDTOOLS-A-31-00-00-00A-040A-D_EN-CA.XML
OUTPUT=s1kd-newupf.1
README=../README.md

PREFIX=/usr/local
MAN_PREFIX=$(PREFIX)/share/man/man1
INSTALL=install

all: $(OUTPUT) $(README)

$(README): $(SOURCE)
	s1kd2db $(SOURCE) | pandoc -f docbook -t markdown_github -s -o $(README)

$(OUTPUT): $(SOURCE)
	s1kd2db $(SOURCE) | pandoc -f docbook -t man -s -o $(OUTPUT)

.PHONY: clean install uninstall

clean:
	rm -f $(OUTPUT) $(README)

install:
	$(INSTALL) $(OUTPUT) $(MAN_PREFIX)

uninstall:
	rm -f $(MAN_PREFIX)/$(OUTPUT)
