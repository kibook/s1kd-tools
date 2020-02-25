.PHONY: all clean

all: PMC-TEST-12345-00001-00_EN-CA.pdf

PMC-TEST-12345-00001-00_EN-CA.pdf: PMC-TEST-12345-00001-00_EN-CA.XML DMC-*.XML
	sh updatefm.sh
	s1kd-flatten -x $< | s1kd2pdf -o $@ -

clean:
	rm -f PMC-TEST-12345-00001-00_EN-CA.pdf
