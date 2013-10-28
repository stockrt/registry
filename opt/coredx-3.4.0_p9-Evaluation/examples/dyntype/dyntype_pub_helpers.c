/****************************************************************
 *
 *  file:  dyntype_pub_helpers.c
 *  desc:  Provides a C publisher on the "SimpleMsg" topic.
 *         The datatype is constructed using helper TYPECODE
 *         This application requires some generated source code.
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
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <dds/dds.h>
#include <dds/dds_dtype.h>

#include "simplemsgTypeCode.h"
#include "simplemsg.h"

#ifdef _WIN32
#  define SLEEP(d) Sleep((d)*1000)
#else
#  define SLEEP(d) sleep((d))
#endif

int all_done = 0;

/****************************************************************
 * main()
 *
 * Perform CoreDX DDS setup activities:
 *   - create a Domain Participant
 *   - create a Publisher
 *   - register the SimpleMsg data type
 *   - create a Topic
 *   - create a DataWriter 
 * Write data
 ****************************************************************/
#if defined(__vxworks) && !defined(__RTP__)
int simplemsg_pub(char * args)
#else
int main(int argc, char * argv[])
#endif
{
  DDS_DomainParticipant  domain;
  DDS_Publisher          publisher;
  DDS_Topic              topic;
  DDS_DataWriter         dw;
  int                    i;
  struct DDS_TypecodeQosPolicy  simplemsg_tc;
  DDS_TypeDefinition     td  = NULL;
  DDS_TypeSupport        dts = NULL;
  SimpleMsg              simpleMsg;
  DDS_DynamicType        simplemsg_data;

  all_done = 0;

  /* create a DDS_DomainParticipant */
  domain = 
    DDS_DomainParticipantFactory_create_participant( 0, 
						 DDS_PARTICIPANT_QOS_DEFAULT, 
						 NULL, 
						 0 );
  if ( domain == NULL )
    {
      printf("ERROR creating domain participant.\n");
      return -1;
    }

  /* Create a DDS Publisher with the default QoS settings */
  publisher = 
    DDS_DomainParticipant_create_publisher(domain, 
				       DDS_PUBLISHER_QOS_DEFAULT, 
				       NULL, 
				       0 );
  if ( publisher == NULL )
    {
      printf("ERROR creating publisher.\n");
      return -1;
    }
  
  /* Construct a 'TypeDefinition' from the TYPECODE */
  SimpleMsg_init_typecode( &simplemsg_tc );
  td       = DDS_create_type_definition_from_typecode(simplemsg_tc.value._buffer, 
						      simplemsg_tc.value._length, 
						      simplemsg_tc.encoding);
  if (td == NULL)
    {
      printf("ERROR creating type definitionn from typecode.\n");
      return -1;
    }

  /* Construct a 'DynamicTypeTypeSupport' for this 'simplemsg' type */
  dts      = DDS_create_dynamic_typesupport( td ); 

  /* Register the data type with the CoreDX middleware. 
   * This is required before creating a Topic with
   * this data type. 
   */
  if (DDS_DomainParticipant_register_type(domain, dts, "SimpleMsg" ) != DDS_RETCODE_OK)
    {
      printf("ERROR registering type\n");
      return -1;
    }
  
  /* create a DDS Topic with the SimpleMsg data type. */
  topic = 
    DDS_DomainParticipant_create_topic(domain, 
				   "SimpleMsg", 
				   "SimpleMsg", 
				   DDS_TOPIC_QOS_DEFAULT, 
				   NULL, 
				   0 );
  if ( topic == NULL )
    {
      printf("ERROR creating topic.\n");
      return -1;
    }

  /* Create a DDS DataWriter on the simplemsg topic, 
   * with default QoS settings and no listeners.
   */
  dw = DDS_Publisher_create_datawriter(publisher,
				       topic, 
				       DDS_DATAWRITER_QOS_DEFAULT, 
				       NULL, 
				       0 );
  if (dw == NULL)
    {
      printf("ERROR creating data writer\n");
      return -1;
    }

  /* 
     Here, we could build a DynamicType instance
     manually, as in dyntype_pub_manual.c, or
     we can use the helper routine to build a 
     DynamicType instance from a data structure.

     - We will use the helper routine to show
       how it works.
  */

  /**********************************************/
  /*   construct sample and write it            */
  /**********************************************/
  simpleMsg.msg = "Hello WORLD from C!";
  simpleMsg.x = 100;
  simpleMsg.y = 3.1415;
  simpleMsg.z = 'A';
  INIT_SEQ(simpleMsg.bytes);
  seq_set_size(&simpleMsg.bytes, 10);
  seq_set_length(&simpleMsg.bytes, 10);
  for (i = 0; i<10; i++)
    simpleMsg.bytes._buffer[i] = 'a'+i;
  for (i = 0; i<10; i++)
    simpleMsg.shorts[i] = '1'+i;

  /* use helper routine to populate DynamicType sample from data structure */
  simplemsg_data = SimpleMsg_create_dynamictype(&simpleMsg);
  
  /* While forever, write a sample. */
  while ( !all_done )
    {
      DDS_ReturnCode_t ret;

      ret = DDS_DynamicTypeDataWriter_write ( dw, simplemsg_data, DDS_HANDLE_NIL ); 
      if ( ret != DDS_RETCODE_OK)
	{
	  printf("ERROR writing sample\n");
	  fflush(stdout);
	  return -1;
	}

      printf("wrote a sample\n");
      fflush(stdout);
      SLEEP(1);
    }

  /* Cleanup */
  DDS_DomainParticipant_delete_contained_entities(domain);
  DDS_DomainParticipantFactory_delete_participant(domain);

  return 0;
}
