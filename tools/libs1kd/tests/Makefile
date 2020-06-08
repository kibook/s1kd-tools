.PHONY: all clean

all: tests

tests: tests.c
	$(CC) -g -I ../include `pkg-config --cflags libxml-2.0` -o $@ $+ `pkg-config --libs libxml-2.0` -L .. -l s1kd

clean:
	rm -f tests
