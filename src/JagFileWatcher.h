#ifndef _jag_file_watcher_h_
#define _jag_file_watcher_h_

#include <sys/types.h>
#include <sys/inotify.h>

class JagFileWatcher
{
  public:
  	JagFileWatcher();
	~JagFileWatcher();

    // op: IN_CTREATE  IN_DELETE  IN_MODIFY
	int watch( const char *fpath, uint32_t op );

  protected:
  	int _fd;
	int _wd;

};


#endif
