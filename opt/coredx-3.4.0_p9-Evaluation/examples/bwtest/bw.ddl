/****************************************************************
 *
 *  file:  bw.ddl
 *  desc:  DDL for the bandwidth test example
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
#ifdef DDS_IDL
#define DDS_KEY __dds_key
#else
#define DDS_KEY
#endif
#include "bw_config.h"

struct cmdType
{	
  long cmd;
  long sample_size;
  long num_writes;
};

struct bwType
{
  long               counter;
  string<MAX_SIZE>   value;
};

