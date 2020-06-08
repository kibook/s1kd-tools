#!/bin/sh

if test $# -ne 2
then
	echo "Usage: sh exchange-rate.sh <from> <to>"
	echo
	echo "Options:"
	echo "  <from>  The 3-character ISO code of the source currency."
	echo "  <to>    The 3-character ISO code of the target currency."
	echo
	echo "Example:"
	echo "  sh exchange-rate.sh CAD USD"
	exit 1
fi

curl -s "https://api.exchangerate-api.com/v4/latest/$1" | jq -r ".rates.$2"
