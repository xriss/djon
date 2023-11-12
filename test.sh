#!/bin/sh
cd `dirname $0`
cd test

echo "# Converting all input json ( note this file is forced 7bit ascii clean so not exact output )" >output.json

files=`find json -name "*.json" -type f -exec echo {} \; && find djon -name "*.djon" -type f -exec echo {} \;`

for fname in $files ; do

	echo Transforming ${fname}
	
	echo "" >>output.json
	echo "#$fname" >>output.json
	{ cat "$fname" | tr -cd '[:print:]\t\n\r' ; } &>>output.json

	echo "" >>output.json
	echo "#c.json" >>output.json
	{ ../c/djon "${fname}" | tr -cd '[:print:]\t\n\r' ; } &>>output.json
	echo "#c.djon" >>output.json
	{ ../c/djon --djon "${fname}" | tr -cd '[:print:]\t\n\r' ; } &>>output.json

done



