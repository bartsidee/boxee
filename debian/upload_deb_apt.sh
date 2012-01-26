#!/bin/bash

# local config to srv.boxee.tv
#http_path=10.0.0.250/apt
#scp_path=10.0.0.250:/home/distro/apt
#up_user=distro
#repo=main

# remote config to apt.boxee.tv
http_path=apt.boxee.tv
scp_path=apt.boxee.tv:public_html
up_user=distro
repo=main

ubuntu=`lsb_release -c | awk '{ print $2; }'`
has_dpkg_dev=`dpkg --list | grep dpkg-dev -c`

if [ ${has_dpkg_dev} -eq 0 ]; then
  echo "dpkg-dev required, please apt-get install it."
  exit 1
fi

if [ "$1" = "" ]; then
  echo "Usage: $0 <.deb file>"
  exit 1
fi

if [ ! -r $1 ]; then
  echo "File does not exist: $1"
  exit 1
fi
echo "will upload ${ubuntu} package!"
rm -rf tmp
mkdir -p tmp/dists/${ubuntu}/${repo}/binary-i386
cd tmp
echo "Downloading Packages.gz"
wget -q http://${http_path}/dists/${ubuntu}/${repo}/binary-i386/Packages.gz
if [ "$?" != "0" ]; then
  echo "Could not download Packages.gz"
  exit 1
fi
gunzip Packages.gz
cp ../$1 dists/${ubuntu}/${repo}/binary-i386
echo "Generating Packages for the requested ${ubuntu} debian package"
dpkg-scanpackages ./dists /dev/null 2>/dev/null > new_package

filename=`grep Filename new_package | sed 's/Filename: //'`
found=`grep $filename Packages | wc -l`

if [ "$found" != "0" ]; then
  echo "Package $1 is already uploaded to the site."
  exit 1
fi

cat new_package >> Packages
gzip Packages

echo "Uploading $1, please provide password"
scp ../$1 ${up_user}@${scp_path}/dists/${ubuntu}/${repo}/binary-i386

echo "Uploading Packages.gz, please provide password"
scp Packages.gz ${up_user}@${scp_path}/dists/${ubuntu}/${repo}/binary-i386

cd ..
rm -rf tmp
