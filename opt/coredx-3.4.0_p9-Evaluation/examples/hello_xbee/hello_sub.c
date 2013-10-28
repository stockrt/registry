/****************************************************************
 *
 *  file:  hello_sub.c
 *  date:  Mar 6 2008
 *  desc:  Provides a simple C 'hello world' DDS subscriber.
 *         This subscribing application will receive data
 *         from the example 'hellow world' publishing 
 *         application (hello_pub).
 * 
 ****************************************************************
 *
 *   Coypright(C) 2008-2011 Twin Oaks Computing, Inc
 *   All rights reserved.   Castle Rock, CO 80108
 *
 *****************************************************************
 *
 *  This file is provided by Twin Oaks Computing, Inc (TOC Inc)
 *  as an example. It is provided in the hope that it will be 
 *  useful but WITHOUT ANY WARRANTY; without even the implied 
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
 *  PURPOSE. TOC Inc assumes no liability or responsibilty for 
 *  the use of this information for any purpose.
 *  
 ****************************************************************/
/****************************************************************
 *
 *  updated: June 2009 -- cleanup
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
#include <dds/coredx_transport.h>
#include <dds/toc_util.h>
#include "hello.h"
#include "helloTypeSupport.h"
#include "helloDataReader.h"

int            all_done    = 0;
unsigned char  print_stats = 0;
int            count       = 0;
int            count_total = 0;
char         * transport_cfg = NULL;
unsigned char  reliable      = 0;
int            history_depth = 1;

/****************************************************************
 * DataReader Listener Method: dr_on_data_avail()
 *
 * This listener method is called when data is available to
 * be read on this DataReader.
 ****************************************************************/
