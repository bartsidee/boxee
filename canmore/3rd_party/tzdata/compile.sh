#!/bin/bash
VERSION=tzdata2010j

rm -rf zoneinfo
mkdir zoneinfo

mkdir $VERSION
cd $VERSION
tar xvfz ../$VERSION.tar.gz

zic -d ../zoneinfo africa antarctica asia australasia backward etcetera europe factory northamerica pacificnew southamerica systemv
cp *.tab ../zoneinfo

