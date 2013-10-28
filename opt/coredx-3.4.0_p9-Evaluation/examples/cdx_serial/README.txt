Example programs to demonstrate serial transport.

Currently supports Linux only.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Prerequistes:

1) CoreDX DDS baseline with serial transport
   Install coredx-3.1.0-Linux_2.6_x86_gcc43-Release.tar.gz

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
To build:

1) ensure that gcc, make, and related development tools 
   are in your path.

2) establish environment variables:
(Use the <coredx>/scripts/cdxenv.sh script to easily
 get the appropriate values.)
 
> export COREDX_TOP=<path to coredx baseline>
> export COREDX_HOST=<host platform identifier>
> export COREDX_TARGET=<target platform identifier>
> export TWINOAKS_LICENSE_FILE=<path to license file>

3) compile the sample code

> make

4) verify successful compilation.  You should now have
three binaries:
      hello_pub
      hello_sub
      cdx_serial_tty_d


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
To run:

1) Start the 'cdx_serial_tty_d' executable 
This executable provides a server that interfaces to a 
serial device.

Specify the serial device with a '-d' command line option.
For example:

> ./cdx_serial_tty_d -d /dev/ttyS0 -u ttyS0

The serial server will open the serial device and configure
it by calling cdx_serial_init_device() (in serial_init.c).

The serial server also opens a UnixDomain Socket with an
endpoint in /tmp/cdx_d_<uds_name>.  The 'uds_name' argument
can be spedified with the '-u' command line argument.  By
default, it is 'ttya'.

2) Start the 'hello_pub' or 'hello_sub' executable.
These executables will initialize CoreDX DDS and create 
a DataWriter or DataReader respectively.  CoreDX DDS
will contact the cdx_serial server via the UnixDomain
Socket, and will use this server to communicate over the
serial device.  Provide the transport information with a
-t argument (referencing the '-u name' provided to the 
serial daemon) and any other desired options. [Use -h to
see available options.]

 > ./hello_pub -t UDS://ttyS0

Use ^C (Control-C) to terminate the demonstration programs.

