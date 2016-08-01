/* SGGC - A LIBRARY SUPPORTING SEGMENTED GENERATIONAL GARBAGE COLLECTION.
          Segmented garbage collection - header file

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
   some required types and constants. */


#include "set-app.h"


/* COMPRESSED POINTER (INDEX, OFFSET) TYPE, AND NO OBJECT CONSTANT. */

typedef set_index_t sggc_index_t;  /* Type of segment index */
typedef set_value_t sggc_cptr_t;   /* Type of compressed point (index,offset) */

#define SGGC_NO_OBJECT SET_NO_VALUE   /* Special "no object" pointer */


/* ARRAYS OF POINTERS TO SPACE FOR MAIN AND AUXILIARY DATA .  The sggc_data
   and (perhaps) sggc_aux[0], sggc_aux[1], etc. arrays have length equal to
   the maximum number of segments, and are allocated at initialization.  
   Pointers in these arrays are set when segments are needed, by the 
   application, since the application may do tricks like making some of
   of these be shared between segments, if it knows this is possible.

   Main data is for one object in a 'big' segment, and for several in a
   'small' segment, having total number of chunks no more than given by
   SGGC_CHUNKS_IN_SMALL_SEGMENT. */

char **sggc_data;                  /* Pointer to array of pointers to arrays of 
                                      data blocks for objects within segments */

#define SGGC_CHUNKS_IN_SMALL_SEGMENT (1 << SET_OFFSET_BITS)

/* Macro to get data pointer for an object, from its index and offset. */

#define SGGC_DATA(cptr) \
  (sggc_data [SET_VAL_INDEX(cptr)] + SGGC_CHUNK_SIZE * SET_VAL_OFFSET(cptr))

#if SGGC_AUX_ITEMS > 0
  char **sggc_aux[SGGC_AUX_ITEMS]; /* Pointer to array of pointers to arrays of 
                                      auxiliary info for objects in segments */
#endif

/* Macro to get pointer to auxiliarly information for an object, from its 
   index and offset, along with the number indicating which auxiliary. 
   information is required, which must be a literal number (not a #define). */

#define SGGC_AUX(cptr,i) \
  (sggc_aux[i][SET_VAL_INDEX(cptr)] + SGGC_AUX##i##_SIZE * SET_VAL_OFFSET(cptr))


/* TYPES AND KINDS OF SEGMENTS.  Types and kinds must fit in 8 bits,
   with the kind being equal to the type if it is for a "big" segment.
   The kind of a small segment is recorded in the segment description.
   The array of types for segments is allocated at initialization. */

typedef unsigned char sggc_type_t;
typedef unsigned char sggc_kind_t;

sggc_type_t *sggc_type;            /* Types of objects in each segment */

/* Macro to access type of object, from the index of its segment. */

#define SGGC_TYPE(cptr) (sggc_type[SET_VAL_INDEX(cptr)])

/* Macro to find the kind of the segment containing an object. */

#define SGGC_KIND(cptr) \
  (SET_SEGMENT(SET_VAL_INDEX(cptr))->x.big.big ? SGGC_TYPE(cptr) \
    : SET_SEGMENT(SET_VAL_INDEX(cptr))->x.small.kind)


/* FUNCTIONS PROVIDED BY THE APPLICATION. */

sggc_kind_t sggc_kind (sggc_type_t type, sggc_length_t length);
sggc_nchunks_t sggc_nchunks (sggc_type_t type, sggc_length_t length);
void sggc_find_root_ptrs (void);
void sggc_find_object_ptrs (sggc_cptr_t cptr);
char *sggc_alloc_segment_aux (sggc_kind_t kind, int n);
void sggc_free_segment_aux (sggc_kind_t kind, int n, char *);


/* FUNCTIONS USED BY THE APPLICATION. */

int sggc_init (int n_segments);
sggc_cptr_t sggc_alloc (sggc_type_t type, sggc_length_t length);
void sggc_collect (int level);
void sggc_look_at (sggc_cptr_t cptr);
void sggc_old_to_new_check (sggc_cptr_t from_ptr, sggc_cptr_t to_ptr);
int sggc_youngest_generation (sggc_cptr_t from_ptr);
