AC_INIT

AC_CONFIG_AUX_DIR(./mk)
AC_CONFIG_MACRO_DIR(./mk)
m4_include([mk/libtool.m4])
m4_include([mk/ltoptions.m4])
m4_include([mk/ltsugar.m4])
m4_include([mk/ltversion.m4])
m4_include([mk/lt~obsolete.m4])
m4_include([mk/ax_require_defined.m4])
m4_include([mk/ax_cxx_compile_stdcxx.m4])
m4_include([mk/ax_cxx_compile_stdcxx_11.m4])

AC_CONFIG_HEADERS([framework/include/sqc_config.h])

if test "${prefix}" = "NONE"; then
    prefix=/usr/local
fi
if test "${exec_prefix}" = "NONE"; then
    exec_prefix='${prefix}'
fi

PREFIX=${prefix}
AC_SUBST(PREFIX)

case `uname` in
Linux)
    SHELL=`which bash` > /dev/null 2>&1
    if test $? -ne 0 -o -z "${SHELL}"; then
        AC_MSG_ERROR([To run libtool you need the GNU bash, not the old-school /bin/sh.])
        exit 1
    fi
;;
*)
    AC_MSG_RESULT([SHELL is "$SHELL". bash is recommended.])
;;
esac

export SHELL
AC_SUBST(SHELL)

LT_INIT
AC_SUBST([LIBTOOL_DEPS])

AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AC_PROG_INSTALL
AC_PROG_CC
AC_PROG_CXX
AC_PROG_EGREP
AC_PROG_AWK

#AC_PROG_YACC
#AC_PROG_LEX
#if test "x$LEX" != xflex -a "x$LEX" != xlex; then
#   AC_MSG_ERROR([flex/lex must be installed.])
#fi

RM="rm -f"
AC_SUBST(RM)

MKDIR="mkdir -p"
AC_SUBST(MKDIR)

LN_S="ln -nfs"
AC_SUBST(LN_S)

READLINK="readlink"
AC_SUBST(READLINK)

BASENAME="basename"
AC_SUBST(BASENAME)

PKGCONF="pkg-config"
AC_SUBST(PKGCONF)

# Assume 32 bit for cross compilation.
AC_CHECK_SIZEOF(short int, 2)
AC_CHECK_SIZEOF(int, 4)
AC_CHECK_SIZEOF(long int, 4)
AC_CHECK_SIZEOF(long long, 8)
AC_CHECK_SIZEOF(int64_t, 8)
AC_CHECK_SIZEOF(size_t, 4)
AC_CHECK_SIZEOF(void *, 4)
AC_CHECK_SIZEOF(pthread_t, 4)

# sysdeps.

eval `echo ${target} | awk -F- '{ printf "tagCPU=%s; tagOS=%s;\n", $1, $3 }'`

changequote(<<, >>)dnl
CPU=`echo ${tagCPU} | tr '[A-Z]' '[a-z]'`
OS=`echo ${tagOS} | tr '[A-Z]' '[a-z]'`
changequote([, ])dnl
unset tagOS tagCPU

case $CPU in
    i*86)
        CPU=i386;;
esac

changequote(<<, >>)dnl
CPUDEF=SQC_CPU_`echo $CPU | tr '[a-z]' '[A-Z]'`
OSDEF=SQC_OS_`echo $OS | tr '[a-z]' '[A-Z]'`
changequote([, ])dnl
AC_SUBST(CPUDEF)
AC_SUBST(OSDEF)

OS_CPPFLAGS="-D_REENTRANT"
OS_LDFLAGS=''
OS_LIBS=''
OS_CFLAGS=''
if test -z "${MAKE}"; then
	MAKE=make
fi

ECHO_N='echo -n'
STAT='stat -c %a'

case $OS in
    sunos*)
        OS=sunos;;
    solaris*)
        OS=solaris;;
    freebsd*)
	MAKE=gmake
	OS_CPPFLAGS="${OS_CPPFLAGS} -I/usr/local/include"
	OS_LDFLAGS="${OS_LDFLAGS} -L/usr/local/lib"
	CPPFLAGS="${CPPFLAGS} ${OS_CPPFLAGS}"
	LDFLAGS="${LDFLAGS} ${OS_LDFLAGS}"
	OS_LIBS="-lpthread -lexecinfo"
	OS=freebsd;;
    linux*)
	OS_CPPFLAGS="${OS_CPPFLAGS} -D_GNU_SOURCE -D_POSIX_SOURCE"
	CPPFLAGS="${CPPFLAGS} ${OS_CPPFLAGS}"
	OS_LIBS="-lrt -lpthread -ldl"
	OS=linux;;
    netbsd*)
	MAKE=gmake
	OS_CPPFLAGS="${OS_CPPFLAGS} -I/usr/pkg/include"
	OS_LDFLAGS="${OS_LDFLAGS} -L/usr/pkg/lib"
	CPPFLAGS="${CPPFLAGS} ${OS_CPPFLAGS}"
	LDFLAGS="${LDFLAGS} ${OS_LDFLAGS}"
	OS_LIBS="-lpthread -lexecinfo"
	OS=netbsd;;
    darwin*)
	ECHO_N='/bin/echo -n'
	STAT='stat -f %p'
	OS_CPPFLAGS="${OS_CPPFLAGS} -D_GNU_SOURCE -D_DARWIN_C_SOURCE"
	OS_LIBS="-lpthread"
	OS=darwin;;
