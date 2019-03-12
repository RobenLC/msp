#!/bin/bash

kount=0
v_cmit=0
v_date=0
v_time=0

RES_VION="res/values/version.xml"

OUT_FILE="tmp01.txt"
RLT_FILE="tmp02.txt"
LOG_FILE="tmp00.txt"
SOURCE_FILE="./source/mothership.c"
git log --pretty="%h %ci" > $LOG_FILE

awk -F" " '
BEGIN {
	count=0
	OUTPUT="tmp02.txt"
}
{
	if (count == 0) {
		print $2 > OUTPUT
		print $3 > OUTPUT
		print $1 > OUTPUT
	}
	count++
}
END {
	#close $OUTPUT
}
' $LOG_FILE 

awk -F"-" '
BEGIN {
	count=0	
	OUTPUT="tmp01.txt"
}
{
	if (count == 0) {
		print $1 $2 $3 > OUTPUT
	}
	count++
}
END {
	#close $OUTPUT
} 
' $RLT_FILE

awk -F":" '
BEGIN {
	count=0	
	OUTPUT="tmp01.txt"
}
{
	if (count == 1) {
		print $1 $2 >> OUTPUT
	}
	if (count == 2) {
		print $1 >> OUTPUT
	}
	count++
}
END {
	print "total: "count
	#close OUTPUT
} 
' $RLT_FILE

awk -F"-" '
BEGIN {
	count=0	
	OUTPUT="tmp01.txt"
	RES_VERSION="res/values/version.xml"
}
{
	if (count == 0) {
		V1=$1
	}
	if (count == 1) {
		V2=$1
	}
	if (count == 2) {
		V3=$1
	}
	count++
}
END {
	print "<?xml version=\"1.0\" encoding=\"utf-8\"?>" > RES_VERSION
	print "<resources>" > RES_VERSION
	print "" > RES_VERSION
	print "<string name=\"versionText\">Version "V1"."V2" "V3"</string>" > RES_VERSION
	print "" > RES_VERSION
	print "</resources>" > RES_VERSION
	print "" > RES_VERSION
	#close RES_VERSION
}
' $OUT_FILE

cat $RES_VION

