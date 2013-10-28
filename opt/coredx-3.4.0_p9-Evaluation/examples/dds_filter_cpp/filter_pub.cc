/****************************************************************
 *
 *  file:  filter_pub.c
 *  desc:  Provides a simple C++ ContentFilter example
 *         This publishing application will send data
 *         to the example 'filter_sub' subscribing 
 *         application.
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
#include <dds/dds.hh>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include "filter.hh"
#include "filterTypeSupport.hh"
#include "filterDataWriter.hh"

#ifdef _WIN32
#  define SLEEP(d) Sleep((d)*1000)
#else
#  define SLEEP(d) sleep((d))
#endif

/****************************************************************
 * main()
 *
 * Perform CoreDX DDS setup activities:
 *   - create a Domain Participant
 *   - create a Publisher
 *   - register the FilterMsg data type
 *   - create a Topic
 *   - create a DataWriter 
 * Write data
 ****************************************************************/

int main(int argc, char * argv[])
{
  DomainParticipant  * domain;
  Publisher          * publisher;
  Topic              * topic;
  DataWriter         * dw;
  FilterMsg          filterMsg;
  ReturnCode_t       retval;

  class DomainParticipantFactory * dpf = 
    DomainParticipantFactory::get_instance();

  /* create a DomainParticipant */
  domain = dpf->create_participant( 0, 
				    PARTICIPANT_QOS_DEFAULT, 
				    NULL, 
				    0 );
  if ( domain == NULL )
    {
      printf("ERROR creating domain participant.\n");
      return -1;
    }
  
  /* create a Publisher */
  publisher = domain->create_publisher(PUBLISHER_QOS_DEFAULT, 
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
  FilterMsgTypeSupport filterMsgTS;
  retval = filterMsgTS.register_type( domain, NULL );
  if (retval != RETCODE_OK)
    {
      printf("ERROR registering type\n");
      return -1;
    }
  
  /* Create a DDS Topic with the FilterMsg data type. */
  topic = domain->create_topic("TestTopic", 
			       "FilterMsg", 
			       TOPIC_QOS_DEFAULT, 
			       NULL, 
			       0 );
  if ( topic == NULL )
    {
      printf("ERROR creating topic.\n");
      return -1;
    }
  
  /* Create a DataWriter on the filter topic, with
   * default QoS settings and no listeners.
   */
  dw = publisher->create_datawriter( topic, 
				     DATAWRITER_QOS_DEFAULT, 
				     NULL, 
				     0 );
  if (dw == NULL)
    {
      printf("ERROR creating data writer\n");
      return -1;
    }

  /* Initialize the data to send.  The FilterMsg data type
   * has just one filter member.
   * Note: Alwyas initialize a filter member with
   * allocated memory -- the destructor will free 
   * all filter members.  
   */
  filterMsg.msg   = new char[strlen("Hello WORLD from C++!")+1];
  strcpy(filterMsg.msg, "Hello WORLD from C++!");
  filterMsg.x     = 0;
  filterMsg.count = 0;

  while ( 1 )
    {
      ReturnCode_t ret = dw->write ( &filterMsg, HANDLE_NIL ); 
      printf("wrote sample: x: %d count: %d\n", filterMsg.x, filterMsg.count);
      fflush(stdout);
      if ( ret != RETCODE_OK)
	{
	  printf("ERROR writing sample\n");
	  return -1;
	}
      SLEEP(1);
      filterMsg.x = (filterMsg.x + 1)%15;
      filterMsg.count ++;
    }

    /* Cleanup */
    retval = domain -> delete_contained_entities();
    if ( retval != DDS_RETCODE_OK )
      printf("ERROR (%s): Unable to cleanup DDS entities\n",
	     DDS_error(retval));


  return 0;
}
