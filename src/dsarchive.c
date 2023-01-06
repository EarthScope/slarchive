/***************************************************************************
 * dsarchive.c
 * Routines to archive Mini-SEED data records.
 *
 * Written by Chad Trabant
 *   ORFEUS/EC-Project MEREDIAN
 *   IRIS Data Management Center
 *
 * The philosophy: a "DataStream" describes an archive that miniSEED
 * records will be saved to.  Each archive can be separated into
 * "DataStreamGroup"s, each unique group will be saved into a unique
 * file.  The definition of the groups is implied by the format of the
 * archive.
 *
 * modified: 2013.316
 ***************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <glob.h>

#include "dsarchive.h"

/* Maximum number of open files */
int ds_maxopenfiles = 0;
int ds_openfilecount = 0;

/* Functions internal to this source file */
static DataStreamGroup *ds_getstream (DataStream *datastream, int reclen,
				      const char *defkey, char *filename,
				      int nondefflags, const char *globmatch);
static int ds_openfile (DataStream *datastream, const char *filename);
static int ds_closeidle (DataStream *datastream, int idletimeout);
static void ds_shutdown (DataStream *datastream);
static double sl_msr_lastsamptime (SLMSrecord *msr);
static char sl_typecode (int type);


/***************************************************************************
 * ds_streamproc:
 *
 * Save MiniSEED records in a custom directory/file structure.  The
 * appropriate directories and files are created if nesecessary.  If
 * files already exist they are appended to.  If 'msr' is NULL then
 * ds_shutdown() will be called to close all open files and free all
 * associated memory.
 *
 * Returns 0 on success, -1 on error.
 ***************************************************************************/
