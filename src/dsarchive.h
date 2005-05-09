
#ifndef DSARCHIVE_H
#define DSARCHIVE_H

#include <stdio.h>
#include <time.h>
#include <libslink.h>

typedef struct DataStreamGroup_s
{
  char   *defkey;
  int     filed;
  time_t  modtime;
  double  lastsample;
  char    futurecontprint;
  char    futureinitprint;
  struct  DataStreamGroup_s *next;
}
DataStreamGroup;

typedef struct DataStream_s
{
  char   *path;
  char    archivetype;
  char    packettype;
  int     idletimeout;
  char    futurecontflag;
  int     futurecont;
  char    futureinitflag;
  int     futureinit;
  struct  DataStreamGroup_s *grouproot;
}
DataStream;

extern int ds_streamproc (DataStream *datastream, char *pathformat,
			  MSrecord *msr, int reclen);
#endif