void dr_on_data_avail(DDS_DataReader dr)
{
  StringMsgPtrSeq       samples;
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
  retval = StringMsgDataReader_take( dr, &samples, &samples_info, 
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
 	  StringMsg * smsg    = samples._buffer[i];
	  DDS_SampleInfo * si = samples_info._buffer[i];
	  
	  /* If this sample does not contain valid data,
	   * it is a dispose or other non-data command,
	   * and, accessing any member from this sample 
	   * would be invalid.
	   */
	  if ( si->valid_data)
	    {
	      if (!print_stats)
		printf("Sample Received:  msg %d = %s\n", i, smsg->msg);
	      count ++;
	    }
	}
      fflush(stdout);

      /* read() and take() always "loan" the data, we need to
       * return it so CoreDX can release resources associated
       * with it.  
       */
      retval = StringMsgDataReader_return_loan( dr, 
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
 *
 ****************************************************************/
int
parse_args(int argc, char * argv[])
{
  int opt;

  while ((opt = toc_getopt(argc, argv, "k:rst:h")) != -1) 
    {
      switch (opt) 
	{
	case 'k':
	  history_depth = atoi(toc_optarg);
	  break;
	case 'r':
	  reliable = 1;
	  break;
	case 's':
	  print_stats = 1;
	  break;
	case 't':
	  transport_cfg = strdup(toc_optarg);
	  break;
	case 'h':
	default:
	  fprintf(stderr, "Usage: %s \n", argv[0]);
	  fprintf(stderr, "           -k <history>         : (int depth) keep history depth (0=>KEEP_ALL).\n");
	  fprintf(stderr, "           -r                   : RELIABLE (default = BEST_EFFORT)\n");
	  fprintf(stderr, "           -s                   : print per second statistics\n");
	  fprintf(stderr, "           -t <transport>       : transport configuration\n");
	  exit(-1);
	}
    }
  return 0;
}

/****************************************************************
 * main()
 *
 * Perform CoreDX DDS setup activities:
 *   - create a Domain Participant
 *   - create a Subscriber
 *   - register the StringMsg data type
 *   - create a Topic
 *   - create a DataReader and attach the listener created above
 * And wait for data
 ****************************************************************/
int main(int argc, char * argv[])
{
  DDS_DomainParticipant    domain;
  DDS_Subscriber           subscriber;
  DDS_Topic                topic;
  DDS_DataReader           dr;
  DDS_ReturnCode_t         retval;
  DDS_DomainParticipantFactoryQos dpf_qos;
  DDS_DomainParticipantQos dp_qos;
  DDS_DataReaderQos        dr_qos;
  CoreDX_Transport       * tport;

  all_done = 0;

  parse_args(argc, argv);

  /* instruct factory to NOT automatically enable the DomainParticipant */
  retval = DDS_DomainParticipantFactory_get_qos( &dpf_qos);
  dpf_qos.entity_factory.autoenable_created_entities = 0;
  retval = DDS_DomainParticipantFactory_set_qos( &dpf_qos);

  DDS_DomainParticipantFactory_get_default_participant_qos(&dp_qos);
  /* reduce the frequency of DPD multicast messages */
  dp_qos.discovery.dpd_push_period.sec              = 30;
  dp_qos.discovery.dpd_push_period.nanosec          = 0;
  dp_qos.discovery.dpd_lease_duration.sec           = 240;
  dp_qos.discovery.dpd_lease_duration.nanosec       = 0;
  dp_qos.discovery.heartbeat_period.sec             = 2;

  /* reduce the frequency of HB/ACK messages */
  dp_qos.discovery.heartbeat_period.sec             = 4;
  dp_qos.discovery.heartbeat_period.nanosec         = 0;
  dp_qos.discovery.nack_response_delay.sec          = 0;
  dp_qos.discovery.nack_response_delay.nanosec      = NSEC_PER_SEC;
  dp_qos.discovery.nack_suppress_delay.sec          = 0;
  dp_qos.discovery.nack_suppress_delay.nanosec      = 0;
  dp_qos.discovery.heartbeat_response_delay.sec     = 0;
  dp_qos.discovery.heartbeat_response_delay.nanosec = NSEC_PER_SEC;
  dp_qos.discovery.send_initial_nack                = 0;

  DDS_DomainParticipantFactory_set_default_participant_qos(&dp_qos);
  
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

  if (transport_cfg)
    {
      /* create, configure and install the serial transport */
      tport = __coredx_uds_transport.alloc();
      tport->init(tport, transport_cfg);
      DDS_DomainParticipant_add_transport(domain, tport);
    }

  /* enable the DomainParticipant */
  DDS_DomainParticipant_enable(domain);

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
  retval = StringMsgTypeSupport_register_type( domain, NULL );
  if (retval != DDS_RETCODE_OK)
    {
      printf("ERROR (%s) registering type\n", DDS_error(retval));
      return -1;
    }
  
  /* create a DDS Topic with the StringMsg data type. */
  topic = DDS_DomainParticipant_create_topic(domain, 
					     "helloTopic", 
					     "StringMsg", 
					     DDS_TOPIC_QOS_DEFAULT, 
					     NULL, 
					     0 );
  if ( ! topic )
    {
      printf("ERROR creating topic\n");
      return -1;
    }
  
  /* create a DDS_DataReader on the hello topic
   * with default QoS settings, and attach our 
   * listener with the on_data_available callback set.
   */
  DDS_Subscriber_get_default_datareader_qos(subscriber, &dr_qos);
  if (reliable)
    dr_qos.reliability.kind = DDS_RELIABLE_RELIABILITY_QOS;
  if (history_depth == 0)
    dr_qos.history.kind = DDS_KEEP_ALL_HISTORY_QOS;
  else
    {
      dr_qos.history.kind = DDS_KEEP_LAST_HISTORY_QOS;
      dr_qos.history.depth = history_depth;
    }
  dr_qos.latency_budget.duration.sec     = 0;
  dr_qos.latency_budget.duration.nanosec = NSEC_PER_MSEC * 50;
  dr = DDS_Subscriber_create_datareader( subscriber,
					 DDS_Topic_TopicDescription(topic), 
					 /* DDS_DATAREADER_QOS_DEFAULT, */
					 &dr_qos,
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
      toc_sleep(USEC_PER_SEC);
      if (print_stats)
	{
	  count_total += count;
	  printf("received:  per_sec: %5d   total: %d  \n", count, 
		 count_total);
	  count = 0;
	}
    }
    
  /* Cleanup */
  DDS_DomainParticipant_delete_contained_entities(domain);
  DDS_DomainParticipantFactory_delete_participant(domain);

  return 0;
}
