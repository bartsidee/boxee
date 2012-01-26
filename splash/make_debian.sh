#!/bin/bash
if [[ `whoami` != 'root' ]]
then
   echo "Must run as root!"
   exit
fi

rm -rf tmp
mkdir -p tmp/DEBIAN tmp/usr/lib/usplash
cp debian/* tmp/DEBIAN
cp *.so tmp/usr/lib/usplash
chown -R root.root tmp
VERSION=`grep Version debian/control  | sed 's/Version: //'`
dpkg-deb --build ./tmp boxee-artwork-usplash-${VERSION}.deb
rm -rf tmp
