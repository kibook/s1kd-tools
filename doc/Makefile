SOURCE1=DMC-S1000DTOOLS-A-00-00-00-00A-040A-D_EN-CA.XML
SOURCE2=DMC-S1000DTOOLS-A-00-00-00-00A-018A-D_EN-CA.XML

MAN_PREFIX=/usr/local/share/man/man1

all: s1kd-tools.1

../README.md: $(SOURCE1)
	s1kd2db $(SOURCE1) | pandoc -f docbook -t markdown_github -s -o ../README.md

../INTRO.md: $(SOURCE2)
	s1kd2db $(SOURCE2) -param include.emphasis.role 1 | pandoc -f docbook -t markdown_github -s -o ../INTRO.md

s1kd-tools.1: $(SOURCE2)
	s1kd2db $(SOURCE2) -param include.emphasis.role 1 | pandoc -f docbook -t man -s -o s1kd-tools.1

install:
	cp s1kd-tools.1 $(MAN_PREFIX)/

clean:
	rm -f s1kd-tools.1
