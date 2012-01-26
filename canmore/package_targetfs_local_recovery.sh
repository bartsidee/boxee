#!/bin/bash
FILE_LIST=$PWD/targetfs-local-recovery-list.txt
rm -rf /tmp/targetfs-local
mkdir -p /tmp/targetfs-local/opt/local
pushd /opt/canmore/local 2>&1 > /dev/null
tar cf - --files-from $FILE_LIST | ( cd /tmp/targetfs-local/opt/local; tar xfp - )
cd /tmp/targetfs-local/opt/local
find . -name "*.so" | sudo xargs /opt/canmore/toolchains/i686-cm-linux-strip
sudo chown -R root.root /tmp/targetfs-local/opt/local
tar cvfJ /tmp/targetfs-local-recovery.tar.xz --files-from $FILE_LIST
popd 2>&1 > /dev/null
mv /tmp/targetfs-local-recovery.tar.xz .
sudo rm -rf /tmp/targetfs-local
