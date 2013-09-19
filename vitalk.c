#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include "vito_io.h"
#include "telnet.h"
#include "vito_parameter.h"

// This define enables communication with Vitodens:
#define VITOCOM

// Global:
fd_set master_fds;  // Aktive Filedeskriptoren f�r select()
fd_set read_fds;    // Ergebnis des select() - Aufrufs

// Signal Handler:
void exit_handler( int exitcode )
{
  printf("\n");
  fprintf(stderr, "\nAbort caught!\n" );
#ifdef VITOCOM
  sleep(3);
  vito_close();
  closetty();
#endif
  exit( exitcode );
}

int main(int argc, char **argv)
{
  // Option processing
  int c; 
  // Diverse Options:
  char *tty_devicename = NULL;
  // Struktur f�r select() timeout
  struct timeval *timeout = (struct timeval *) malloc( sizeof(struct timeval) );
  
  // Option processing with GNU getopt
  while ((c = getopt (argc, argv, "hft:")) != -1)
    switch(c)
      {
      case 'h':
	printf("Vitalk, Viessmann Vitodens 300 (B3HA) Interface\n"
	       " (c) by KWS, 2013\n\n"
	       "Usage: vitalk [option...]\n"
	       "  -h            give this help list\n"
	       "  -f            activate framedebugging\n"
	       "  -t <tty_dev>  set tty Devicename\n"
               );
	exit(1);
      case 'f':
	frame_debug = 1;
	break;
      case 't':
	tty_devicename = optarg;
	break;
      case '?':
	exit (8);
      }
  
  // Do some checks:
  if ( !tty_devicename )
    {
      fprintf(stderr, "ERROR: Need tty Devicename!\n");
      exit(5);
    }


  /////////////////////////////////////////////////////////////////////////////

  signal(SIGINT, exit_handler);
  signal(SIGHUP, exit_handler);

#ifdef VITOCOM
  opentty( tty_devicename );
  vito_init();
#endif
  
  // Das machen wir sicherheitshalber erst nach vito_init(), f�r den Fall dass
  // ein client sehr schnell ist:
  telnet_init();

  // Main Event-Loop. (Kann nur durch die Signalhandler beendet werden.)
  for (;;)
    {
      timeout->tv_sec = 60;
      timeout->tv_usec = 0;
      
      read_fds = master_fds;
      
      if ( select ( MAX_DESCRIPTOR+1, &read_fds, NULL, NULL, timeout ) > 0 )  // SELECT
	  telnet_task();

      // Nach einer gewissen Zeit der Inaktivit�t wird das 300er Protokoll
      // anscheinend wieder deaktiviert. (ca. nach 10 minuten)
      // Daher haben wir hier eine Keepalive-Funktion:
      if ( time(NULL) - vito_keepalive > 500 )
	{
	  fprintf( stderr, "Keepalive: %s\n", get_v("deviceid") );
	}
    }
}

