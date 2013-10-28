# ***************************************************************
#    
#   file:  Makefile
#   desc:  Makefile for CoreDX bwtest application
#    
# ***************************************************************
# 
#    Coypright(C) 2009-2011 Twin Oaks Computing, Inc
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
EXECUTABLE      = bwtest

# Source files
bwtest_CSRCS    = main.c
bwtest_COBJS    = ${bwtest_CSRCS:.c=.o}

# Generated code
bwtest_GENSRCS  = bw.c bwTypeSupport.c bwDataReader.c bwDataWriter.c
bwtest_GENHDRS  = bw.h bwDataReader.h bwDataWriter.h bwTypeSupport.h
bwtest_GENOBJS  = ${bwtest_GENSRCS:.c=.o}

# Put it all together
bwtest_OBJS     = ${bwtest_GENOBJS} ${bwtest_COBJS}

##########################################################
# Setup Compile Environment
# We assume gcc is in your path!

CC       = gcc
DDL      = ${COREDX_TOP}/host/${COREDX_HOST}/bin/coredx_ddl
INC_PATH = -I${COREDX_TOP}/target/include
LIB_PATH = -L${COREDX_TOP}/target/${COREDX_TARGET}/lib
LIBS     = -Wl,-Bstatic -ldds -Wl,-Bdynamic

CFLAGS   = -Wall -O2 ${INC_PATH}
LDFLAGS  = ${LIB_PATH} ${LIBS} -ldl -lm -lnsl -lrt

##########################################################
# Rules for compiling

all: ${EXECUTABLE} 

${EXECUTABLE}: ${bwtest_GENSRCS} ${bwtest_OBJS}
	${CC} -o $@ ${bwtest_OBJS} ${LDFLAGS} 


##########################################################
# Rules for generating code from IDL

%.c :%.ddl
	${DDL} -f $<

%TypeSupport.c :%.ddl
	${DDL} -f $<

%DataWriter.c :%.ddl
	${DDL} -f $<

%DataReader.c :%.ddl
	${DDL} -f $<


##########################################################
# Rules for cleaning

clean:
	\rm -f ${bwtest_OBJS} ${EXECUTABLE}

reallyclean: clean
	\rm -f ${bwtest_GENSRCS} ${bwtest_GENHDRS}

##########################################################
# Additional dependencies
main.o:          ${bwtest_GENHDRS} main.c
bw.o:            ${bwtest_GENHDRS} bw.c
bwTypeSupport.o: ${bwtest_GENHDRS} bwTypeSupport.c
bwDataReader.o:  ${bwtest_GENHDRS} bwDataReader.c
bwDataWriter.o:  ${bwtest_GENHDRS} bwDataWriter.c

