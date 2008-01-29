
#ifndef DSARCHIVE_H
#define DSARCHIVE_H

#include <stdio.h>
#include <time.h>
#include <libslink.h>

/* Maximum file name length for output files */
#define MAX_FILENAME_LEN 400

/* Define pre-formatted archive layouts */
#define SDSLAYOUT   "%Y/%n/%s/%c.%t/%n.%s.%l.%c.%t.%Y.%j"
#define BUDLAYOUT   "%n/%s/%s.%n.%l.%c.%Y.%j"
#define CSSLAYOUT   "%Y/%j/%s.%c.%Y:%j:#H:#M:#S"
#define CHANLAYOUT  "%n.%s.%l.%c"
#define QCHANLAYOUT "%n.%s.%l.%c.%q"
#define CDAYLAYOUT  "%n.%s.%l.%c.%Y:%j:#H:#M:#S"

typedef struct DataStreamGroup_s
{
  char   *defkey;
  int     filed;
  time_t  modtime;
  double  lastsample;
  char    futurecontprint;
  char    futureinitprint;
  char    filename[MAX_FILENAME_LEN];
  struct  DataStreamGroup_s *next;
}
DataStreamGroup;

typedef struct DataStream_s
{
  char   *path;
  char    packettype;
  int     idletimeout;
  char    futurecontflag;
  int     futurecont;
  char    futureinitflag;
  int     futureinit;
  struct  DataStreamGroup_s *grouproot;
}
DataStream;

/* Global maximum number of open files */
extern int ds_maxopenfiles;

extern int ds_streamproc (DataStream *datastream, SLMSrecord *msr, long suffix);

#endif
