#!/bin/sh

echo "digraph \"DM refs\" {"
echo "rankdir=LR"

refs=$(s1kd-refs -cDfR $@ | sort -u)

# Create relationships
echo "$refs" | sed 's/: /\t/' | while read srcpath dstpath
do
	srcid=$(basename "$srcpath" | sed 's/[-\.]/_/g')
	dstid=$(basename "$dstpath" | sed 's/[-\.]/_/g')

	echo "$srcid -> $dstid"
done

# Create labels
echo "$refs" | sed 's/: /\n/' | sort -u | while read path
do
	id=$(basename "$path" | sed 's/[-\.]/_/g')
	title=$(s1kd-metadata -n title "$path")
	
	echo "$id [label=\"$path\n$title\"]"
done

echo "}"
