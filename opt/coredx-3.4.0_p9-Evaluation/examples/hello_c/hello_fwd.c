/****************************************************************
 *
 *  file:  hello_fwd.c
 *  date:  Oct 23 2009
 *  desc:  Provides a simple C 'hello world' DDS forwarder.
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
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#endif
#include <dds/dds.h>
#include <dds/dds_builtin.h>
#include <dds/dds_builtinDataReader.h>
#include "hello.h"
#include "helloTypeSupport.h"
#include "helloDataReader.h"
#include "helloDataWriter.h"


/* DATA TYPES: */
typedef struct dds_receiver
{
  DDS_DomainParticipant dp;
  DDS_Subscriber        sub;
  DDS_DataReader        dr;
  DDS_Topic             top;
} dds_receiver;

typedef struct dds_transmitter
{
  DDS_DomainParticipant dp;
  DDS_Publisher         pub;
  DDS_DataWriter        dw;
  DDS_Topic             top;
} dds_transmitter;


/* GLOBAL VARIABLES: */
int               all_done = 0;
dds_receiver    * receiver    = NULL;
dds_transmitter * transmitter = NULL;


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
 	  StringMsg      * smsg = samples._buffer[i];
	  DDS_SampleInfo * si   = samples_info._buffer[i];
	  
	  switch (si->instance_state)
	    {
	    case DDS_ALIVE_INSTANCE_STATE:
	      printf(" re-transmit msg: %s\n", smsg->msg);
	      StringMsgDataWriter_write ( transmitter->dw, smsg, si->instance_handle ); 
	      break;
	    case DDS_NOT_ALIVE_NO_WRITERS_INSTANCE_STATE:
	    case DDS_NOT_ALIVE_DISPOSED_INSTANCE_STATE:
	      if (si->valid_data)
		StringMsgDataWriter_unregister_instance ( transmitter->dw, smsg, si->instance_handle ); 
	      break;
	    }
	}

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
 * DataReader Listener Method: dp_on_data_avail()
 *
 * This listener method is called when data is available to
 * be read on the Builtin DomainParticipant discovery reader.
 ****************************************************************/
void dp_on_data_avail(DDS_DataReader dr)
{
  DDS_DCPSParticipantPtrSeq      samples;
  DDS_SampleInfoSeq     samples_info;
  DDS_ReturnCode_t      retval;
  DDS_SampleStateMask   ss = DDS_NOT_READ_SAMPLE_STATE;
  DDS_ViewStateMask     vs = DDS_NEW_VIEW_STATE;
  DDS_InstanceStateMask is = DDS_ALIVE_INSTANCE_STATE;

  INIT_SEQ(samples);
  INIT_SEQ(samples_info);

  /* Take any and all available samples.  The take() operation
   * will remove the samples from the DataReader so they
   * won't be available on subsequent read() or take() calls.
   */
  retval = DDS_DCPSParticipantDataReader_read( dr, &samples, &samples_info, 
                                               DDS_LENGTH_UNLIMITED, 
                                               ss, vs, is );
  if ( retval == DDS_RETCODE_OK )
    {
      unsigned int i;
      
      /* iterrate through the samples */
      for ( i = 0;i < samples._length; i++)
	{
 	  DDS_DCPSParticipant      * smsg = samples._buffer[i];
	  DDS_SampleInfo           * si   = samples_info._buffer[i];
	  
	  if (si->valid_data)
	    {
              /* see if this is a 'forwarding' domain participant...
               * if so, we want to ignore them so we don't
               * re-forward data.
               */
	      int l = seq_get_length(&smsg->user_data.value);
              if (l>0)
                {
		  char * usr_data = malloc(l+1);
		  memcpy(usr_data, (char*)smsg->user_data.value._buffer, l);
		  usr_data[l] = '\0';
                  if (strncmp(usr_data, "forwarder", 9)==0)
                    {
                      /* ignore forwarded data... so we don't re-forward it */
                      retval = DDS_DomainParticipant_ignore_participant(receiver->dp, si->instance_handle);
                      if (retval != DDS_RETCODE_OK)
                        printf("ignore_participant(%p) failed.\n", (void*)si->instance_handle);
                    }
		  free(usr_data);
                }
	    }
	}

      /* read() and take() always "loan" the data, we need to
       * return it so CoreDX can release resources associated
       * with it.  
       */
      retval = DDS_DCPSParticipantDataReader_return_loan( dr, 
                                                          &samples, &samples_info );
      if ( retval != DDS_RETCODE_OK )
	printf("ERROR (%s): unable to return loan of samples\n",
	       DDS_error(retval));
    }
}

DDS_DataReaderListener dp_listener = 
{
  /* .on_requested_deadline_missed  */ NULL,
  /* .on_requested_incompatible_qos */ NULL,
  /* .on_sample_rejected            */ NULL,
  /* .on_liveliness_changed         */ NULL,
  /* .on_data_available             */ dp_on_data_avail,
  /* .on_subscription_matched       */ NULL,
  /* .on_sample_lost                */ NULL
};


/****************************************************************
 * 
 ****************************************************************/
