/* SGGC - A LIBRARY SUPPORTING SEGMENTED GENERATIONAL GARBAGE COLLECTION.
          Header file for the garbage collection routines.

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


/* This header file must be included from the sggc-app.h file provided
   by the application using the set facility, after the definitions of
   sggc_length_t, sggc_type_t, SGGC_CHUNK_SIZE, SGGC_AUX_ITEMS, and
   SGGC_N_KINDS. */


#include "set-app.h"


/* COMPRESSED POINTER (INDEX, OFFSET) TYPE, AND UNCOMPRESS MACRO. */

typedef set_value_t sggc_cptr;

#define SGGC_UPTR(cptr) \
  (sggc_data [SET_VAL_INDEX(cptr)] + SGGC_CHUNK_SIZE * SET_VAL_OFFSET(cptr))


/* MAIN DATA, AUXILIARY DATA, AND LENGTH FOR SEGMENTS. */

char **sggc_data;
sggc_length_t **sggc_length;

#if SGGC_AUX_ITEMS > 0
  char **sggc_aux[SGGC_AUX_ITEMS];
#endif


/* FUNCTIONS USED BY THE APPLICATION. */

void sggc_init (int n_segments);
sggc_cptr sggc_alloc (sggc_type_t type, sggc_length_t length);
void sggc_collect (int level);
