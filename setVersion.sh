#!/bin/bash

kount=0
v_cmit=0
v_date=0
v_time=0

RES_VION="./include/git_version.h"
MSP_VERSION="./include/version.h"

OUT_FILE="tmp01.txt"
RLT_FILE="tmp02.txt"
LOG_FILE="tmp00.txt"
MSP_TEMP="tmp03.txt"
OLD_VERSION="tmp04.txt"
OLD_NUM_F="tmp_old_version.txt"
SOURCE_FILE="./source/mothership.c"
TMP_FILE1="mothership_git.tmp"
TMP_FILE2="mothership_ver.tmp"
git log --pretty="%h %ci" > $LOG_FILE

awk -F"." '
BEGIN {
	count=0
	OUTPUT="tmp03.txt"
}
{
	if (count == 0) {		
	    #print $1
		#print $2
		#print $3
		
		V1=$1
		V2=$2
		V3=$3
	}
	count++
}
END {
    V4=V3+1
    #print ""V1"."V2"."V3""
    print ""V1"."V2"."V4"" > OUTPUT
	#close $OUTPUT
}
' $MSP_VERSION 

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
	RES_VERSION="./include/git_version.h"
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

#echo $GIT_VERSION

sed -n /char\ gitcommit.*$/p $SOURCE_FILE > $OLD_VERSION

awk -F"\"" '
BEGIN {
    OUTPUT="tmp_old_version.txt"
}
{
    print $2 > OUTPUT
}
END {
}
' $OLD_VERSION

OLD=$(<$OLD_NUM_F)
OLD_MSP_VER=$(<$MSP_VERSION)
NEW_MSP_VER=$(<$MSP_TEMP)
echo "old $OLD_MSP_VER: $OLD"
echo "new $NEW_MSP_VER: $GIT_VERSION"

if [[ "$OLD" == "$GIT_VERSION" ]]; then
    echo "the version is the same $OLD vs $GIT_VERSION"
else
    sed s/char\ gitcommit.*$/char\ gitcommit[]\ =\ \""$GIT_VERSION"\"\;/ $SOURCE_FILE > $TMP_FILE1
    sed s/char\ mver.*$/char\ mver[]\ =\ \""$NEW_MSP_VER"\"\;/ $TMP_FILE1 > $TMP_FILE2
    mv $TMP_FILE2 $SOURCE_FILE
    mv $MSP_TEMP $MSP_VERSION
fi

rm ./tmp*.txt



