/****************************************************************
 *
 *  file:  init_license.c
 *  desc:  
 * 
 *   This is a simple routine to set a TLM_LICENSE environment variable 
 *   for configuring the CoreDX DDS Run-Time License under VxWorks.
 *
 *   The license string is often quite long, beyond the default limits 
 *   of the VxWorks shell, so this routine allows installation of an 
 *   extended length environment variable.
 * 
 ****************************************************************
 *
 *   Coypright(C) 2008-2011 Twin Oaks Computing, Inc
 *   All rights reserved.   Castle Rock, CO 80108
 *
 *****************************************************************
 *
 *  This file is provided by Twin Oaks Computing, Inc
 *  as an example. It is provided in the hope that it will be 
 *  useful but WITHOUT ANY WARRANTY; without even the implied 
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
 *  PURPOSE. TOC Inc assumes no liability or responsibilty for 
 *  the use of this information for any purpose.
 *  
 ****************************************************************/

#include <envLib.h>
int init_license(void)
{
  /* change the text 'YOUR LICENSE STRING' and replace with the full run-time license string */
  /* For example: 
     putenv("TLM_LICENSE = <LICENSE ..... SIG=ab3ac03e03f......111aa>");
  */
  putenv("TLM_LICENSE = < your license string >");
  return 0;
}
