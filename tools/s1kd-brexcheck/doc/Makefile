SOURCE=DMC-S1KDTOOLS-A-04-00-00-00A-040A-D_EN-CA.XML
OUTPUT=s1kd-brexcheck.1
README=../README.md

PREFIX=/usr/local
MAN_PREFIX=$(PREFIX)/share/man/man1
INSTALL=install -D

all: $(OUTPUT) $(README)

$(README): $(SOURCE)
	s1kd2db $(SOURCE) | pandoc -f docbook -t markdown_github -s -o $(README)

$(OUTPUT): $(SOURCE)
	s1kd2db $(SOURCE) | pandoc -f docbook -t man -s -o $(OUTPUT)

.PHONY: clean install uninstall

clean:
	rm -f $(OUTPUT) $(README)

install:
	$(INSTALL) $(OUTPUT) $(MAN_PREFIX)/$(OUTPUT)

uninstall:
	rm -f $(MAN_PREFIX)/$(OUTPUT)
