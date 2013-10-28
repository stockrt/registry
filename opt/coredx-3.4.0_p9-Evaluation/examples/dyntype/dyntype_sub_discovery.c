/****************************************************************
 *
 *  file:  dyntype_sub_discovery.c
 *  desc:  Provides a simple C DynamicType subscriber.
 *         This subscribing application will 
 *          1) wait until it discovers a writer publishing 
 *             on a topic named "SimpleMsg",
 *          2) construct a DynamicType based on the discovered 
 *              data type, 
 *          3) receive and print data on the "SimpleMsg" topic 
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
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <dds/dds.h>
#include <dds/dds_dtype.h>
#include <dds/dds_builtin.h>
#include <dds/dds_builtinDataReader.h>
#include <dds/toc_util.h>

#ifdef _WIN32
#  define SLEEP(d) Sleep((d)*1000)
#else
#  define SLEEP(d) sleep((d))
#endif

int all_done = 0;


/*************************************************************/
/*************************************************************/
static char * typecode_str[] = 
  {
    "NULL",
    "SHORT",
    "LONG",
    "USHORT",
    "ULONG",
    "FLOAT",
    "DOUBLE",
    "BOOLEAN",
    "CHAR",
    "OCTET",
    "STRUCT",
    "UNION",
    "ENUM",
    "STRING",
    "SEQUENCE",
    "ARRAY",
    "ALIAS",
    "LONGLONG",
    "ULONGLONG",
    "LONGDOUBLE",
    "WCHAR",
    "WSTRING",
    "VALUE",
    "VALUE_PARAM"
  };

/*****************************************************************
 * 
 *****************************************************************/
static void dtype_print(struct _DynamicType * dt, 
			int indent)
{
  int i;
  DDS_TypeCodeKind dtype;
  char * spaces = calloc(indent*2+1, 1);
  memset(spaces, ' ', indent*2);

  dtype = DDS_DynamicType_get_type(dt);
  switch (dtype)
    {
    case DDS_TYPECODE_STRUCT:
      {
        fprintf(stderr, "%sSTRUCT:\n", spaces);
        for (i = 0;i < DDS_DynamicType_get_num_fields(dt); i++)
	  {
	    fprintf(stderr,"%s%s :: \n", spaces, DDS_DynamicType_get_field_name(dt, i));
	    dtype_print(DDS_DynamicType_get_field(dt,i), indent+2);
	  }
      }
      break;
    case DDS_TYPECODE_UNION:
      {
        fprintf(stderr, "%sUNION:\n", spaces);
        for (i = 0;i < DDS_DynamicType_get_num_fields(dt); i++)
          dtype_print(DDS_DynamicType_get_field(dt, i), indent+2);
      }
      break;

    case DDS_TYPECODE_SEQUENCE:
      {
        fprintf(stderr, "%sSEQUENCE[%d]:\n", spaces, DDS_DynamicType_get_length(dt));
	fprintf(stderr, "%s  {\n", spaces);
	for (i = 0; i<DDS_DynamicType_get_length(dt); i++)
	  dtype_print(DDS_DynamicType_get_element(dt, i), indent+4);
	fprintf(stderr, "%s  }\n", spaces);
      }
      break;
    case DDS_TYPECODE_ARRAY:
      {
        fprintf(stderr, "%sARRAY[%d]:\n", spaces, DDS_DynamicType_get_length(dt));
	fprintf(stderr, "%s  {\n", spaces);
	for (i = 0; i<DDS_DynamicType_get_length(dt); i++)
	  dtype_print(DDS_DynamicType_get_element(dt, i), indent+4);
	fprintf(stderr, "%s  }\n", spaces);
      }
      break;

    case DDS_TYPECODE_STRING:
      {
        fprintf(stderr, "%sSTRING[%d]: '%s'\n", spaces, DDS_DynamicType_get_max_length(dt), 
		DDS_DynamicType_get_string(dt));
      }
      break;

    case DDS_TYPECODE_SHORT:
      fprintf(stderr, "%s%s: %d\n", spaces, typecode_str[dtype], (int)DDS_DynamicType_get_short(dt));
      break;

    case DDS_TYPECODE_LONG:
    case DDS_TYPECODE_ENUM:
      fprintf(stderr, "%s%s: %d\n", spaces, typecode_str[dtype], DDS_DynamicType_get_long(dt));
      break;

    case DDS_TYPECODE_LONGLONG:
      fprintf(stderr, "%s%s: %ld\n", spaces, typecode_str[dtype], DDS_DynamicType_get_longlong(dt));
      break;

    case DDS_TYPECODE_USHORT:
    case DDS_TYPECODE_ULONG:
    case DDS_TYPECODE_ULONGLONG:
      fprintf(stderr, "%s%s: 0x%.2x\n", spaces, typecode_str[dtype], DDS_DynamicType_get_octet(dt));
      break;

    case DDS_TYPECODE_FLOAT:
    case DDS_TYPECODE_DOUBLE:
      fprintf(stderr, "%s%s: %f\n", spaces, typecode_str[dtype], DDS_DynamicType_get_double(dt));
      break;

    case DDS_TYPECODE_BOOLEAN:
    case DDS_TYPECODE_CHAR:
    case DDS_TYPECODE_OCTET:
      fprintf(stderr, "%s%s: 0x%.2x\n", spaces, typecode_str[dtype], DDS_DynamicType_get_octet(dt));
      break;

    case DDS_TYPECODE_ALIAS:
    case DDS_TYPECODE_VALUE:
    case DDS_TYPECODE_VALUE_PARAM:
    case DDS_TYPECODE_WCHAR:
    case DDS_TYPECODE_WSTRING:
    case DDS_TYPECODE_LONGDOUBLE:
    default:
      fprintf(stderr, "%s%s (UNHANDLED!)\n", spaces, typecode_str[dtype]);
      break;
    }
  free(spaces);
}

