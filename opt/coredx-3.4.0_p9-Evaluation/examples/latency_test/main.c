/*****************************************************************
 *   file:   main.c
 *   auth:   ctucker
 *   date:   10/30/2009
 *   desc:   latency tester
 *****************************************************************
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
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>

#if defined(_WIN32)
#  include <winsock2.h>
#  include <time.h>
#  include <windows.h>
#else
#  include <unistd.h>
#  include <sys/ioctl.h>
#endif

#include <assert.h>

#include <dds/dds.h>
#include <dds/toc_util.h>
#include <dds/coredx_transport.h>

#include "latency.h"
#include "latencyTypeSupport.h"
#include "latencyDataReader.h"
#include "latencyDataWriter.h"

#ifndef MSEC_PER_SEC
#define MSEC_PER_SEC    1000L
#define USEC_PER_SEC    1000000L
#define USEC_PER_MSEC   1000L
#define NSEC_PER_SEC    1000000000L
#define NSEC_PER_MSEC   1000000L
#define NSEC_PER_USEC   1000L
#endif

/****************************************************************
 *  STRUCTURES
 ****************************************************************/
struct {
  float delay;          /* seconds between samples in usecs */
  char  mode;           /* 1 == sender, 2 == reflector, 3 == receiver */
  int   sample_size;    /* 1... */
  float latency_budget; /* seconds */
  int   keep_depth;     /* int */
  int   use_threads;    
  float percent_drop;
  char *transport_cfg;
  DDS_ReliabilityQosPolicyKind reliability_kind;
  DDS_DomainId_t domain_id;
} args = 
  {                       /* defaults: */
    /* .delay            = */ .5,     /* 1/2 sec */
    /* .mode             = */ 0,      /* invalid (must specify on cmdline) */
    /* .sample_size      = */ 4,      /* 4 bytes */
    /* .latency_budget   = */ 0.0,  /* seconds */
    /* .keep_depth       = */ -1,     /* int, -1=keep_all */
    /* .use_threads      = */ 1,      /* use threads by default */
    /* .percent_drop     = */ 0.01,
    /* .transport_cfg    = */ NULL,
    /* .reliability_kind = */ DDS_BEST_EFFORT_RELIABILITY_QOS,
    /* .domain_id        = */ 0,
  };

/****************************************************************
 * FORWARD DECLS
 ****************************************************************/


/****************************************************************
 * GLOBALS
 ****************************************************************/
#define NUM_SENDS 1000

int    all_done     = 0;
int    interrupted  = 0;
int    test_start   = 0;
int    test_done    = 0;
int    sample_count = 0;
int    sample_size  = 0;
float  send_interval = 0.0;
int    cmd_readers  = 0;

#if defined(_WIN32)
LARGE_INTEGER proc_freq;
#endif

double measures[NUM_SENDS+1]; /* store up to 1000 measurements */

DDS_DomainParticipant        dp;
DDS_Publisher                pub;
DDS_Subscriber               sub;

DDS_Topic                    send_topic;
DDS_Topic                    reflect_topic;
DDS_DataWriter               reflector_dw;  

DDS_Topic                    cmd_topic;
DDS_DataWriter               cmd_dw;  
DDS_DataReader               cmd_dr;

/****************************************************************
 *
 ****************************************************************/
