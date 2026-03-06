/***************************************************************************
 * strutils.c
 *
 * Routines for string manipulation
 *
 * This file is part of the SeedLink Library.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Copyright (C) 2022:
 * @author Chad Trabant, EarthScope Data Services
 ***************************************************************************/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "strutils.h"

/***************************************************************************
 * sl_strparse:
 *
 * splits a 'string' on 'delim' and puts each part into a linked list
 * pointed to by 'list' (a pointer to a pointer).  The last entry has
 * it's 'next' set to 0.  All elements are NULL terminated strings.
 * If both 'string' and 'delim' are NULL then the linked list is
 * traversed and the memory used is free'd and the list pointer is
 * set to NULL.
 *
 * Returns the number of elements added to the list, or 0 when freeing
 * the linked list.
 ***************************************************************************/
int
sl_strparse (const char *string, const char *delim, SLstrlist **list)
{
  const char *beg; /* beginning of element */
  const char *del; /* delimiter */
  int stop  = 0;
  int count = 0;
  int total;

  SLstrlist *curlist = 0;
  SLstrlist *tmplist = 0;

  if (string != NULL && delim != NULL)
  {
    total = strlen (string);
    beg   = string;

    while (!stop)
    {

      /* Find delimiter */
      del = strstr (beg, delim);

      /* Delimiter not found or empty */
      if (del == NULL || strlen (delim) == 0)
      {
        del  = string + strlen (string);
        stop = 1;
      }

      tmplist       = (SLstrlist *)malloc (sizeof (SLstrlist));
      tmplist->next = 0;

      tmplist->element = (char *)malloc (del - beg + 1);
      strncpy (tmplist->element, beg, (del - beg));
      tmplist->element[(del - beg)] = '\0';

      /* Add this to the list */
      if (count++ == 0)
      {
        curlist = tmplist;
        *list   = curlist;
      }
      else
      {
        curlist->next = tmplist;
        curlist       = curlist->next;
      }

      /* Update 'beg' */
      beg = (del + strlen (delim));
      if ((beg - string) > total)
        break;
    }

    return count;
  }
  else
  {
    curlist = *list;
    while (curlist != NULL)
    {
      tmplist = curlist->next;
      free (curlist->element);
      free (curlist);
      curlist = tmplist;
    }
    *list = NULL;

    return 0;
  }
} /* End of sl_strparse() */
