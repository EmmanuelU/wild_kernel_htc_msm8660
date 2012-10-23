#!/bin/bash
if [ -e "changelog.txt" ]
then
	rm changelog.txt
fi

git log --after="yesterday" > changelog.txt
