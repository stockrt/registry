/*****************************************************************
 *   file:   main.c
 *   auth:   ctucker
 *   date:   10/30/2009
 *   desc:   throughput tester
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
 *****************************************************************/

#if defined(_WIN32)
#  include <windows.h>
#else
#  include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>


#include <dds/dds.h>
#include <dds/toc_util.h>
#include "bw_config.h"
#include "bwTypeSupport.h"
#include "bwDataWriter.h"
#include "bwDataReader.h"

#define MILLISEC_PER_SEC      (1000)
#define MICROSEC_PER_SEC      (1000000)
#define MICROSEC_PER_MILLISEC (1000)
#define NANOSEC_PER_SEC       (1000000000)
#define NANOSEC_PER_MILLISEC  (1000000)
#define NANOSEC_PER_MICROSEC  (1000)

#define DEFAULT_NUM_WRITES (10)

#define CMD_START (1)
#define CMD_STOP  (2)

#if !defined(MIN)
#  define MIN(a,b) ((a)<=(b)?(a):(b))
#endif

/****************************************************************
 *  STRUCTURES
 ****************************************************************/
typedef struct _Args
{
  int sample_size; /* bytes per sample */
  int delay;       /* nanosec between writes */
  int num_writes;  /* up to this many writes per iter      */
  int test_length; /* test length in seconds */
  int num_readers; /* number of readers to expect */
  int keep_last;   /* keep last or keep all */
  int latency_budget; /* nanosec latency budget */
  int reliable;    /* reliable vs best_effort */
  int use_threads; /* run with threads */
  int run_forever; 
  int verbose;
} Args;

/****************************************************************
 * FORWARD DECLS
 ****************************************************************/
void cmd_on_data_avail(const DDS_DataReader the_reader);
void bw_on_offered_incompatible_qos(DDS_DataWriter writer, 
					DDS_OfferedIncompatibleQosStatus status);
void bw_on_publication_matched(DDS_DataWriter writer, 
			       DDS_PublicationMatchedStatus status);
void bw_on_data_avail(const DDS_DataReader dr);
     
/****************************************************************
 * GLOBALS
 ****************************************************************/
struct _Args args;

struct cmdType   _cmd = {0, 0, 0};

int _is_source   = 1;
int _num_readers = 0;
int _samples_sent    = 0;
int _samples_recvd   = 0;
int _missed_samples  = 0;
int _next_counter    = 0;

DDS_DataReaderListener cmd_listener = 
{
  NULL, 
  NULL, 
  NULL, 
  NULL, 
  cmd_on_data_avail,
  NULL,
  NULL
};

DDS_DataReaderListener dr_listener = 
{
  NULL, 
  NULL, 
  NULL, 
  NULL, 
  bw_on_data_avail,
  NULL,
  NULL
};

DDS_DataWriterListener dwListener = 
{
  NULL,
  bw_on_offered_incompatible_qos, 
  NULL,
  bw_on_publication_matched
};

/****************************************************************
 *
 ****************************************************************/
int
print_usage(char *pgm)
{
  printf("%s:\n", pgm);
  printf("  -p | -c  -- producer or consumer\n");
  printf("  -s <sample_size>\n");
  printf("  -w <num_writes_per_iter>\n");
  printf("  -d <microsec between iter's>\n");
  printf("  -n <num_readers>\n");
  printf("  -l <test_length(sec)>\n");
  printf("  -k <keep_last samples [0 --> all]>\n");
  printf("  -b <latency budget (microsec)>\n");
  printf("  -v -- verbose\n");
  printf("  -f -- run forever\n");
  printf("  -T -- disable threads\n");
  printf("  -h -- print this message\n");
  return 0;
}

/****************************************************************
 *
 ****************************************************************/
