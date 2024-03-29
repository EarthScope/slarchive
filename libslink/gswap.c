/***************************************************************************
 * gswap.c:
 *
 * Functions for generalized byte in-place swapping between LSBF and
 * MSBF byte orders.
 *
 * Some standard integer types are needed, namely uint8_t and
 * uint32_t, (these are normally declared by including inttypes.h or
 * stdint.h).  Each function expects it's input to be a void pointer
 * to an quantity of the appropriate size.
 *
 * There are two versions of most routines, one that works on
 * quantities regardless of alignment (gswapX) and one that works on
 * memory aligned quantities (gswapXa).  The memory aligned versions
 * (gswapXa) are much faster than the other versions (gswapX), but the
 * memory *must* be aligned.
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

#include <memory.h>

#include "slplatform.h"

/* Swap routines that work on any (aligned or not) quantities */

void
sl_gswap2 (void *data2)
{
  uint8_t temp;

  union {
    uint8_t c[2];
  } dat;

  memcpy (&dat, data2, sizeof (dat));
  temp     = dat.c[0];
  dat.c[0] = dat.c[1];
  dat.c[1] = temp;
  memcpy (data2, &dat, sizeof (dat));
}

void
sl_gswap3 (void *data3)
{
  uint8_t temp;

  union {
    uint8_t c[3];
  } dat;

  memcpy (&dat, data3, sizeof (dat));
  temp     = dat.c[0];
  dat.c[0] = dat.c[2];
  dat.c[2] = temp;
  memcpy (data3, &dat, sizeof (dat));
}

void
sl_gswap4 (void *data4)
{
  uint8_t temp;

  union {
    uint8_t c[4];
  } dat;

  memcpy (&dat, data4, sizeof (dat));
  temp     = dat.c[0];
  dat.c[0] = dat.c[3];
  dat.c[3] = temp;
  temp     = dat.c[1];
  dat.c[1] = dat.c[2];
  dat.c[2] = temp;
  memcpy (data4, &dat, sizeof (dat));
}

void
sl_gswap8 (void *data8)
{
  uint8_t temp;

  union {
    uint8_t c[8];
  } dat;

  memcpy (&dat, data8, sizeof (dat));
  temp     = dat.c[0];
  dat.c[0] = dat.c[7];
  dat.c[7] = temp;

  temp     = dat.c[1];
  dat.c[1] = dat.c[6];
  dat.c[6] = temp;

  temp     = dat.c[2];
  dat.c[2] = dat.c[5];
  dat.c[5] = temp;

  temp     = dat.c[3];
  dat.c[3] = dat.c[4];
  dat.c[4] = temp;
  memcpy (data8, &dat, sizeof (dat));
}

/* Swap routines that work on memory aligned quantities */

void
sl_gswap2a (void *data2)
{
  uint16_t *data = data2;

  *data = (((*data >> 8) & 0xff) | ((*data & 0xff) << 8));
}

void
sl_gswap4a (void *data4)
{
  uint32_t *data = data4;

  *data = (((*data >> 24) & 0xff) | ((*data & 0xff) << 24) |
           ((*data >> 8) & 0xff00) | ((*data & 0xff00) << 8));
}

void
sl_gswap8a (void *data8)
{
  uint32_t *data4 = data8;
  uint32_t h0, h1;

  h0 = data4[0];
  h0 = (((h0 >> 24) & 0xff) | ((h0 & 0xff) << 24) |
        ((h0 >> 8) & 0xff00) | ((h0 & 0xff00) << 8));

  h1 = data4[1];
  h1 = (((h1 >> 24) & 0xff) | ((h1 & 0xff) << 24) |
        ((h1 >> 8) & 0xff00) | ((h1 & 0xff00) << 8));

  data4[0] = h1;
  data4[1] = h0;
}
