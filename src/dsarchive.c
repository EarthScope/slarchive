/***************************************************************************
 * dsarchive.c
 * Routines to archive Mini-SEED data records.
 *
 * Written by Chad Trabant, ORFEUS/EC-Project MEREDIAN
 *
 * The philosophy: a "DataStream" describes an archive that miniSEED
 * records will be saved to.  Each archive can be separated into
 * "DataStreamGroup"s, each unique group will be saved into a unique
 * file.  The definition of the groups is implied by the format of the
 * archive.
 *
 * modified: 2004.198
 ***************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include "dsarchive.h"

/* Functions internal to this source file */
static DataStreamGroup *ds_getstream (DataStream *datastream, MSrecord *msr,
				      int reclen, const char *defkey,
				      const char *filename);
static int ds_openfile (DataStream *datastream, const char *filename);
static int ds_closeidle (DataStream *datastream, int idletimeout);
static void ds_shutdown (DataStream *datastream);
static double msr_lastsamptime (MSrecord *msr);
static char sl_typecode (int type);


/***************************************************************************
 * ds_streamproc:
 *
 * Save MiniSEED records in a custom directory/file structure.  The
 * appropriate directories and files are created if nesecessary.  If
 * files already exist they are appended to.  If both 'pathformat' and
 * 'msr' are NULL then ds_shutdown() will be called to close all open files
 * and free all associated memory.
 *
 * Returns 0 on success, -1 on error.
 ***************************************************************************/
