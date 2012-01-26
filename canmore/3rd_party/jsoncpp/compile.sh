#!/bin/bash -x
source ../common.sh
pushd jsoncpp-src-0.5.0
mkdir out
$CXX -O2 -o out/json_reader.o -c -Wall -Iinclude src/lib_json/json_reader.cpp
$CXX -O2 -o out/json_writer.o -c -Wall -Iinclude src/lib_json/json_writer.cpp
$CXX -O2 -o out/json_value.o -c -Wall -Iinclude src/lib_json/json_value.cpp
cd out
$CXX -shared -o libjsoncpp.so json_reader.o json_writer.o json_value.o
$AR cru libjsoncpp.a json_reader.o json_writer.o json_value.o
sudo cp libjsoncpp.so libjsoncpp.a /opt/canmore/local/lib
cd ../include
sudo cp -R json /opt/canmore/local/include
popd
