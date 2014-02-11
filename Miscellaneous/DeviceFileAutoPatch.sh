#!/bin/bash
for file in *.xml
do
	line=`grep -n "<parameter id=\"CENTRAL_ADDRESS_SPOOFED\" operations=\"read,write,event\" ui_flags=\"service,sticky\" control=\"NONE\">" $file | cut -d ':' -f 1`
	if [ ! -z "$line" ]; then
		continue
	fi
	line=`grep -n "<parameter id=\"RSSI_PEER\" operations=\"read,event\">" $file | cut -d ':' -f 1`
	if [ -z "$line" ]; then
		continue
	fi
	lines=`wc -l $file | cut -d ' ' -f 1`
	searchlines=`expr $lines - $line`
	line2=`tail -n $searchlines $file | grep -m 1 -n "</parameter>" | cut -d ':' -f 1`
	line=`expr $line + $line2`
	text='\t\t\t\t<parameter id="CENTRAL_ADDRESS_SPOOFED" operations="read,write,event" ui_flags="service,sticky" control="NONE">\n\t\t\t\t\t<logical type="option">\n\t\t\t\t\t\t<option id="UNSET" index="0" default="true"/>\n\t\t\t\t\t\t<option id="CENTRAL_ADDRESS_SPOOFED" index="1"/>\n\t\t\t\t\t</logical>\n\t\t\t\t\t<physical type="integer" interface="internal" value_id="CENTRAL_ADDRESS_SPOOFED" />\n\t\t\t\t</parameter>'
	sed -i "$[line]a\ $text" $file
done