dds_receiver * create_receiver(int          domain_id, 
			       const char * ip_addr, 
			       const char * topic_name)
{
  DDS_ReturnCode_t  err;
  dds_receiver    * retval = malloc(sizeof(dds_receiver));
  char              cdx_ip_addr[128];

  DDS_DomainParticipantFactoryQos dpf_qos;
  DDS_Subscriber bi_sub;
  DDS_DataReader bi_dp_reader;


  /* create a DDS_DomainParticipant */
  sprintf(cdx_ip_addr, "COREDX_IP_ADDR=%s", ip_addr);
  putenv(cdx_ip_addr);

  dpf_qos.entity_factory.autoenable_created_entities = 0;
  DDS_DomainParticipantFactory_set_qos(&dpf_qos);

  retval->dp = 
    DDS_DomainParticipantFactory_create_participant( domain_id, 
						     DDS_PARTICIPANT_QOS_DEFAULT,
						     NULL, 
						     0 );
  if ( retval->dp == NULL )
    {
      printf("ERROR creating domain participant.\n");
      free(retval);
      return NULL;
    }

  /* do this before DomainParticipant is enabled, so we don't miss anything: */
  {
    /* get builtin reader for Discovered DomainParticipants */
    bi_sub       = DDS_DomainParticipant_get_builtin_subscriber(retval->dp);
    bi_dp_reader = DDS_Subscriber_lookup_datareader(bi_sub, "DCPSParticipant");
    /* install listener to detect discovered participants */
    DDS_DataReader_set_listener(bi_dp_reader, &dp_listener, DDS_DATA_AVAILABLE_STATUS);
  }

  /* enable the domain participant */
  DDS_DomainParticipant_enable(retval->dp);

  printf("rx dp handle: %p\n", (void*)DDS_DomainParticipant_get_instance_handle(retval->dp));


  /* create a DDS_Subscriber */
  retval->sub
    = DDS_DomainParticipant_create_subscriber(retval->dp, 
					      DDS_SUBSCRIBER_QOS_DEFAULT,
					      NULL,
					      0 );
  if ( retval->sub == NULL )
    {
      printf("ERROR creating subscriber\n");
      free(retval);
      return NULL;
    }
  
  /* Register the data type with the CoreDX middleware. 
   * This is required before creating a Topic with
   * this data type. 
   */
  err = StringMsgTypeSupport_register_type( retval->dp, NULL );
  if (err != DDS_RETCODE_OK)
    {
      printf("ERROR (%s) registering type\n", DDS_error(err));
      free(retval);
      return NULL;
    }
  
  /* create a DDS Topic with the StringMsg data type. */
  retval->top = DDS_DomainParticipant_create_topic(retval->dp, 
						   topic_name,
						   "StringMsg", 
						   DDS_TOPIC_QOS_DEFAULT, 
						   NULL, 
						   0 );
  if ( ! retval->top )
    {
      printf("ERROR creating topic\n");
      free(retval);
      return NULL;
    }
  
  /* create a DDS_DataReader on the hello topic
   * with default QoS settings, and attach our 
   * listener with the on_data_available callback set.
   */
  retval->dr = DDS_Subscriber_create_datareader( retval->sub,
						 DDS_Topic_TopicDescription(retval->top), 
						 DDS_DATAREADER_QOS_DEFAULT,
						 &drListener, 
						 DDS_DATA_AVAILABLE_STATUS );
  if ( ! retval->dr )
    {
      printf("ERROR creating data reader\n");
      free(retval);
      return NULL;
    }

  return retval;
}


/****************************************************************
 * 
 ****************************************************************/
dds_transmitter * create_transmitter(int          domain_id, 
				     const char * ip_addr, 
				     const char * topic_name)
{
  char cdx_ip_addr[128];
  dds_transmitter * retval = malloc(sizeof(dds_transmitter));
  DDS_DomainParticipantQos dp_qos;


  /* create a DDS_DomainParticipant */
  sprintf(cdx_ip_addr, "COREDX_IP_ADDR=%s", ip_addr);
  putenv(cdx_ip_addr);
  DDS_DomainParticipantFactory_get_default_participant_qos(&dp_qos);
  octet_seq_copy(&dp_qos.user_data.value, "forwarder");
  retval->dp = 
    DDS_DomainParticipantFactory_create_participant( domain_id, 
                                                     &dp_qos, /* DDS_PARTICIPANT_QOS_DEFAULT,  */
                                                     NULL, 
                                                     0 );
  seq_clear(&dp_qos.user_data.value);
  if ( retval->dp == NULL )
    {
      printf("ERROR creating domain participant.\n");
      free(retval);
      return NULL;
    }

  
  DDS_DomainParticipant_enable(retval->dp);

  /* Create a DDS Publisher with the default QoS settings */
  retval->pub = 
    DDS_DomainParticipant_create_publisher(retval->dp, 
					   DDS_PUBLISHER_QOS_DEFAULT, 
					   NULL, 
					   0 );
  if ( retval->pub == NULL )
    {
      printf("ERROR creating publisher.\n");
      free(retval);
      return NULL;
    }
  
  /* Register the data type with the CoreDX middleware. 
   * This is required before creating a Topic with
   * this data type. 
   */
  if (StringMsgTypeSupport_register_type( retval->dp, NULL ) != DDS_RETCODE_OK)
    {
      printf("ERROR registering type\n");
      free(retval);
      return NULL;
    }
  
  /* create a DDS Topic with the StringMsg data type. */
  retval->top = 
    DDS_DomainParticipant_create_topic(retval->dp, 
				       topic_name,
				       "StringMsg", 
				       DDS_TOPIC_QOS_DEFAULT, 
				       NULL, 
				       0 );
  if ( retval->top == NULL )
    {
      printf("ERROR creating topic.\n");
      free(retval);
      return NULL;
    }

  /* Create a DDS DataWriter on the hello topic, 
   * with default QoS settings and no listeners.
   */
  retval->dw = DDS_Publisher_create_datawriter(retval->pub,
					       retval->top, 
					       DDS_DATAWRITER_QOS_DEFAULT, 
					       NULL, 
					       0 );
  if (retval->dw == NULL)
    {
      printf("ERROR creating data writer\n");
      free(retval);
      return NULL;
    }
  return retval;
}

