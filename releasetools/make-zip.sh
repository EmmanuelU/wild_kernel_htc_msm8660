#!/bin/bash
#***CREATE ANYKERNEL PACKAGE***:

DATE=$1

#append NO-CHANGES to archive name if there is not git commit log:
if [ -e "changelog.txt" ]
then
	DATE=$DATE
else
	DATE=$DATE-NO-CHANGES
fi

if [ -e "../arch/arm/boot/zImage" ]
then
	cp -f ../arch/arm/boot/zImage anykernel/kernel/zImage
else
	DATE=$DATE-FAIL
fi

if [ -e "../drivers/net/wireless/bcmdhd/bcmdhd.ko" ]
then
	cp -f ../drivers/net/wireless/bcmdhd/bcmdhd.ko anykernel/system/lib/modules/bcmdhd.ko
else
	DATE=$DATE-NOWIFI
fi

#copy new changelog to ANYKERNEL folder:
cp -f changelog.txt anykernel/changelog.txt

cd anykernel
zip -r9 ../kernel-zip/$DATE.zip *

cd ..

#copy new changelog to kernel-zip folder:
cp changelog.txt kernel-zip/$DATE.txt

