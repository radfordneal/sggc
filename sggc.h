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
   types sggc_length_t and sggc_nchunks_t, and constants SGGC_N_KINDS,
   SGGC_CHUNK_SIZE, and SGGC_AUX_ITEMS. */


#include "set-app.h"


/* COMPRESSED POINTER (INDEX, OFFSET) TYPE. */

typedef set_value_t sggc_cptr_t;


/* POINTERS TO SPACE FOR MAIN AND AUXILIARY DATA. */

char **sggc_data;

#define SGGC_DATA(cptr) \
  (sggc_data [SET_VAL_INDEX(cptr)] + SGGC_CHUNK_SIZE * SET_VAL_OFFSET(cptr))

#if SGGC_AUX_ITEMS > 0
  char **sggc_aux[SGGC_AUX_ITEMS];
#endif

#define SGGC_AUX(cptr,i) \
  (sggc_aux[i][SET_VAL_INDEX(cptr)] + SGGC_AUX##i##_SIZE * SET_VAL_OFFSET(cptr))


/* TYPES AND KINDS OF SEGMENTS. */

typedef unsigned char sggc_type_t;

sggc_type_t *sggc_type;

#define SGGC_TYPE(cptr) (sggc_type[SET_VAL_INDEX(cptr)])

typedef unsigned char sggc_kind_t;

#define SGGC_KIND(cptr) \
  (SET_SEGMENT(cptr)->x.big.bih ? SGGC_TYPE(cptr) \
                                : SET_SEGMENT(cptr)->x.small.kind)


/* FUNCTIONS PROVIDED BY THE APPLICATION. */

sggc_kind_t sggc_kind (sggc_type_t type, sggc_length_t length);
sggc_nchunks_t sggc_chunks (sggc_type_t type, sggc_length_t length);
void sggc_find_root_ptrs (void);
void sggc_find_ptrs (sggc_cptr_t cptr);


/* FUNCTIONS USED BY THE APPLICATION. */

int sggc_init (int n_segments);
sggc_cptr_t sggc_alloc (sggc_type_t type, sggc_length_t length);
void sggc_collect (int level);
void sggc_look_at (sggc_cptr_t cptr);