extern int
ds_streamproc (DataStream *datastream, char *pathformat, MSrecord *msr,
	       int reclen)
{
  DataStreamGroup *foundgroup = NULL;
  strlist *fnlist, *fnptr;
  char net[3], sta[6], loc[3], chan[4];
  char filename[400];
  char definition[400];

  /* Special case for stream shutdown */
  if ( pathformat == NULL && msr == NULL )
    {
      ds_shutdown ( datastream );
      return 0;
    }

  /* Build file path and name from pathformat */
  filename[0] = '\0';
  definition[0] = '\0';
  strparse (pathformat, "/", &fnlist);

  fnptr = fnlist;

  /* Special case of an absolute path (first entry is empty) */
  if ( *fnptr->element == '\0' )
    {
      if ( fnptr->next != 0 )
	{
	  strncat (filename, "/", sizeof(filename));
	  fnptr = fnptr->next;
	}
      else
	{
	  sl_log (2, 0, "ds_streamproc(): empty path format\n");
	  strparse (NULL, NULL, &fnlist);
	  return -1;
	}
    }

  while ( fnptr != 0 )
    {
      int tdy;
      int fnlen = 0;
      char *w, *p, def;
      char tstr[20];

      p = fnptr->element;

      /* Special case of no file given */
      if ( *p == '\0' && fnptr->next == 0 )
	{
	  sl_log (2, 0, "ds_streamproc(): no file name specified, only %s\n",
		  filename);
	  strparse (NULL, NULL, &fnlist);
	  return -1;
	}

      while ( (w = strpbrk (p, "%#")) != NULL )
	{
	  def = ( *w == '%' );
	  *w = '\0';
	  strncat (filename, p, (sizeof(filename) - fnlen));
	  fnlen = strlen (filename);

	  w += 1;

	  switch ( *w )
	    {
	    case 't' :
	      snprintf (tstr, sizeof(tstr), "%c", sl_typecode(datastream->packettype));
	      strncat (filename, tstr, (sizeof(filename) - fnlen));
	      if ( def ) strncat (definition, tstr, (sizeof(definition) - fnlen));
	      fnlen = strlen (filename);
	      p = w + 1;
	      break;
	    case 'n' :
	      strncpclean (net, msr->fsdh.network, 2);
	      strncat (filename, net, (sizeof(filename) - fnlen));
	      if ( def ) strncat (definition, net, (sizeof(definition) - fnlen));
	      fnlen = strlen (filename);
	      p = w + 1;
	      break;
	    case 's' :
	      strncpclean (sta, msr->fsdh.station, 5);
	      strncat (filename, sta, (sizeof(filename) - fnlen));
	      if ( def ) strncat (definition, sta, (sizeof(definition) - fnlen));
	      fnlen = strlen (filename);
	      p = w + 1;
	      break;
	    case 'l' :
	      strncpclean (loc, msr->fsdh.location, 2);
	      strncat (filename, loc, (sizeof(filename) - fnlen));
	      if ( def ) strncat (definition, loc, (sizeof(definition) - fnlen));
	      fnlen = strlen (filename);
	      p = w + 1;
	      break;
	    case 'c' :
	      strncpclean (chan, msr->fsdh.channel, 3);
	      strncat (filename, chan, (sizeof(filename) - fnlen));
	      if ( def ) strncat (definition, chan, (sizeof(definition) - fnlen));
	      fnlen = strlen (filename);
	      p = w + 1;
	      break;
	    case 'Y' :
	      snprintf (tstr, sizeof(tstr), "%04d", (int) msr->fsdh.start_time.year);
	      strncat (filename, tstr, (sizeof(filename) - fnlen));
	      if ( def ) strncat (definition, tstr, (sizeof(definition) - fnlen));
	      fnlen = strlen (filename);
	      p = w + 1;
	      break;
	    case 'y' :
	      tdy = (int) msr->fsdh.start_time.year;
	      while ( tdy > 100 )
		{
		  tdy -= 100;
		}
	      snprintf (tstr, sizeof(tstr), "%02d", tdy);
	      strncat (filename, tstr, (sizeof(filename) - fnlen));
	      if ( def ) strncat (definition, tstr, (sizeof(definition) - fnlen));
	      fnlen = strlen (filename);
	      p = w + 1;
	      break;
	    case 'j' :
	      snprintf (tstr, sizeof(tstr), "%03d", (int) msr->fsdh.start_time.day);
	      strncat (filename, tstr, (sizeof(filename) - fnlen));
	      if ( def ) strncat (definition, tstr, (sizeof(definition) - fnlen));
	      fnlen = strlen (filename);
	      p = w + 1;
	      break;
	    case 'H' :
	      snprintf (tstr, sizeof(tstr), "%02d", (int) msr->fsdh.start_time.hour);
	      strncat (filename, tstr, (sizeof(filename) - fnlen));
	      if ( def ) strncat (definition, tstr, (sizeof(definition) - fnlen));
	      fnlen = strlen (filename);
	      p = w + 1;
	      break;
	    case 'M' :
	      snprintf (tstr, sizeof(tstr), "%02d", (int) msr->fsdh.start_time.min);
	      strncat (filename, tstr, (sizeof(filename) - fnlen));
	      if ( def ) strncat (definition, tstr, (sizeof(definition) - fnlen));
	      fnlen = strlen (filename);
	      p = w + 1;
	      break;
	    case 'S' :
	      snprintf (tstr, sizeof(tstr), "%02d", (int) msr->fsdh.start_time.sec);
	      strncat (filename, tstr, (sizeof(filename) - fnlen));
	      if ( def ) strncat (definition, tstr, (sizeof(definition) - fnlen));
	      fnlen = strlen (filename);
	      p = w + 1;
	      break;
	    case 'F' :
	      snprintf (tstr, sizeof(tstr), "%04d", (int) msr->fsdh.start_time.fract);
	      strncat (filename, tstr, (sizeof(filename) - fnlen));
	      if ( def ) strncat (definition, tstr, (sizeof(definition) - fnlen));
	      fnlen = strlen (filename);
	      p = w + 1;
	      break;
	    case '%' :
	      strncat (filename, "%", (sizeof(filename) - fnlen));
	      fnlen = strlen (filename);
	      p = w + 1;
	      break;
	    case '#' :
	      strncat (filename, "#", (sizeof(filename) - fnlen));
	      fnlen = strlen (filename);
	      p = w + 1;
	      break;
	    default :
	      sl_log (2, 0, "unknown file name format code: %c\n", *w);
	      p = w;
	      break;
	    }
	}
      
      strncat (filename, p, (sizeof(filename) - fnlen));
      fnlen = strlen (filename);

      /* If not the last entry then it should be a directory */
      if ( fnptr->next != 0 )
	{
	  if ( access (filename, F_OK) )
	    {
	      if ( errno == ENOENT )
		{
		  sl_log (1, 1, "Creating directory: %s\n", filename);
		  if (mkdir
		      (filename, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH))
		    {
		      sl_log (2, 1, "ds_streamproc: mkdir(%s) %s\n", filename,
			      strerror (errno));
		      strparse (NULL, NULL, &fnlist);
		      return -1;
		    }
		}
	      else
		{
		  sl_log (2, 0, "%s: access denied, %s\n", filename, strerror(errno));
		  strparse (NULL, NULL, &fnlist);
		  return -1;
		}
	    }

	  strncat (filename, "/", (sizeof(filename) - fnlen));
	  fnlen++;
	}

      fnptr = fnptr->next;
    }

  strparse (NULL, NULL, &fnlist);

  /* Check for previously used stream entry, otherwise create it */
  foundgroup = ds_getstream (datastream, msr, reclen, definition, filename);

  if (foundgroup != NULL)
    {
      /* Initial check (existing files) for future data, a negative
       * last sample time indicates it was derived from an existing
       * file.
       */
      if ( datastream->packettype == SLDATA &&
	   datastream->futureinitflag &&
	   foundgroup->lastsample < 0 )
	{
	  int overlap = (int) ((-1.0 * foundgroup->lastsample) - msr_depochstime(msr));
	  
	  if ( overlap > datastream->futureinit )
	    {
	      if ( foundgroup->futureinitprint )
		{
		  sl_log (2, 0,
			  "%d sec. overlap of existing archive data in %s, skipping\n",
			  overlap, filename);
		  foundgroup->futureinitprint = 0;  /* Suppress further messages */
		}
	      
	      return 0;
	    }
	}
      
      /* Continuous check for future data, a positive last sample time
       * indicates it was derived from the last packet received.
       */
      if ( datastream->packettype == SLDATA &&
	   datastream->futurecontflag &&
	   foundgroup->lastsample > 0 )
	{
	  int overlap = (int) (foundgroup->lastsample - msr_depochstime(msr));
	  
          if ( overlap > datastream->futurecont )
	    {
	      if ( foundgroup->futurecontprint )
		{
		  sl_log (2, 0,
			  "%d sec. overlap of continuous data for %s, skipping\n",
			  overlap, filename);
		  foundgroup->futurecontprint = 0;  /* Suppress further messages */
		}
	      
	      return 0;
	    }
	  else if ( foundgroup->futurecontprint == 0 )
	    foundgroup->futurecontprint = 1;  /* Reset message printing */
	}

      /*  Write the record to the appropriate file */
      sl_log (1, 3, "Writing data to data stream file %s\n", filename);
      
      if ( !write (foundgroup->filed, msr->msrecord, reclen) )
	{
	  sl_log (2, 1,
		  "ds_streamproc: failed to write record\n");
	  return -1;
	}
      else
	{
	  foundgroup->modtime = time (NULL);
	  
	  if ( datastream->packettype == SLDATA &&
	       (datastream->futureinitflag || datastream->futurecontflag) )
	    foundgroup->lastsample = msr_lastsamptime (msr);
	}

      return 0;
    }
  
  return -1;
}				/* End of ds_streamproc() */


