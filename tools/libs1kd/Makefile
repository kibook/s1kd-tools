.PHONY: all clean

CFLAGS=-g -DLIBS1KD -shared -fPIC -I ../common -I ../s1kd-instance -I ../s1kd-metadata -I ../s1kd-brexcheck `pkg-config --cflags libxml-2.0 libxslt libexslt`
LDFLAGS=`pkg-config --libs libxml-2.0 libxslt libexslt`

all: libs1kd.so

libs1kd.so: ../common/s1kd_tools.c ../s1kd-instance/s1kd-instance.c ../s1kd-metadata/s1kd-metadata.c ../s1kd-brexcheck/s1kd-brexcheck.c
	$(MAKE) -C ../s1kd-instance
	$(MAKE) -C ../s1kd-metadata
	$(MAKE) -C ../s1kd-brexcheck
	$(CC) $(CFLAGS) -o $@ $+ $(LDFLAGS)

clean:
	$(MAKE) -C ../s1kd-instance clean
	$(MAKE) -C ../s1kd-metadata clean
	$(MAKE) -C ../s1kd-brexcheck clean
	$(RM) libs1kd.so