int
parse_args(int argc, char * argv[])
{
  int opt;
  float d;
  while ((opt = toc_getopt(argc, argv, "b:d:i:k:n:rs:w:ht:T")) != -1) 
    {
      switch (opt) 
	{
	case 'b':
	  args.latency_budget = atof(toc_optarg);
	  break;
	case 'd':
	  args.percent_drop = atof(toc_optarg);
	  args.percent_drop = args.percent_drop / 100;
	  if (args.percent_drop < 0.0) args.percent_drop = 0.0;
	  if (args.percent_drop > 0.1) args.percent_drop = 0.1; /* 10% */
	  break;
	case 'i':
	  args.domain_id = atoi(toc_optarg);
	  break;
	case 'k':
	  args.keep_depth = atoi(toc_optarg);
	  break;
        case 'n':
          args.sample_size = atoi(toc_optarg);
          break;
	case 'r':
	  args.reliability_kind = DDS_RELIABLE_RELIABILITY_QOS;
	  break;
	case 's':
          d = atof(toc_optarg);
	  if (d > 0.00000)
	    {
	      if (d < 0.00001)
		d = 0.00001;
	      args.delay = d;
	    }
	  break;
	case 't':
	  args.transport_cfg = strdup(toc_optarg);
	  break;
	case 'T':
	  args.use_threads = 0;
	  break;
	case 'w':
	  args.mode = atoi(toc_optarg);
	  break;
        usage:
	case 'h':
	default: /* '?' */
	  fprintf(stderr, "Usage: %s \n", argv[0]);
	  fprintf(stderr, "           -d <percent>         : (float) ignore highest 'percent' measurements\n");
	  fprintf(stderr, "           -i <domainId>        : (int) domain id\n");
	  fprintf(stderr, "           -k <keep_depth>      : (int) set keep history depth (-1=keep_all), (default=1) \n");
	  fprintf(stderr, "           -n <num bytes>       : (int) set payload size in bytes\n");
	  fprintf(stderr, "           -r                   : turn on RELIABLE QoS \n");
	  fprintf(stderr, "           -s <delay seconds>   : (float) delay between data writes\n");
	  fprintf(stderr, "           -T                   : don't use Threads\n");
	  fprintf(stderr, "           -w <write mode>      : 1: commander, 2: reflector, 3: sender/receiver\n");
	  	  
	  exit(-1);
	}
    }

  if (args.mode == 0)
    {
      printf("you must specify mode with -w <int>\n");
      goto usage;
    }

  return 0;
}

/****************************************************************
 *
 ****************************************************************/
void
cmd_incompat_qos(const DDS_DataReader dr, 
                 DDS_RequestedIncompatibleQosStatus status)
{
  const char * policy_str = DDS_qos_policy_str(status.last_policy_id); 
  printf("dr requested incompatible qos: %s\n", policy_str );
}

/****************************************************************
 *
 ****************************************************************/
void
cmd_data_available(const DDS_DataReader dr)
{
  cmdTypePtrSeq        samples;
  DDS_SampleInfoSeq    samples_info;
  unsigned int         i;
  DDS_ReturnCode_t     retval;

  INIT_SEQ(samples);
  INIT_SEQ(samples_info);

  retval = cmdTypeDataReader_take( dr, &samples, &samples_info, 
                                   DDS_LENGTH_UNLIMITED, 
                                   DDS_ANY_SAMPLE_STATE, 
                                   DDS_ANY_VIEW_STATE, 
                                   DDS_ANY_INSTANCE_STATE );
  
  if ( retval == DDS_RETCODE_OK )
    {
      for ( i = 0;i < samples._length; i++)
        {
          if ( samples_info._buffer[i]->valid_data)
            {
              cmdType * data = samples._buffer[i];

              if (data->cmd == 1)
                {
		  test_start    = 1;
		  sample_size   = data->sample_size;
		  send_interval = data->interval;
		}
	      else if (data->cmd == 2 )
		{
                  test_done    = 1;
                  /* sample_size = data->sample_size; */
                }
              else if (data->cmd == 3)
                {
                  all_done    = 1;
                }
            }
        }
      
      /* return the 'loaned' lists */
      cmdTypeDataReader_return_loan( dr, &samples, &samples_info );
    }
}

/****************************************************************
 *
 ****************************************************************/
void cmd_subscription_matched(DDS_DataReader dr, 
			      DDS_SubscriptionMatchedStatus status)
{
  /* printf("sub matched: current: %d\n",  status.current_count); */
}
/****************************************************************
 *
 ****************************************************************/
DDS_DataReaderListener cmd_listener = 
  {
    NULL, 
    cmd_incompat_qos, 
    NULL, 
    NULL, 
    cmd_data_available,
    cmd_subscription_matched,
    NULL
  };

/****************************************************************
 *
 ****************************************************************/
