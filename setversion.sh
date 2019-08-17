#!/bin/bash

echo "Buildning new version file"
d=`date +"%Y-%m-%d %H:%M:%S" | tr -d "\n"`
g=`git describe --match=NeVeRmAtCh --always --dirty | tr -d "\n"`
version="$d $g"
echo $version
sed "s/PLACEHOLDER/${version}/g" version.c.t > version.c


