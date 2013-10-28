/****************************************************************
 *
 *  file:  simple.ddl
 *  desc:  DDL for the content filter example
 * 
 ****************************************************************
 *
 *   Coypright(C) 2009-2011 Twin Oaks Computing, Inc
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

struct SimpleMsg
{
   string msg;
   long   x;
   double y;
   octet  z;
   sequence<octet,10> bytes;
   short  shorts[10];
};
