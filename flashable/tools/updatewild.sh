#!/bin/sh
#DO NOT EDIT FILE - AUTO GENERATED FOR doubleshot
latestversion=001
latestdate=141018
latestdateliteral='October 18 2014'
latestDL=http://forum.xda-developers.com/showthread.php?t=2551715
input="$1"
if [[ -z "$input" ]]
	then
	exit 1
fi

if [[ "$input" == "latestversion" ]]
	then
	echo $latestversion

elif [[ "$input" == "latestdate" ]]
	then
	echo $latestdate

elif [[ "$input" == "latestDL" ]]
	then
	echo $latestDL
elif [[ "$input" == "latestdateliteral" ]]
	then
	echo $latestdateliteral
fi

# echo ${!input} Apparently not all roms have bash installed ._.
