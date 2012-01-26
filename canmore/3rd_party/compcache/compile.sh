#!/bin/bash
source ../common.sh
pushd compcache-0.6.2
KERNEL_BUILD_PATH=/home/yuvalt/IntelCE-18.226902/project_build_i686/IntelCE/kernel-18.3.10471.226317/linux-2.6.35 make -j6
sudo cp ramzswap.ko /opt/canmore/local/lib/modules/2.6.35
sudo cp sub-projects/rzscontrol/rzscontrol /opt/canmore/local/sbin
popd