void
send_command (int c)
{
  DDS_ReturnCode_t retval;
  cmdType          cmd;

  cmd.cmd         = c;
  cmd.sample_size = args.sample_size;
  cmd.interval    = args.delay;
  retval = DDS_DataWriter_write(cmd_dw, &cmd, DDS_HANDLE_NIL);
  if (retval != DDS_RETCODE_OK)
    fprintf(stderr, "error writing end test cmd: %s\n", DDS_error(retval));

  toc_sleep(USEC_PER_SEC); /* let it propagate */
}

/****************************************************************
 *
 ****************************************************************/
void dw_offered_incompatible_qos(DDS_DataWriter writer, 
				 DDS_OfferedIncompatibleQosStatus status)
{
  const char * policy_str = DDS_qos_policy_str(status.last_policy_id); 
  printf("dw offered incompatible qos policy: %s",  policy_str);
}
/****************************************************************
 *
 ****************************************************************/
void dw_publication_matched(DDS_DataWriter writer, 
			    DDS_PublicationMatchedStatus status)
{
  /* printf("pub matched: current: %d\n",  status.current_count); */
  cmd_readers = status.current_count;
}

/****************************************************************
 *
 ****************************************************************/
DDS_DataWriterListener dw_listener = 
  {
    NULL, /* on_offered_deadline_missed, */
    dw_offered_incompatible_qos,
    NULL, /* on_liveliness_lost,    */
    dw_publication_matched  /* on_publication_matched */
  };

/****************************************************************
 *
 ****************************************************************/
