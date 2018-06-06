SOURCE=DMC-S1000DTOOLS-A-00-0E-00-00A-040A-D_EN-CA.XML

PREFIX=/usr/local
MAN_PREFIX=$(PREFIX)/share/man/man1

all: s1kd-neutralize.1

../README.md: $(SOURCE)
	s1kd2db $(SOURCE) | pandoc -f docbook -t markdown_github -o ../README.md

s1kd-neutralize.1: $(SOURCE)
	s1kd2db $(SOURCE) | pandoc -f docbook -t man -s -o s1kd-neutralize.1

clean:
	rm -f s1kd-neutralize.1

install:
	cp s1kd-neutralize.1 $(MAN_PREFIX)/
