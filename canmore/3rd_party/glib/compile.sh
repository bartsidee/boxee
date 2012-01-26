#!/bin/bash
source ../common.sh
pushd glib-2.26.1
ac_cv_func_posix_getgrgid_r=yes ac_cv_func_nonposix_getpwuid_r=no ac_cv_func_posix_getpwuid_r=yes glib_cv_uscore=no glib_cv_stack_grows=no ./configure --host=${HOST} --prefix=${PREFIX} --with-libiconv=gnu
make -j6
#sudo make install
popd
