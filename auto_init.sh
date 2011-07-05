#!/bin/sh
autoreconf -f -i -Wall,no-obsolete
libtoolize --force --copy --automake
aclocal
autoheader
automake --foreign --copy --add-missing
autoconf

