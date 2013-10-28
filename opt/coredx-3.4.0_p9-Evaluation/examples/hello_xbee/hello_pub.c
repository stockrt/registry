/****************************************************************
 *
 *  file:  hello_pub.c
 *  date:  Mar 6 2008
 *  desc:  Provides a simple C 'hello world' DDS publisher.
 *         This publishing application will send data
 *         to the example 'hellow world' subscribing 
 *         application (hello_sub).  Supports UDS transport
 *         for serial communication.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <dds/dds.h>
#include <dds/toc_util.h>
#include <dds/coredx_transport.h>

#include "hello.h"
#include "helloTypeSupport.h"
#include "helloDataWriter.h"


int            all_done = 0;
int            tdelay    = USEC_PER_SEC; /* loop delay in usec */
unsigned char  verbose = 0;
char         * transport_cfg = NULL;
char         * string_msg    = NULL;
int            test_length   = 60; /* seconds */
unsigned char  reliable      = 0;
int            history_depth = 1;
int            num_written   = 0;

/****************************************************************
 *
 ****************************************************************/
int
parse_args(int argc, char * argv[])
{
  int opt;

  while ((opt = toc_getopt(argc, argv, "d:k:l:m:rt:u:vh")) != -1) 
    {
      switch (opt) 
	{
	case 'd': 
	  tdelay    = atoi(toc_optarg) * USEC_PER_MSEC;
	  break;
	case 'k':
	  history_depth = atoi(toc_optarg);
	  break;
	case 'l':
	  test_length = atoi(toc_optarg);
	  break;
	case 'm':
	  string_msg = strdup(toc_optarg);
	  break;
	case 'r':
	  reliable = 1;
	  break;
	case 't':
	  transport_cfg = strdup(toc_optarg);
	  break;
	case 'v':
	  verbose = 1;
	  break;
	case 'h':
	default:
	  fprintf(stderr, "Usage: %s \n", argv[0]);
	  fprintf(stderr, "           -d <msec>            : (int default: 1000 (1sec)) loop delay in milli seconds\n");
	  fprintf(stderr, "           -l <length>          : (int seconds) length of run.\n");
	  fprintf(stderr, "           -h <history>         : (int depth) keep history depth (0=>KEEP_ALL).\n");
	  fprintf(stderr, "           -m <message>         : (string) message to send.\n");
	  fprintf(stderr, "           -r                   : RELIABLE (default = BEST_EFFORT)\n");
	  fprintf(stderr, "           -t <transport>       : configuration for UDS transport\n");
	  fprintf(stderr, "           -v                   : verbose\n");
	  fprintf(stderr, "                                :                \n");
	  exit(-1);
	}
    }
  return 0;
}

/****************************************************************
 *
 ****************************************************************/
unsigned char 
test_time_elapsed(struct timespec * start )
{
  struct timespec nowts;
  double test_end;
  double now;

  clock_gettime(CLOCK_REALTIME, &nowts);

  test_end = start->tv_sec + ((double)start->tv_nsec / NSEC_PER_SEC) + test_length;
  now      = nowts.tv_sec + ((double)nowts.tv_nsec / NSEC_PER_SEC);

  if ( now >= test_end)
    return 1;
  else
    return 0;
}

#if !defined(_WIN32)
/*****************************************************************
 *
 *****************************************************************/
void 
handle_sig(int sig)
{
  if (sig == SIGINT)
    {
      all_done = 1;
    }
}

/*****************************************************************
 *
 *****************************************************************/
static int
install_sig_handlers()
{
  struct sigaction int_action;
  sigset_t         ssnew;
  int_action.sa_handler = handle_sig;
  sigemptyset(&int_action.sa_mask);
  sigaddset(&int_action.sa_mask, SIGINT);
  sigaddset(&int_action.sa_mask, SIGUSR1);
  sigaddset(&int_action.sa_mask, SIGPIPE);
  int_action.sa_flags     = 0;

  sigaction(SIGINT,  &int_action, NULL);
  sigaction(SIGUSR1, &int_action, NULL);
  sigaction(SIGPIPE, &int_action, NULL);

  sigemptyset(&ssnew);
  sigaddset(&ssnew, SIGINT);
  sigaddset(&ssnew, SIGUSR1);
  sigaddset(&ssnew, SIGPIPE);
  pthread_sigmask(SIG_UNBLOCK, &ssnew, NULL);

  return 0;
}
#endif

/****************************************************************
 * main()
 *
 * Perform CoreDX DDS setup activities:
 *   - create a Domain Participant
 *   - create a Publisher
 *   - register the StringMsg data type
 *   - create a Topic
 *   - create a DataWriter 
 * Write data
 ****************************************************************/
