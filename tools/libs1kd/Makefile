.PHONY: all clean

CFLAGS=-g -DLIBS1KD -shared -fPIC -I ../common `pkg-config --cflags libxml-2.0 libxslt libexslt`
LDFLAGS=`pkg-config --libs libxml-2.0 libxslt libexslt`

all: libs1kd.so

libs1kd.so: ../common/s1kd_tools.c ../s1kd-instance/s1kd-instance.c ../s1kd-metadata/s1kd-metadata.c ../s1kd-brexcheck/s1kd-brexcheck.c
	$(CC) $(CFLAGS) -o $@ $+ $(LDFLAGS)

clean:
	rm -f libs1kd.so

libs1kd_tests: libs1kd_tests.c
	$(CC) -g -I include `pkg-config --cflags libxml-2.0` -o $@ $+ `pkg-config --libs libxml-2.0` -L . -l s1kd
