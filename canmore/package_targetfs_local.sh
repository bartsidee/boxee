#!/bin/bash
FILE_LIST=$PWD/targetfs-local-list.txt
rm -rf /tmp/targetfs-local
mkdir -p /tmp/targetfs-local/opt/local
pushd /opt/canmore/local 2>&1 > /dev/null
tar cf - --files-from $FILE_LIST | ( cd /tmp/targetfs-local/opt/local; tar xfp - )
cd /tmp/targetfs-local/opt/local
find . -name "*.so" | sudo xargs /opt/canmore/toolchains/i686-cm-linux-strip
sudo chown -R root.root /tmp/targetfs-local/opt/local
cp $FILE_LIST /tmp/file_list
echo "var" >> /tmp/file_list
sudo ln -s ../../var /tmp/targetfs-local/opt/local/var
tar cvfJ /tmp/targetfs-local.tar.xz --files-from /tmp/file_list
popd 2>&1 > /dev/null
mv /tmp/targetfs-local.tar.xz .
sudo rm -rf /tmp/targetfs-local
rm /tmp/file_list