int
parse_cmd_line(int argc, char * argv[])
{
  int retval = 0;
  int c;

  while (1) 
    {
      c = toc_getopt (argc, argv, "s:w:n:l:d:hpcvk:fb:rT");

      if (c == -1)
	break;
      
      switch (c) 
	{
	case 'p': /* producer */
	  _is_source = 1;
	  break;
	case 'c': /* consumer */
	  _is_source = 0;
	  break;
	case 's': /* sample size */
	  args.sample_size = atoi(toc_optarg);
	  break;
	case 'w': /* min writes  */
	  args.num_writes = atoi(toc_optarg);
	  break;
	case 'n': /* num readers */
	  args.num_readers = atoi(toc_optarg);
	  break;
	case 'l': /* test length  - seconds */
	  args.test_length = atoi(toc_optarg);
	  break;
	case 'd': /* delay between writes (provided in usec)->nsec*/
	  args.delay          = (atof(toc_optarg) * NANOSEC_PER_MICROSEC);
	  break;
	case 'b': /* latency budget (in microsec)->nsec*/
	  args.latency_budget = (atof(toc_optarg) * NANOSEC_PER_MICROSEC);
	  break;
	case 'f':
	  args.run_forever = 1;
	  break;
	case 'k':
	  args.keep_last = atoi(toc_optarg);
	  break;
	case 'r':
	  args.reliable = 1;
	  break;
	case 'T':
	  args.use_threads = 0;
	  break;
	case 'v':
	  args.verbose = 1;
	  break;
	case 'h':
	  print_usage(argv[0]);
	  return -1;
	  break;
	default:
	  printf ("?? getopt returned character code 0%o ??\n", c);
	}
    }

  return retval;
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

  CLOCK_GETTIME(TOC_CLOCK_REALTIME, &nowts);
  test_end = start->tv_sec + ((double)start->tv_nsec / NANOSEC_PER_SEC) + args.test_length;
  now      = nowts.tv_sec + ((double)nowts.tv_nsec / NANOSEC_PER_SEC);

  if ( now >= test_end)
    return 1;
  else
    return 0;
}

/****************************************************************
 *
 ****************************************************************/
void cmd_on_data_avail(const DDS_DataReader dr)
{
  cmdTypePtrSeq     samples;
  DDS_SampleInfoSeq samples_info;
  DDS_ReturnCode_t  retval;
  
  INIT_SEQ(samples);
  INIT_SEQ(samples_info);

  retval = cmdTypeDataReader_take( dr, &samples, &samples_info, 
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
	      _cmd  = *samples._buffer[i];
	      if (args.verbose)
		printf("    (got cmd = %d)\n", _cmd.cmd);
	    }
	}
      
      /* return the 'loaned' lists */
      cmdTypeDataReader_return_loan( dr, &samples, &samples_info );
    }
}

/****************************************************************
 *
 ****************************************************************/
void bw_on_data_avail(const DDS_DataReader dr)
{
  bwTypePtrSeq      samples;
  DDS_SampleInfoSeq samples_info;
  DDS_ReturnCode_t  retval;
  
  INIT_SEQ(samples);
  INIT_SEQ(samples_info);

  retval = bwTypeDataReader_take( dr, &samples, &samples_info, 
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
	      /* check for missed samples */
	      if (samples._buffer[i]->counter > _next_counter)
		{
		  if (args.verbose ) 
                    printf("got counter = %d, expected %d.\n", samples._buffer[i]->counter, _next_counter);
		  _missed_samples += samples._buffer[i]->counter - _next_counter;
		}
	      
	      _samples_recvd ++;
	      _next_counter = samples._buffer[i]->counter+1;
	    }
	}
      
      /* return the 'loaned' lists */
      bwTypeDataReader_return_loan( dr, &samples, &samples_info );
    }
}

void bw_on_offered_incompatible_qos(DDS_DataWriter writer, 
					DDS_OfferedIncompatibleQosStatus status)
{
  if ( args.verbose)
    printf("App_DW_Listener::on_offered_incompat_qos()\n");
}

/****************************************************************
 *
 ****************************************************************/
void bw_on_publication_matched(DDS_DataWriter writer, 
			       DDS_PublicationMatchedStatus status)
{
  if ( args.verbose)
    printf("App_DW_Listener::on_pub_matched() - %d current (%d total) readers\n",
	   status.current_count, status.total_count );
  _num_readers = status.current_count;
}

/****************************************************************
 *
 ****************************************************************/