void
init_dds(void)
{
  DDS_StatusMask           sm  = 0;
  DDS_DataReaderQos        dr_qos;
  DDS_DataWriterQos        dw_qos;
  DDS_ReturnCode_t         retval;
  CoreDX_Transport       * tport = NULL;
  DDS_DomainParticipantFactoryQos dpf_qos;
  DDS_DomainParticipantQos dp_qos;

  /* instruct factory to NOT automatically enable the DomainParticipant */
  DDS_DomainParticipantFactory_get_qos( &dpf_qos);
  dpf_qos.entity_factory.autoenable_created_entities = 0;
  DDS_DomainParticipantFactory_set_qos( &dpf_qos);

  DDS_DomainParticipantFactory_get_default_participant_qos(&dp_qos);
  dp_qos.thread_model.use_threads = args.use_threads;

  dp   = DDS_DomainParticipantFactory_create_participant( args.domain_id, 
							  &dp_qos,
							  NULL, sm );
  if (args.transport_cfg != NULL)
    {
      /* create, configure and install the serial transport */
#  if defined(COREDX_HAS_UDS_TRANSPORT)
      if (strncmp(args.transport_cfg, "UDS://", 6) == 0)
	{
	  tport = __coredx_uds_transport.alloc();
	}
      else 
#  endif
      if (strncmp(args.transport_cfg, "IF://", 5) == 0)
	{
	  tport = __coredx_udp_transport.alloc();
	}
      
      if (tport != NULL)
	{
	  printf("Configuring transport: %s\n", args.transport_cfg);
	  tport->init(tport, args.transport_cfg);
	  DDS_DomainParticipant_add_transport(dp, tport);
	}
    }
  
  /* enable the DomainParticipant */
  DDS_DomainParticipant_enable(dp);

  pub  = DDS_DomainParticipant_create_publisher(dp, 
						DDS_PUBLISHER_QOS_DEFAULT, 
						NULL, sm );
  sub  = DDS_DomainParticipant_create_subscriber(dp, 
						 DDS_SUBSCRIBER_QOS_DEFAULT, 
						 NULL, sm );
  
  latencyTypeTypeSupport_register_type( dp, NULL );
  send_topic    = DDS_DomainParticipant_create_topic(dp, 
						     "latency_send",    
						     "latencyType", 
						     DDS_TOPIC_QOS_DEFAULT, 
						     NULL, sm );
  reflect_topic = DDS_DomainParticipant_create_topic(dp, 
						     "latency_reflect", 
						     "latencyType", 
						     DDS_TOPIC_QOS_DEFAULT, 
						     NULL, sm );

  cmdTypeTypeSupport_register_type( dp, NULL );
  cmd_topic = DDS_DomainParticipant_create_topic(dp, 
						 "latency_cmd", 
						 "cmdType", 
						 DDS_TOPIC_QOS_DEFAULT, 
						 NULL, sm );

  DDS_Subscriber_get_default_datareader_qos(sub, &dr_qos);
  dr_qos.latency_budget.duration.sec     = 0;
  dr_qos.latency_budget.duration.nanosec = 0;
  dr_qos.reliability.kind = args.reliability_kind;
  dr_qos.history.kind = 
    (args.keep_depth == -1)?DDS_KEEP_ALL_HISTORY_QOS:DDS_KEEP_LAST_HISTORY_QOS;
  dr_qos.history.depth= args.keep_depth;
  if (dr_qos.reliability.kind == DDS_RELIABLE_RELIABILITY_QOS)
    {
      dr_qos.rtps_reader.heartbeat_response_delay.sec     = 0;
      dr_qos.rtps_reader.heartbeat_response_delay.nanosec = 0;
      dr_qos.resource_limits.max_samples     = 10;
      dr_qos.resource_limits.max_instances   = 10;
      dr_qos.resource_limits.max_samples_per_instance = 10;
    }
  retval = DDS_Subscriber_set_default_datareader_qos(sub, &dr_qos);
  if (retval != DDS_RETCODE_OK)
    fprintf(stderr, "ERROR setting default dr qos: %s\n", DDS_error(retval));


  DDS_Subscriber_get_default_datareader_qos(sub, &dr_qos);
  dr_qos.history.kind     = DDS_KEEP_ALL_HISTORY_QOS;
  dr_qos.reliability.kind = DDS_RELIABLE_RELIABILITY_QOS;
  cmd_dr = DDS_Subscriber_create_datareader(sub, (DDS_TopicDescription)cmd_topic, &dr_qos,
					    &cmd_listener, 
					    DDS_DATA_AVAILABLE_STATUS | 
					    DDS_REQUESTED_INCOMPATIBLE_QOS_STATUS |
					    DDS_SUBSCRIPTION_MATCHED_STATUS);
  if (cmd_dr == NULL)
    fprintf(stderr, "error creating command reader...\n");

  DDS_Publisher_get_default_datawriter_qos(pub, &dw_qos);
  dw_qos.latency_budget.duration.sec     = 0;
  dw_qos.latency_budget.duration.nanosec = 0;
  dw_qos.reliability.kind = args.reliability_kind;
  dw_qos.history.kind =
    (args.keep_depth == -1)?DDS_KEEP_ALL_HISTORY_QOS:DDS_KEEP_LAST_HISTORY_QOS;
  dw_qos.history.depth= args.keep_depth;
  dw_qos.rtps_writer.max_buffer_size = 65400;
  dw_qos.rtps_writer.min_buffer_size = 65400;
  if (dw_qos.reliability.kind == DDS_RELIABLE_RELIABILITY_QOS)
    {
      dw_qos.rtps_writer.heartbeat_period.sec        = 0;
      dw_qos.rtps_writer.heartbeat_period.nanosec    = NSEC_PER_USEC*100;
      dw_qos.rtps_writer.nack_response_delay.sec     = 0;
      dw_qos.rtps_writer.nack_response_delay.nanosec = 0;
      dw_qos.resource_limits.max_samples     = 10;
      dw_qos.resource_limits.max_instances   = 10;
      dw_qos.resource_limits.max_samples_per_instance = 10;
    }

  retval = DDS_Publisher_set_default_datawriter_qos(pub, &dw_qos);
  if (retval != DDS_RETCODE_OK)
    fprintf(stderr, "ERROR setting default dw qos: %s\n", DDS_error(retval));

  DDS_Publisher_get_default_datawriter_qos( pub, &dw_qos );
  dw_qos.history.kind     = DDS_KEEP_ALL_HISTORY_QOS;
  dw_qos.reliability.kind = DDS_RELIABLE_RELIABILITY_QOS;
  cmd_dw = DDS_Publisher_create_datawriter(pub, cmd_topic, &dw_qos, &dw_listener, 
                                           DDS_OFFERED_INCOMPATIBLE_QOS_STATUS |
                                           DDS_PUBLICATION_MATCHED_STATUS);
  if (cmd_dw == NULL)
    fprintf(stderr, "ERROR creating cmd writer\n");

  toc_sleep(USEC_PER_SEC*2);
}