esac

# checking CC
TOOLCHAIN_TARGET=''
if test -z "${CC}"; then
    CC=gcc
fi
AC_MSG_CHECKING([toolchain target arch])
_v=`${CC} -v 2>&1 | egrep '^Target:' | awk '{ print $NF }'`
if test $? -eq 0 -a ! -z "${_v}"; then
    case ${CC} in
    	*gcc)
	    TOOLCHAIN_TARGET=${_v};;
    esac
    if test -z "${TOOLCHAIN_TARGET}"; then
        AC_MSG_RESULT([unknown, yet.])
    else
	AC_MSG_RESULT([${TOOLCHAIN_TARGET}])
    fi
fi
AC_SUBST(TOOLCHAIN_TARGET)

# checking C++11
AX_CXX_COMPILE_STDCXX_11([noext], [mandatory])

# checking commands
AC_SUBST(MAKE)
AC_SUBST(OS_CPPFLAGS)
AC_SUBST(OS_CFLAGS)
AC_SUBST(OS_LDFLAGS)
AC_SUBST(OS_LIBS)
AC_SUBST(ECHO_N)
AC_SUBST(STAT)

MKRULES_BUILDDIR=${ac_pwd}/mk
AC_SUBST(MKRULES_BUILDDIR)

AC_SUBST(CPPFLAGS)
AC_SUBST(CFLAGS)
AC_SUBST(CXXFLAGS)
AC_SUBST(LDFLAGS)
AC_SUBST(TARGET_ARCH)

NKF=nkf

developer=no
AC_ARG_ENABLE([developer],
	[AS_HELP_STRING([--enable-developer],
	[enable build for developers, like unit tests [default is no]])]
	 ,[developer="${enableval}"], [developer=no])

IS_DEVELOPER=''
HAVE_UNITY=''
UNITY_DIR=''
unity_dir=''
if test "X${developer}" != "Xno"; then
  IS_DEVELOPER=yes

  AC_CHECK_PROG([HAVE_NKF], [nkf], [yes], [no])
  if test "x${HAVE_NKF}" != "xyes"; then
    AC_MSG_ERROR([nkf is needed for developer mode.])
  fi

  AC_ARG_WITH([unity],
	[AS_HELP_STRING([--with-unity=/path/to/unity],
	[spwcify Unity install directory.])],
	[unity_dir="${withval}"], [unity_dir=''])
  AC_MSG_CHECKING([Unity])
  if test ! -z "${unity_dir}" -a -d "${unity_dir}" -a -f "${unity_dir}/include/unity/unity.h" -a -f "${unity_dir}/lib/libunity.a"; then
    AC_MSG_RESULT([available.])
    UNITY_DIR="${unity_dir}"
    HAVE_UNITY=yes
  else
    AC_MSG_RESULT([not avaolable.])
    AC_MSG_ERROR([installing Unity is required for developer mode.])
  fi
fi
AC_SUBST(IS_DEVELOPER)
AC_SUBST(HAVE_NKF)
AC_SUBST(HAVE_UNITY)
AC_SUBST(UNITY_DIR)


# /proc/self/exe
AC_MSG_CHECKING([/proc/self/exe])
if test -h /proc/self/exe; then
  AC_MSG_RESULT([exists.])
  AC_DEFINE([HAVE_PROCFS_SELF_EXE], [1], [Define if available])
else
  AC_MSG_RESULT([does noe exists.])
fi

# headers/macros

AC_C_BIGENDIAN
AC_CHECK_INCLUDES_DEFAULT

AC_CHECK_HEADER(inttypes.h,
	[AC_DEFINE([HAVE_INTTYPES_H], [1], [Define if available])])
AC_CHECK_HEADER(stdint.h,
	[AC_DEFINE([HAVE_STDINT_H], [1], [Define if available])])
AC_CHECK_HEADER(limits.h,
	[AC_DEFINE([HAVE_LIMITS_H], [1], [Define if available])])