/***************************************************************************
 * ds_getstream:
 *
 * Find the DataStreamGroup entry that matches the definition key, if
 * no matching entries are found allocate a new entry and open the
 * given file.
 *
 * Resource maintenance is performed here: the modification time of
 * each stream, modtime, is compared to the current time.  If the
 * stream entry has been idle for 'DataStream.idletimeout' seconds
 * then the stream will be closed (file closed and memory freed).
 *
 * Returns a pointer to a DataStreamGroup on success or NULL on error.
 ***************************************************************************/
static DataStreamGroup *
ds_getstream (DataStream *datastream, MSrecord *msr, int reclen,
	      const char *defkey, const char *filename)
{
  DataStreamGroup *foundgroup  = NULL;
  DataStreamGroup *searchgroup = NULL;
  DataStreamGroup *prevgroup   = NULL;
  time_t curtime;
  
  searchgroup = datastream->grouproot;
  curtime = time (NULL);
  
  /* Traverse the stream chain looking for matching streams */
  while (searchgroup != NULL)
    {
      DataStreamGroup *nextgroup  = (DataStreamGroup *) searchgroup->next;
      
      if ( !strcmp (searchgroup->defkey, defkey) )
	{
	  sl_log (1, 3, "Found data stream entry for key %s\n", defkey);
	  
	  foundgroup = searchgroup;
	  
	  /* Keep ds_closeidle from closing this stream */
	  if ( foundgroup->modtime > 0 )
	    {
	      foundgroup->modtime *= -1;
	    }

	  break;
	}
      
      prevgroup = searchgroup;
      searchgroup = nextgroup;
    }

  /* If not found, create a stream entry */
  if ( foundgroup == NULL )
    {
      sl_log (1, 2, "Creating data stream entry for key %s\n", defkey);

      foundgroup = (DataStreamGroup *) malloc (sizeof (DataStreamGroup));

      foundgroup->defkey = strdup (defkey);
      foundgroup->filed = 0;
      foundgroup->modtime = curtime;
      foundgroup->lastsample = 0.0;
      foundgroup->futurecontprint = datastream->futurecontflag;
      foundgroup->futureinitprint = datastream->futureinitflag;
      foundgroup->next = NULL;

      /* Set the stream root if this is the first entry */
      if (datastream->grouproot == NULL)
	{
	  datastream->grouproot = foundgroup;
	}
      /* Otherwise add to the end of the chain */
      else if (prevgroup != NULL)
	{
	  prevgroup->next = foundgroup;
	}
      else
	{
	  sl_log (2, 0, "stream chain is broken!\n");
	  return NULL;
	}
    }
  
  /* Close idle stream files */
  ds_closeidle (datastream, datastream->idletimeout);
  
  /* If no file is open, well, open it */
  if ( foundgroup->filed == 0 )
    {
      int filepos;
      
      sl_log (1, 2, "Opening data stream file %s\n", filename);
      
      if ( (foundgroup->filed = ds_openfile (datastream, filename)) == -1 )
	{
	  sl_log (2, 0, "cannot open data stream file, %s\n", strerror (errno));
	  return NULL;
	}
      
      if ( (filepos = (int) lseek (foundgroup->filed, (off_t) 0, SEEK_END)) < 0 )
	{
	  sl_log (2, 0, "cannot seek in data stream file, %s\n", strerror (errno));
	  return NULL;
	}
      
      /* Initial future data check (existing files) needs the last
       * sample time from the last record.  Only read the last record
       * if this stream has not been used and there is at least one
       * record to read.
       */
      if ( datastream->packettype == SLDATA &&
	   datastream->futureinitflag  &&
	   !foundgroup->lastsample )
	{
	  if ( filepos >= reclen )
	    {
	      MSrecord *lmsr = NULL;
	      char *lrecord;
	      
	      sl_log (1, 2, "Reading last record in existing file\n");

	      lrecord = (char *) malloc (reclen);
	      
	      if ( (lseek (foundgroup->filed, (off_t) (reclen * -1), SEEK_END)) < 0 )
		{
		  sl_log (2, 0, "cannot seek in data stream file, %s\n", strerror (errno));
		  free (lrecord);
		  return NULL;
		}
	      
	      if ( (read (foundgroup->filed, lrecord, reclen)) != reclen )
		{
		  sl_log(2, 0, "cannot read the last record of stream file\n");
		  free (lrecord);
		  return NULL;
		}
	      
	      if ( msr_parse (NULL, lrecord, &lmsr, 0, 0) != NULL )
		{
		  /* A negative last sample time means it came from an existing file */
		  foundgroup->lastsample = (-1 * msr_lastsamptime (lmsr));
		}
	      else
		{
		  /* Zero means last sample time is unknown, disabling checks */
		  foundgroup->lastsample = 0.0;
		}

	      msr_free (&lmsr);
	      free (lrecord);
	    }
	}
    }
  
  /* There used to be a further check here, but it shouldn't be reached, just in
     case this is left for the moment until I'm convinced. */
  else if ( strcmp (defkey, foundgroup->defkey) )
    sl_log (2, 0, "Arg! open file for a key that no longer matches\n");
  
  return foundgroup;
}				/* End of ds_getstream() */