/****************************************************************
 *
 ****************************************************************/
int
run_sender(void)
{
  DDS_DataWriter   dw;
  latencyType      data;
  DDS_ReturnCode_t retval;
  int              udelay;
  DDS_Duration_t   work_slice;

  udelay        = send_interval * USEC_PER_SEC;
  

  /*INIT_SEQ(data.value);
  seq_set_size(&data.value,   sample_size);
  seq_set_length(&data.value, sample_size);
  */
  memset(data.value, 'A', sample_size);
  data.value[sample_size-1] = '\0';
  data.counter = 0;

  dw = DDS_Publisher_create_datawriter(pub, send_topic, 
				       DDS_DATAWRITER_QOS_DEFAULT, NULL, 0);
  
  work_slice.sec  = 4;
  work_slice.nanosec = 0;
  
  if (args.use_threads)
    toc_sleep(USEC_PER_SEC*4); /* time for readers and writers discover each other */
  else
    DDS_DomainParticipant_do_work(dp, &work_slice);

  work_slice.sec = (int)send_interval;
  work_slice.nanosec = (send_interval-((int)send_interval))*NSEC_PER_SEC;

  while(!all_done)
    {
      
#if defined(_WIN32)
      DDS_Time_t tp;
      LARGE_INTEGER t; 
      double dt;

      unsigned __int64 tmpres = 0; 
      QueryPerformanceCounter(&t); 
      dt         = t.QuadPart / (double)proc_freq.QuadPart;
      tp.sec     = (uint32_t)dt;
      tp.nanosec = (int32_t)((dt - (double)tp.sec) * NSEC_PER_SEC);
      retval = DDS_DataWriter_write_w_timestamp ( dw, &data, DDS_HANDLE_NIL, tp ); 
#else
      retval = DDS_DataWriter_write ( dw, &data, DDS_HANDLE_NIL ); 
#endif
      data.counter++;

      if (data.counter >= NUM_SENDS) /* make configurable? */
        break;
      else
	{
	  if (args.use_threads)
	    toc_sleep(udelay); 
	  else
	    DDS_DomainParticipant_do_work(dp, &work_slice);
	}
    }

  work_slice.sec  = 1;
  work_slice.nanosec = 0;

  if (args.use_threads)
    toc_sleep(USEC_PER_SEC);
  else
    DDS_DomainParticipant_do_work(dp, &work_slice);

  /* clean up */
  DDS_Publisher_delete_datawriter(pub, dw);

  /* end the test */
  send_command(2);
  return 0;
}

/****************************************************************
 *
 ****************************************************************/
void
reflector_data_available(const DDS_DataReader dr)
{
  latencyTypePtrSeq    samples;
  DDS_SampleInfoSeq    samples_info;
  DDS_ReturnCode_t     retval;

  INIT_SEQ(samples);
  INIT_SEQ(samples_info);
  
  retval = latencyTypeDataReader_take( dr, &samples, &samples_info, 
				       DDS_LENGTH_UNLIMITED, 
				       DDS_ANY_SAMPLE_STATE, 
				       DDS_ANY_VIEW_STATE, 
				       DDS_ANY_INSTANCE_STATE );
	  
  if ( retval == DDS_RETCODE_OK )
    {
      unsigned int i;
      for ( i = 0;i < samples._length; i++)
	{
	  if ( samples_info._buffer[i]->valid_data)
	    {
	      retval = 
		DDS_DataWriter_write_w_timestamp ( reflector_dw, samples._buffer[i], 
						   DDS_HANDLE_NIL, 
						   samples_info._buffer[i]->source_timestamp); 
	    }
	}
      latencyTypeDataReader_return_loan( dr, &samples, &samples_info );
    }
}

