SOURCE1=DMC-S1000DTOOLS-A-00-00-00-00A-040A-D_EN-CA.XML
SOURCE2=DMC-S1000DTOOLS-A-00-00-00-00A-018A-D_EN-CA.XML
SOURCE3=DMC-S1000DTOOLS-A-00-00-00-00A-014A-D_EN-CA.XML

OUTPUT1=../README.md
OUTPUT2=../INTRO.md
OUTPUT3=../defaults_file.md

EXAMPLE_SRC=DMC-S1000DTOOLS-A-00-00-00-00A-130A-D_EN-CA.XML
EXAMPLE_OUT=../EXAMPLE.md

MAN_PREFIX=/usr/local/share/man/man1

all: s1kd-tools.1

$(OUTPUT1): $(SOURCE1)
	s1kd2db $(SOURCE1) | pandoc -f docbook -t markdown_github -s -o $@

$(OUTPUT2): $(SOURCE2)
	s1kd2db $(SOURCE2) -param include.emphasis.role 1 | pandoc -f docbook -t markdown_github -s -o $@

$(OUTPUT3): $(SOURCE3)
	s1kd2db $(SOURCE3) -param include.emphasis.role 1 | pandoc -f docbook -t markdown_github -s -o $@

$(EXAMPLE_OUT): $(EXAMPLE_SRC)
	s1kd2db $(EXAMPLE_SRC) -param include.emphasis.role 1 | pandoc -f docbook -t markdown_github -s -o $(EXAMPLE_OUT)

s1kd-tools.1: $(SOURCE2)
	s1kd2db $(SOURCE2) -param include.emphasis.role 1 | pandoc -f docbook -t man -s -o $@

install:
	cp s1kd-tools.1 $(MAN_PREFIX)/

clean:
	rm -f s1kd-tools.1
