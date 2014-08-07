#!/bin/bash

REPOSITORY="$HOME/fatcat/"
NAME=fatcat

if [ $# -lt 1 ]; then
 echo "Usage: ./deb.sh version"
 exit
fi

WORKDIR=`pwd`
VERSION=$1
rm -rf $WORKDIR/$NAME*

# Create the archive and the directory
UPSTREAM=$WORKDIR/fatcat_$VERSION.orig.tar.gz
cd $REPOSITORY &&
git archive --format=tar.gz debian_$VERSION > $UPSTREAM &&
cd $WORKDIR &&
mkdir $NAME &&
cd $NAME &&
tar zxvf $UPSTREAM &&

# Run the debian packaging
dpkg-buildpackage -us -uc && # -ai386 &&

cd $WORKDIR &&
scp fatcat_$VERSION* root@gregwar.com:/home/www/gregwar/fatcat/
