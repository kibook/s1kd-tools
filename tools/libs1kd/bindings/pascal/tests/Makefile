.PHONY: all clean

all: tests

tests tests.o: tests.pp
	fpc -Fu.. -Fl../../.. -o$@ $+

clean:
	rm -f tests tests.o
