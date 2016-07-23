/* SGGC - A LIBRARY SUPPORTING SEGMENTED GENERATIONAL GARBAGE COLLECTION.
          Garbage collection routines.

   Copyright (c) 2016 Radford M. Neal.

   The SGGC library is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

#include "sggc-app.h"


/* SETS OF OBJECTS. */

static struct set free_or_new[SGGC_N_KINDS];
static struct set old_gen1;
static struct set old_gen2;
static struct set old_to_new;
static struct set to_look_at;


void sggc_init (int n_segments)
{
}


sggc_cptr sggc_alloc (sggc_type_t type, sggc_length_t length)
{
  return SET_NO_VALUE;
}


void sggc_collect (int level)
{
}