/***************************************************************************
 * ds_openfile:
 *
 * Open a specified file, if the open file limit has been reach try
 * once to increase the limit, if that fails or has already been done
 * start closing idle files with decreasing idle timeouts until a file
 * can be opened.
 *
 * Return the result of open(2), normally this a the file descriptor
 * on success and -1 on error.
 ***************************************************************************/
static int
ds_openfile (DataStream *datastream, const char *filename)
{
  static char rlimit = 0;
  struct rlimit rlim;
  int idletimeout = datastream->idletimeout;
  int oret = 0;
  
  if ( (oret = open (filename, O_RDWR | O_CREAT | O_APPEND, 0644)) == -1 )
    {
      
      /* Check if max number of files open */
      if ( errno == EMFILE && rlimit == 0 )
	{
	  rlimit = 1;
	  sl_log (1, 1, "Too many open files, trying to increase limit\n");
	  
	  /* Set the soft open file limit to the hard open file limit */
	  if ( getrlimit (RLIMIT_NOFILE, &rlim) == -1 )
	    {
	      sl_log (2, 0, "getrlimit failed to get open file limit\n");
	    }
	  else
	    {
	      rlim.rlim_cur = rlim.rlim_max;
	      
	      if ( rlim.rlim_cur == RLIM_INFINITY )
		sl_log (1, 3, "Setting open file limit to 'infinity'\n");
	      else
		sl_log (1, 3, "Setting open file limit to %d\n", rlim.rlim_cur);
	      
	      if ( setrlimit (RLIMIT_NOFILE, &rlim) == -1 )
		{
		  sl_log (2, 0, "setrlimit failed to set open file limit\n");
		}
	      else
		{
		  /* Try to open the file again */
		  if ( (oret = open (filename, O_RDWR | O_CREAT | O_APPEND, 0644)) != -1 )
		    return oret;
		}
	    }
	}
      
      if ( errno == EMFILE || errno == ENFILE )
	{
	  sl_log (1, 2, "Too many open files, closing idle stream files\n");
	  
	  /* Close idle streams until we have free descriptors */
	  while ( ds_closeidle (datastream, idletimeout) == 0 && idletimeout >= 0 )
	    {
	      idletimeout = (idletimeout / 2) - 1;
	    }
	  
	  /* Try to open the file again */
	  if ( (oret = open (filename, O_RDWR | O_CREAT | O_APPEND, 0644)) != -1 )
	    return oret;
	}
    }
  
  return oret;
}				/* End of ds_openfile() */


