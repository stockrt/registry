/****************************************************************
 *
 *  file:  hello_pub.c
 *  desc:  Provides a simple C++ 'hello world' DDS publisher.
 *         This publishing application will send data
 *         to the example 'hello world' subscribing 
 *         application (hello_sub).
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
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include "hello.hh"
#include "helloTypeSupport.hh"
#include "helloDataWriter.hh"


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
  DomainParticipant  * domain;
  Publisher          * publisher;
  Topic              * topic;
  DataWriter         * dw;
  StringMsg          stringMsg;
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
  StringMsgTypeSupport stringMsgTS;
  retval = stringMsgTS.register_type( domain, NULL );
  if (retval != RETCODE_OK)
    {
      printf("ERROR registering type\n");
      return -1;
    }
  
  /* Create a DDS Topic with the StringMsg data type. */
  topic = domain->create_topic("helloTopic", 
			       "StringMsg", 
			       TOPIC_QOS_DEFAULT, 
			       NULL, 
			       0 );
  if ( topic == NULL )
    {
      printf("ERROR creating topic.\n");
      return -1;
    }
  
  /* Create a DataWriter on the hello topic, with
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

  /* Initialize the data to send.  The StringMsg data type
   * has just one string member.
   * Note: Alwyas initialize a string member with
   * allocated memory -- the destructor will free 
   * all string members.  
   */
  stringMsg.msg = new char[strlen("Hello WORLD from C++!")+1];
  strcpy(stringMsg.msg, "Hello WORLD from C++!");
  
  while ( 1 )
    {
      ReturnCode_t ret = dw->write ( &stringMsg, HANDLE_NIL ); 
      printf("wrote a sample\n");
      fflush(stdout);
      if ( ret != RETCODE_OK)
	{
	  printf("ERROR writing sample\n");
	  return -1;
	}
#ifdef _WIN32
      Sleep(1000);
#else
      sleep(1);
#endif
    }

    /* Cleanup */
    retval = domain -> delete_contained_entities();
    if ( retval != DDS_RETCODE_OK )
      printf("ERROR (%s): Unable to cleanup DDS entities\n",
	     DDS_error(retval));


  return 0;
}
