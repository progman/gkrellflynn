#!/bin/sh
#
#

targetdir=$2
instpath=""
test -x $HOME/.$targetdir && instpath=$HOME/.$targetdir
test -x /usr/local/lib/$targetdir && instpath=/usr/local/lib/$targetdir
test -x /usr/lib/$targetdir && instpath=/usr/lib/$targetdir

if test -z $instpath; then
	echo	"Error: cannot find install path"
	exit 1
fi

instpath=$instpath/plugins

echo "installing plugin "$1" in "$instpath

if ! test -x $instpath; then
 mkdir $instpath
fi

cp $1 $instpath
