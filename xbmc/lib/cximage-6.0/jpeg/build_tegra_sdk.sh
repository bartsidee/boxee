#!/bin/sh
#! Build Script used in Release Builds

set -x
set -e

cd `dirname $0`
src=`pwd`

NV_TOPDIR=/opt/tegra2/emPower
NV_PLATFORM_SDK_INC=${NV_TOPDIR}/include
NV_PLATFORM_SDK_LIB=${NV_TOPDIR}/lib-target

export CPPFLAGS="-I${NV_PLATFORM_SDK_INC}/OpenMAX/il -I${NV_PLATFORM_SDK_INC}/OpenMAX/ilclient -I ."
export LDFLAGS="-L${NV_PLATFORM_SDK_LIB} -L/opt/tegra2/emPower/oss/alsa/alsa-lib/lib/"
#export LIBS="-lnvomx -lnvmm -lnvos -lnvrm -lnvrm_graphics -lnvmm_utils -lnvddk_2d -lnvodm_imager -lnvddk_audiomixer -lEGL -lnvmm_tracklist -lnvmm_contentpipe -lnvdispmgr_d -lnvsm -lnvodm_query -lnvodm_dtvtuner -lnvmm_service -lnvmm_manager -lnvwsi -lnvddk_2d_v2 -lnvddk_disp -lnvodm_disp -lc -lpthread"
export LIBS="-lnvomx -lnvmm -lnvos -lnvrm -lnvrm_graphics -lnvmm_utils -lnvddk_2d -lnvodm_imager -lnvddk_audiomixer -lEGL -lnvmm_tracklist -lnvmm_contentpipe -lnvdispmgr_d -lnvsm -lnvodm_query -lnvodm_dtvtuner -lnvmm_service -lnvmm_manager -lnvwsi -lnvddk_2d_v2 -lnvddk_disp -lnvodm_disp -lc -lpthread -lasound -lKD -lnvcwm"
             
# ${src} finds our fake ldconfig, which does nothing, so doesn't require root
export PATH=${src}:${NV_TOPDIR}/toolchains/tegra2-4.3.2-nv/bin:${PATH}
./configure --enable-static --host=arm-none-linux-gnueabi

make clean
make V=1

# UnComment these When test needs to be copied to target by setting TARGET variable.

#mkdir -p $TARGET/root/samples
#mkdir -p $TARGET/root/samples/jpeg-7
#make DESTDIR=$TARGET/root/samples/jpeg-7 install

exit 0
