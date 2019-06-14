.PHONY: all clean

all: S1000DBIKE.png

S1000DBIKE.dot: bike/DMC-*.XML
	sh refsgraph.sh -d bike $+ > $@

S1000DBIKE.png: S1000DBIKE.dot
	dot $< -Tpng > $@

clean:
	rm -rf S1000DBIKE.dot S1000DBIKE.png