/****************************************************************
 * DataReader Listener Method: dr_on_data_avail()
 *
 * This listener method is called when data is available to
 * be read on this DataReader.
 ****************************************************************/
void dr_on_data_avail(DDS_DataReader dr)
{
  DDS_DynamicTypePtrSeq  samples;
  DDS_SampleInfoSeq      samples_info;
  DDS_ReturnCode_t       retval;
  DDS_SampleStateMask    ss = DDS_ANY_SAMPLE_STATE;
  DDS_ViewStateMask      vs = DDS_ANY_VIEW_STATE;
  DDS_InstanceStateMask  is = DDS_ANY_INSTANCE_STATE;
  
  INIT_SEQ(samples);
  INIT_SEQ(samples_info);
  
  /* Take any and all available samples.  The take() operation
   * will remove the samples from the DataReader so they
   * won't be available on subsequent read() or take() calls.
   */
  retval = DDS_DynamicTypeDataReader_take( dr, &samples, &samples_info, 
					   DDS_LENGTH_UNLIMITED, 
					   ss, vs, is );
  
  if ( retval == DDS_RETCODE_OK )
    {
      unsigned int i;

      for ( i = 0;i < samples._length; i++)
	{
 	  DDS_DynamicType  mtp  = samples._buffer[i];
	  DDS_SampleInfo * si   = samples_info._buffer[i];
	  
	  printf("  SAMPLE %d\n", i);
	  printf("  SAMPLE_INFO %d\n", i);
	  printf("   instance         pub              sample  gen    abs     disp   no wr   time                  sample   view       instance\n");
	  printf("   handle           handle           rank    rank   genrnk  genct  genct   stamp                 state    state      state\n");
	  printf("   %16p %16p %.5d   %.5d  %.5d   %.5d  %.5d  %11.11ul.%9.9d  %-8s %-8s   %-s\n",
		 (void*)si->instance_handle,
		 (void*)si->publication_handle,
		 si->sample_rank,
		 si->generation_rank,
		 si-> absolute_generation_rank,
		 si-> disposed_generation_count,
		 si-> no_writers_generation_count,
		 si-> source_timestamp.sec,
		 si-> source_timestamp.nanosec,
		 DDS_sample_state(si-> sample_state),
		 DDS_view_state(si-> view_state),
		 DDS_instance_state(si-> instance_state));
	  
	  if ( si->valid_data)
	    {
	      dtype_print(mtp, 4);
	    }
	  else
	    printf("     <no sample data>\n");
	}
      
      /* read() and take() always "loan" the data, we need to
       * return it so CoreDX can release resources associated
       * with it.  
       */
      retval = DDS_DynamicTypeDataReader_return_loan( dr, &samples, &samples_info );
      if ( retval != DDS_RETCODE_OK )
	printf("ERROR (%s): unable to return loan of samples\n",
	       DDS_error(retval));
    }
  else
    {
      printf("ERROR (%s) taking samples from DataReader\n",
	     DDS_error(retval));
    }
  fflush(stdout);
}

/****************************************************************
 * Create a DataReaderListener with a pointer to the function 
 * that should be called on data_available events.  All other 
 * function pointers are NULL. (no listener method defined).
 ****************************************************************/
DDS_DataReaderListener drListener = 
{
  /* .on_requested_deadline_missed  */ NULL,
  /* .on_requested_incompatible_qos */ NULL,
  /* .on_sample_rejected            */ NULL,
  /* .on_liveliness_changed         */ NULL,
  /* .on_data_available             */ dr_on_data_avail,
  /* .on_subscription_matched       */ NULL,
  /* .on_sample_lost                */ NULL
};


/****************************************************************
 * main()
 *
 * Perform CoreDX DDS setup activities:
 *   - create a Domain Participant
 *   - create a Subscriber
 *   - Use discovery to get data type
 *   - register the Dynamic StringMsg data type
 *   - create a Topic
 *   - create a DataReader and attach the listener created above
 *   - wait for data
 ****************************************************************/

