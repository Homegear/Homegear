#!/bin/bash
for i in *.xml; do
	content=`cat $i`
	echo $content | xmllint --noblanks - > $i
done
for i in *.xml; do
	content=`cat $i`
	echo $content | xmllint --format - > $i
done
