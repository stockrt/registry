/****************************************************************
 *
 *  file:  filter_sub.c
 *  desc:  Provides a simple C 'filter world' DDS subscriber.
 *         This subscribing application will receive data
 *         from the example 'filter world' publishing 
 *         application (filter_pub).
 * 
 ****************************************************************
 *
 *   Coypright(C) 2007-2011 Twin Oaks Computing, Inc
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
#include "filter.h"
#include "filterTypeSupport.h"
#include "filterDataReader.h"

#ifdef _WIN32
#  define SLEEP(d) Sleep((d)*1000)
#else
#  define SLEEP(d) sleep((d))
#endif

int all_done = 0;

/****************************************************************
 * DataReader Listener Method: dr_on_data_avail()
 *
 * This listener method is called when data is available to
 * be read on this DataReader.
 ****************************************************************/
void dr_on_data_avail(DDS_DataReader dr)
{
  FilterMsgPtrSeq       samples;
  DDS_SampleInfoSeq     samples_info;
  DDS_ReturnCode_t      retval;
  DDS_SampleStateMask   ss = DDS_ANY_SAMPLE_STATE;
  DDS_ViewStateMask     vs = DDS_ANY_VIEW_STATE;
  DDS_InstanceStateMask is = DDS_ANY_INSTANCE_STATE;

  INIT_SEQ(samples);
  INIT_SEQ(samples_info);

  /* Take any and all available samples.  The take() operation
   * will remove the samples from the DataReader so they
   * won't be available on subsequent read() or take() calls.
   */
  retval = FilterMsgDataReader_take( dr, &samples, &samples_info, 
				     DDS_LENGTH_UNLIMITED, 
				     ss, 
				     vs, 
				     is );
  if ( retval == DDS_RETCODE_OK )
    {
      unsigned int i;
      
      /* iterrate through the samples */
      for ( i = 0;i < samples._length; i++)
	{
 	  FilterMsg * smsg    = samples._buffer[i];
	  DDS_SampleInfo * si = samples_info._buffer[i];
	  
	  /* If this sample does not contain valid data,
	   * it is a dispose or other non-data command,
	   * and, accessing any member from this sample 
	   * would be invalid.
	   */
	  if ( si->valid_data)
	    printf("Sample Received:  msg: %s x: %d count: %d\n",
		   smsg->msg, smsg->x, smsg->count);
	}
      fflush(stdout);

      /* read() and take() always "loan" the data, we need to
       * return it so CoreDX can release resources associated
       * with it.  
       */
      retval = FilterMsgDataReader_return_loan( dr, 
						&samples, &samples_info );
      if ( retval != DDS_RETCODE_OK )
	printf("ERROR (%s): unable to return loan of samples\n",
	       DDS_error(retval));
    }
  else
    {
      printf("ERROR (%s) taking samples from DataReader\n",
	     DDS_error(retval));
    }
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
 *   - register the FilterMsg data type
 *   - create a Topic
 *   - create a DataReader and attach the listener created above
 * And wait for data
 ****************************************************************/

#if defined(__vxworks) && !defined(__RTP__)
int filter_sub(char * args)
#else
int main(int argc, char * argv[])
#endif
{
  DDS_DomainParticipant    domain;
  DDS_Subscriber           subscriber;
  DDS_Topic                topic;
  DDS_ContentFilteredTopic filtered_topic;
  DDS_StringSeq            filter_params;
  DDS_DataReader           dr;
  DDS_ReturnCode_t         retval;
  int                      count = 0;

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
  
  /* Register the data type with the CoreDX middleware. 
   * This is required before creating a Topic with
   * this data type. 
   */
  retval = FilterMsgTypeSupport_register_type( domain, NULL );
  if (retval != DDS_RETCODE_OK)
    {
      printf("ERROR (%s) registering type\n", DDS_error(retval));
      return -1;
    }
  
  /* create a DDS Topic with the FilterMsg data type. */
  topic = DDS_DomainParticipant_create_topic(domain, 
					     "TestTopic", 
					     "FilterMsg", 
					     DDS_TOPIC_QOS_DEFAULT, 
					     NULL, 
					     0 );
  if ( ! topic )
    {
      printf("ERROR creating topic\n");
      return -1;
    }
  

  /* create a CONTENT_FILTERED TOPIC associated with 'filterTopic' */
  filtered_topic = DDS_DomainParticipant_create_contentfilteredtopic(domain,
								     "FilteredTestTopic", 
								     topic,   
								     "x>%0 and x<%1", 
								     NULL);
  if (filtered_topic == NULL)
    printf("FAILED to create ContentFilteredTopic!\n");

  /* install paramters into the filter
     -- these values are used in the '%0' and '%1' fields 
        specified in the filter string "x>%0 && x<%1"
     -- this also allows the filter to be modified dynamically
        on the fly
     -- alternatively, the values could have been specified
        directly in the query string, like: "x>5 && x<10"
  */
  INIT_SEQ(filter_params);
  seq_set_size(&filter_params, 2);
  seq_set_length(&filter_params, 2);
  filter_params._buffer[0] = "0";
  filter_params._buffer[1] = "5";
  DDS_ContentFilteredTopic_set_expression_parameters(filtered_topic, &filter_params);
  printf("Set Filter: x>0 and x<5\n");
  
  /* create a DDS_DataReader on the filter topic
   * with default QoS settings, and attach our 
   * listener with the on_data_available callback set.
   */
  dr = DDS_Subscriber_create_datareader( subscriber,
					 DDS_ContentFilteredTopic_TopicDescription(filtered_topic), 
					 DDS_DATAREADER_QOS_DEFAULT,
					 &drListener, 
					 DDS_DATA_AVAILABLE_STATUS );
  if ( ! dr )
    {
      printf("ERROR creating data reader\n");
      return -1;
    }

  /* Wait forever.  When data arrives at our DataReader, 
   * our dr_on_data_avilalbe method will be invoked.
   */
  while ( !all_done )
    {
      count++;

      /* periodically, change the filter paramters to show dynamic nature of filters */
      if ((count%10)==0) /* multiple of 10 */
	{
	  filter_params._buffer[0] = "0";
	  filter_params._buffer[1] = "5";
	  DDS_ContentFilteredTopic_set_expression_parameters(filtered_topic, &filter_params);
	  printf("Set Filter: x>0 and x<5\n");
	}
      else if ((count%5)==0) /* multiple of 5 */
	{
	  filter_params._buffer[0] = "5";
	  filter_params._buffer[1] = "10";
	  DDS_ContentFilteredTopic_set_expression_parameters(filtered_topic, &filter_params);
	  printf("Set Filter: x>5 and x<10\n");
	}
      fflush(stdout); 
      SLEEP(1);
    }
    
  /* Cleanup */
  DDS_DomainParticipant_delete_contained_entities(domain);
  DDS_DomainParticipantFactory_delete_participant(domain);

  return 0;
}
