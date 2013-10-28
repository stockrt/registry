/****************************************************************
 *
 *  file:  hello_pub.c
 *  desc:  Provides a simple C 'hello world' DDS publisher.
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
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <dds/dds.h>
#include "hello.h"
#include "helloTypeSupport.h"
#include "helloDataWriter.h"

#ifdef _WIN32
#  define SLEEP(d) Sleep((d)*1000)
#else
#  define SLEEP(d) sleep((d))
#endif

int all_done = 0;

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
#if defined(__vxworks) && !defined(__RTP__)
int hello_pub(char * args)
#else
int main(int argc, char * argv[])
#endif
{
  DDS_DomainParticipant  domain;
  DDS_Publisher          publisher;
  DDS_Topic              topic;
  DDS_DataWriter         dw;
  StringMsg              stringMsg;
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
   * with default QoS settings and no listeners.
   */
  dw = DDS_Publisher_create_datawriter(publisher,
				   topic, 
				   DDS_DATAWRITER_QOS_DEFAULT, 
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
  stringMsg.msg = "Hello WORLD from C!";
  
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
	SLEEP(1);
    }

  /* While forever, write a sample. */
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

      printf("wrote a sample\n");
      fflush(stdout);
      SLEEP(1);
    }

  /* Cleanup */
  DDS_DomainParticipant_delete_contained_entities(domain);
  DDS_DomainParticipantFactory_delete_participant(domain);

  return 0;
}
