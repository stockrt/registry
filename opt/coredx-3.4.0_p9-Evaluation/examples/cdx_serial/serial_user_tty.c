/****************************************************************
 *
 *  file:  serial_user_tty.c
 *  desc:  User provided cdx serial interface routines for
 *         supporting a serial transport.
 *
 *         This version supports the standard unix tty serial
 *         device.
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

#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <termios.h>
#include <unistd.h>

int cdx_serial_init_device( int fd, int baud, int async );                             /* provided by user */
int cdx_serial_read( int fd, unsigned char *buf, int num_bytes ); /* provided by user */
int cdx_serial_writev(int fd, struct iovec  *iov, int num_iovec ); /* provided by user */

/* convert 'baud' to B constants */
static int get_baud(int bps)
{
  int retval;
  switch (bps)
    {
    case 1200: retval = B1200; break;
    case 2400: retval = B2400; break;
    case 4800: retval = B4800; break;
    case 9600: retval = B9600; break;
    case 19200: retval = B19200; break;
    case 38400: retval = B38400; break;
    case 57600: retval = B57600; break;
    case 115200: retval = B115200; break;
#if defined(__linux__)
    case 230400: retval = B230400; break;
    case 460800: retval = B460800; break;
    case 500000: retval = B500000; break;
    case 576000: retval = B576000; break;
    case 921600: retval = B921600; break;
    case 1000000: retval = B1000000; break;
#endif
    default: fprintf(stderr, "unknown baud '%d' using 115200.\n", bps);
      retval = B115200; break;
    }
  return retval;
}

/*********************************************************
 * cdx_serial_init_device()
 *
 *  Apply special configuration to the device file 
 *  descriptor.  Use ioctl(), tcsetattr(), or other
 *  as appropriate for the specific hardware.
 *
 * This version supports a standard Linux serial device.
 *
 *********************************************************/
int 
cdx_serial_init_device(int fd, int baud, int async)
{
  struct termios stermios;
  tcgetattr(fd, &stermios);
  cfsetospeed(&stermios, get_baud(baud));
  cfsetispeed(&stermios, get_baud(baud));
  cfmakeraw(&stermios);
  tcsetattr(fd, TCSANOW, &stermios);
  return 0;
}

int cdx_serial_read( int fd, unsigned char *buf, int num_bytes )
{
  return read(fd, buf, num_bytes);
}

int cdx_serial_writev(int fd, struct iovec  *iov, int num_iovec )
{
  return writev(fd, iov, num_iovec);
}


