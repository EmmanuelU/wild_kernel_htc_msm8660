#!/bin/sh
#DO NOT EDIT FILE - AUTO GENERATED FOR doubleshot
latestversion=001
<<<<<<< HEAD
latestdate=141118
latestdateliteral='November 18 2014'
latestDL=http://forum.xda-developers.com/showthread.php?t=2551715
=======
latestdate=141013
latestdateliteral='October 13 2014'
latestDL=http://forum.xda-developers.com/showthread.php?t=2765196
>>>>>>> parent of e0bfb00... cpufreq: Cleanup cpu_gov_sync
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