AC_CHECK_HEADER(sys/param.h,
	[AC_DEFINE([HAVE_SYS_PARAM_H], [1], [Define if available])])
AC_CHECK_HEADER(sys/types.h,
	[AC_DEFINE([HAVE_SYS_TYPES_H], [1], [Define if available])])
AC_CHECK_HEADER(ctype.h,
	[AC_DEFINE([HAVE_CTYPE_H], [1], [Define if available])])
AC_CHECK_HEADER(stdio.h,
	[AC_DEFINE([HAVE_STDIO_H], [1], [Define if available])])
AC_CHECK_HEADER(stdlib.h,
	[AC_DEFINE([HAVE_STDLIB_H], [1], [Define if available])])
AC_CHECK_HEADER(unistd.h,
	[AC_DEFINE([HAVE_UNISTD_H], [1], [Define if available])])
AC_CHECK_HEADER(memory.h,
	[AC_DEFINE([HAVE_MEMORY_H], [1], [Define if available])])
AC_CHECK_HEADER(string.h,
	[AC_DEFINE([HAVE_STRING_H], [1], [Define if available])])
AC_CHECK_HEADER(malloc.h,
	[AC_DEFINE([HAVE_MALLOC_H], [1], [Define if available])])
AC_CHECK_HEADER(stdarg.h,
	[AC_DEFINE([HAVE_STDARG_H], [1], [Define if available])])
AC_CHECK_HEADER(dirent.h,
	[AC_DEFINE([HAVE_DIRENT_H], [1], [Define if available])])
AC_CHECK_HEADER(time.h,
	[AC_DEFINE([HAVE_TIME_H], [1], [Define if available])]) 
AC_CHECK_HEADER(sys/time.h,
	[AC_DEFINE([HAVE_SYS_TIME_H], [1], [Define if available])]) 
AC_CHECK_HEADER(termios.h,
	[AC_DEFINE([HAVE_TERMIOS_H], [1], [Define if available])]) 
AC_CHECK_HEADER(sys/stat.h,
	[AC_DEFINE([HAVE_SYS_STAT_H], [1], [Define if available])])
AC_CHECK_HEADER(fcntl.h,
	[AC_DEFINE([HAVE_FCNTL_H], [1], [Define if available])]) 
AC_CHECK_HEADER(errno.h,
	[AC_DEFINE([HAVE_ERRNO_H], [1], [Define if available])]) 
AC_CHECK_HEADER(signal.h,
	[AC_DEFINE([HAVE_SIGNAL_H], [1], [Define if available])])  
AC_CHECK_HEADER(netdb.h,
	[AC_DEFINE([HAVE_NETDB_H], [1], [Define if available])])
AC_CHECK_HEADER(netinet/tcp.h,
	[AC_DEFINE([HAVE_NETINET_TCP_H], [1], [Define if available])])
AC_CHECK_HEADER(sys/socket.h,
	[AC_DEFINE([HAVE_SYS_SOCKET_H], [1], [Define if available])]) 
AC_CHECK_HEADER(arpa/inet.h,
	[AC_DEFINE([HAVE_ARPA_INET_H], [1], [Define if available])]) 
AC_CHECK_HEADER(sys/ioctl.h,
	[AC_DEFINE([HAVE_SYS_IOCTL_H], [1], [Define if available])])
AC_CHECK_HEADER(sys/wait.h,
	[AC_DEFINE([HAVE_SYS_WAIT_H], [1], [Define if available])])
AC_CHECK_HEADER(pthread.h,
	[AC_DEFINE([HAVE_PTHREAD_H], [1], [Define if available])]) 
AC_CHECK_HEADER(stdbool.h,
	[AC_DEFINE([HAVE_STDBOOL_H], [1], [Define if available])])
AC_CHECK_HEADER(sys/resource.h,
	[AC_DEFINE([HAVE_SYS_RESOURCE_H], [1], [Define if available])])
AC_CHECK_HEADER(regex.h,
	[AC_DEFINE([HAVE_REGEX_H], [1], [Define if available])])
AC_CHECK_HEADER(syslog.h,
	[AC_DEFINE([HAVE_SYSLOG_H], [1], [Define if available])]) 
AC_CHECK_HEADER(getopt.h,
	[AC_DEFINE([HAVE_GETOPT_H], [1], [Define if available])])
AC_CHECK_HEADER(bsd/sys/queue.h,
	[AC_DEFINE([HAVE_BSD_SYS_QUEUE_H], [1], [Define if available])])
AC_CHECK_HEADER(sys/queue.h,
	[AC_DEFINE([HAVE_SYS_QUEUE_H], [1], [Define if available])])
