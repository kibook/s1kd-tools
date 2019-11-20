#!/bin/sh

cat <<EOF
# s1kd-uom example

The following tables are all produced from the same source table using
semantic quantity markup.

The exchange rates used to convert the currencies were pulled
automatically from (ExchangeRate-API)[https://www.exchangerate-api.com]
on $(date -I).

EOF

for file in $@
do
	echo "## ${file}"
	echo
	cat "$file"
	echo
done
