#include <JagFileWatcher.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#define JAGEVENT_SIZE  ( sizeof (struct inotify_event) )
#define JAGBUF_LEN     ( 1024 * ( JAGEVENT_SIZE + 16 ) )

JagFileWatcher::JagFileWatcher()
{
}
JagFileWatcher::~JagFileWatcher()
{
}

// op: IN_CREATE or IN_DELETE or IN_MODIFY or IN_MODIFY | IN_CREATE | IN_DELETE
int watch(const char *fpath, uint32_t op )
{
    int length, i = 0;
    int fd;
    int wd;
    char buffer[JAGBUF_LEN];
   
    fd = inotify_init();
    if ( fd < 0 ) {
       return -1;
    }
   
    wd = inotify_add_watch( fd, fpath, op );
    length = read( fd, buffer, JAGBUF_LEN );  
    if ( length < 0 ) {
       return -2;
    }  
   
    struct inotify_event *event;
    while ( i < length ) {
        event = ( struct inotify_event * ) &buffer[ i ];
        if ( event->len ) {
          if ( event->mask & op ) {
  		  /**
            if ( event->mask & IN_ISDIR ) {
              printf( "The directory %s was created.\n", event->name );       
            }
            else {
              printf( "The file %s was created.\n", event->name );
            }
  		  ***/
  		  break;
        }
      }
      i += JAGEVENT_SIZE + event->len;
    }
   
    ( void ) inotify_rm_watch( fd, wd );
    ( void ) close( fd );

	return 0;
}