#if defined(__vxworks) && !defined(__RTP__)
int dyntype_sub_discovery(char * args)
#else
int main(int argc, char * argv[])
#endif
{
  DDS_DomainParticipant    domain;
  DDS_Subscriber           subscriber;
  DDS_Topic                topic;
  DDS_DataReader           dr;
  DDS_Subscriber           bi_subscriber;
  DDS_DataReader           bi_pub_reader;
  int                      found  = 0;
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

  /* create a DDS_Subscriber */
  subscriber
    = DDS_DomainParticipant_create_subscriber(domain, 
					      DDS_SUBSCRIBER_QOS_DEFAULT,
					      NULL,
					      0 );
  if ( subscriber == NULL )
    {
      printf("ERROR creating subscriber\n");
      return -1;
    }
  

  /* Get access to the built-in "publication" data reader
   * so we can learn about discovered writers
   */
  bi_subscriber = DDS_DomainParticipant_get_builtin_subscriber(domain);
  bi_pub_reader = DDS_Subscriber_lookup_datareader(bi_subscriber, "DCPSPublication");
  if (bi_pub_reader == NULL)
    {
      printf("Failed to get built-in reader for PUBLICATIONS topic!\n");
      return -1;
    }

  /* Now, we wait to discover a writer on the "SimpleMsg" topic
   */
  while(!found)
    {
      DDS_DCPSPublicationPtrSeq  writers;
      DDS_SampleInfoSeq          samples_info;
      DDS_ReturnCode_t           retval;
      
      toc_sleep(USEC_PER_SEC);

      INIT_SEQ(writers); 
      INIT_SEQ(samples_info);
      retval = DDS_DCPSPublicationDataReader_read ( bi_pub_reader, &writers, &samples_info, 
						    DDS_LENGTH_UNLIMITED, 
						    DDS_ANY_SAMPLE_STATE, 
						    DDS_NEW_VIEW_STATE,
						    DDS_ALIVE_INSTANCE_STATE);
      if (retval == DDS_RETCODE_OK)
	{
	  unsigned int i;
	  for ( i = 0;i < writers._length; i++)
	    {
	      DDS_DCPSPublication * pub  = writers._buffer[i];
	      DDS_SampleInfo      * si   = samples_info._buffer[i];

	      DDS_TypeDefinition       td  = NULL;
	      DDS_TypeSupport          dts = NULL;
	      
	      if ( si->valid_data)
		{
		  if (strcmp(pub->topic_name, "SimpleMsg") == 0) /* found one! */
		    {
		      if (pub->typecode.value._length>0) /* make sure the writer published its 'TypeCode' */
			{
			  printf("Found a writer, creating DynamicTypeTypeSupport.\n");
			  found    = 1;

			  /* Now, we construct a DynamicTypeTypeSupport to handle the discovered data type */
			  td       = DDS_create_type_definition(pub->typecode.value._buffer, 
								pub->typecode.value._length, 
								pub->typecode.encoding);
			  dts      = DDS_create_dynamic_typesupport( td ); 

			  /* Register the new  TypeSupport */
			  DDS_DomainParticipant_register_type(domain, dts, pub->type_name);
			  
			  /* Create a Topic associated based on that type */
			  topic = DDS_DomainParticipant_create_topic(domain, pub->topic_name, pub->type_name, 
								     DDS_TOPIC_QOS_DEFAULT, NULL, 0 );
			  if ( ! topic )
			    {
			      printf("DP_create_topic() failed.\n");
			      return -1;
			    }
			  
			  /* create a DynamicTypeDataReader */
			  dr = DDS_Subscriber_create_datareader( subscriber, 
								 DDS_Topic_TopicDescription(topic), 
								 DDS_DATAREADER_QOS_DEFAULT, 
								 &drListener, 
								 DDS_DATA_AVAILABLE_STATUS );
			  if (dr == NULL)
			    {
			      fprintf(stderr, "Failed to create DataReader<%s>...\n", pub->topic_name);
			      return -1;
			    }
			}
		      else
			{
			  printf("Error: Found a writer on the \"SimpleMsg\" topic, but the\n");
			  printf("writer did not advertise a TypeCode. Can't create a DynamicType.\n");
			}
		    }
		}
	      
	      DDS_DCPSPublicationDataReader_return_loan( bi_pub_reader, &writers, &samples_info);
	    }
	}
    }
  
  printf("reading samples...\n");
  fflush(stdout);

  /* Wait forever.  When data arrives at our DataReader, 
   * our dr_on_data_avilalbe method will be invoked.
   */
  while ( !all_done )
    SLEEP(1);
    
  /* Cleanup */
  DDS_DomainParticipant_delete_contained_entities(domain);
  DDS_DomainParticipantFactory_delete_participant(domain);

  return 0;
}
