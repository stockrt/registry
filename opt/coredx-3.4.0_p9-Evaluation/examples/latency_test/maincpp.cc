/*****************************************************************
 *   file:   maincpp.cc
 *   auth:   ctucker
 *   date:   04/13/2010
 *   desc:   latency tester
 *****************************************************************
 *
 *   Coypright(C) 2010-2011 Twin Oaks Computing, Inc
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
#  include <time.h>
#  include <windows.h>
#else
#  include <unistd.h>
#  include <sys/ioctl.h>
#endif

#include <assert.h>

#include <dds/dds.hh>
#include <dds/toc_util.h>

#include "latencycpp.hh"
#include "latencycppTypeSupport.hh"
#include "latencycppDataReader.hh"
#include "latencycppDataWriter.hh"

#ifndef MSEC_PER_SEC
#define MSEC_PER_SEC    1000L
#define USEC_PER_SEC    1000000L
#define USEC_PER_MSEC   1000L
#define NSEC_PER_SEC    1000000000L
#define NSEC_PER_MSEC   1000000L
#define NSEC_PER_USEC   1000L
#endif


using namespace DDS;

/****************************************************************
 *  STRUCTURES
 ****************************************************************/
struct Args {
  float delay;          /* seconds between samples in usecs */
  char  mode;           /* 1 == sender, 2 == reflector, 3 == receiver */
  int   sample_size;    /* 1... */
  float latency_budget; /* seconds */
  int   keep_depth;     /* int */
  float percent_drop;
  ReliabilityQosPolicyKind reliability_kind;
} ;



/****************************************************************
 * FORWARD DECLS
 ****************************************************************/

/****************************************************************
 * GLOBALS
 ****************************************************************/
struct Args args = 
  {                       /* defaults: */
    /* .delay            = */ .5,     /* 1/2 sec */
    /* .mode             = */ 0,      /* invalid (must specify on cmdline) */
    /* .sample_size      = */ 4,      /* 4 bytes */
    /* .latency_budget   = */ 0.0,  /* seconds */
    /* .keep_depth       = */ -1,     /* int, -1=keep_all */
    /* .percent_drop     = */ 0.01,
    /* .reliability_kind = */ BEST_EFFORT_RELIABILITY_QOS,
  };


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

DomainParticipant      * dp;
Publisher              * pub;
Subscriber             * sub;

Topic                  * send_topic;
Topic                  * reflect_topic;
DataWriter             * reflector_dw;  
DataReaderListener     * reflector_listener;

Topic                  * cmd_topic;
DataWriter             * cmd_dw;  
DataWriterListener     * cmd_dwlistener = NULL;
DataReader             * cmd_dr;
DataReaderListener     * cmd_drlistener = NULL;
DataReaderListener     * receiver_listener = NULL;



