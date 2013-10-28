#!/bin/sh
# ***************************************************************
# 
#   file:  compile.sh
#   desc:  Example Script to compile a 'hello' CoreDX DDS
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
# Note: we use the 'ddl' file from ../hello_c/
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

SRC="HelloPub.java HelloSub.java"
GEN_SRC="StringMsg.java StringMsgDataReader.java StringMsgDataWriter.java StringMsgSeq.java StringMsgTypeSupport.java"

${COREDX_TOP}/host/${COREDX_HOST}/bin/coredx_ddl -l java -f ../hello_c/hello.ddl

mkdir -p classes
javac -d classes -cp classes -cp ${COREDX_TOP}/target/java/coredx_dds.jar ${SRC} ${GEN_SRC}