int main(int argc, char * argv[])
{
  DDS_DomainParticipant  domain;
  DDS_Publisher          publisher;
  DDS_Topic              topic;
  DDS_DataWriter         dw;
  StringMsg              stringMsg;
  DDS_DomainParticipantFactoryQos dpf_qos;
  DDS_DomainParticipantQos dp_qos;
  DDS_DataWriterQos        dw_qos;
  CoreDX_Transport       * tport;
  char                   * config_str = NULL;
  struct timespec          start_time;

  all_done = 0;

  parse_args(argc, argv);

#if !defined(_WIN32)
  install_sig_handlers();
#endif

  /* instruct factory to NOT automatically enable the DomainParticipant */
  DDS_DomainParticipantFactory_get_qos( &dpf_qos);
  dpf_qos.entity_factory.autoenable_created_entities = 0;
  DDS_DomainParticipantFactory_set_qos( &dpf_qos);

  DDS_DomainParticipantFactory_get_default_participant_qos(&dp_qos);
  /* reduce the frequency of DPD multicast messages */
  dp_qos.discovery.dpd_push_period.sec              = 30;
  dp_qos.discovery.dpd_push_period.nanosec          = 0;
  dp_qos.discovery.dpd_lease_duration.sec           = 240;
  dp_qos.discovery.dpd_lease_duration.nanosec       = 0;

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
  
  /* Register the data type with the CoreDX middleware. 
   * This is required before creating a Topic with
   * this data type. 
   */
  if (StringMsgTypeSupport_register_type( domain, NULL ) != DDS_RETCODE_OK)
    {
      printf("ERROR registering type\n");
      return -1;
    }
  
  /* create a DDS Topic with the StringMsg data type. */
  topic = 
    DDS_DomainParticipant_create_topic(domain, 
				   "helloTopic", 
				   "StringMsg", 
				   DDS_TOPIC_QOS_DEFAULT, 
				   NULL, 
				   0 );
  if ( topic == NULL )
    {
      printf("ERROR creating topic.\n");
      return -1;
    }

  /* Create a DDS DataWriter on the hello topic, 
   */
  DDS_Publisher_get_default_datawriter_qos(publisher, &dw_qos);
  if (reliable) 
    {
      dw_qos.reliability.kind = DDS_RELIABLE_RELIABILITY_QOS;
      dw_qos.reliability.max_blocking_time.sec = 60;
      dw_qos.reliability.max_blocking_time.nanosec = 0;
      dw_qos.resource_limits.max_samples              = 1000;
      dw_qos.resource_limits.max_samples_per_instance = 100;
      dw_qos.resource_limits.max_instances            = 1;
    }
  if (history_depth == 0)
    dw_qos.history.kind = DDS_KEEP_ALL_HISTORY_QOS;
  else
    {
      dw_qos.history.kind = DDS_KEEP_LAST_HISTORY_QOS;
      dw_qos.history.depth = history_depth;
    }
  dw_qos.latency_budget.duration.sec     = 0;
  dw_qos.latency_budget.duration.nanosec = NSEC_PER_MSEC * 50;
  dw = DDS_Publisher_create_datawriter(publisher,
				       topic, 
				       &dw_qos,
				       NULL, 
				       0 );
  if (dw == NULL)
    {
      printf("ERROR creating data writer\n");
      return -1;
    }

  /* Initialize the data to send.  The StringMsg data type
   * has just one string member.
   * Note: be careful when initializing a string member with
   * non-allocated memory -- the clear() operation 
   * (StringMsg_clear() for example) will free all string
   * members.  In this simple example, we do not use
   * the clear() operation, so using a static string
   * will work.
   */
  if (string_msg != NULL)
    stringMsg.msg = string_msg;
  else
    stringMsg.msg = "Hello WORLD!";
  
  printf("Message will be %d bytes.\n", strlen(stringMsg.msg)+4);
  /* +4 considers the string 'length' value which is added by CDR marshal protocol */
  /* NOTE: by using an array type (instead of a string type), the 'length' field overhead can be avoided. */

  /* Wait for at least one subscriber to appear 
   */
  printf("Waiting for a subscriber...\n");
  while(!all_done)
    {
      DDS_InstanceHandleSeq dr_handles;
      DDS_DataWriter_get_matched_subscriptions(dw, &dr_handles);
      if (dr_handles._length > 0)
	{
	  seq_clear(&dr_handles);
	  break;
	}
      else
	toc_sleep(USEC_PER_SEC);
    }

  /* While forever, write a sample. */
  toc_sleep(USEC_PER_SEC);
  printf("Publishing for %d seconds.\n", test_length);
  clock_gettime(CLOCK_REALTIME, &start_time);
  while ( !all_done )
    {
      DDS_ReturnCode_t ret;

      ret = StringMsgDataWriter_write ( dw, &stringMsg, DDS_HANDLE_NIL ); 
      if ( ret != DDS_RETCODE_OK)
	{
	  printf("ERROR writing sample\n");
	  fflush(stdout);
	  return -1;
	}
      
      if (verbose) 
	printf("wrote a sample\n");
      num_written ++;

      toc_sleep(tdelay);

      if (!all_done)
	all_done = test_time_elapsed(&start_time);
    }

  toc_sleep(USEC_PER_SEC);
  printf("Wrote %d %d byte messages.\n", num_written, strlen(stringMsg.msg) + 4);
  
  /* Cleanup */
  if (verbose) 
    printf("clean up ...\n");

  free(string_msg);
  DDS_DomainParticipant_delete_contained_entities(domain);
  DDS_DomainParticipantFactory_delete_participant(domain);

  return 0;
}
