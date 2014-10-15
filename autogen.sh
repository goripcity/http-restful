#! /bin/sh
set -x
aclocal
autoconf
touch NEWS README AUTHORS ChangeLog
automake --add-missing
