#!/bin/bash
source ../common.sh
pushd gpgme-1.3.0
./configure --host=${HOST} --prefix=${PREFIX} \
  --with-pth=${PREFIX} \
  --with-gpg-error-prefix=${PREFIX} \
  --with-libassuan-prefix=${PREFIX} \
  --with-gpg=${PREFIX}/bin/gpg2 \
  --with-gpgsm=${PREFIX}/bin/gpgsm \
  --with-gpgconf=${PREFIX}/bin/gpgconf \
  --with-g13=${PREFIX}/bin/g13 

#make -j6
#sudo make install
popd