/****************************************************************
 *
 ****************************************************************/
DDS_DataReaderListener reflector_listener = 
  {
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    reflector_data_available,
    NULL,
    NULL
  };

/****************************************************************
 *
 ****************************************************************/
int
run_reflector(void)
{
  DDS_DataReader       dr;
  DDS_Duration_t   work_slice;

  work_slice.sec  = 1;
  work_slice.nanosec = 0;

  /* create a writer to re-send samples */
  reflector_dw = DDS_Publisher_create_datawriter(pub, reflect_topic,
						 DDS_DATAWRITER_QOS_DEFAULT,  NULL, 0);
  
  dr = DDS_Subscriber_create_datareader(sub, (DDS_TopicDescription)send_topic,
					DDS_DATAREADER_QOS_DEFAULT, 
					&reflector_listener, DDS_DATA_AVAILABLE_STATUS);
  while (!all_done)
    {
      if (args.use_threads)
	toc_sleep(USEC_PER_SEC);
      else
	DDS_DomainParticipant_do_work(dp, &work_slice);
    }

  return 0;
}

/****************************************************************
 *
 ****************************************************************/
void
receiver_data_available(const DDS_DataReader dr)
{
  struct timespec      rx_time;
  latencyTypePtrSeq    samples;
  DDS_SampleInfoSeq    samples_info;
  unsigned int         i;
  DDS_ReturnCode_t     retval;
#if defined(_WIN32)
  DDS_Time_t tp;
  LARGE_INTEGER t; 
  unsigned __int64 tmpres = 0; 
#endif
  double rtime;

  INIT_SEQ(samples);
  INIT_SEQ(samples_info);

#if defined(_WIN32)
  QueryPerformanceCounter(&t); 
  rtime            = t.QuadPart / (double)proc_freq.QuadPart;
#else
  clock_gettime(CLOCK_REALTIME, &rx_time);
  rtime = (uint32_t)rx_time.tv_sec +(rx_time.tv_nsec / (double)NSEC_PER_SEC);
#endif
  
  retval = latencyTypeDataReader_take( dr, &samples, &samples_info, 
                                       DDS_LENGTH_UNLIMITED, 
                                       DDS_ANY_SAMPLE_STATE, 
                                       DDS_ANY_VIEW_STATE, 
                                       DDS_ANY_INSTANCE_STATE );
  
  if ( retval == DDS_RETCODE_OK )
    {
      if (test_start)
	{
	  for ( i = 0;i < samples._length; i++)
	    {
	      if ( samples_info._buffer[i]->valid_data)
		{
		  double stime = samples_info._buffer[i]->source_timestamp.sec +
		    (samples_info._buffer[i]->source_timestamp.nanosec / (double)NSEC_PER_SEC);
		  double rtt   = rtime - stime;
		  double latency = rtt / 2.0;
		  
#if 0
		  
		  printf("sample: %8d tx time: %d.%.9ld rx time: %d.%.9ld latency: %f usec\n", 
			 samples._buffer[i]->counter, 
			 samples_info._buffer[i]->source_timestamp.sec,
			 samples_info._buffer[i]->source_timestamp.nanosec,
			 (int)rx_time.tv_sec, rx_time.tv_nsec, 
			 latency * USEC_PER_SEC); 
#endif
		  if (sample_count < NUM_SENDS)
		    measures[sample_count++] = latency * USEC_PER_SEC;
		}
	    }
	}
      
      /* return the 'loaned' lists */
      latencyTypeDataReader_return_loan( dr, &samples, &samples_info );
    }
}

/****************************************************************
 *
 ****************************************************************/
DDS_DataReaderListener receiver_listener = 
  {
    NULL, 
    NULL, 
    NULL, 
    NULL, 
    receiver_data_available,
    NULL,
    NULL
  };

/****************************************************************
 *
 ****************************************************************/
