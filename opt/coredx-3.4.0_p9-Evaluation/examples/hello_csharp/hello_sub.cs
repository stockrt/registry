/****************************************************************
 *
 *  file:  hello_sub.cs
 *  desc:  Provides a simple C# 'hello world' DDS subscriber.
 *         This publishing application will send data
 *         to the example 'hello world' subscribing 
 *         application.
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
using System;
using System.Runtime.InteropServices;
using System.Threading;
using System.Collections.Generic;

using com.toc.coredx.DDS;

public class hello_sub {

  class TestDataReaderListener : DataReaderListener 
    {
      public TestDataReaderListener()
        {
          this.on_requested_deadline_missed   = requested_deadline_missed;
          this.on_requested_incompatible_qos  = requested_incompatible_qos;
          this.on_sample_rejected             = sample_rejected;
          this.on_liveliness_changed          = liveliness_changed;
          this.on_data_available              = data_available;
          this.on_subscription_matched        = subscription_matched;
          this.on_sample_lost                 = sample_lost;
        }

      public void requested_deadline_missed(DataReader dr,
					       RequestedDeadlineMissedStatus status) 
      { 
	System.Console.WriteLine(" @@@@@@@@@@@     REQUESTED DEADLINE MISSED    @@@@@"); 
	System.Console.WriteLine(" @@@@@@@@@@@                                  @@@@@" );
	System.Console.WriteLine(" @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"); 
      }

      public void requested_incompatible_qos(DataReader dr,
						RequestedIncompatibleQosStatus status) 
      { 
	System.Console.WriteLine(" @@@@@@@@@@@     REQUESTED INCOMPAT QOS    @@@@@@@@"); 
	System.Console.WriteLine(" @@@@@@@@@@@        dr      = " + dr);
	System.Console.WriteLine(" @@@@@@@@@@@                               @@@@@@@@" );
	System.Console.WriteLine(" @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"); 
      }

      public void sample_rejected(DataReader dr, 
				     SampleRejectedStatus status)
      { 
      }

      public void liveliness_changed(DataReader dr,
					LivelinessChangedStatus status)
      {
	TopicDescription   td = dr.get_topicdescription();
	System.Console.WriteLine(" @@@@@@@@@@@     LIVELINESS CHANGED      @@@@@@@@@@"); 
	System.Console.WriteLine(" @@@@@@@@@@@        topic   = " + td.get_name() + " (type: " + td.get_type_name() + ")");
	System.Console.WriteLine(" @@@@@@@@@@@        change  = " + status.alive_count_change);
	System.Console.WriteLine(" @@@@@@@@@@@        current = " + status.alive_count);
	System.Console.WriteLine(" @@@@@@@@@@@                             @@@@@@@@@@" );
	System.Console.WriteLine(" @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"); 
      }

      public void subscription_matched(DataReader dr, 
					  SubscriptionMatchedStatus status)
      { 
	TopicDescription   td = dr.get_topicdescription();
	System.Console.WriteLine(" @@@@@@@@@@@     SUBSCRIPTION MATCHED    @@@@@@@@@@"); 
	System.Console.WriteLine(" @@@@@@@@@@@        topic   = " + td.get_name() + " (type: " + td.get_type_name() + ")");
	System.Console.WriteLine(" @@@@@@@@@@@        current = " + status.current_count);
	System.Console.WriteLine(" @@@@@@@@@@@                             @@@@@@@@@@" );
	System.Console.WriteLine(" @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"); 
      }

      public void sample_lost(DataReader dr, 
                              SampleLostStatus status) 
      { 
      }


      public void data_available(DataReader dr)
      { 
	TopicDescription td = dr.get_topicdescription();

	System.Console.WriteLine(" @@@@@@@@@@@     DATA AVAILABLE          @@@@@@@@@@"); 
	System.Console.WriteLine(" @@@@@@@@@@@        topic = " + td.get_name() + " (type: " + td.get_type_name() + ")");

	StringMsgDataReader string_dr = (StringMsgDataReader)dr;
	List<StringMsg>     samples = new List<StringMsg>();
	List<SampleInfo>    si      = new List<SampleInfo>();
	ReturnCode_t        retval  = string_dr.take(samples, si, 100, 
                                                     DDS.ANY_SAMPLE_STATE, 
                                                     DDS.ANY_VIEW_STATE, 
                                                     DDS.ANY_INSTANCE_STATE);
	System.Console.WriteLine(" @@@@@@@@@@@        DR.read() ===> " + retval);

	if (retval == ReturnCode_t.RETCODE_OK)
	  {
            System.Console.WriteLine(" @@@@@@@@@@@        samples.Count= " + samples.Count);
            for (int i = 0; i < samples.Count; i++)
              {
                System.Console.WriteLine("    State       : " + 
                                         (si[i].instance_state == 
                                          DDS.ALIVE_INSTANCE_STATE?"ALIVE":"NOT ALIVE") );
                System.Console.WriteLine("    TimeStamp   : " + 
                                         si[i].source_timestamp.sec + "." +
                                         si[i].source_timestamp.nanosec);
                System.Console.WriteLine("    Handle      : " + si[i].instance_handle.value);
                System.Console.WriteLine("    WriterHandle: " + si[i].publication_handle.value);
                System.Console.WriteLine("    SampleRank  : " + si[i].sample_rank);
                if (si[i].valid_data)
                  System.Console.WriteLine("       msg      : " + samples[i].msg);
              }
	    string_dr.return_loan(samples, si);
	  }
	System.Console.WriteLine(" @@@@@@@@@@@                             @@@@@@@@@@" );
	System.Console.WriteLine(" @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"); 
      }
    }

  // -----------------------------------------------------------------------
  //
  // -----------------------------------------------------------------------
  public static void Main()
  {

    System.Console.WriteLine("STARTING -------------------------");
    DomainParticipantFactory dpf = DomainParticipantFactory.get_instance();
    DomainParticipant dp = null;

    System.Console.WriteLine("CREATE PARTICIPANT ---------------");
    dp = dpf.create_participant(0,    /* domain Id   */
				DDS.PARTICIPANT_QOS_DEFAULT, /* default qos */
				null, /* no listener */
				0);

    System.Console.WriteLine("REGISTERING TYPE -----------------");
    ReturnCode_t retval = StringMsgTypeSupport.register_type(dp, null); //"StringMsg");
    
    System.Console.WriteLine("CREATE TOPIC ---------------------");
    Topic              top          = dp.create_topic("helloTopic", 
                                                      StringMsgTypeSupport.get_type_name(), 
						      DDS.TOPIC_QOS_DEFAULT, 
						      null, 0); // no listener
    
    System.Console.WriteLine("CREATE SUBSCRIBER ----------------");
    SubscriberQos       sub_qos      = null;
    SubscriberListener  sub_listener = null;
    Subscriber          sub          = dp.create_subscriber(sub_qos, sub_listener, 0);  

    System.Console.WriteLine("CREATE DATAREADER ----------------");
    DataReaderQos dr_qos = new DataReaderQos();
    sub.get_default_datareader_qos(dr_qos);
    dr_qos.history.depth = 10;
    DataReaderListener dr_listener = new TestDataReaderListener();
    StringMsgDataReader   dr = (StringMsgDataReader) sub.create_datareader(top, 
								     dr_qos, 
								     dr_listener, 
								     DDS.ALL_STATUS);

    
    while ( true ) {
      Thread.Sleep(5000);   // 5 second sleep
    }
    
  }
};
