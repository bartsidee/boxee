#!/bin/bash
. cross_env

SRC=./
DEST=$CANMORE_HOME/targetfs/opt/boxee
STRIP=$CANMORE_HOME/toolchains/i686-cm-linux-strip

${STRIP} RecoveryConsole/RecoveryConsole

rm -rf ${DEST} 

mkdir -p ${DEST}/
cp ${SRC}/RecoveryConsole/{RecoveryConsole,logo86t.png} ${DEST}/
