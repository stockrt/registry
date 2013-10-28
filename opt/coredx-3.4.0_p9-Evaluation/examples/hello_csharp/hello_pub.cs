/****************************************************************
 *
 *  file:  hello_pub.cs
 *  desc:  Provides a simple C# 'hello world' DDS publisher.
 *         This publishing application will send data
 *         to the example 'hello world' subscribing 
 *         application (hello_sub).
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

public class hello_pub {

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
    ReturnCode_t retval = StringMsgTypeSupport.register_type(dp, null); 
      
    System.Console.WriteLine("CREATE TOPIC ---------------------");
    Topic              top          = dp.create_topic("helloTopic", StringMsgTypeSupport.get_type_name(), 
						      null,     // default qos
						      null, 0); // no listener
      
    System.Console.WriteLine("CREATE PUBLISHER -----------------");
    PublisherQos       pub_qos      = null;
    PublisherListener  pub_listener = null;
    Publisher          pub          = dp.create_publisher(pub_qos, pub_listener, 0);  

    System.Console.WriteLine("CREATE DATAWRITER ----------------");
    DataWriterQos dw_qos = new DataWriterQos();
    pub.get_default_datawriter_qos(dw_qos);
    DataWriterListener dw_listener = null;
    StringMsgDataWriter   dw = (StringMsgDataWriter) pub.create_datawriter(top, 
								     dw_qos, 
								     dw_listener, 
								     0);

    Thread.Sleep(500); 

    while ( true ) {
      StringMsg data = new StringMsg();
      data.msg    = "Hello WORLD from C#!";
      
      System.Console.WriteLine("WRITE SAMPLE. ");

      /* DDS.HANDLE_NIL says datawriter should compute handle */
      retval = dw.write ( data, DDS.HANDLE_NIL ); 
      
      if ( retval != ReturnCode_t.RETCODE_OK )
	System.Console.WriteLine( "   ====  DDS_DataWriter_write() error... ");

      Thread.Sleep(1000);   // 1 second sleep
    }
  }
};
