/****************************************************************
 *
 *  file:  hello_sub.c
 *  desc:  Provides a simple C++ 'hello world' DDS subscriber.
 *         This subscribing application will receive data
 *         from the example 'hello world' publishing 
 *         application (hello_pub).
 * 
 ****************************************************************
 *
 *   Coypright(C) 2006-2011 Twin Oaks Computing, Inc
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
#include <dds/dds.hh>

#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include "hello.hh"
#include "helloTypeSupport.hh"
#include "helloDataReader.hh"

#ifdef _WIN32
#  define SLEEP(a)    Sleep(a*1000)
#else
#  define SLEEP(a)    sleep(a);
#endif

int all_done = 0;

/****************************************************************
 * Construct a DataReaderListener and override the 
 * on_data_available() method with our own.  All other
 * listener methods will be default (no-op) functions.
 ****************************************************************/
class SubListener : public DataReaderListener
{
public:
  void on_data_available( DataReader * dr );
};

/****************************************************************
 * DataReader Listener Method: on_data_avail()
 *
 * This listener method is called when data is available to
 * be read on this DataReader.
 ****************************************************************/
void SubListener::on_data_available( DataReader * dr)
{
  StringMsgPtrSeq   samples;
  SampleInfoSeq     samples_info;
  ReturnCode_t      retval;
  SampleStateMask   ss = DDS_ANY_SAMPLE_STATE;
  ViewStateMask     vs = DDS_ANY_VIEW_STATE;
  InstanceStateMask is = DDS_ANY_INSTANCE_STATE;

  /* Convert to our type-specific DataReader */
  StringMsgDataReader * reader = StringMsgDataReader::narrow( dr );

  /* Take any and all available samples.  The take() operation
   * will remove the samples from the DataReader so they
   * won't be available on subsequent read() or take() calls.
   */
  retval = reader->take( &samples, &samples_info, 
			 LENGTH_UNLIMITED, 
			 ss, 
			 vs, 
			 is );
  if ( retval == RETCODE_OK )
    {
      /* iterrate through the samples */
      for ( unsigned int i = 0;i < samples.size(); i++)
	{
	  /* If this sample does not contain valid data,
	   * it is a dispose or other non-data command,
	   * and, accessing any member from this sample 
	   * would be invalid.
	   */
	  if ( samples_info[i]->valid_data)
	    printf("Sample Received:  msg %d = %s\n", i, 
		   samples[i]->msg);
	}

      fflush(stdout);

      /* read() and take() always "loan" the data, we need to
       * return it so CoreDX can release resources associated
       * with it.  
       */
      reader->return_loan( &samples, &samples_info );
    }
  else
    {
      printf("ERROR (%s) taking samples from DataReader\n",
	     DDS_error(retval));
    }
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

#if defined(__vxworks) && !defined(__RTP__)
int hello_sub(char * args)
#else
int main(int argc, char * argv[])
#endif
{
  DomainParticipant   * domain;
  Subscriber          * subscriber;
  Topic               * topic;
  DataReader          * dr;
  SubListener           drListener;
  ReturnCode_t          retval;

  /* Get an instance of the DDS DomainPartiticpantFactory -- 
   * we will use this to create a DomainParticipant.
   */
  DomainParticipantFactory * dpf = 
    DomainParticipantFactory::get_instance();

  /* create a DomainParticipant */
  domain = 
    dpf->create_participant( 0, 
			     PARTICIPANT_QOS_DEFAULT, 
			     NULL, 
			     0 );
  if ( domain == NULL )
    {
      printf("ERROR creating domain participant.\n");
      return -1;
    }

  /* create a Subscriber */
  subscriber = domain->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
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
  StringMsgTypeSupport stringMsgTS;
  retval = stringMsgTS.register_type( domain, NULL );
  if (retval != RETCODE_OK)
    {
      printf("ERROR (%s) registering type\n", DDS_error(retval));
      return -1;
    }
  
  /* create a DDS Topic with the StringMsg data type. */
  topic = domain->create_topic( "helloTopic", 
				"StringMsg", 
				TOPIC_QOS_DEFAULT, 
				NULL, 
				0 );
  if ( ! topic )
    {
      printf("ERROR creating topic\n");
      return -1;
    }
  
  /* create a DDS_DataReader on the hello topic (notice
   * the TopicDescription is used) with default QoS settings, 
   * and attach our listener with our on_data_available method.
   */
  dr = subscriber->create_datareader( (TopicDescription*)topic, 
				      DATAREADER_QOS_DEFAULT,
				      &drListener, 
				      DATA_AVAILABLE_STATUS );
  if ( ! dr )
    {
      printf("ERROR creating data reader\n");
      return -1;
    }
  
  /* Wait forever.  When data arrives at our DataReader, 
   * our dr_on_data_avilalbe method will be invoked.
   */
  while ( !all_done )
    SLEEP(30);

  /* Cleanup */
  retval = domain -> delete_contained_entities();
  if ( retval != DDS_RETCODE_OK )
    printf("ERROR (%s): Unable to cleanup DDS entities\n",
	   DDS_error(retval));
  
  return 0;
}
