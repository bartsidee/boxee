#!/bin/sh

FILES_TO_COPY=$(cat targetfs-local-list-25.txt | grep -v ^\# | xargs)
BASE_3RDPARTY_PATH=/opt/canmore/3rdparty
PACK_PATH=/tmp/_${0}.$$
STRIP_BIN=/opt/canmore/IntelCE/bin/i686-cm-linux-strip
MISSING_FILES=""

CWD=$(pwd)
OUTNAME=opt_local_25_$(date +"%Y%m%d-%H%M").tar.bz2

if [ $(id -u) -ne 0 ]
then
        echo 'Must run with root privs to use.'
        exit 1
fi

rm -rf ${PACK_PATH}
mkdir -p ${PACK_PATH}
echo "COPYING..."
for f in ${FILES_TO_COPY}
do
	f=${BASE_3RDPARTY_PATH}/${f}
	if [ -e ${f} ]
	then
		fn=$(basename ${f})
		echo -n "${fn}.. "
#		echo "file ${f} exists"
		pn=$(echo $(dirname ${f}) | sed " {
s_${BASE_3RDPARTY_PATH}/target/usr_opt/local_g
s_${BASE_3RDPARTY_PATH}/target_opt/local_g
s_${BASE_3RDPARTY_PATH}/staging/usr_opt/local_g
s_${BASE_3RDPARTY_PATH}/staging__g
s_${BASE_3RDPARTY_PATH}/static/usr__g
s_${BASE_3RDPARTY_PATH}/static__g
s_${BASE_3RDPARTY_PATH}__g
} ")
		fp=${PACK_PATH}/${pn}
		if [ ! -d ${fp} ];
		then
			mkdir -p ${fp}
		fi
		if [ -f ${f} -o -h ${f} ];
		then
#			cp -p ${f} ${fp} 2>&1 >/dev/null
			rsync -a ${f} ${fp}
			if [ 0 != $? ];
			then
				echo "COPY FAILURE, ABORTING"
				rm -rf ${PACK_PATH} 2>&1 >/dev/null
				exit 1
			fi
#			${STRIP_BIN} ${fp}/${fn} 2>&1 >/dev/null
		fi
	else
		MISSING_FILES="${f} ${MISSING_FILES}"
	fi
done
echo
echo "PACKING..."
(cd ${PACK_PATH} ; tar cjf ${CWD}/${OUTNAME} . )

if [ 0 != $? ];
then
	echo "PACK FAILURE, ABORTING"
	rm -rf ${PACK_PATH} 2>&1 >/dev/null
	exit 1
fi
rm -rf ${PACK_PATH} 2>&1 >/dev/null
echo "DONE, RESULT IN ${CWD}/${OUTNAME}"
echo "MISSING FILES:"
echo ${MISSING_FILES} | sed "s_ _\n_g"
