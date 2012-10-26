#!/bin/bash

#store current directory:
DIR="$( cd "$( dirname "$0" )" && pwd )"
cd "$DIR"

#get current date/time:
DATE=`date +%Y-%m-%d-%H-%M`

#go to parent directory:
cd ..

#if no branch is specified, default to exp branch, otherwise used branch given:
#make sure that the exp branch is checked out:
if [ "$1" != "" ]; then
        git checkout $1
else
        git checkout exp
fi

#check out specified branch:
git checkout 

#update repository:
git pull origin

#back to releasetools folder:
cd releasetools

#create nightly changelog from git commit log:
./make-changelog.sh

#go to parent directory:
cd ..

#clean build directory:
make mrproper

#set up make:
make doubleshot_defconfig

#make zImage:
make -j4 zImage

#make modules:
make modules

#go to releasetools folder:
cd releasetools

#create zip file:
./make-zip.sh $1-$DATE

