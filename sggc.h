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

typedef set_index_t sggc_index_t; /* Type of segment index */
typedef set_value_t sggc_cptr_t;  /* Type of compressed pointer (index,offset)*/

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

#ifdef SGGC_AUX1_SIZE
char **sggc_aux1;                  /* Pointer to array of pointers to arrays of 
                                      auxiliary info for objects in segments */
#endif

#ifdef SGGC_AUX2_SIZE
char **sggc_aux2;                  /* Pointer to array of pointers to arrays of 
                                      auxiliary info for objects in segments */
#endif

/* Macros to get pointer to auxiliarly information for an object, from its 
   index and offset. */

#define SGGC_AUX1(cptr) \
  (sggc_aux1[SET_VAL_INDEX(cptr)] + SGGC_AUX1_SIZE * SET_VAL_OFFSET(cptr))

#define SGGC_AUX2(cptr) \
  (sggc_aux2[SET_VAL_INDEX(cptr)] + SGGC_AUX2_SIZE * SET_VAL_OFFSET(cptr))


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

#ifdef SGGC_AUX1_READ_ONLY
char *sggc_aux1_read_only (sggc_kind_t kind);
#endif
#ifdef SGGC_AUX1_READ_ONLY
char *sggc_aux2_read_only (sggc_kind_t kind);
#endif

#ifdef SGGC_AFTER_COLLECT
void sggc_after_collect (int level, int rep);
#endif

/* FUNCTIONS USED BY THE APPLICATION. */

int sggc_init (int max_segments);
sggc_cptr_t sggc_alloc (sggc_type_t type, sggc_length_t length);
void sggc_collect (int level);
int sggc_look_at (sggc_cptr_t cptr);
void sggc_old_to_new_check (sggc_cptr_t from_ptr, sggc_cptr_t to_ptr);
int sggc_youngest_generation (sggc_cptr_t from_ptr);
int sggc_not_marked (sggc_cptr_t ptr);

sggc_cptr_t sggc_constant (sggc_type_t type, sggc_kind_t kind, set_bits_t bits,
                           char *data
#ifdef SGGC_AUX1_SIZE
                         , char *aux1
#endif
#ifdef SGGC_AUX2_SIZE
                         , char *aux2
#endif
);
