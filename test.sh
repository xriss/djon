#!/bin/sh
cd `dirname $0`
cd test

echo " Converting all input json "

echo "" >output.json
for fname in `find json -name "*.json" -type f -exec echo {} \;` ; do

	echo Transforming ${fname}
	
	echo "" >>output.json
	echo "#$fname" >>output.json
	{ cat "$fname" | tr -cd '[:print:]\t\n\r' ; } &>>output.json

	echo "" >>output.json
	echo "#c" >>output.json
	{ ../c/djon "${fname}" | tr -cd '[:print:]\t\n\r' ; } &>>output.json

done