class CmdDataReaderListener : public DataReaderListener
{
  /****************************************************************
   *
   ****************************************************************/
  void on_data_available( DataReader * dr )
  {
    cmdTypePtrSeq    samples;
    SampleInfoSeq    samples_info;
    unsigned int     i;
    ReturnCode_t     retval;
  
    cmdTypeDataReader * cmd_dr = cmdTypeDataReader::narrow(dr);
  
    retval = cmd_dr -> take( &samples, &samples_info, 
			     LENGTH_UNLIMITED, 
			     ANY_SAMPLE_STATE, 
			     ANY_VIEW_STATE, 
			     ANY_INSTANCE_STATE );
  
    if ( retval == RETCODE_OK )
      {
	for ( i = 0 ; i < samples.size() ; i++ )
	  {
	    if ( samples_info[i] -> valid_data)
	      {
		cmdType * data = samples[i];
	      
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
	cmd_dr -> return_loan( &samples, &samples_info );
      }
  }



  /****************************************************************
   *
   ****************************************************************/
  void on_subscription_matched( DataReader * dr, 
				SubscriptionMatchedStatus status )
  {
    /* printf("sub matched: current: %d\n",  status.current_count); */
  }

  /****************************************************************
   *
   ****************************************************************/
  void on_requested_incompatible_qos( DataReader * dr,
				      RequestedIncompatibleQosStatus status )
  {
    const char * policy_str = DDS_qos_policy_str(status.last_policy_id); 
    printf("dr requested incompatible qos: %s\n", policy_str );
  }

};


class CmdDataWriterListener : public DataWriterListener
{
  /****************************************************************
   *
   ****************************************************************/
  void on_offered_incompatible_qos(DataWriter * writer, 
						     OfferedIncompatibleQosStatus status)
  {
    const char * policy_str = DDS_qos_policy_str(status.last_policy_id); 
    printf("dw offered incompatible qos policy: %s",  policy_str);
  }


  /****************************************************************
   *
   ****************************************************************/
  void on_publication_matched(DataWriter * writer, 
						PublicationMatchedStatus status)
  {
    /* printf("pub matched: current: %d\n",  status.current_count); */
    cmd_readers = status.current_count;
  }
};


class ReceiverDataReaderListener : public DataReaderListener
{
  /****************************************************************
   *
   ****************************************************************/
  void on_data_available(DataReader * dr)
  {
    struct timespec         rx_time;
    latencyTypeDataReader * latency_dr;
    latencyTypePtrSeq       samples;
    SampleInfoSeq           samples_info;
    unsigned int            i;
    ReturnCode_t            retval;
#if defined(_WIN32)
    Time_t tp;
    LARGE_INTEGER t; 
    unsigned __int64 tmpres = 0; 
#endif
    double rtime;
  
  
#if defined(_WIN32)
    QueryPerformanceCounter(&t); 
    rtime            = t.QuadPart / (double)proc_freq.QuadPart;
#else
    clock_gettime(CLOCK_REALTIME, &rx_time);
    rtime = (uint32_t)rx_time.tv_sec +(rx_time.tv_nsec / (double)NSEC_PER_SEC);
#endif
  

    latency_dr = latencyTypeDataReader::narrow( dr );
  
    retval = latency_dr -> take( &samples, &samples_info, 
				 LENGTH_UNLIMITED, 
				 ANY_SAMPLE_STATE, 
				 ANY_VIEW_STATE, 
				 ANY_INSTANCE_STATE );
  
    if ( retval == RETCODE_OK )
      {
	if (test_start)
	  {
	    for ( i = 0;i < samples.size(); i++)
	      {
		if ( samples_info[i]->valid_data)
		  {
		    double stime = samples_info[i]->source_timestamp.sec +
		      (samples_info[i]->source_timestamp.nanosec / (double)NSEC_PER_SEC);
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
	latency_dr -> return_loan( &samples, &samples_info );
      }
  }
};

class ReflectorDataReaderListener : public DataReaderListener
{
  /****************************************************************
   *
   ****************************************************************/
  void on_data_available(DataReader * dr)
  {
    latencyTypePtrSeq  samples;
    SampleInfoSeq      samples_info;
    ReturnCode_t       retval;
    
    latencyTypeDataReader * latency_dr = latencyTypeDataReader::narrow( dr );

    retval = latency_dr -> take( &samples, &samples_info, 
				 LENGTH_UNLIMITED, 
				 ANY_SAMPLE_STATE, 
				 ANY_VIEW_STATE, 
				 ANY_INSTANCE_STATE );
    
    if ( retval == RETCODE_OK )
      {
	unsigned int i;
	for ( i = 0;i < samples.size(); i++)
	  {
	    if ( samples_info[i]->valid_data)
	      {
		retval = 
		  reflector_dw -> write_w_timestamp ( samples[i], 
						      HANDLE_NIL, 
						      samples_info[i]->source_timestamp); 
	      }
	  }
	latency_dr -> return_loan( &samples, &samples_info );
      }
  }
};

/****************************************************************
 *
 ****************************************************************/
int
parse_args(int argc, char * argv[])
{
  int opt;
  float d;
  while ((opt = toc_getopt(argc, argv, "b:d:k:n:rs:w:h")) != -1) 
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
	case 'k':
	  args.keep_depth = atoi(toc_optarg);
	  break;
        case 'n':
          args.sample_size = atoi(toc_optarg);
          break;
	case 'r':
	  args.reliability_kind = RELIABLE_RELIABILITY_QOS;
	  break;
	case 's':
          d = atof(toc_optarg);
	  if (d > 0.00000)
	    {
	      if (d < 0.0001)
		d = 0.0001;
	      args.delay = d;
	    }
	  break;
	case 'w':
	  args.mode = atoi(toc_optarg);
	  break;
        usage:
	case 'h':
	default: /* '?' */
	  fprintf(stderr, "Usage: %s \n", argv[0]);
	  fprintf(stderr, "           -d <percent>         : (float) ignore highest 'percent' measurements\n");
	  fprintf(stderr, "           -k <keep_depth>      : (int) set keep history depth (-1=keep_all), (default=1) \n");
	  fprintf(stderr, "           -n <num bytes>       : (int) set payload size in bytes\n");
	  fprintf(stderr, "           -r                   : turn on RELIABLE QoS \n");
	  fprintf(stderr, "           -s <delay seconds>   : (float) delay between data writes \n");
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
send_command (int c)
{
  ReturnCode_t retval;
  cmdType      cmd;

  cmd.cmd         = c;
  cmd.sample_size = args.sample_size;
  cmd.interval    = args.delay;
  retval = cmd_dw -> write(&cmd, HANDLE_NIL);
  if (retval != RETCODE_OK)
    fprintf(stderr, "error writing end test cmd: %s\n", DDS_error(retval));

  toc_sleep(100); /* let it propagate */
}

/****************************************************************
 *
 ****************************************************************/
void
init_dds(void)
{
  DomainId_t           did = 0;
  StatusMask           sm  = 0;
  DataReaderQos        dr_qos;
  DataWriterQos        dw_qos;
  ReturnCode_t         retval;

  receiver_listener = new ReceiverDataReaderListener();
  cmd_drlistener = new CmdDataReaderListener();
  cmd_dwlistener = new CmdDataWriterListener();
  reflector_listener = new ReflectorDataReaderListener();


  DomainParticipantFactory * dpf = DomainParticipantFactory::get_instance();

  dp   = dpf -> create_participant( did, 
				    PARTICIPANT_QOS_DEFAULT, 
				    NULL, sm );

  pub  = dp -> create_publisher( PUBLISHER_QOS_DEFAULT, 
				 NULL, sm );

  sub  = dp -> create_subscriber( SUBSCRIBER_QOS_DEFAULT, 
				  NULL, sm );
  
  latencyTypeTypeSupport  latencyTS; 
  latencyTS . register_type( dp, NULL );

  send_topic    = dp -> create_topic( "latency_send",    
				      "latencyType", 
				      TOPIC_QOS_DEFAULT, 
				      NULL, sm );
  reflect_topic = dp -> create_topic( "latency_reflect", 
				      "latencyType", 
				      TOPIC_QOS_DEFAULT, 
				      NULL, sm );

  cmdTypeTypeSupport  cmdTS;
  cmdTS . register_type( dp, NULL );

  cmd_topic = dp -> create_topic( "latency_cmd", 
				  "cmdType", 
				  TOPIC_QOS_DEFAULT, 
				  NULL, sm );

  sub -> get_default_datareader_qos(&dr_qos);
  dr_qos.latency_budget.duration.sec     = 0;
  dr_qos.latency_budget.duration.nanosec = 0;
  dr_qos.reliability.kind = args.reliability_kind;
  dr_qos.history.kind = 
    (args.keep_depth == -1)?KEEP_ALL_HISTORY_QOS:KEEP_LAST_HISTORY_QOS;
  dr_qos.history.depth= args.keep_depth;
  if (dr_qos.reliability.kind == DDS::RELIABLE_RELIABILITY_QOS)
    {
      dr_qos.rtps_reader.heartbeat_response_delay.sec     = 0;
      dr_qos.rtps_reader.heartbeat_response_delay.nanosec = 0;
      dr_qos.resource_limits.max_samples     = 10;
      dr_qos.resource_limits.max_instances   = 10;
      dr_qos.resource_limits.max_samples_per_instance = 10;
    }
  retval = sub -> set_default_datareader_qos(dr_qos);
  if (retval != RETCODE_OK)
    fprintf(stderr, "ERROR setting default dr qos: %s\n", DDS_error(retval));


  sub -> get_default_datareader_qos(&dr_qos);
  dr_qos.history.kind     = KEEP_ALL_HISTORY_QOS;
  dr_qos.reliability.kind = RELIABLE_RELIABILITY_QOS;
  cmd_dr = sub -> create_datareader((TopicDescription *)cmd_topic, 
				    dr_qos,
				    cmd_drlistener, 
				    DATA_AVAILABLE_STATUS | 
				    REQUESTED_INCOMPATIBLE_QOS_STATUS |
				    SUBSCRIPTION_MATCHED_STATUS);
  if (cmd_dr == NULL)
    fprintf(stderr, "error creating command reader...\n");

  pub -> get_default_datawriter_qos(&dw_qos);
  dw_qos.latency_budget.duration.sec     = 0;
  dw_qos.latency_budget.duration.nanosec = 0;
  dw_qos.reliability.kind = args.reliability_kind;
  dw_qos.history.kind =
    (args.keep_depth == -1)?KEEP_ALL_HISTORY_QOS:KEEP_LAST_HISTORY_QOS;
  dw_qos.history.depth= args.keep_depth;
  if (dw_qos.reliability.kind == DDS::RELIABLE_RELIABILITY_QOS)
    {
      dw_qos.rtps_writer.heartbeat_period.sec        = 0;
      dw_qos.rtps_writer.heartbeat_period.nanosec    = NSEC_PER_USEC*100;
      dw_qos.rtps_writer.nack_response_delay.sec     = 0;
      dw_qos.rtps_writer.nack_response_delay.nanosec = 0; 
      dw_qos.resource_limits.max_samples     = 10;
      dw_qos.resource_limits.max_instances   = 10;
      dw_qos.resource_limits.max_samples_per_instance = 10;
    }
  retval = pub -> set_default_datawriter_qos(dw_qos);
  if (retval != RETCODE_OK)
    fprintf(stderr, "ERROR setting default dw qos: %s\n", DDS_error(retval));

  pub -> get_default_datawriter_qos( &dw_qos );
  dw_qos.history.kind     = KEEP_ALL_HISTORY_QOS;
  dw_qos.reliability.kind = RELIABLE_RELIABILITY_QOS;
  cmd_dw = pub -> create_datawriter(cmd_topic, dw_qos, cmd_dwlistener, 
				    OFFERED_INCOMPATIBLE_QOS_STATUS |
				    PUBLICATION_MATCHED_STATUS);
  if (cmd_dw == NULL)
    fprintf(stderr, "ERROR creating cmd writer\n");

  toc_sleep(USEC_PER_SEC);
}

/****************************************************************
 *
 ****************************************************************/
int
run_sender(void)
{
  DataWriter            * dw;
  latencyTypeDataWriter * latency_dw;
  latencyType             data;
  ReturnCode_t            retval;
  int                     udelay;

  udelay = (int)(send_interval * USEC_PER_SEC);
  /*
  data.value.resize( sample_size );
  */
  memset(data.value, 'A', sample_size);
  data.value[sample_size-1] = '\0';
  data.counter = 0;

  dw = pub -> create_datawriter(send_topic, 
				DATAWRITER_QOS_DEFAULT, NULL, 0);
  latency_dw = latencyTypeDataWriter::narrow( dw );

  toc_sleep(USEC_PER_SEC); /* time for readers and writers discover each other */

  while(!all_done)
    {
#if defined(_WIN32)
      DDS_Time_t       tp;
      LARGE_INTEGER    t; 
      double           dt;
      unsigned __int64 tmpres = 0; 

      QueryPerformanceCounter(&t); 
      dt         = t.QuadPart / (double)proc_freq.QuadPart;
      tp.sec     = (uint32_t)dt;
      tp.nanosec = (int32_t)((dt - (double)tp.sec) * NSEC_PER_SEC);
      retval = latency_dw -> write_w_timestamp ( &data, HANDLE_NIL, tp ); 
#else
      retval = latency_dw -> write ( &data, HANDLE_NIL ); 
#endif
      data.counter++;

      if (data.counter >= NUM_SENDS) /* make configurable? */
        break;
      else
	toc_sleep(udelay);
    }
  toc_sleep(USEC_PER_SEC);
  
  /* clean up */
  pub->delete_datawriter(dw);

  /* end the test */
  send_command(2);
  return 0;
}

/****************************************************************
 *
 ****************************************************************/
int
run_reflector(void)
{
  DataReader                 * dr;
  DataWriter                 * dw;

  /* create a writer to re-send samples */
  dw = pub -> create_datawriter(reflect_topic,
				DATAWRITER_QOS_DEFAULT,  NULL, 0);
  reflector_dw = latencyTypeDataWriter::narrow( dw );
  
  dr = sub -> create_datareader( (TopicDescription *)send_topic,
				 DATAREADER_QOS_DEFAULT, 
				 reflector_listener, DATA_AVAILABLE_STATUS);

  while (!all_done)
    toc_sleep(USEC_PER_SEC);
  
  return 0;
}

/****************************************************************
 *
 ****************************************************************/
int 
init_receiver(void)
{
  DataReader  * dr;

  dr = sub -> create_datareader( (TopicDescription *)reflect_topic, 
				 DATAREADER_QOS_DEFAULT, 
				 receiver_listener, 
				 DATA_AVAILABLE_STATUS);
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
  int drop_count = (int)(args.percent_drop * sample_count + 0.5);

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
  sigset_t           ssnew;
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
	toc_sleep(USEC_PER_SEC);

      if (!all_done)
	{
	  printf("Run test - size: %d ... ", args.sample_size);
	  fflush(stdout);
	  send_command(1);
	  while(!test_done && !all_done)
	    toc_sleep(USEC_PER_SEC/2);
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

      printf("  size  count   avg (usec)   min (usec)   max (usec)       stddev  ignored\n");
      printf("==========================================================================\n");

      while(!all_done)
        {
          test_done    = 0;
          sample_count = 0;
          sample_size  = 0;
	  
	  while  (!test_start && !all_done) 
	    toc_sleep(USEC_PER_SEC/10);

	  run_sender();
	  test_start = 0;
          print_stats();
        }
    }

  if (dp)
    {
      dp -> delete_contained_entities();

      DomainParticipantFactory * dpf = DomainParticipantFactory::get_instance();
      dpf -> delete_participant(dp);
    }

  return 0;
}
