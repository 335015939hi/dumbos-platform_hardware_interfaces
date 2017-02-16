#!/bin/bash

if [ ! -d hardware/interfaces ] ; then
  echo "Where is hardware/interfaces?";
  exit 1;
fi

if [ ! -d system/libhidl/transport ] ; then
  echo "Where is system/libhidl/transport?";
  exit 1;
fi

packages=$(pushd hardware/interfaces > /dev/null; \
           find . -type f -name \*.hal -exec dirname {} \; | sort -u | \
           cut -c3- | \
           awk -F'/' \
                '{printf("android.hardware"); for(i=1;i<NF;i++){printf(".%s", $i);}; printf("@%s\n", $NF);}'; \
           popd > /dev/null)

for p in $packages; do
  echo "Updating $p";
  hidl-gen -o temp -Lvts -r android.hardware:hardware/interfaces -r android.hidl:system/libhidl/transport $p;
done

vtsfile=$(find temp/ -name \*.vts)

for f in $vtsfile; do
  echo "copy $f"
  path=$(dirname $f | cut -c23-)
  cp $f hardware/interfaces/$path/vts/
done