/*****************************************************************
 *
 *****************************************************************/
void 
handle_sig(int sig)
{
  if (sig == SIGINT)
    all_done = 1;
}

/*****************************************************************
 *
 *****************************************************************/
static int
install_sig_handlers()
{
#ifdef _WIN32

#else
  struct sigaction int_action;
  int_action.sa_handler = handle_sig;
  sigemptyset(&int_action.sa_mask);
  sigaddset(&int_action.sa_mask, SIGINT);
  sigaddset(&int_action.sa_mask, SIGUSR1);
  sigaddset(&int_action.sa_mask, SIGPIPE);
  int_action.sa_flags     = 0;

  sigaction(SIGINT,  &int_action, NULL);
  sigaction(SIGUSR1, &int_action, NULL);
  sigaction(SIGPIPE, &int_action, NULL);

#endif
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
  DDS_ReturnCode_t         retval;
  /* default rx / tx parameters -- override on command line */
  int               rx_domainid = 123;
  int               tx_domainid = 123;
  char            * rx_ipaddr   = NULL;
  char            * tx_ipaddr   = NULL;
  char            * rx_topic    = strdup("helloTopic");
  char            * tx_topic    = strdup("helloTopic");
  int opt;


  install_sig_handlers();

  while ((opt = getopt(argc, argv, "i:o:a:b:c:d:")) != -1)
    {
      switch(opt)
	{
	case 'i': /* input domain id */
	  rx_domainid = atoi(optarg);
	  break;
	case 'a': /* input ip addr */
	  rx_ipaddr = strdup(optarg);
	  break;
	case 'c': /* input topic name */
	  free(rx_topic);
	  rx_topic = strdup(optarg);
	  break;
	case 'o': /* output domain id */
	  tx_domainid = atoi(optarg);
	  break;
	case 'b': /* output ip addr */
	  tx_ipaddr = strdup(optarg);
	  break;
	case 'd': /* input topic name */
	  free(tx_topic);
	  tx_topic = strdup(optarg);
	  break;
	case 'h':
	default:
	  goto usage;
	}
    }

  if ((rx_ipaddr == NULL) || (tx_ipaddr == NULL))
    {
    usage:
      printf("usage: %s [-i <input domain>] [-o <output domain>] -a <input ip addr> "
	     "-b <output ip addr> [-c <input topic name>] [-d <output topic name>]\n", 
	     argv[0]);
      return -1;
    }
    
  receiver    = create_receiver(rx_domainid, rx_ipaddr, rx_topic);
  transmitter = create_transmitter(tx_domainid, tx_ipaddr, tx_topic);

  if ((receiver==NULL)||(transmitter==NULL))
    return -1;

  /* Wait forever.  When data arrives at our DataReader, 
   * our dr_on_data_avilable method will be invoked.
   * -- we will then send it out on the transmitter...
   */
  while ( !all_done )
    {
#ifdef _WIN32
      Sleep(30000);
#else
      sleep(30);
#endif
    }
    
  /* Cleanup */
  printf("Cleaning up...\n");
  retval = DDS_DomainParticipant_delete_contained_entities(receiver->dp);
  if ( retval != DDS_RETCODE_OK )
    printf("ERROR (%s): Unable to cleanup DDS entities\n",
	   DDS_error(retval));
  retval = DDS_DomainParticipantFactory_delete_participant(receiver->dp);
  
  retval = DDS_DomainParticipant_delete_contained_entities(transmitter->dp);
  if ( retval != DDS_RETCODE_OK )
    printf("ERROR (%s): Unable to cleanup DDS entities\n",
	   DDS_error(retval));
  retval = DDS_DomainParticipantFactory_delete_participant(transmitter->dp);
  

  free(receiver);
  free(transmitter);
  free(rx_ipaddr);
  free(tx_ipaddr);
  free(rx_topic);
  free(tx_topic);

  return 0;
}