AC_CHECK_HEADER(mcheck.h,
	[AC_DEFINE([HAVE_MCHECK_H], [1], [Define if available])]) 
AC_CHECK_HEADER(assert.h,
	[AC_DEFINE([HAVE_ASSERT_H], [1], [Define if available])]) 
AC_CHECK_HEADER(poll.h,
	[AC_DEFINE([HAVE_POLL_H], [1], [Define if available])]) 
AC_CHECK_HEADER(sys/mman.h,
	[AC_DEFINE([HAVE_SYS_MMAN_H], [1], [Define if available])]) 
AC_CHECK_HEADER(sys/prctl.h,
	[AC_DEFINE([HAVE_SYS_PRCTL_H], [1], [Define if available])])
AC_CHECK_HEADER(pwd.h,
	[AC_DEFINE([HAVE_PWD_H], [1], [Define if available])])
AC_CHECK_HEADER(grp.h,
	[AC_DEFINE([HAVE_GRP_H], [1], [Define if available])])

oLIBS=${LIBS}
LIBS="${LIBS} -lpthread"
AC_CHECK_FUNCS([strlcpy strlcat pthread_mutex_timedlock])
LIBS=${oLIBS}
AC_CHECK_FUNC(strtold,
	[AC_DEFINE([HAVE_STRTOLD], [1], [Define if available])]) 

# check pcap
oLIBS=${LIBS}
LIBS=''
PCAP_LIBS=''
have_pcap_header=no
have_pcap_lib=no
AC_CHECK_HEADER(pcap/pcap.h, [have_pcap_header=yes], [])
AC_CHECK_LIB(pcap, pcap_create, [have_pcap_lib=yes], [])
if test "x${have_pcap_header}" = "xyes" -a "x${have_pcap_lib}" = "xyes"; then
   PCAP_LIBS=${LIBS}
fi
AC_SUBST(PCAP_LIBS)
LIBS=${oLIBS}

# use libaio or not
use_libaio=yes
AC_ARG_ENABLE([libaio],
	[AS_HELP_STRING([--enable-libaio],[enable libaio. [default: yes]])]
	 ,[use_libaio="${enableval}"], [use_libaio=yes])

