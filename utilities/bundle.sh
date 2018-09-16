PROG=`basename $0`
VERFILE=common/data.cpp
VERSION=
BUILD=false
BASEDIR=`pwd`
DASHX=
APPSTOBUNDLE=(metasql importmqlgui importrptgui rptrender openrpt)
PLUGINSFLAGS=(D       D            D            DI        DI     )
OS=`uname -s`
if [ "$OS" = Darwin ] ; then
  ARCH=`uname -p`
  OSVER=MAC${ARCH}
fi

if [ -z "$QTDIR" ] ; then
  for TRYME in `echo $PATH | tr : ' '` ; do
    TRYME=`dirname $TRYME`
    if [ -e "$TRYME/bin/qmake" -a -e "$TRYME/lib/libQtCore.dylib" ] ; then
      export QTDIR="$TRYME"
      break;
    fi
  done
  if [ -n "QTDIR" ] ; then
    echo "$PROG: guessing that Qt is installed in $QTDIR"
  else
    echo "$PROG: Cannot run properly without the QTDIR environment variable set"
    exit 2
  fi
fi
if [ -z "$PGDIR" ] ; then
  for TRYME in `echo $PATH | tr : ' '` ; do
    TRYME=`dirname $TRYME`
    if [ -e "$TRYME/bin/psql" -a -e "$TRYME/lib/libpq.a" ] ; then
      export PGDIR="$TRYME"
      break;
    fi
  done
  if [ -n "PGDIR" ] ; then
    echo "$PROG: guessing that PostgreSQL is installed in $PGDIR"
  else
    echo "$PROG: Cannot run properly without the PGDIR environment variable set"
    exit 2
  fi
fi

if [ "$BASEDIR" = . ] ; then
  BASEDIR=`pwd`
fi

usage() {
  echo "$PROG -h [ -b ] [ -V VersionString ] [ -x ]"
  echo
  fmt <<EOF
$PROG bundles OpenRPT for release on the current platform
(currently only Mac OS X is supported).
$PROG reads $VERFILE to get release information used to
name the release package.
EOF
  echo
  echo "-h	get this usage message"
  echo "-b	force build (run qmake and make before bundling)"
  echo "	$PROG runs qmake and make if the required binaries are missing "
  echo "	from the bin directory even if -b is not specified."
  echo "-V      use the VersionString argument to name the bundle instead of "
  echo "        deriving the version number from $VERFILE"
  echo "-x	turn on shell debugging output"
  echo
  echo "Environment Variables:"
  echo "QTDIR   Qt Installation [ $QTDIR ]"
  echo "PGDIR   PostgreSQL Installation [ $PGDIR ]"
}

bundle() {
  if [ "$#" -ne 1 ] ; then
    echo "Illegal call to bundle: bad arg count"
    echo "usage: bundle dir_to_put_it_in"
    return 1
  fi

  STARTDIR=`pwd`

  BUNDLENAME="openrpt-${VERSION}-${OSVER}"
  BUNDLEDIR="${1}/${BUNDLENAME}"

  if [ -d "$BUNDLEDIR" ] ; then
    rm -rf "$BUNDLEDIR"                                         || return 3
  fi
  mkdir "$BUNDLEDIR"                                            || return 3
  for FILE in ${APPSTOBUNDLE[@]} ; do
    if [ "$OSVER" = MACUniversal ] ; then
      cp -R "$BASEDIR/bin/${FILE}.app" $BUNDLEDIR               || return 4
    else
      ditto --rsrc --arch $ARCH "$BASEDIR/bin/${FILE}.app" "$BUNDLEDIR/${FILE}.app" || return 4
    fi
  done
  cp COPYING $BUNDLEDIR                                         || return 4

  cd "$BUNDLEDIR"/..                                            || return 5
  if [ -f "$BUNDLENAME".dmg ] && ! rm "$BUNDLENAME".dmg ; then
    return 5
  fi
  hdiutil create -fs HFS+ -volname "$BUNDLENAME" \
                 -srcfolder "$BUNDLENAME" "$BUNDLENAME".dmg     || return 5

  cd $STARTDIR                                                  || return 5
}

ARGS=`getopt hbV:x $*`
if [ $? != 0 ] ; then
  usage
  exit 1
fi
set -- $ARGS

while [ "$1" != -- ] ; do
  case "$1" in
    -h) usage
	exit 0
	;;
    -b) BUILD=true
	;;
    -V) VERSION=$2
        shift
        ;;
    -x) set -x
        DASHX=-x
	;;
     *) usage
	exit 1
	;;
  esac
  shift
done
shift # past the --

if [ "$#" -gt 0 ] ; then
  echo "$PROG: ignoring extra arguments $*"
fi

if [ `uname -s` != Darwin ] ; then
  echo "$PROG: only supports Macintosh OS X (Darwin) at this time"
  usage
  exit 2
fi

for FILE in ${APPSTOBUNDLE[@]} ; do
  if [ ! -e "$BASEDIR"/bin/$FILE.app ] ; then
    if ! $BUILD ; then
      echo "Building even though not explicitly told to do so"
    fi
    BUILD=true
    break
  fi
done

if [ -z "$VERSION" ] ; then
  if [ ! -f "$VERFILE" ] ; then
    echo "Could not find $VERFILE"
    exit 2
  fi
  VERSION=`awk '/OpenRPT::version / { split($0,vers,"\""); print vers[2]}' $VERFILE | \
           tr -d [:blank:]`
  if [ -z "$VERSION" ] ; then
    echo "Could not extract version from $VERFILE"
    exit 3
  fi
fi

if $BUILD ; then
  cd "$BASEDIR"                                                 || exit 3
  qmake                                                         || exit 3
  make                                                          || exit 3
fi

cd "$BASEDIR"
INDEX=0
for FILE in ${APPSTOBUNDLE[@]} ; do
  utilities/fixPackage $DASHX -p${PLUGINSFLAGS[$INDEX]} "$BASEDIR/bin/${FILE}.app" || exit 4
  INDEX=`expr $INDEX + 1`
done

cd "$BASEDIR"                                                   || exit 4
bundle "${BASEDIR}/.."                                          || exit 4

echo "DONE!"
