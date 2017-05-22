SOURCE=DMC-S1000DTOOLS-A-00-02-00-00A-040A-D_000-01_EN-CA.XML

MAN_PREFIX=/usr/local/share/man/man1

all: s1kd-validate.1

../README.md: $(SOURCE)
	s1kd2db $(SOURCE) | pandoc -f docbook -t markdown_github -s -o ../README.md

s1kd-validate.1: $(SOURCE)
	s1kd2db $(SOURCE) | pandoc -f docbook -t man -s -o s1kd-validate.1

install: s1kd-validate.1
	cp s1kd-validate.1 $(MAN_PREFIX)/

clean:
	rm -f s1kd-validate.1
