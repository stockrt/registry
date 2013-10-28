# ***************************************************************
# 
#   file:  Makefile
#   desc:  Makefile for CoreDX DDS example Hello World applications
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
# This Makefile uses the following environment variables:
#  COREDX_TOP
#  COREDX_HOST
#  COREDX_TARGET
# These variables can be set manually, or determined by
# running the script  coredx/scripts/cdxenv.sh or cdxenv.bat
#
##########################################################

# Define our sources and objects
EXECUTABLE      = hello_pub hello_sub

##########################################################
# Setup Compile Environment
# We assume both Mono (gmcs) is in your path
CS         = gmcs
DDL        = ${COREDX_TOP}/host/${COREDX_HOST}/bin/coredx_ddl
LIB_PATH   = -L${COREDX_TOP}/target/${COREDX_TARGET}/lib

##########################################################
# Rules for compiling

all: ${EXECUTABLE} 

hello_pub: hello_pub.cs hello.ddl
	mkdir -p ./gen
	${DDL} -l csharp -d gen -f hello.ddl
	${CS} hello_pub.cs /recurse:'gen/*.cs' /lib:${COREDX_TOP}/target/${COREDX_TARGET}/lib /r:coredx_csharp.dll

hello_sub: hello_sub.cs hello.ddl
	mkdir -p ./gen
	${DDL} -l csharp -d gen -f hello.ddl
	${CS} hello_sub.cs /recurse:'gen/*.cs' /lib:${COREDX_TOP}/target/${COREDX_TARGET}/lib /r:coredx_csharp.dll

##########################################################
# Rules for cleaning

clean:
	\rm -f ${EXECUTABLE}

reallyclean: clean
	\rm -rf ./gen/