int 
main(int argc, char * argv[] )
{
  DDS_DomainId_t             did = 123;
  DDS_StatusMask             sm;
  DDS_DomainParticipant      domain1;
  DDS_DomainParticipantQos   dp_qos;
  DDS_Publisher              publisher1;
  DDS_Subscriber             subscriber;
  DDS_Topic                  bw_topic;
  DDS_Topic                  cmd_topic;
  char                     * topic_name = "bwTopic";
  char                     * type_name  = "bwType";
  char                     * cmd_topic_name = "cmdTopic";
  char                     * cmd_type  = "cmdType";

  args.sample_size = 1;
  args.delay       = 1000; /* nanosec */
  args.num_writes  = DEFAULT_NUM_WRITES;
  args.test_length = 60; /* ten seconds */
  args.num_readers = 1;
  args.keep_last   = 0;
  args.latency_budget = NSEC_PER_USEC*1000;
  args.reliable    = 0;
  args.run_forever = 0;
  args.use_threads = 1;
  args.verbose     = 0;
  
  if (parse_cmd_line(argc, argv) < 0 )
    exit(0);

  /**************************************************/
  /* Create domain participant                      */
  /**************************************************/
  DDS_DomainParticipantFactory_get_default_participant_qos(&dp_qos);
  dp_qos.thread_model.use_threads = args.use_threads;
  dp_qos.discovery.send_initial_nack = 0;
  dp_qos.discovery.heartbeat_period.sec = 0;
  dp_qos.discovery.heartbeat_period.nanosec = NSEC_PER_SEC/10;

  sm = 0;
  domain1   = 
    DDS_DomainParticipantFactory_create_participant( did, &dp_qos, NULL, sm );

  /**************************************************/
  /* create a DDS_Publisher                             */
  /**************************************************/
  sm = 0;
  publisher1 = DDS_DomainParticipant_create_publisher(domain1, 
						      DDS_PUBLISHER_QOS_DEFAULT,
						      NULL, sm );
  /**************************************************/
  /* create a DDS_Publisher                             */
  /**************************************************/
  sm = 0;
  subscriber = DDS_DomainParticipant_create_subscriber(domain1, 
					      DDS_SUBSCRIBER_QOS_DEFAULT, 
					      NULL, sm );

  /**************************************************/
  /*  Create Type/DDS_Topic        for TEST DATA    */
  /**************************************************/
  /* register my data type */
  bwTypeTypeSupport_register_type( domain1, NULL );
  
  /* create a DDS_Topic */
  bw_topic =
    DDS_DomainParticipant_create_topic(domain1, topic_name, type_name, 
		    DDS_TOPIC_QOS_DEFAULT, NULL, sm );
  
  if ( bw_topic == NULL )
    {
      fprintf(stderr, "ERROR creating topic.\n");
      exit ( 1 );
    }

  /**************************************************/
  /*  Create Type/DDS_Topic for Commands            */
  /**************************************************/
  
  cmdTypeTypeSupport_register_type( domain1, NULL);
  cmd_topic =
    DDS_DomainParticipant_create_topic(domain1, cmd_topic_name, cmd_type,
		    DDS_TOPIC_QOS_DEFAULT, NULL, 0);
  

  if (_is_source)
    {
      DDS_DataWriterQos dwqos;
      DDS_DataWriter    cmd_dw;
      DDS_DataWriter    bw_dw;
      DDS_ReturnCode_t  ret;
      struct timespec   delay;
      struct timespec   start_time;
      int               i            = args.num_writes;
      int               test_done    = 0;
      int               num_sent     = 0;
      DDS_Time_t        dds_time     = { 0, 0 }; /* not important in this test  */
      struct bwType     data;
      struct cmdType    cmd;
#if defined(_WIN32)
      int               udelay       = args.delay / NSEC_PER_USEC;
#endif

      if (args.verbose)
	printf("Running as SOURCE\n");
	 
      /**************************************************/
      /**************************************************/
      /*  Init and run data source                      */
      /**************************************************/
      /**************************************************/

      /* create the command writer */

      DDS_Publisher_get_default_datawriter_qos( publisher1, &dwqos );
      dwqos.history.kind     = DDS_KEEP_ALL_HISTORY_QOS;
      dwqos.reliability.kind = DDS_RELIABLE_RELIABILITY_QOS;
      cmd_dw = 
	DDS_Publisher_create_datawriter(publisher1, 
			    cmd_topic,
			    &dwqos,
			    NULL, 
			    0);
      if ( ! cmd_dw )
	{
	  fprintf(stderr, "ERROR creating COMMAND data writer.\n");
	  exit(1);
	}

      /* create the 'data' writer */
      DDS_Publisher_get_default_datawriter_qos( publisher1, &dwqos );
      dwqos.rtps_writer.max_buffer_size = 65400;
      dwqos.rtps_writer.min_buffer_size = 65400;
      if (args.keep_last==0)
        dwqos.history.kind=DDS_KEEP_ALL_HISTORY_QOS;
      else
        {
          dwqos.history.kind  = DDS_KEEP_LAST_HISTORY_QOS;
          dwqos.history.depth = args.keep_last;
        }
      if (args.latency_budget > 0)
        {
          dwqos.latency_budget.duration.sec     = args.latency_budget / NANOSEC_PER_SEC;
          dwqos.latency_budget.duration.nanosec = args.latency_budget % NANOSEC_PER_SEC;
        }
      if (args.reliable)
	{
	  dwqos.reliability.kind = DDS_RELIABLE_RELIABILITY_QOS;
	  dwqos.reliability.max_blocking_time.sec     = 1;
	  dwqos.reliability.max_blocking_time.nanosec = 0;
	  dwqos.resource_limits.max_samples              = 1000;
	  dwqos.resource_limits.max_instances            = 1;
	  dwqos.resource_limits.max_samples_per_instance = 1000;
	}
          
      sm = DDS_LIVELINESS_LOST_STATUS | 
	DDS_PUBLICATION_MATCHED_STATUS | 
	DDS_OFFERED_INCOMPATIBLE_QOS_STATUS | 
	DDS_OFFERED_DEADLINE_MISSED_STATUS ;
      
      bw_dw = 
	DDS_Publisher_create_datawriter(publisher1, 
			    bw_topic, 
			    &dwqos, //DDS_DATAWRITER_QOS_DEFAULT, 
			    &dwListener, 
			    sm );
      if ( ! bw_dw )
	{
	  fprintf(stderr, "ERROR creating TEST data writer.\n");
	  exit(1);
	}

      /* pause for everything to settle */
      if (args.use_threads)
	toc_sleep(USEC_PER_SEC);
      else
	{
	  DDS_Duration_t    work_slice;
	  work_slice.sec     = 1;
	  work_slice.nanosec = 0;
	  DDS_DomainParticipant_do_work(domain1, &work_slice);
	}

      /********************************************/
      /* initialize the sample                    */
      /********************************************/
#if USE_SEQUENCE
      INIT_SEQ(data.value);
      seq_set_size(&data.value, args.sample_size);
      seq_set_length(&data.value, args.sample_size);
      /* initialize seq data to '0' */
      { 
	int si;
	for(si=0;si<args.sample_size;si++)
	  data.value._buffer[si] = 0;
      }
#else
      memset(data.value, 'a', MIN(args.sample_size, MAX_SIZE));
#endif      
      data.counter=0;
      
      /********************************************/
      /* wait for all the 'readers' to check in   */
      /********************************************/
      while ( _num_readers < args.num_readers )
	{
	  if (args.use_threads) 
	    toc_sleep(100);
	  else
	    {
	      DDS_Duration_t    work_slice;
	      work_slice.sec     = 0;
	      work_slice.nanosec = NSEC_PER_SEC/10;
	      DDS_DomainParticipant_do_work(domain1, &work_slice);
	    }
	}
      
      if (args.use_threads) 
	toc_sleep(MICROSEC_PER_SEC);
      else
	{
	  DDS_Duration_t    work_slice;
	  work_slice.sec     = 1;
	  work_slice.nanosec = 0;
	  DDS_DomainParticipant_do_work(domain1, &work_slice);
	}

      if (args.verbose)
	printf(" - Starting test: %i writes per iter\n", i);
      
      /********************************************/
      /* send a 'start' command */
      /********************************************/
      cmd.cmd         = CMD_START;
      cmd.sample_size = args.sample_size;
      cmd.num_writes  = i;
      DDS_DataWriter_write_w_timestamp(cmd_dw, &cmd, DDS_HANDLE_NIL, dds_time);
      
      CLOCK_GETTIME(TOC_CLOCK_REALTIME, &start_time);
      
      /********************************************/
      /* run a test                               */
      /********************************************/
      test_done = 0;
      while ( ! test_done )
	{
	  int j;
	  
	  /* write 'i' samples */
	  for (j = 0; j < i; j++)
	    {
	      ret = DDS_DataWriter_write_w_timestamp ( bw_dw, &data, DDS_HANDLE_NIL, dds_time );
	      num_sent++;
	      
	      if ( ret != DDS_RETCODE_OK) 
		fprintf(stderr, "DataWriter_write() error...: %s\n", DDS_error(ret));
	      data.counter++;
	    }
	  test_done = test_time_elapsed(&start_time);
	  if ( (!test_done) && (args.delay!=0))
	    {
	      /* pause for 'delay' */
	      if (args.use_threads)
		{
#if defined(_WIN32)
		  toc_sleep(udelay); /* not as accurate as nanosleep(), but windows has nothing better */
#else	      
		  delay.tv_sec = 0;
		  delay.tv_nsec = args.delay;
		  while ((nanosleep(&delay, &delay) < 0) && 
			 (errno==EINTR))
		    ; /* do nothing */
#endif
		}
	      else
		{
		  DDS_Duration_t    work_slice;
		  work_slice.sec     = 0;
		  work_slice.nanosec = args.delay;
		  DDS_DomainParticipant_do_work(domain1, &work_slice);
		}
	      test_done = test_time_elapsed(&start_time);
	    }
	}
      
      if (args.verbose)
	printf(" - Test done\n");
      
      /********************************************/
      /* send a 'stop' command */
      /********************************************/
      cmd.cmd         = CMD_STOP;
      DDS_DataWriter_write(cmd_dw, &cmd, DDS_HANDLE_NIL);
      
      printf("%d, %d\n", 
	     cmd.sample_size, 
	     num_sent);
      
      /********************************************/
      /* wait for all the 'readers' to check out */
      /********************************************/
      if (args.verbose)
	printf("waiting for %d readers to exit...\n", _num_readers);
      while (_num_readers > 0)
	{
	  if (args.use_threads)
	    toc_sleep(100);
	  else
	    {
	      DDS_Duration_t    work_slice;
	      work_slice.sec     = 0;
	      work_slice.nanosec = NSEC_PER_SEC/100;
	      DDS_DomainParticipant_do_work(domain1, &work_slice);
	    }
	}
      
      /* clean up writers */
      DDS_Publisher_delete_datawriter(publisher1, bw_dw);
      DDS_Publisher_delete_datawriter(publisher1, cmd_dw);
    }
  else
    {
      DDS_DataReader cmd_dr;
      DDS_DataReader bw_dr;
      struct timespec start_time;
      struct timespec end_time;
      DDS_ReturnCode_t retval;
      DDS_DataReaderQos drqos;
      int done = 0;


      /**************************************************/
      /**************************************************/
      /*  Init and run data reader                      */
      /**************************************************/
      /**************************************************/
      if (args.verbose)
	printf("Running as CONSUMER.\n");


      DDS_Subscriber_get_default_datareader_qos(subscriber, &drqos);
      drqos.history.kind     = DDS_KEEP_ALL_HISTORY_QOS;
      drqos.reliability.kind = DDS_RELIABLE_RELIABILITY_QOS;

      /* create COMMAND DDS_DataReader     */
      cmd_dr = 
	DDS_Subscriber_create_datareader(subscriber, 
                                         DDS_Topic_TopicDescription(cmd_topic),
                                         &drqos,
                                         &cmd_listener, 
                                         DDS_DATA_AVAILABLE_STATUS);
      if ( ! cmd_dr )
	{
	  fprintf(stderr, "ERROR creating COMMAND data reader.\n");
	  exit(1);
	}

      /* run multiple tests */
      while (!done)
	{
	  struct timespec waittime;
	  waittime.tv_sec  = 0;
	  waittime.tv_nsec = 10*NANOSEC_PER_MILLISEC;

	  /*   create TEST DDS_DataReader      */
	  DDS_Subscriber_get_default_datareader_qos(subscriber, &drqos);
	  if (args.keep_last==0)
	    drqos.history.kind=DDS_KEEP_ALL_HISTORY_QOS;
	  else
	    {
	      drqos.history.kind  = DDS_KEEP_LAST_HISTORY_QOS;
	      drqos.history.depth = args.keep_last;
	    }
          if (args.latency_budget > 0)
            {
              drqos.latency_budget.duration.sec     = args.latency_budget / NANOSEC_PER_SEC;
              drqos.latency_budget.duration.nanosec = args.latency_budget % NANOSEC_PER_SEC;
            }
	  if (args.reliable)
	    {
	      drqos.reliability.kind = DDS_RELIABLE_RELIABILITY_QOS;
#if 0
	      drqos.reliability.max_blocking_time.sec     = 1;
	      drqos.reliability.max_blocking_time.nanosec = 0;
	      drqos.resource_limits.max_samples=100;
	      drqos.resource_limits.max_instances=1;
	      drqos.resource_limits.max_samples_per_instance=100;
#endif
	    }

	  if (args.verbose)
	    printf("creating data DR\n");
	  bw_dr =
	    DDS_Subscriber_create_datareader(subscriber,
				DDS_Topic_TopicDescription(bw_topic), 
				&drqos, /*DDS_DATAWRITER_QOS_DEFAULT,  */
				&dr_listener, 
				DDS_DATA_AVAILABLE_STATUS );
	  if ( ! bw_dr )
	    {
	      fprintf(stderr, "ERROR creating TEST data reader.\n");
	      exit(1);
	    }

	  if (args.verbose)
	    printf(" -- waiting for 'START'.\n");

	  /* reset counters */
	  _samples_recvd  = 0;
	  _missed_samples = 0;
	  _next_counter   = 0;

	  /*   wait for CMD_START ...      */
	  while ( _cmd.cmd != CMD_START)
	    if (args.use_threads)
	      toc_sleep(USEC_PER_MSEC * 10);
	    else
	      {
		DDS_Duration_t    work_slice;
		work_slice.sec     = 0;
		work_slice.nanosec = NSEC_PER_SEC/100;
		DDS_DomainParticipant_do_work(domain1, &work_slice);
	      }

	  if (args.verbose)
	    printf(" -- starting... (%d writes)\n", _cmd.num_writes);

	  CLOCK_GETTIME(TOC_CLOCK_REALTIME, &start_time);

	  /* wait for end of this test     */
	  while ( _cmd.cmd != CMD_STOP )
	    if (args.use_threads)
	      toc_sleep(USEC_PER_MSEC * 10);
	    else
	      {
		DDS_Duration_t    work_slice;
		work_slice.sec     = 0;
		work_slice.nanosec = NSEC_PER_SEC/100;
		DDS_DomainParticipant_do_work(domain1, &work_slice);
	      }

	  CLOCK_GETTIME(TOC_CLOCK_REALTIME, &end_time);

	  if (args.verbose)
	    printf(" -- done\n");

	  if (args.use_threads)
	    toc_sleep(USEC_PER_SEC);
	  else
	    {
	      DDS_Duration_t    work_slice;
	      work_slice.sec     = 1;
	      work_slice.nanosec = 0;
	      DDS_DomainParticipant_do_work(domain1, &work_slice);
	    }

	  /*    delete TEST DDS_DataReader     */
	  if (args.verbose)
	    printf(" -- deleting datareader\n");
	  
	  retval = DDS_Subscriber_delete_datareader(subscriber, bw_dr);
	  if (retval != DDS_RETCODE_OK)
	    printf("failed to delete data DR\n");

	  /* compute throughput */
	  {
	    double num_bytes = 
	      ((double)_samples_recvd) * (_cmd.sample_size + 4); /* 4 is for 'counter'  */
	    double seconds = 
	      (end_time.tv_sec + (end_time.tv_nsec / (double)NANOSEC_PER_SEC)) - 
	      (start_time.tv_sec + (start_time.tv_nsec / (double)NANOSEC_PER_SEC));
	    
	    double Mbps    = num_bytes * 8  / (1000000*seconds) ;
	    double KBps    = num_bytes / (1024* seconds);

	    /*   sample_size, num_samples, num_bytes, Mbps, KBps, missed */
	    printf("size: %d sec: %f ns: %d sps: %.0f nb: %.0f Mbps: %f KBps: %f missed: %d\n", 
		   _cmd.sample_size, 
		   seconds,
		   _samples_recvd, 
		   _samples_recvd / seconds,
		   num_bytes, 
		   Mbps, 
		   KBps, 
		   _missed_samples);
	  }
	  /* see if that was the last test */
	  if (!args.run_forever)
	    done = (_cmd.num_writes == _cmd.num_writes); 

	  if (!done)
	    {
	      if (args.use_threads)
		toc_sleep(USEC_PER_SEC * 2); /* make sure "source" notices the DR is gone */
	      else
		{
		  DDS_Duration_t    work_slice;
		  work_slice.sec     = 1;
		  work_slice.nanosec = 0;
		  DDS_DomainParticipant_do_work(domain1, &work_slice);
		}
	    }
	}

      DDS_Subscriber_delete_datareader(subscriber, cmd_dr);
    }

  /* clean up participant */
  DDS_DomainParticipant_delete_contained_entities(domain1);
  DDS_DomainParticipantFactory_delete_participant(domain1);

  return 0;
}
