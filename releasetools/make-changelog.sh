#!/bin/bash
if [ -e "changelog.txt" ]
then
	rm changelog.txt
fi

git log --after="1 day ago" > changelog.txt
