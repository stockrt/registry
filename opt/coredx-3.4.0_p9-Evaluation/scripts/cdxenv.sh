#!/bin/bash
# ****************************************************************
#    file:   cdxenv.sh
#    auth:   ctucker
#    date:   2006/08/26
#    desc:   A script to determine the values of some helpful
#            environment variables.
# 
# ****************************************************************
# 
#    Coypright(C) 2006-2011 Twin Oaks Computing, Inc
#    All rights reserved.   Castle Rock, CO 80109
# 
# ****************************************************************
#   This software has been provided pursuant to a License Agreement
#   containing restrictions on its use.  This software contains
#   valuable trade secrets and proprietary information of
#   Twin Oaks Computing, Inc and is protected by law.  It may not be
#   copied or distributed in any form or medium, disclosed to third
#   parties, reverse engineered or used in any manner not provided
#   for in said License Agreement except with the prior written
#   authorization from Twin Oaks Computing, Inc.
# ****************************************************************/

SAVED_CWD=`pwd`;
PROG=`basename $0`
if [ "$PROG" = "cdxenv.sh" ] ; then 
  cd `dirname $0`/..;
  COREDX_TOP=`pwd`; 
else
  if [ "x$COREDX_TOP" = "x" ] ; then 
    echo "Can't determine COREDX_TOP.  Please set the COREDX_TOP environment variable."
    return ;
  else
    cd ${COREDX_TOP}
  fi
fi

export COREDX_TOP;

######
# determine the current host platform
UNAME=`which uname`
if [ "x$UNAME" = "x" ] ; then 
    if [ -f /usr/bin/uname ]; then UNAME="/usr/bin/uname"; fi
    if [ -f /bin/uname ]; then UNAME="/bin/uname"; fi
fi

ARCH_OS=`${UNAME} -s | awk -F_ '{ print $1}'`
ARCH_VER=`${UNAME} -r | awk -F. '{ print $1"."$2}'`
ARCH_MACH=`${UNAME} -m`
ARCH=${ARCH_OS}_${ARCH_VER}_${ARCH_MACH}

######
# find available HOST platforms
num_hosts=0
cd ${COREDX_TOP}/host
if [ $? -eq 0 ] ; then 
    for f in `ls ` ; do
	if [ -d $f -a "$f" != "bin" ] ; then 
	    num_hosts=$(($num_hosts+1))
	    eval hosts${num_hosts}="$f"
	fi
    done
fi

if  [ $num_hosts -eq 0 ] ; then 
    echo "ERR: Unable to find any CoreDX DDS host platforms."
    # exit 1;
    export COREDX_HOST="UNKNOWN"
######
# if there is only one option, use it, otherwise, let the user 
# select from a list.
elif [ $num_hosts -eq 1 ] ; then 
    export COREDX_HOST=${hosts1}
else
  n=1
  DEFAULT_COREDX_HOST="";
  DEFAULT_COREDX_HOST_NUM=1;
  while [ $n -le $num_hosts -a "x${DEFAULT_COREDX_HOST}" = "x" ] ; do
    eval h=\$hosts$n;
    case "$h" in 
	${ARCH}*) 
	          DEFAULT_COREDX_HOST_NUM=$n;
	          DEFAULT_COREDX_HOST=$h;
		  ;;
	*)  ;;
    esac
    n=$(($n+1))
  done

  # print HOST option(s)
  n=1
  while [ $n -le $num_hosts ] ; do
    eval echo "\ \ \ $n:  \$hosts$n"
    n=$(($n+1))
  done

  # read input
  ans=1000;
  while [ $ans -le 0 -o $ans -gt $num_hosts ] ; do
      echo -n "Please select the HOST platform [${DEFAULT_COREDX_HOST_NUM}]: "
      read tans
      if [ "x$tans" = "x" ] ; then 
	  ans=${DEFAULT_COREDX_HOST_NUM};
      else
	  ans=$(($tans)) # convert to a number
	  if [ "$ans" != "$tans" ] ; then ans=1000; fi
      fi
  done
  eval export COREDX_HOST=\$hosts${ans}
fi

######
# Now, find available TARGET platforms
cd ${COREDX_TOP}/target
num_targets=0
for f in `ls ` ; do
    if [ -d $f -a "$f" != "bin" -a "$f" != "include" -a "$f" != "java" ] ; then 
	num_targets=$(($num_targets+1))
	eval targets${num_targets}="$f"
    fi
done
if  [ $num_targets -eq 0 ] ; then 
  echo "ERR: Unable to find any CoreDX DDS target platforms."
  exit 1;
fi

######
# if there is only one option, use it, otherwise, let the user 
# select from a list.
if [ $num_targets -eq 1 ] ; then 
    export COREDX_TARGET=${targets1}
else
  # try to guess a default 'COREDX_TARGET'
  
  n=1
  DEFAULT_COREDX_TARGET="";
  DEFAULT_COREDX_TARGET_NUM=1;
  while [ $n -lt $num_targets -a "x${DEFAULT_COREDX_TARGET}" = "x" ] ; do
    eval h=\$targets$n;
    case "$h" in 
	${ARCH}*) 
	          DEFAULT_COREDX_TARGET_NUM=$n;
	          DEFAULT_COREDX_TARGET=$h;
		  ;;
	*)  ;;
    esac
    n=$(($n+1))
  done

  # print TARGET option(s)
  n=1
  while [ $n -le $num_targets ] ; do
    eval echo "\ \ \ $n:  \$targets$n"
    n=$(($n+1))
  done

  # read input
  ans=1000;
  while [ $ans -le 0 -o $ans -gt $num_targets ] ; do
      echo -n "Please select the TARGET platform [${DEFAULT_COREDX_TARGET_NUM}]: "
      read tans
      if [ "x$tans" = "x" ] ; then 
	  ans=${DEFAULT_COREDX_TARGET_NUM};
      else
	  ans=$(($tans)) # convert to a number
	  if [ "$ans" != "$tans" ] ; then ans=1000; fi
      fi
  done
  eval export COREDX_TARGET=\$targets${ans}
fi

# print results:
echo "export COREDX_TOP=${COREDX_TOP};"
echo "export COREDX_HOST=${COREDX_HOST};"
echo "export COREDX_TARGET=${COREDX_TARGET};"
if [ "x$LD_LIBRARY_PATH" = "x" ]; then
  echo "export LD_LIBRARY_PATH=${COREDX_TOP}/target/${COREDX_TARGET}/lib"
else
  echo "export LD_LIBRARY_PATH=${COREDX_TOP}/target/${COREDX_TARGET}/lib:${LD_LIBRARY_PATH}"
fi
cd ${SAVED_CWD}