# checking for libaio
resultOK=0
LIBAIO_LIBS=''
if test "x${use_libaio}" = "xyes"; then
    oLIBS=${LIBS}
    LIBS='-laio'
    AC_MSG_CHECKING([libaio and linux/aio_abi.h])
    AC_RUN_IFELSE(
	[AC_LANG_PROGRAM(
	    [[#include <stdio.h>]
	     [#include <stddef.h>]
	     [#include <string.h>]
	     [#include <linux/aio_abi.h>]],
	    [[(void)io_submit(NULL, 0, NULL);]
	     [return 0;]])],
	[resultOK=1],
	[AC_MSG_RESULT([not avairable.])],
	[AC_MSG_RESULT([not checkable since cross-compilation.])])
    if test ${resultOK} -eq 1; then
        AC_MSG_RESULT([available.])
	LIBAIO_LIBS='-laio'
    fi
    LIBS=${oLIBS}
fi
AC_SUBST(LIBAIO_LIBS)

# use ivberbs or not
use_ibverbs=yes
AC_ARG_ENABLE([ibverbs],
	[AS_HELP_STRING([--enable-ibverbs],[enable RDMA by libibverbs. [default: yes]])]
	 ,[use_ibverbs="${enableval}"], [use_ibverbs=yes])

# checking for ibverbs
resultOK=0
RDMA_LIBS=''
if test "x${use_ibverbs}" = "xyes"; then
    oLIBS=${LIBS}
    LIBS='-libverbs'
    AC_MSG_CHECKING([libibverbs and infiniband/ibverbs.h])
    AC_RUN_IFELSE(
	[AC_LANG_PROGRAM(
	    [[#include <stdio.h>]
	     [#include <stddef.h>]
	     [#include <string.h>]
	     [#include <infiniband/verbs.h>]],
	    [[(void)ibv_fork_init();]
	     [return 0;]])],
	[resultOK=1],
	[AC_MSG_RESULT([not avairable.])],
	[AC_MSG_RESULT([not checkable since cross-compilation.])])
    if test ${resultOK} -eq 1; then
        AC_MSG_RESULT([available.])
	RDMA_LIBS='-libverbs'
    fi
    LIBS=${oLIBS}
fi
AC_SUBST(RDMA_LIBS)

# use DPDK or not
use_dpdk=no
AC_ARG_ENABLE([dpdk],
	[AS_HELP_STRING([--enable-dpdk],[enable DPDK. [default: no]])]
	 ,[use_dpdk="${enableval}"], [use_dpdk=no])

spdk_dir=''
AC_ARG_WITH([spdk],
	[AS_HELP_STRING([--with-spdk=/path/to/spdk],[specify SPDK installation directory (implies --with-dpdk.)])]
	 ,[spdk_dir="${withval}"], [spdk_dir=''])
if test "x${enable_spdk}" = "xyes" -o "x${enable_spdk}" = "xno"; then
    spdk_dir=''
fi
if test ! -z "${spdk_dir}"; then
    use_dpdk=yes
fi

# checking for DPDK
DPDK_DIR=''
if test "x${use_dpdk}" = "xyes"; then
    AC_MSG_CHECKING([DPDK and the environmental variable])
    if test ! -z "${RTE_SDK}" -a ! -z "${RTE_TARGET}"; then
        if test -d "${RTE_SDK}" -a -d "${RTE_SDK}/lib/${RTE_TARGET}"; then
            DPDK_DIR="${RTE_SDK}/lib/${RTE_TARGET}"
	    AC_MSG_RESULT([available.])
        else
	    AC_MSG_RESULT([not available.])
	    AC_MSG_ERROR([RTE_SDK and/or RTE_TARGET not properly set.])
        fi
    else
	AC_MSG_RESULT([not available.])
	AC_MSG_ERROR([RTE_SDK and/or RTE_TARGET not set.])
    fi
fi
AC_SUBST(DPDK_DIR)

# checking for SPDK
SPDK_DIR=''
if test ! -z "${spdk_dir}"; then
    AC_MSG_CHECKING([SPDK])
    if test -d "${spdk_dir}" -a -r "${spdk_dir}/include/spdk/config.h"; then
        SPDK_DIR="${spdk_dir}"
        AC_MSG_RESULT([available.])
    else
        AC_MSG_RESULT([not available.])
	AC_MSG_ERROR([SPDK directory is not properly specified or not exists.])
    fi
fi
AC_SUBST(SPDK_DIR)

if test "x${enable_spdk}" = "xyes" -o "x${enable_spdk}" = "xno"; then
    spdk_dir=''
fi
if test ! -z "${spdk_dir}"; then
    use_dpdk=yes
fi

# checking for protoc
HAVE_PROTOC=''
AC_SUBST(HAVE_PROTOC)

# checking for protoc-c
HAVE_PROTOC_C=''
PROTOBUF_C_LIBS=''
PROTOBUF_C_CFLAGS=''
PROTOC_C='protoc-c'
proobuf_c_dir=''
AC_ARG_WITH([protobuf_c],
	[AS_HELP_STRING([--with-protobuf_c=/path/to/protobuf-c],[specify protobuf-c installation directory.])]
	 ,[protobuf_c_dir="${withval}"], [protobuf_c_dir=''])
AC_MSG_CHECKING([protobuf-c])
if test -d "${protobuf_c_dir}" -a -d "${protobuf_c_dir}/lib/pkgconfig" -a -x "${protobuf_c_dir}/bin/protoc-c"; then
    PROTOBUF_C_LIBS=`PKG_CONFIG_PATH="${protobuf_c_dir}/lib/pkgconfig" pkg-config libprotobuf-c --libs`
    PROTOBUF_C_CFLAGS=`PKG_CONFIG_PATH="${protobuf_c_dir}/lib/pkgconfig" pkg-config libprotobuf-c --cflags`
    PROTOC_C="${protobuf_c_dir}/bin/protoc-c"
    AC_MSG_RESULT([available.])
    HAVE_PROTOC_C=yes
else
    AC_MSG_RESULT([not available.])
    AC_MSG_ERROR([installing protobuf-c is required.])
fi
AC_SUBST(HAVE_PROTOC_C)
AC_SUBST(PROTOBUF_C_LIBS)
AC_SUBST(PROTOBUF_C_CFLAGS)
AC_SUBST(PROTOC_C)

# check for munge
HAVE_MUNGE=''
MUNGE_LIBS=''
MUNGE_CFLAGS=''
munge_dir=''
AC_ARG_WITH([munge],
	[AS_HELP_STRING([--with-munge=/path/to/munge],[specify munge installation directory.])]
	 ,[munge_dir="${withval}"], [munge_dir='/usr'])
AC_MSG_CHECKING([munge])
if test -d "${munge_dir}" -a -d "${munge_dir}/lib/pkgconfig"; then
    MUNGE_LIBS=`PKG_CONFIG_PATH="${munge_dir}/lib/pkgconfig" pkg-config munge --libs`
    MUNGE_CFLAGS=`PKG_CONFIG_PATH="${munge_dir}/lib/pkgconfig" pkg-config munge --cflags`
    AC_MSG_RESULT([available.])
    HAVE_MUNGE=yes
elif test -d "${munge_dir}" -a -d "${munge_dir}/lib64/pkgconfig"; then
    MUNGE_LIBS=`PKG_CONFIG_PATH="${munge_dir}/lib64/pkgconfig" pkg-config munge --libs`
    MUNGE_CFLAGS=`PKG_CONFIG_PATH="${munge_dir}/lib64/pkgconfig" pkg-config munge --cflags`
    AC_MSG_RESULT([available.])
    HAVE_MUNGE=yes
else
    AC_MSG_RESULT([not available.])
    AC_MSG_ERROR([installing munge is required.])
fi
AC_SUBST(HAVE_MUNGE)
AC_SUBST(MUNGE_LIBS)
AC_SUBST(MUNGE_CFLAGS)

# check for jwt-cpp
JWT_CPP_CPPFLAGS=''
JWT_CPP_LIBS=''
jwt_cpp_dir=''
AC_ARG_WITH([jwt-cpp],
	[AS_HELP_STRING([--with-jwt-cpp=/path/to/jwt-cpp],[specify jwt-cpp installation directory.])]
	 ,[jwt_cpp_dir="${withval}"], [jwt_cpp_dir=''])
AC_MSG_CHECKING([jwt-cpp])
if test -f "${jwt_cpp_dir}/include/jwt-cpp/jwt.h" -a \
   -f "${jwt_cpp_dir}/include/picojson/picojson.h"; then
    JWT_CPP_CPPFLAGS="-I${jwt_cpp_dir}/include"
    JWT_CPP_LIBS='-lcrypto'
    AC_MSG_RESULT([available.])
else
    AC_MSG_RESULT([not available.])
    AC_MSG_ERROR([installing jwt-cpp is required.])
fi
AC_SUBST(JWT_CPP_CPPFLAGS)
AC_SUBST(JWT_CPP_LIBS)

# check ssl library
AC_CHECK_HEADER(openssl/ssl.h, [true], [AC_MSG_ERROR([openssl/ssl.h is required!])])
oLIBS=${LIBS}
SSL_LIBS=''
LIBS=''
AC_CHECK_LIB(ssl, SSL_new,, [AC_MSG_ERROR([libssl is required!])])
AC_CHECK_LIB(crypto, X509_new,, [AC_MSG_ERROR([libcrypto is required!])])
SSL_LIBS="${LIBS}"
AC_CHECK_FUNCS([ERR_get_error_all poll])
AC_SUBST(SSL_LIBS)
LIBS=${oLIBS}

# check for gmp
AC_CHECK_HEADER(gmp.h,
	[AC_DEFINE([HAVE_GMP_H], [1], [Define if available])],
	[AC_MSG_ERROR([The GNU MP must be installed.])])

# checking REX APIS
rexapis_dir=''
AC_ARG_WITH([rexapis],
            [AS_HELP_STRING([--with-rexapis=/path/to/rexapis],[specify REX APIS installation directory (implies --with-rexapis.)])]
           ,[rexapis_dir="${withval}"], [rexapis_dir=''])
if test ! -z "${rexapis_dir}"; then
    HAVE_REXAPIS=yes
    REXAPIS_DIR="${rexapis_dir}"
else
    AC_MSG_RESULT([not available.])
    AC_MSG_ERROR([REX APIS directory is not properly specified or not exists.])
fi
AC_SUBST(HAVE_REXAPIS)
AC_SUBST(REXAPIS_DIR)

# checking Job Broker
jobbroker_dir=''
AC_ARG_WITH([jobbroker],
            [AS_HELP_STRING([--with-jobbroker=/path/to/jobbroker],[specify JobBroker installation directory (implies --with-jobbroker.)])]
           ,[jobbroker_dir="${withval}"], [jobbroker_dir=''])
if test ! -z "${jobbroker_dir}"; then
    HAVE_JOBBROKER=yes
    JOBBROKER_DIR="${jobbroker_dir}"
else
    AC_MSG_RESULT([not available.])
    AC_MSG_ERROR([Job Broker directory is not properly specified or not exists.])
fi
AC_SUBST(HAVE_JOBBROKER)
AC_SUBST(JOBBROKER_DIR)

resultOK=0
GMP_LIBS=''
oLIBS=${LIBS}
LIBS='-lgmp'
AC_MSG_CHECKING([mpz_init_set_str])
AC_RUN_IFELSE(
	[AC_LANG_PROGRAM(
		[[#include <stdio.h>]
		 [#include <stddef.h>]
		 [#include <string.h>]
		 [#include <gmp.h>]],
		[[char *str = "12345678";]
		 [MP_INT i;]
		 [int r;]
		 [r = mpz_init_set_str(&i, str, 10);]
		 [return r;]])],
	[resultOK=1],
	[AC_MSG_RESULT([not available.])],
	[AC_MSG_RESULT([not checkable since cross-compilation.])])
LIBS=${oLIBS}
if test ${resultOK} -eq 1; then
    AC_MSG_RESULT([available.])
    GMP_LIBS='-lgmp'
else
    AC_MSG_ERROR([The GNU MP library must be installed.])
fi
AC_SUBST(GMP_LIBS)

# size_t printf() designator.
oCFLAGS=${CFLAGS}
CFLAGS="${CFLAGS} -Wall -Werror"
AC_MSG_CHECKING([print format for size_t])
AC_RUN_IFELSE(
	[AC_LANG_PROGRAM(
		[[#include <stdio.h>]
		 [#include <stddef.h>]
		 [#include <string.h>]],
		[[char buf[24];]
		 [snprintf(buf, sizeof(buf), "%zu", (size_t)1);]
		 [return (strcmp("1", buf) == 0 && strlen(buf) == 1) ? ]
		 [0 : 1;]])],
	[AC_DEFINE([HAVE_PRINT_FORMAT_FOR_SIZE_T],
		[1], [Define if available])
	 AC_MSG_RESULT([available.])],
	[AC_MSG_RESULT([not available.])],
	[AC_MSG_RESULT([not checkable since cross-compilation.])])
CFLAGS=${oCFLAGS}

# pthread_setname_np()
oCFLAGS=${CFLAGS}
oLDFLAGS=${LDFLAGS}
CFLAGS="${CFLAGS} -Wall -Werror -D_REENTRANT -D_GNU_SOURCE -pthread"
LDFLAGS="${LDFLAGS} -lpthread"
AC_MSG_CHECKING([pthread_setname_np()])
AC_RUN_IFELSE(
	[AC_LANG_PROGRAM(
		[[#include <stdio.h>]
		 [#include <stddef.h>]
		 [#include <string.h>]
		 [#include <pthread.h>]],
		[[char buf[16];]
		 [snprintf(buf, sizeof(buf), "a thread");]
		 [return pthread_setname_np(pthread_self(), buf);]])],
	[AC_DEFINE([HAVE_PTHREAD_SETNAME_NP],
		[1], [Define if available])
	 AC_MSG_RESULT([available.])],
	[AC_MSG_RESULT([not available.])],
	[AC_MSG_RESULT([not checkable since cross-compilation.])])
CFLAGS=${oCFLAGS}
LDFLAGS="${oLDFLAGS} ${LIBS}"

# pthread_setaffinity_np()
oCFLAGS=${CFLAGS}
oLDFLAGS=${LDFLAGS}
CFLAGS="${CFLAGS} -Wall -Werror -D_REENTRANT -D_GNU_SOURCE -pthread"
LDFLAGS="${LDFLAGS} -lpthread"
AC_MSG_CHECKING([pthread_setaffinity_np()])
AC_RUN_IFELSE(
	[AC_LANG_PROGRAM(
		[[#include <stdio.h>]
		 [#include <stddef.h>]
		 [#include <string.h>]
		 [#include <sched.h>]
		 [#include <pthread.h>]],
		[[cpu_set_t c;]
		 [CPU_ZERO(&c);]
		 [CPU_SET(0, &c);]
		 [return pthread_setaffinity_np(pthread_self(), sizeof(c), &c);]])],
	[AC_DEFINE([HAVE_PTHREAD_SETAFFINITY_NP],
		[1], [Define if available])	
	 AC_MSG_RESULT([available.])],
	[AC_MSG_RESULT([not available.])],
	[AC_MSG_RESULT([not checkable since cross-compilation.])])
CFLAGS=${oCFLAGS}
LDFLAGS="${oLDFLAGS} ${LIBS}"

# numa

has_numa_lib=0
numa_h=0
numaif_h=0
AC_CHECK_HEADER(numa.h, [numa_h=1])
AC_CHECK_HEADER(numaif.h, [numaif_h=1])

oCFLAGS=${CFLAGS}
oLDFLAGS=${LDFLAGS}
oLIBS=${LIBS}
CFLAGS="${CFLAGS} -Wall -Werror -D_REENTRANT -D_GNU_SOURCE -pthread"
LDFLAGS="${LDFLAGS} -lpthread"
LIBS="${LIBS} -lnuma"
AC_MSG_CHECKING([NUMA support])
AC_RUN_IFELSE(
	[AC_LANG_PROGRAM(
		[[#include <stdio.h>]
		 [#include <stddef.h>]
		 [#include <stdlib.h>]
		 [#include <string.h>]
		 [#include <numa.h>]
		 [#include <numaif.h>]],
		[[int node;]
		 [struct bitmask *bmp;]
		 [node = numa_node_of_cpu(0);]
		 [bmp = numa_allocate_nodemask();]
		 [numa_bitmask_setbit(bmp, node);]
		 [(void)set_mempolicy(MPOL_BIND, bmp->maskp, bmp->size + 1);]
		 [(void)numa_alloc_onnode(1024, node);]
		 [return 0;]])],
	[has_numa_lib=1
	 AC_MSG_RESULT([available.])],
	[AC_MSG_RESULT([not available.])],
	[AC_MSG_RESULT([not checkable since cross-compilation.])])
CFLAGS=${oCFLAGS}
LDFLAGS=${oLDFLAGS}
LIBS=${oLIBS}

if test ${numa_h} -eq 1 -a ${numaif_h} -eq 1 -a ${has_numa_lib} -eq 1; then
   numa_supported=yes
fi

NUMA_LIBS=''
do_numa=${numa_supported}
AC_ARG_ENABLE([numa],
	[AS_HELP_STRING([--enable-numa],[Support NUMA (if supported).])],
	[do_numa=${enableval}], [do_numa=${numa_supported}])
if test "X${do_numa}" = "Xyes"; then
  if test "X${numa_supported}" != "Xyes"; then
    AC_MSG_ERROR([The NUMA support library is not usable.])
    exit 1
  else
    AC_DEFINE([HAVE_NUMA_H], [1], [Define if available])
    AC_DEFINE([HAVE_NUMAIF_H], [1], [Define if available])
    AC_DEFINE([HAVE_NUMA], [1], [Define if available])
    NUMA_LIBS='-lnuma'
    LIBS="${LIBS} ${NUMA_LIBS}"
    OS_LIBS="${OS_LIBS} ${NUMA_LIBS}"
  fi
fi
AC_SUBST(NUMA_LIBS)

# emit.
AC_CONFIG_FILES(
	mk/vars.mk
	mk/DPDK-build/Makefile.dpdk
	mk/DPDK-build/Makefile
	mk/doxygen.conf
	Makefile
	framework/include/Makefile
	framework/include/sqc_platform.h
	framework/lib/Makefile
	framework/lib/test/Makefile
	framework/lib/check/Makefile
	app/Makefile
	app/include/Makefile
	app/sqc_rpc_common/Makefile
	app/sqc_rpc_sched/Makefile
	app/module_tmpl/Makefile
	app/dbmgr/Makefile
	app/dbmgr/test/Makefile
	app/grpc_broker/Makefile
	app/grpc_broker/test/Makefile
	app/req_invoker/Makefile
	app/req_invoker/test/Makefile
	app/job_sched/Makefile
	app/job_sched/test/Makefile
	app/rpc/Makefile
	app/rpc/test_jwt/Makefile
	app/rpc/test_munge/Makefile
	app/srv_session/Makefile
	app/srv_session/test/Makefile
	app/sqc_rpc_client/Makefile
	app/sqc_rpc_perf_client/Makefile
	tools/Makefile
)

AC_CONFIG_LINKS(
	mk/rules.mk:mk/rules.mk
	mk/unit-test.mk:mk/unit-test.mk
)

# emit and set executable attribute.
AC_DEFUN([CONFIG_EXECS],
[
m4_foreach_w(
    [FILE_NAME],
    [$1],
    [AC_CONFIG_FILES(FILE_NAME, chmod +x FILE_NAME)])
])

CONFIG_EXECS(
	mk/ut-init.sh
	mk/genutdriver.sh
	mk/changename
)

AC_CONFIG_COMMANDS_PRE([
	sh "${srcdir}/mk/genecode.sh"
	if test $? -ne 0; then
	    AC_MSG_ERROR([Failed to generate error code and error string.])
	fi
])


AC_CONFIG_COMMANDS_POST([sh ${srcdir}/mk/finalizeconfig ${OS_CPPFLAGS}])
AC_CONFIG_COMMANDS_POST([rm -rf ./config.log ./config.status.* autom4te.cache])

AC_OUTPUT

${MAKE} prerequisite
if test $? -ne 0; then
    AC_MSG_ERROR([Prerequisite failure.])
fi

echo ""
echo "CC:	'${CC}'"
echo "CXX:	'${CXX}'"
echo "CFLAGS:	'${OS_CFLAGS}'"
echo "CPPFLAGS:	'${CPPFLAGS} -D${CPUDEF} -D${OSDEF}'"
echo "LDFLAGS:	'${LDFLAGS} ${OS_LDFLAGS} ${OS_LIBS}'"
echo ""
