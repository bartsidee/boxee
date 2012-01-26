#!/bin/sh

set -x

# abort on any errors
set -e

# vars
output=Platinum

# remove old archives
rm -rf "$output"
find -path "./Platinum*" -delete

# get source
svn export -q svn+ssh://soothe@plutinosoft.com/home/soothe/svn/Platinum/trunk $output

# extract version from PltVersion.h source
version=`cat  $output/Source/Core/PltVersion.h | sed -n "s/.*[^0-9]\([0-9]*\.[0-9]*\.[0-9]*\).*/\1/p" | tail -n 1`;

# remove unnecessary stuff
rm -rf "$output/Extras"
rm -rf "$output/ThirdParty/Ozone"
rm -rf "$output/ThirdParty/SQLite"

#create tar
tar -czf "Platinum-$version.0.tgz" "$output/"

# create zip
zip -qr "Platinum-$version.0.zip" "$output/"

#cleanup
rm -rf "$output"