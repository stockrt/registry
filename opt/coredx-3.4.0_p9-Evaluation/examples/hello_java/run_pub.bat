@rem ************************************************************
@rem
@rem file:  run_pub.bat
@rem desc:  Example script to run the 'hello_pub' CoreDX DDS
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


@rem ##########################################################
@rem 
@rem This script uses the following environment variables:
@rem  COREDX_TOP
@rem  COREDX_HOST
@rem  COREDX_TARGET
@rem These variables can be set manually, or determined by
@rem running the script  coredx/scripts/cdxenv.sh or cdxenv.bat
@rem 
@rem ##########################################################
set PATH=%PATH%;%COREDX_TOP%\target\%COREDX_TARGET%\lib
java -cp %COREDX_TOP%\target\java\coredx_dds.jar;classes HelloPub
