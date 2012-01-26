#!/bin/bash
source ../common.sh
cd lirc-0.8.7
./configure --host=${HOST} --prefix=${PREFIX} --with-driver=mceusb --with-kerneldir=/opt/canmore/IntelCE/kernel/linux-2.6.28 --without-x
make -j6
cd daemons
sudo make install
cd ../tools
sudo make install
cd ..
sudo mkdir -p /opt/canmore/local/lib/modules/2.6.28
sudo cp drivers/lirc_mceusb/lirc_mceusb.ko drivers/lirc_dev/lirc_dev.ko /opt/canmore/local/lib/modules/2.6.28
sudo cp remotes/mceusb/lircd.conf.mceusb /opt/canmore/local/etc/lircd.conf

# on target:
# /bin/mknod /dev/lirc c 61 0
# ln -s /opt/local/modules/2.6.28/ /lib/modules/2.6.28
# insmod /opt/local/lib/modules/2.6.29/lirc_dev.ko       
# insmod /opt/local/lib/modules/2.6.29/lirc_mceusb2.ko
# /opt/local/sbin/lircd --device=/dev/lirc0 /opt/local/etc/lircd.conf
