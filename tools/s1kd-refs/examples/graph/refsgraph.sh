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
	base=$(basename "$path")
	id=$(echo "$base" | sed 's/[-\.]/_/g')
	title=$(s1kd-metadata -n title "$path")
	
	echo "$id [label=\"$base\n$title\"]"
done

echo "}"
