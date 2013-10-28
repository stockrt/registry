/****************************************************************
 *
 *  file:  dyntype_pub_manual.c
 *  desc:  Provides a C publisher on the "SimpleMsg" topic.
 *         The datatype is constructed manually at run-time.
 *         This application requires no generated source code.
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
  DDS_DynamicType        simplemsg_dt;
  DDS_TypeDefinition     td  = NULL;
  DDS_TypeSupport        dts = NULL;
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
  
  /*********
   * Build a DynamicType structure representing our "SimpleMsg" data type 
   */
  simplemsg_dt = DDS_DynamicType_alloc_struct();
  DDS_DynamicType_set_num_fields(simplemsg_dt, 6);
  {
    DDS_DynamicType          temp_dt;
    
    temp_dt = DDS_DynamicType_alloc_string();
    DDS_DynamicType_set_field(simplemsg_dt, 0, "msg", temp_dt, 0);
    /* data->x: */
    temp_dt = DDS_DynamicType_alloc_basic(DDS_TYPECODE_LONG);
    DDS_DynamicType_set_field(simplemsg_dt, 1, "x", temp_dt, 0);
    /* data->y: */
    temp_dt = DDS_DynamicType_alloc_basic(DDS_TYPECODE_DOUBLE);
    DDS_DynamicType_set_field(simplemsg_dt, 2, "y", temp_dt, 0);
    /* data->z: */
    temp_dt = DDS_DynamicType_alloc_basic(DDS_TYPECODE_OCTET);
    DDS_DynamicType_set_field(simplemsg_dt, 3, "z", temp_dt, 0);
    /* data->bytes: */
    temp_dt = DDS_DynamicType_alloc_sequence();
    {
      DDS_DynamicType dt2;
      DDS_DynamicType_set_max_length(temp_dt, 10);
      dt2 = DDS_DynamicType_alloc_basic(DDS_TYPECODE_OCTET);
      DDS_DynamicType_set_element_type(temp_dt, dt2);
    }
    DDS_DynamicType_set_field(simplemsg_dt, 4, "bytes", temp_dt, 0);
    /* data->shorts: */
    {
      DDS_DynamicType dt2;
      temp_dt = DDS_DynamicType_alloc_array();
      DDS_DynamicType_set_max_length(temp_dt, 10);
      dt2 = DDS_DynamicType_alloc_basic(DDS_TYPECODE_SHORT);
      DDS_DynamicType_set_element_type(temp_dt, dt2);
    }
    DDS_DynamicType_set_field(simplemsg_dt, 5, "shorts", temp_dt, 0);
  }

  /* Construct a 'TypeDefinition' from the DynamicType */
  td       = DDS_create_type_definition_from_dynamictype(simplemsg_dt);

  DDS_DynamicType_free(simplemsg_dt); /* no longer needed */

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


  /**********************************************/
  /*   construct sample and write it            */
  /**********************************************/
  simplemsg_data = DDS_DynamicType_alloc_struct();
  DDS_DynamicType_set_num_fields(simplemsg_data, 6);
  {
     DDS_DynamicType dt1;
     /* data->msg: */
     dt1 = DDS_DynamicType_alloc_string();
     DDS_DynamicType_set_string(dt1, "Hello WORLD from C!");
     DDS_DynamicType_set_field(simplemsg_data, 0, "msg", dt1, 0);
     /* data->x: */
     dt1 = DDS_DynamicType_alloc_basic(DDS_TYPECODE_LONG);
     DDS_DynamicType_set_long(dt1, 100);
     DDS_DynamicType_set_field(simplemsg_data, 1, "x", dt1, 0);
     /* data->y: */
     dt1 = DDS_DynamicType_alloc_basic(DDS_TYPECODE_DOUBLE);
     DDS_DynamicType_set_double(dt1, 3.1415);
     DDS_DynamicType_set_field(simplemsg_data, 2, "y", dt1, 0);
     /* data->z: */
     dt1 = DDS_DynamicType_alloc_basic(DDS_TYPECODE_OCTET);
     DDS_DynamicType_set_octet(dt1, (unsigned char)'A');
     DDS_DynamicType_set_field(simplemsg_data, 3, "z", dt1, 0);
     /* data->bytes: */
     dt1 = DDS_DynamicType_alloc_sequence();
     DDS_DynamicType_set_length(dt1, 10);
     {
        DDS_DynamicType dt2;
        int i2;
        for (i2 = 0; i2 < 10; i2++) {
           /* data->bytes._buffer[i2]: */
           dt2 = DDS_DynamicType_alloc_basic(DDS_TYPECODE_OCTET);
           DDS_DynamicType_set_octet(dt2, 'a' + i2);
           DDS_DynamicType_set_element(dt1, i2, dt2);
        }
     }
     DDS_DynamicType_set_field(simplemsg_data, 4, "bytes", dt1, 0);
     /* data->shorts: */
     dt1 = DDS_DynamicType_alloc_array();
     DDS_DynamicType_set_length(dt1, 10);
     {
       int i2;
       DDS_DynamicType dt2;
       for (i2 = 0; i2 < 10; i2++) {
	 /* data->shorts[i2]: */
	 dt2 = DDS_DynamicType_alloc_basic(DDS_TYPECODE_SHORT);
	 DDS_DynamicType_set_short(dt2, '1' + i2);
	 DDS_DynamicType_set_element(dt1, i2, dt2);
       }
     }
     DDS_DynamicType_set_field(simplemsg_data, 5, "shorts", dt1, 0);
  }  

#if 0
  simpleMsg.msg = "Hello WORLD from C!";
  simpleMsg.x = 100;
  simpleMsg.y = 3.145;
  simpleMsg.z = 'A';
  INIT_SEQ(simpleMsg.bytes);
  seq_set_size(&simpleMsg.bytes, 10);
  seq_set_length(&simpleMsg.bytes, 10);
  for (i = 0; i<10; i++)
    simpleMsg.bytes._buffer[i] = 'a'+i;
  for (i = 0; i<10; i++)
    simpleMsg.shorts[i] = '1'+i;
#endif
  
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
  DDS_DynamicType_free(simplemsg_data);
  DDS_DomainParticipant_delete_contained_entities(domain);
  DDS_DomainParticipantFactory_delete_participant(domain);
  DDS_destroy_dynamic_typesupport(dts); /* typesupport must persist until all users are gone */
  return 0;
}
