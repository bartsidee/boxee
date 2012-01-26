#!/bin/bash
source ../common.sh

pushd samba-3.0.37
#patch -p1 < ../samba-3.0.37-CVE-2010-2063.patch
#patch -p1 < ../wle-fix.patch
#patch -p1 < ../silence-receive-warning.patch

cd source
# look at http://www.nabble.com/Tough-question:-errors-while-cross-compiling-to-MIPS-td11657205.html
#patch -p2 < ../../configure.in.patch 
#patch -p2 < ../../vfs_default.c.patch 


#./autogen.sh

export SMB_BUILD_CC_NEGATIVE_ENUM_VALUES=yes
export libreplace_cv_READDIR_GETDIRENTRIES=no 
export libreplace_cv_READDIR_GETDENTS=no
export linux_getgrouplist_ok=no
export samba_cv_REPLACE_READDIR=no
export samba_cv_HAVE_WRFILE_KEYTAB=yes
export samba_cv_HAVE_KERNEL_OPLOCKS_LINUX=yes
export samba_cv_HAVE_IFACE_IFCONF=yes

./configure --host=${HOST} --target=${HOST} --prefix=/opt/local --disable-cups --enable-static --enable-shared --disable-pie --disable-fam --disable-shared-libs --without-ldap --without-ads -disable-iprint  --without-automount --without-sendfile-support --with-included-popt --with-included-iniparser --without-sys-quotas --without-krb5 --without-swat 

make -j6

#!!!!!! IMPORTANT
# before doing make install, make sure that /usr/local is symbolic to /opt/tegra2/local
#sudo make install
popd
