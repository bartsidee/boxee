#!/bin/bash
source ../common.sh
#pushd gnupg-1.4.9
pushd gnupg-2.0.15
./configure --host=${HOST} --prefix=${PREFIX} \
  --with-gpg-error-prefix=${PREFIX} \
  --with-libgcrypt-prefix=${PREFIX} \
  --with-ksba-prefix=${PREFIX} \
  --with-pth-prefix=${PREFIX} \
  --with-libassuan-prefix=${PREFIX}

make -j6
#sudo make install
popd
