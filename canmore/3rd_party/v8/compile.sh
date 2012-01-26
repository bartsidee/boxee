#!/bin/bash
source ../common.sh
pushd v8-svn/v8
scons -Q toolchain=gcc arch=ia32 library=static os=linux sample=shell debuggersupport=off profilingsupport=off
popd
