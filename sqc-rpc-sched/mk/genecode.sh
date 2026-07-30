#!/bin/sh

srcdir=`dirname "$0"`
srcdir=${srcdir:-"."}

if  test ! -x "${srcdir}/../configure" || \
    test ! -d "${srcdir}" || \
    test ! -r "${srcdir}/ecode.def"; then
    exit 1
fi

prfx=sqc
if test $# -ge 1; then
    prfx="${1}"
fi
cprfx=`echo ${prfx} | tr '[a-z]' '[A-Z]'`

h=./framework/include/${prfx}_ecode.h
h_dir=`dirname "${h}"`
mkdir -p "${h_dir}" || exit $?

if test -r "${srcdir}/ecode2h.awk"; then
    nkf -Lu -d "${srcdir}/ecode.def" | \
    awk -f "${srcdir}/ecode2h.awk" -v prfx=${cprfx}_RESULT_ | \
    nkf -Lw -c > "${h}"
    st=$?
    if test ${st} -ne 0; then
        exit ${st}
    fi
fi

c=./framework/lib/ecode.c
c_dir=`dirname "${c}"`
mkdir -p "${c_dir}" || exit $?

if test -r "${srcdir}/ecode2c.awk"; then
    nkf -Lu -d "${srcdir}/ecode.def" | \
    awk -F'"' -f "${srcdir}/ecode2c.awk" | \
    nkf -Lw -c > "${c}"
    st=$?
    if test ${st} -ne 0; then
        exit ${st}
    fi
fi

exit 0
