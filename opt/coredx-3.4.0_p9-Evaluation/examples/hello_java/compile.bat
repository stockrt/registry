#!/bin/sh
@rem ************************************************************
@rem
@rem file:  compile.bat
@rem desc:  Example batch file to compile a 'hello' CoreDX DDS
@rem        Java application.
@rem
@rem ************************************************************
@rem
@rem Coypright(C) 2006-2011 Twin Oaks Computing, Inc
@rem All rights reserved.   Castle Rock, CO 80108
@rem
@rem *************************************************************
@rem
@rem This file is provided by Twin Oaks Computing, Inc
@rem as an example. It is provided in the hope that it will be 
@rem useful but WITHOUT ANY WARRANTY; without even the implied 
@rem warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
@rem PURPOSE. TOC Inc assumes no liability or responsibilty for 
@rem the use of this information for any purpose.
@rem
@rem ************************************************************/

@rem ######################################################
@rem 
@rem  This script uses the following environment variables:
@rem   COREDX_TOP
@rem   COREDX_HOST
@rem   COREDX_TARGET
@rem  These variables can be set manually, or determined by
@rem  running the script  coredx/scripts/cdxenv.sh or cdxenv.bat
@rem
@rem  Note: javac must be accessible from the current PATH
@rem  Note: we use the 'ddl' file from ../hello_c/
@rem 
@rem ######################################################

set SRC=HelloPub.java HelloSub.java
set GEN_SRC=StringMsg.java StringMsgDataReader.java StringMsgDataWriter.java StringMsgSeq.java StringMsgTypeSupport.java

%COREDX_TOP%\host\%COREDX_HOST%\bin\coredx_ddl.exe -l java -f ..\hello_c\hello.ddl

mkdir classes
javac -d classes -cp classes -cp %COREDX_TOP%\target\java\coredx_dds.jar %SRC% %GEN_SRC%

