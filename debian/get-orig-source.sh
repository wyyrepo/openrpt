#!/bin/sh -e

TARBALLDIR=${1:-.}
SUFFIX=$2
set -- $(head -n1 debian/changelog | tr -d '()')

PACKAGE=$1
VERSION=$2
SRC_VERSION=${2%%-*}
SRC_VERSION=${SRC_VERSION%%+*}

for d in ../${PACKAGE}_${SRC_VERSION}.orig.tar.*
do
    if [ -r "$d" ]
    then
        TARBALL="${d##*/}"
        break
    fi
done

WDIR=debian/orig-source

rm -rf ${WDIR}
mkdir ${WDIR}
tar -axf ../${TARBALL} --strip-components=1 -C ${WDIR}
(
    cd debian/orig-source
    find -type f -a \( -name '*.o' -o -name '*.a' -o -name .DS_Store -o -name '*.qm' -o -name Makefile -o -path './bin/*' -o -path './*/tmp/*' \) -exec rm -f {} \;
)
TARBALL=${TARBALLDIR}/${PACKAGE}_${SRC_VERSION}${SUFFIX}.orig.tar.bz2
GZIP='--best --no-name' tar --owner=root --group=root --mode=a+rX -cjf ${TARBALL} -C debian orig-source --xform "s,^orig-source,${PACKAGE}-${SRC_VERSION}${SUFFIX},g"
rm -rf debian/orig-source

