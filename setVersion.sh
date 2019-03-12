#!/bin/bash

kount=0
v_cmit=0
v_date=0
v_time=0

RES_VION="./include/version.h"

OUT_FILE="tmp01.txt"
RLT_FILE="tmp02.txt"
LOG_FILE="tmp00.txt"
SOURCE_FILE="./source/mothership.c"
TMP_FILE="mothership.tmp"
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

awk -F"+" '
BEGIN {
	count=0	
	OUTPUT="tmp01.txt"
	RES_VERSION="./include/version.h"
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
	
	print ""V1" "V2" "V3"" > RES_VERSION

	#close RES_VERSION
}
' $RLT_FILE

cat $RES_VION

GIT_VERSION=$(<$RES_VION)

echo $GIT_VERSION

sed s/char\ gitcommit.*$/char\ gitcommit[]\ =\ \""$GIT_VERSION"\"\;/ $SOURCE_FILE > $TMP_FILE
