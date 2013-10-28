#!/bin/sh
# ***************************************************************
# 
#   file:  run_sub.sh
#   desc:  Example Script to run the 'hello_sub' CoreDX DDS
#          Java application.
#  
# ***************************************************************
# 
#    Coypright(C) 2006-2011 Twin Oaks Computing, Inc
#    All rights reserved.   Castle Rock, CO 80108
# 
# ****************************************************************
# 
#   This file is provided by Twin Oaks Computing, Inc
#   as an example. It is provided in the hope that it will be 
#   useful but WITHOUT ANY WARRANTY; without even the implied 
#   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
#   PURPOSE. TOC Inc assumes no liability or responsibilty for 
#   the use of this information for any purpose.
#   
# ***************************************************************/


##########################################################
#
# This script uses the following environment variables:
#  COREDX_TOP
#  COREDX_HOST
#  COREDX_TARGET
# These variables can be set manually, or determined by
# running the script  coredx/scripts/cdxenv.sh or cdxenv.bat
#
##########################################################
DO_EXIT=0;
if [ "x${COREDX_TOP}" = "x" ]; then
    echo "Environment variable COREDX_TOP is not set.";
    DO_EXIT=1;
fi

if [ "x${COREDX_HOST}" = "x" ]; then
    echo "Environment variable COREDX_HOST is not set.";
    DO_EXIT=1;
fi

if [ "x${COREDX_TARGET}" = "x" ]; then
    echo "Environment variable COREDX_TARGET is not set.";
    DO_EXIT=1;
fi

if [ ${DO_EXIT} -eq 1 ] ; then 
    exit 1;
fi

if [ "x${LD_LIBRARY_PATH}" = "x" ]; then
  LD_LIBRARY_PATH=${COREDX_TOP}/target/${COREDX_TARGET}/lib
else
  LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${COREDX_TOP}/target/${COREDX_TARGET}/lib
fi
export LD_LIBRARY_PATH;

java -cp ${COREDX_TOP}/target/java/coredx_dds.jar:classes HelloSub