extern int
ds_streamproc (DataStream *datastream, SLMSrecord *msr, long suffix)
{
  DataStreamGroup *foundgroup = NULL;
  char *tptr;
  char tstr[20];
  char net[3], sta[6], loc[3], chan[4];
  char filename[MAX_FILENAME_LEN] = "";
  char definition[MAX_FILENAME_LEN] = "";
  char pathformat[MAX_FILENAME_LEN] = "";
  char globmatch[MAX_FILENAME_LEN] = "";
  int fnlen = 0;
  int globlen = 0;
  int nondefflags = 0;
  int writebytes;
  int writeloops;
  int rv;
  int tdy;
  char *w, *p, def;
  double dsamprate = 0.0;

  int reclen = SLRECSIZE;

  /* Special case for stream shutdown */
  if ( ! msr )
    {
      sl_log (1, 2, "Closing archive for %s\n", datastream->path);

      ds_shutdown ( datastream );
      return 0;
    }

  /* Check for empty path */
  if ( ! datastream->path || strlen (datastream->path) <= 0 )
    {
      sl_log (2, 0, "ds_streamproc(): empty path format\n");
      return -1;
    }

  /* Create a copy of the specified path, it will be modified during parsing */
  snprintf (pathformat, sizeof(pathformat), "%s", datastream->path);
  pathformat[sizeof(pathformat)-1] = '\0';

  /* Count all of the non-defining flags */
  tptr = pathformat;
  while ( (tptr = strchr(tptr, '#')) )
    {
      if ( *(tptr+1) != '#' )
	nondefflags++;
      tptr++;
    }

  /* Process each converstion flag */
  p = pathformat;
  while ( (w = strpbrk (p, "%#")) != NULL )
    {
      def = ( *w == '%' );
      *w = '\0';

      strncat (filename, p, (sizeof(filename) - fnlen));
      fnlen = strlen (filename);

      if ( nondefflags > 0 )
	{
	  strncat (globmatch, p, (sizeof(globmatch) - globlen));
	  globlen = strlen (globmatch);
	}

      /* The conversion code is the next character, replace with content */
      w += 1;
      switch ( *w )
	{
	case 't' :
	  snprintf (tstr, sizeof(tstr), "%c", sl_typecode(datastream->packettype));
	  strncat (filename, tstr, (sizeof(filename) - fnlen));
	  if ( def ) strncat (definition, tstr, (sizeof(definition) - fnlen));
	  if ( nondefflags > 0 )
	    {
	      if ( def ) strncat (globmatch, tstr, (sizeof(globmatch) - globlen));
	      else strncat (globmatch, "?", (sizeof(globmatch) - globlen));
	      globlen = strlen (globmatch);
	    }
	  fnlen = strlen (filename);
	  p = w + 1;
	  break;
	case 'n' :
	  sl_strncpclean (net, msr->fsdh.network, 2);
	  strncat (filename, net, (sizeof(filename) - fnlen));
	  if ( def ) strncat (definition, net, (sizeof(definition) - fnlen));
	  if ( nondefflags > 0 )
	    {
	      if ( def ) strncat (globmatch, net, (sizeof(globmatch) - globlen));
	      else strncat (globmatch, "*", (sizeof(globmatch) - globlen));
	      globlen = strlen (globmatch);
	    }
	  fnlen = strlen (filename);
	  p = w + 1;
	  break;
	case 's' :
	  sl_strncpclean (sta, msr->fsdh.station, 5);
	  strncat (filename, sta, (sizeof(filename) - fnlen));
	  if ( def ) strncat (definition, sta, (sizeof(definition) - fnlen));
	  if ( nondefflags > 0 )
	    {
	      if ( def ) strncat (globmatch, sta, (sizeof(globmatch) - globlen));
	      else strncat (globmatch, "*", (sizeof(globmatch) - globlen));
	      globlen = strlen (globmatch);
	    }
	  fnlen = strlen (filename);
	  p = w + 1;
	  break;
	case 'l' :
	  sl_strncpclean (loc, msr->fsdh.location, 2);
	  strncat (filename, loc, (sizeof(filename) - fnlen));
	  if ( def ) strncat (definition, loc, (sizeof(definition) - fnlen));
	  if ( nondefflags > 0 )
	    {
	      if ( def ) strncat (globmatch, loc, (sizeof(globmatch) - globlen));
	      else strncat (globmatch, "*", (sizeof(globmatch) - globlen));
	      globlen = strlen (globmatch);
	    }
	  fnlen = strlen (filename);
	  p = w + 1;
	  break;
	case 'c' :
	  sl_strncpclean (chan, msr->fsdh.channel, 3);
	  strncat (filename, chan, (sizeof(filename) - fnlen));
	  if ( def ) strncat (definition, chan, (sizeof(definition) - fnlen));
	  if ( nondefflags > 0 )
	    {
	      if ( def ) strncat (globmatch, chan, (sizeof(globmatch) - globlen));
	      else strncat (globmatch, "*", (sizeof(globmatch) - globlen));
	      globlen = strlen (globmatch);
	    }
	  fnlen = strlen (filename);
	  p = w + 1;
	  break;
	case 'Y' :
	  snprintf (tstr, sizeof(tstr), "%04d", (int) msr->fsdh.start_time.year);
	  strncat (filename, tstr, (sizeof(filename) - fnlen));
	  if ( def ) strncat (definition, tstr, (sizeof(definition) - fnlen));
	  if ( nondefflags > 0 )
	    {
	      if ( def ) strncat (globmatch, tstr, (sizeof(globmatch) - globlen));
	      else strncat (globmatch, "[0-9][0-9][0-9][0-9]", (sizeof(globmatch) - globlen));
	      globlen = strlen (globmatch);
	    }
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
	  if ( nondefflags > 0 )
	    {
	      if ( def ) strncat (globmatch, tstr, (sizeof(globmatch) - globlen));
	      else strncat (globmatch, "[0-9][0-9]", (sizeof(globmatch) - globlen));
	      globlen = strlen (globmatch);
	    }
	  fnlen = strlen (filename);
	  p = w + 1;
	  break;
	case 'j' :
	  snprintf (tstr, sizeof(tstr), "%03d", (int) msr->fsdh.start_time.day);
	  strncat (filename, tstr, (sizeof(filename) - fnlen));
	  if ( def ) strncat (definition, tstr, (sizeof(definition) - fnlen));
	  if ( nondefflags > 0 )
	    {
	      if ( def ) strncat (globmatch, tstr, (sizeof(globmatch) - globlen));
	      else strncat (globmatch, "[0-9][0-9][0-9]", (sizeof(globmatch) - globlen));
	      globlen = strlen (globmatch);
	    }
	  fnlen = strlen (filename);
	  p = w + 1;
	  break;
	case 'H' :
	  snprintf (tstr, sizeof(tstr), "%02d", (int) msr->fsdh.start_time.hour);
	  strncat (filename, tstr, (sizeof(filename) - fnlen));
	  if ( def ) strncat (definition, tstr, (sizeof(definition) - fnlen));
	  if ( nondefflags > 0 )
	    {
	      if ( def ) strncat (globmatch, tstr, (sizeof(globmatch) - globlen));
	      else strncat (globmatch, "[0-9][0-9]", (sizeof(globmatch) - globlen));
	      globlen = strlen (globmatch);
	    }
	  fnlen = strlen (filename);
	  p = w + 1;
	  break;
	case 'M' :
	  snprintf (tstr, sizeof(tstr), "%02d", (int) msr->fsdh.start_time.min);
	  strncat (filename, tstr, (sizeof(filename) - fnlen));
	  if ( def ) strncat (definition, tstr, (sizeof(definition) - fnlen));
	  if ( nondefflags > 0 )
	    {
	      if ( def ) strncat (globmatch, tstr, (sizeof(globmatch) - globlen));
	      else strncat (globmatch, "[0-9][0-9]", (sizeof(globmatch) - globlen));
	      globlen = strlen (globmatch);
	    }
	  fnlen = strlen (filename);
	  p = w + 1;
	  break;
	case 'S' :
	  snprintf (tstr, sizeof(tstr), "%02d", (int) msr->fsdh.start_time.sec);
	  strncat (filename, tstr, (sizeof(filename) - fnlen));
	  if ( def ) strncat (definition, tstr, (sizeof(definition) - fnlen));
	  if ( nondefflags > 0 )
	    {
	      if ( def ) strncat (globmatch, tstr, (sizeof(globmatch) - globlen));
	      else strncat (globmatch, "[0-9][0-9]", (sizeof(globmatch) - globlen));
	      globlen = strlen (globmatch);
	    }
	  fnlen = strlen (filename);
	  p = w + 1;
	  break;
	case 'F' :
	  snprintf (tstr, sizeof(tstr), "%04d", (int) msr->fsdh.start_time.fract);
	  strncat (filename, tstr, (sizeof(filename) - fnlen));
	  if ( def ) strncat (definition, tstr, (sizeof(definition) - fnlen));
	  if ( nondefflags > 0 )
	    {
	      if ( def ) strncat (globmatch, tstr, (sizeof(globmatch) - globlen));
	      else strncat (globmatch, "[0-9][0-9][0-9][0-9]", (sizeof(globmatch) - globlen));
	      globlen = strlen (globmatch);
	    }
	  fnlen = strlen (filename);
	  p = w + 1;
	  break;
	case 'q' :
	  snprintf (tstr, sizeof(tstr), "%c", msr->fsdh.dhq_indicator);
	  strncat (filename, tstr, (sizeof(filename) - fnlen));
	  if ( def ) strncat (definition, tstr, (sizeof(definition) - fnlen));
	  if ( nondefflags > 0 )
	    {
	      if ( def ) strncat (globmatch, tstr, (sizeof(globmatch) - globlen));
	      else strncat (globmatch, "?", (sizeof(globmatch) - globlen));
	      globlen = strlen (globmatch);
	    }
	  fnlen = strlen (filename);
	  p = w + 1;
	  break;
	case 'L' :
	  snprintf (tstr, sizeof(tstr), "%d", SLRECSIZE);
	  strncat (filename, tstr, (sizeof(filename) - fnlen));
	  if ( def ) strncat (definition, tstr, (sizeof(definition) - fnlen));
	  if ( nondefflags > 0 )
	    {
	      if ( def ) strncat (globmatch, tstr, (sizeof(globmatch) - globlen));
	      else strncat (globmatch, "*", (sizeof(globmatch) - globlen));
	      globlen = strlen (globmatch);
	    }
	  fnlen = strlen (filename);
	  p = w + 1;
	  break;
	case 'r' :
	  sl_msr_dsamprate (msr, &dsamprate);
	  snprintf (tstr, sizeof(tstr), "%ld", (long int) (dsamprate+0.5));
	  strncat (filename, tstr, (sizeof(filename) - fnlen));
	  if ( def ) strncat (definition, tstr, (sizeof(definition) - fnlen));
	  if ( nondefflags > 0 )
	    {
	      if ( def ) strncat (globmatch, tstr, (sizeof(globmatch) - globlen));
	      else strncat (globmatch, "*", (sizeof(globmatch) - globlen));
	      globlen = strlen (globmatch);
	    }
	  fnlen = strlen (filename);
	  p = w + 1;
	  break;
	case 'R' :
	  sl_msr_dsamprate (msr, &dsamprate);
	  snprintf (tstr, sizeof(tstr), "%.6f", dsamprate);
	  strncat (filename, tstr, (sizeof(filename) - fnlen));
	  if ( def ) strncat (definition, tstr, (sizeof(definition) - fnlen));
	  if ( nondefflags > 0 )
	    {
	      if ( def ) strncat (globmatch, tstr, (sizeof(globmatch) - globlen));
	      else strncat (globmatch, "*", (sizeof(globmatch) - globlen));
	      globlen = strlen (globmatch);
	    }
	  fnlen = strlen (filename);
	  p = w + 1;
	  break;
	case '%' :
	  strncat (filename, "%", (sizeof(filename) - fnlen));
	  strncat (globmatch, "%", (sizeof(globmatch) - globlen));
	  fnlen = strlen (filename);
	  globlen = strlen (globmatch);
	  p = w + 1;
	  break;
	case '#' :
	  strncat (filename, "#", (sizeof(filename) - fnlen));
	  nondefflags--;
	  if ( nondefflags > 0 )
	    {
	      strncat (globmatch, "#", (sizeof(globmatch) - globlen));
	      globlen = strlen (globmatch);
	    }
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

  if ( nondefflags > 0 )
    {
      strncat (globmatch, p, (sizeof(globmatch) - globlen));
      globlen = strlen (globmatch);
    }

  /* Add ".suffix" to filename and definition if suffix is not 0 */
  if ( suffix )
    {
      snprintf (tstr, sizeof(tstr), ".%ld", suffix);
      strncat (filename, tstr, (sizeof(filename) - fnlen));
      strncat (definition, tstr, (sizeof(definition) - fnlen));
      fnlen = strlen (filename);
    }

  /* Make sure the filename and definition are NULL terminated */
  *(filename + sizeof(filename) - 1) = '\0';
  *(definition + sizeof(definition) - 1) = '\0';

  /* Check for previously used stream entry, otherwise create it */
  foundgroup = ds_getstream (datastream, reclen, definition, filename,
			     nondefflags, globmatch);

  if ( foundgroup != NULL )
    {
      /* Initial check (existing files) for future data, a negative
       * last sample time indicates it was derived from an existing
       * file.
       */
      if ( datastream->packettype == SLDATA &&
	   datastream->futureinitflag &&
	   foundgroup->lastsample < 0 )
	{
	  int overlap = (int) ((-1.0 * foundgroup->lastsample) - sl_msr_depochstime(msr));

	  if ( overlap > datastream->futureinit )
	    {
	      if ( foundgroup->futureinitprint )
		{
		  sl_log (2, 0,
			  "%d sec. overlap of existing archive data in %s, skipping\n",
			  overlap, foundgroup->filename);
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
	  int overlap = (int) (foundgroup->lastsample - sl_msr_depochstime(msr));

          if ( overlap > datastream->futurecont )
	    {
	      if ( foundgroup->futurecontprint )
		{
		  sl_log (2, 0,
			  "%d sec. overlap of continuous data for %s, skipping\n",
			  overlap, foundgroup->filename);
		  foundgroup->futurecontprint = 0;  /* Suppress further messages */
		}

	      return 0;
	    }
	  else if ( foundgroup->futurecontprint == 0 )
	    foundgroup->futurecontprint = 1;  /* Reset message printing */
	}

      /*  Write the record to the appropriate file */
      sl_log (1, 3, "Writing data to data stream file %s\n", foundgroup->filename);

      /* Try up to 10 times to write the data out, could be interrupted by signal */
      writebytes = 0;
      writeloops = 0;
      while ( writeloops < 10 )
        {
	  rv = write (foundgroup->filed, msr->msrecord+writebytes, reclen-writebytes);

	  if ( rv > 0 )
	    writebytes += rv;

	  /* Done if the entire record was written */
	  if ( writebytes == reclen )
	    break;

	  if ( rv < 0 )
	    {
	      if ( errno != EINTR )
		{
		  sl_log (2, 1, "ds_streamproc: failed to write record: %s (%s)\n",
			  strerror(errno), foundgroup->filename);
		  return -1;
		}
	      else
		{
		  sl_log (1, 1, "ds_streamproc: Interrupted call to write (%s), retrying\n",
			  foundgroup->filename);
		}
	    }

	  writeloops++;
        }

      if ( writeloops >= 10 )
	{
	  sl_log (2, 0, "ds_streamproc: Tried 10 times to write record, interrupted each time\n");
	  return -1;
	}

      /* Update mod time for this entry */
      foundgroup->modtime = time (NULL);

      /* Update time of last sample if future checking */
      if ( datastream->packettype == SLDATA &&
	   (datastream->futureinitflag || datastream->futurecontflag) )
	foundgroup->lastsample = sl_msr_lastsamptime (msr);

      return 0;
    }

  return -1;
}  /* End of ds_streamproc() */


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
ds_getstream (DataStream *datastream, int reclen,
	      const char *defkey, char *filename,
	      int nondefflags, const char *globmatch)
{
  DataStreamGroup *foundgroup  = NULL;
  DataStreamGroup *searchgroup = NULL;
  DataStreamGroup *prevgroup   = NULL;
  time_t curtime;
  char *matchedfilename = 0;

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

	  break;
	}

      prevgroup = searchgroup;
      searchgroup = nextgroup;
    }

  /* If no matching stream entry was found but the format included
     non-defining flags, try to use globmatch to find a matching file
     and resurrect a stream entry */
  if ( foundgroup == NULL && nondefflags > 0 )
    {
      glob_t pglob;
      int rval;

      sl_log (1, 3, "No stream entry found, searching for: %s\n", globmatch);

      rval = glob (globmatch, 0, NULL, &pglob);

      if ( rval && rval != GLOB_NOMATCH )
	{
	  switch (rval)
	    {
	    case GLOB_ABORTED : sl_log (2, 1, "glob(): Unignored lower-level error\n");
	    case GLOB_NOSPACE : sl_log (2, 1, "glob(): Not enough memory\n");
	    case GLOB_NOSYS : sl_log (2, 1, "glob(): Function not supported\n");
	    default : sl_log (2, 1, "glob(): %d\n", rval);
	    }
	}
      else if ( rval == 0 && pglob.gl_pathc > 0 )
	{
	  if ( pglob.gl_pathc > 1 )
	    sl_log (1, 3, "Found %lu files matching %s, using last match\n",
		    pglob.gl_pathc, globmatch);

	  matchedfilename = pglob.gl_pathv[pglob.gl_pathc-1];
	  sl_log (1, 2, "Found matching file for non-defining flags: %s\n", matchedfilename);

	  /* Now that we have a match use it instead of filename */
	  strncpy (filename, matchedfilename, MAX_FILENAME_LEN-2);
          filename[MAX_FILENAME_LEN-1] = '\0';
	}

      globfree (&pglob);
    }

  /* If not found, create a stream entry */
  if ( foundgroup == NULL )
    {
      if ( matchedfilename )
	sl_log (1, 2, "Resurrecting data stream entry for key %s\n", defkey);
      else
	sl_log (1, 2, "Creating data stream entry for key %s\n", defkey);

      foundgroup = (DataStreamGroup *) malloc (sizeof (DataStreamGroup));

      foundgroup->defkey = strdup (defkey);
      foundgroup->filed = 0;
      foundgroup->modtime = curtime;
      foundgroup->lastsample = 0.0;
      foundgroup->futurecontprint = datastream->futurecontflag;
      foundgroup->futureinitprint = datastream->futureinitflag;
      strncpy (foundgroup->filename, filename, sizeof(foundgroup->filename));
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

  /* Keep ds_closeidle from closing this stream */
  if ( foundgroup->modtime > 0 )
    {
      foundgroup->modtime *= -1;
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
	  /* Do not complain if the call was interrupted (signals are used for shutdown) */
	  if ( errno == EINTR )
	    foundgroup->filed = 0;
	  else
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
	      SLMSrecord *lmsr = NULL;
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

	      if ( sl_msr_parse (NULL, lrecord, &lmsr, 0, 0) != NULL )
		{
		  /* A negative last sample time means it came from an existing file */
		  foundgroup->lastsample = (-1 * sl_msr_lastsamptime (lmsr));
		}
	      else
		{
		  /* Zero means last sample time is unknown, disabling checks */
		  foundgroup->lastsample = 0.0;
		}

	      sl_msr_free (&lmsr);
	      free (lrecord);
	    }
	}
    }

  /* There used to be a further check here, but it shouldn't be reached, just in
     case this is left for the moment until I'm convinced. */
  else if ( strcmp (defkey, foundgroup->defkey) )
    sl_log (2, 0, "Arg! open file for a key that no longer matches\n");

  return foundgroup;
}  /* End of ds_getstream() */


/***************************************************************************
 * ds_openfile:
 *
 * Open a specified file, if the open file limit has been reach try
 * once to increase the limit, if that fails or has already been done
 * start closing idle files with decreasing idle timeouts until a file
 * can be opened.
 *
 * Parent directories that are needed are created.
 *
 * Directories are created with full (0777) permissions and files are created
 * with full (0666) permissions.
 *
 * Return the result of open(2), normally this a the file descriptor
 * on success and -1 on error.
 ***************************************************************************/
static int
ds_openfile (DataStream *datastream, const char *filename)
{
  static char rlimit = 0;
  struct rlimit rlim;
  SLstrlist *dplist = NULL;
  SLstrlist *dpptr = NULL;
  char dirpath[MAX_FILENAME_LEN] = "";
  int dplen = 0;
  int idletimeout = datastream->idletimeout;
  int oret = 0;
  int flags = (O_RDWR | O_CREAT | O_APPEND);
  mode_t mode = (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH); /* Mode 0666 */

  if ( ! datastream )
    return -1;

  /* Lookup process open file limit and change ds_maxopenfiles if needed */
  if ( ! rlimit )
    {
      rlimit = 1;

      if ( getrlimit (RLIMIT_NOFILE, &rlim) == -1 )
	{
	  sl_log (2, 0, "getrlimit failed to get open file limit\n");
	}
      else
	{
	  /* Increase process open file limit to ds_maxopenfiles or hard limit */
	  if ( ds_maxopenfiles && (rlim_t)ds_maxopenfiles > rlim.rlim_cur )
	    {
	      if ( (rlim_t)ds_maxopenfiles > rlim.rlim_max )
		rlim.rlim_cur = rlim.rlim_max;
	      else
		rlim.rlim_cur = ds_maxopenfiles;

	      sl_log (1, 3, "Setting open file limit to %lld\n", (int64_t) rlim.rlim_cur);

	      if ( setrlimit (RLIMIT_NOFILE, &rlim) == -1 )
		{
		  sl_log (2, 0, "setrlimit failed to set open file limit\n");
		}

	      ds_maxopenfiles = rlim.rlim_cur;
	    }
	  /* Set max to current soft limit if not already specified */
	  else if ( ! ds_maxopenfiles )
	    {
	      ds_maxopenfiles = rlim.rlim_cur;
	    }
	}
    }

  /* Close open files from the DataStream if already at the limit of (ds_maxopenfiles - 10) */
  if ( (ds_openfilecount + 10) > ds_maxopenfiles )
    {
      sl_log (1, 2, "Maximum open archive files reached (%d), closing idle stream files\n",
	      (ds_maxopenfiles - 10));

      /* Close idle streams until we have free descriptors */
      while ( ds_closeidle (datastream, idletimeout) == 0 && idletimeout >= 0 )
	{
	  idletimeout = (idletimeout / 2) - 1;
	}
    }

  /* Parse filename into path components */
  sl_strparse (filename, "/", &dplist);
  dpptr = dplist;

  /* Special case of an absolute path (first entry is empty) */
  if ( *dpptr->element == '\0' )
    {
      strncat (dirpath, "/", (sizeof(dirpath) - dplen));
      dplen += 1;
      dpptr = dpptr->next;
    }

  /* Verify existence of each parent directory and create if needed.
   * If the parsed path entry is not the last then it should be a directory. */
  while ( dpptr && dpptr->next )
    {
      /* Add entry to directory path */
      strncat (dirpath, dpptr->element, (sizeof(dirpath) - dplen));
      dplen = strlen (dirpath);

      if ( access (dirpath, F_OK) )
	{
	  if ( errno == ENOENT )
	    {
	      sl_log (1, 1, "Creating directory: %s\n", dirpath);
	      if ( mkdir (dirpath, S_IRWXU | S_IRWXG | S_IRWXO) ) /* Mode 0777 */
		{
		  sl_log (2, 1, "ds_openfile: mkdir(%s) %s\n", dirpath, strerror (errno));
		  sl_strparse (NULL, NULL, &dplist);
		  return -1;
		}
	    }
	  else
	    {
	      sl_log (2, 0, "%s: access denied, %s\n", dirpath, strerror(errno));
	      sl_strparse (NULL, NULL, &dplist);
	      return -1;
	    }
	}

      strncat (dirpath, "/", (sizeof(dirpath) - dplen));
      dplen += 1;

      dpptr = dpptr->next;
    }

  /* Free path component list */
  sl_strparse (NULL, NULL, &dplist);

  /* Open file */
  if ( (oret = open (filename, flags, mode)) != -1 )
    {
      ds_openfilecount++;
    }

  return oret;
}  /* End of ds_openfile() */


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

      if ( searchgroup->modtime > 0 && (curtime - searchgroup->modtime) >= idletimeout )
	{
	  sl_log (1, 2, "Closing idle stream with key %s\n", searchgroup->defkey);

	  /* Re-link the stream chain */
	  if ( prevgroup != NULL )
	    {
	      prevgroup->next = searchgroup->next;
	    }
	  else
	    {
	      datastream->grouproot = searchgroup->next;
	    }

	  /* Close the associated file */
	  if ( close (searchgroup->filed) )
	    sl_log (2, 0, "ds_closeidle(), closing data stream file, %s\n", strerror (errno));
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

  ds_openfilecount -= count;

  return count;
}  /* End of ds_closeidle() */


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

      if ( prevgroup->filed )
	if ( close (prevgroup->filed) )
	  sl_log (2, 0, "ds_shutdown(), closing data stream file, %s\n",
		  strerror (errno));

      free (prevgroup->defkey);
      free (prevgroup);
    }
}  /* End of ds_shutdown() */


/***************************************************************************
 * sl_msr_lastsamptime:
 *
 * Calculate the time of the last sample in the record; this is the actual
 * last sample time and *not* the time "covered" by the last sample.
 *
 * Returns the time of the last sample as a double precision epoch time.
 ***************************************************************************/
double
sl_msr_lastsamptime (SLMSrecord *msr)
{
  double startepoch;
  double dsamprate;
  double span = 0.0;

  sl_msr_dsamprate (msr, &dsamprate);

  if ( dsamprate )
    span = (double) (msr->fsdh.num_samples - 1 ) * (1.0 / dsamprate);

  startepoch = sl_msr_depochstime(msr);

  return (startepoch + span);
}  /* End of sl_msr_lastsamptime() */


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
