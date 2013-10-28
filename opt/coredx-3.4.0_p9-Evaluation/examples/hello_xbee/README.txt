This example is configured to demonstrate the XBee (ZigBee) transport.

REQUIREMENTS:
------------

 - Two or more XBee or XBee Pro radio modules (from Digi) connected to a host 
    computer via Serial port (or Serial over USB)
 
OVERVIEW:
--------

The XBee transport daemon is '<coredx_top>/target/<target_platform>/bin/cdx_xbee_d'.
One daemon is required for each locally connected XBee radio module.
If a machine is connected to one XBee radio (typical case), then run one instance 
of the cdx_xbee_d daemon.

cdx_xbee_d accepts the following command-line options:

usage: ./cdx_xbee_d [-h] [-d] [-v]
    -b <bps>     :  set serial baud rate (default: 115200)
    -d <device>  :  set serial device (default: /dev/ttya)
    -h           :  print this help
    -u <name>    :  specify 'name' for this connection (default: ttya)
    -v           :  verbose output


The XBee radio modules are configured by default to use 9600 baud, and this is the 
only mode that has been tested.

An example invocation:

   ./cdx_xbee_d -b 9600 -d /dev/ttyUSB0 -u xbee0 


Once the daemon is running, then CoreDX DDS enabled applications can be started.  
The applications must be configured to use the XBee transport.

A sample invocation:

   ./hello_pub -t UDS://xbee0 

This instructs CoreDX DDS to connect to the local XBee daemon named 'xbee0' (matching 
the name provided as argument to cdx_xbee_d).


NOTE:
The hello_pub and hello_sub applications in examples/hello_xbee include QoS settings 
that are tuned for the low datarate of the XBee radios.

Because of the low throughput of the XBee transport, the exchange of discovery 
information takes longer (up to ~5 seconds) with only 2 or three participants 
on the network.