/***************************************************************************
 * ds_closeidle:
 *
 * Close all stream files that have not been active for the specified
 * idletimeout.
 *
 * Return the number of files closed.
 ***************************************************************************/
static int
ds_closeidle (DataStream *datastream, int idletimeout)
{
  int count = 0;
  DataStreamGroup *searchgroup = NULL;
  DataStreamGroup *prevgroup   = NULL;
  DataStreamGroup *nextgroup   = NULL;
  time_t curtime;
  
  searchgroup = datastream->grouproot;
  curtime = time (NULL);

  /* Traverse the stream chain */
  while (searchgroup != NULL)
    {
      nextgroup = searchgroup->next;
      
      if ( searchgroup->modtime > 0 && (curtime - searchgroup->modtime) > idletimeout )
	{
	  sl_log (1, 2, "Closing idle stream with key %s\n", searchgroup->defkey);
	  
	  /* Re-link the stream chain */
	  if ( prevgroup != NULL )
	    {
	      if ( searchgroup->next != NULL )
		prevgroup->next = searchgroup->next;
	      else
		prevgroup->next = NULL;
	    }
	  else
	    {
	      if ( searchgroup->next != NULL )
		datastream->grouproot = searchgroup->next;
	      else
		datastream->grouproot = NULL;
	    }
	  
	  /* Close the associated file */
	  if ( close (searchgroup->filed) )
	    sl_log (2, 0, "ds_closeidle(), closing data stream file, %s\n",
		    strerror (errno));
	  else
	    count++;
	  
	  free (searchgroup->defkey); 
	  free (searchgroup);
	}
      else
	{
	  prevgroup = searchgroup;
	}
      
      searchgroup = nextgroup;
    }
  
  return count;
}				/* End of ds_closeidle() */


/***************************************************************************
 * ds_shutdown:
 *
 * Close all stream files and release all of the DataStreamGroup memory
 * structures.
 ***************************************************************************/
static void
ds_shutdown (DataStream *datastream)
{
  DataStreamGroup *curgroup = NULL;
  DataStreamGroup *prevgroup = NULL;

  curgroup = datastream->grouproot;

  while ( curgroup != NULL )
    {
      prevgroup = curgroup;
      curgroup = curgroup->next;

      sl_log (1, 3, "Shutting down stream with key: %s\n", prevgroup->defkey);

      if ( close (prevgroup->filed) )
	sl_log (2, 0, "ds_shutdown(), closing data stream file, %s\n",
		strerror (errno));
      
      free (prevgroup->defkey);
      free (prevgroup);
    }
}				/* End of ds_shutdown() */


/***************************************************************************
 * msr_lastsamptime:
 *
 * Calculate the time of the last sample in the record; this is the actual
 * last sample time and *not* the time "covered" by the last sample.
 *
 * Returns the time of the last sample as a double precision epoch time.
 ***************************************************************************/
double
msr_lastsamptime (MSrecord *msr)
{
  double startepoch;
  double dsamprate;
  double span = 0.0;
  
  msr_dsamprate (msr, &dsamprate);
  
  if ( dsamprate )
    span = (double) (msr->fsdh.num_samples - 1 ) * (1.0 / dsamprate);
  
  startepoch = msr_depochstime(msr);
  
  return (startepoch + span);
}				/* End of msr_lastsamptime() */


/***************************************************************************
 * sl_typecode:
 * Look up the one character code that corresponds to the packet type.
 *
 * Returns the type character on success and '?' if no matching type.
 ***************************************************************************/
char
sl_typecode (int packettype)
{
  switch (packettype)
    {
    case SLDATA:
      return 'D';
    case SLDET:
      return 'E';
    case SLCAL:
      return 'C';
    case SLTIM:
      return 'T';
    case SLMSG:
      return 'L';
    case SLBLK:
      return 'O';
    case SLCHA:
      return 'U';
    case SLINF:
    case SLINFT:
    case SLKEEP:
      return 'I';

    default:
      return '?';
    }
}