int 
init_receiver(void)
{
  static DDS_DataReader  dr;

  dr = DDS_Subscriber_create_datareader(sub, 
					(DDS_TopicDescription)reflect_topic, 
					DDS_DATAREADER_QOS_DEFAULT, 
					&receiver_listener, 
					DDS_DATA_AVAILABLE_STATUS);
  if (dr == NULL)
    fprintf(stderr, "Error creating receiver DataReader.\n");

  return 0;
}

/****************************************************************
 *
 ****************************************************************/
static int compare_time(const void *v1, const void *v2)
{
  double d1, d2;
  int result = 0;
  
  assert(v1 != NULL);
  assert(v2 != NULL);
  
  d1 = *((double *)v1);
  d2 = *((double *)v2);
  
  if (d1 > d2) result = 1;
  if (d2 > d1) result = -1;
  return result;
}

/****************************************************************
 *
 ****************************************************************/
void
print_stats(void)
{
  int i;
  double total = 0;
  double avg;
  double min = 100000;
  double max = -1;
  double std;
  int drop_count = (int)(args.percent_drop * sample_count + .5);

  /* sort */
  qsort(&measures, sample_count, sizeof(measures[0]), compare_time);

  /* omit high and low value from stats */
  for (i = 0;i < sample_count-drop_count; i++)
    {
      total += measures[i];
      if (measures[i] < min)
        min = measures[i];
      if (measures[i] > max)
        max = measures[i];
    }
  avg = total / (sample_count-drop_count);

  /* compute std deviation */
  total = 0;
  for (i = 0; i < sample_count-drop_count; i++)
    {
      double d = measures[i] - avg;
      total   += d*d;
    }
  std = sqrt(total / (sample_count-drop_count));
  
  printf("%6d %6d %12.4f %12.4f %12.4f %12.4f  %7d\n", sample_size, sample_count, avg, min, max, std, drop_count);
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
      interrupted = 1;
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
 *
 ****************************************************************/
int main(int argc, char * argv[])
{
  DDS_Duration_t   work_slice;
  work_slice.sec  = 1;
  work_slice.nanosec = 0;

#if !defined(_WIN32)
  install_sig_handlers();
#endif

  if (parse_args(argc, argv))
    exit(-1);

#if defined(_WIN32)
  if (!QueryPerformanceFrequency(&proc_freq))
    fprintf(stderr, "Windows high-resolution timer not available. Best resolution avaiable is '1ms'.\n");
#endif

  init_dds();
  
  if (args.mode == 1)
    {
      /* start test */
      test_done    = 0;

      /*printf("Wait for test participants...\n"); */
      while ((cmd_readers < 3) && (!all_done))
	if (args.use_threads)
	  toc_sleep(USEC_PER_SEC);
	else
	  DDS_DomainParticipant_do_work(dp, &work_slice);

      if (!all_done)
	{
	  printf("Run test - size: %d ... ", args.sample_size);
	  fflush(stdout);
	  send_command(1);

	  while(!test_done && !all_done)
	    if (args.use_threads)
	      toc_sleep(USEC_PER_SEC/2);
	    else
	      DDS_DomainParticipant_do_work(dp, &work_slice);

	  printf("done.\n");
	}
    }
  else if (args.mode == 2)
    {
      run_reflector();
    }
  else if (args.mode == 3)
    {
      init_receiver();

      work_slice.sec  = 0;
      work_slice.nanosec = NSEC_PER_SEC/10;

      printf("  size  count   avg (usec)   min (usec)   max (usec)       stddev  ignored\n");
      printf("==========================================================================\n");

      while(!all_done)
        {
          test_done    = 0;
          sample_count = 0;
          sample_size  = 0;

	  while  (!test_start && !all_done) 
	    if (args.use_threads)
	      toc_sleep(USEC_PER_SEC/10);
	    else
	      DDS_DomainParticipant_do_work(dp, &work_slice);
	  
	  if (!interrupted)
	    {
	      run_sender();
	      test_start = 0;
	      if (!interrupted)
		print_stats();
	    }
        }
    }

  if (dp)
    {
      DDS_DomainParticipant_delete_contained_entities(dp);
      DDS_DomainParticipantFactory_delete_participant(dp);
    }

  return 0;
}
