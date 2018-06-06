SOURCE=DMC-S1000DTOOLS-A-00-0Q-00-00A-040A-D_EN-CA.XML

PREFIX=/usr/local
MAN_PREFIX=$(PREFIX)/share/man/man1

all: s1kd-aspp.1

../README.md: $(SOURCE)
	s1kd2db $(SOURCE) | pandoc -f docbook -t markdown_github -s -o ../README.md

s1kd-aspp.1: $(SOURCE)
	s1kd2db $(SOURCE) | pandoc -f docbook -t man -s -o s1kd-aspp.1

install: s1kd-aspp.1
	cp s1kd-aspp.1 $(MAN_PREFIX)/

clean:
	rm -f s1kd-aspp.1
