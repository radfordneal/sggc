/* SGGC - A LIBRARY SUPPORTING SEGMENTED GENERATIONAL GARBAGE COLLECTION.
          Header for use of the facility for maintaining sets of objects

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


/* NUMBER OF OFFSET BITS IN A (SEGMENT INDEX, OFFSET) PAIR. */

#define SET_OFFSET_BITS 6  /* Maximum for using 64-bit shift/mask operations */


/* CHAINS FOR LINKING SEGMENTS IN SETS.  All sets used have their own chain,
   except that the SET_FREE_OR_NEW chain is shared amongst a collection of sets,
   one set for each possible kind of object. */

#define SET_CHAINS 5       /* Number of chains that can be used for sets */
#  define SET_FREE_OR_NEW 0   /* Free or newly alloc'd objects, sets by kind */
#  define SET_OLD_GEN1 1      /* Objects that survived one GC */
#  define SET_OLD_GEN2 2      /* Objects that survived more than one GC */
#  define SET_OLD_TO_NEW 3    /* Objects maybe having old-to-new references */
#  define SET_TO_LOOK_AT 4    /* Objects that still need to be looked at */


/* EXTRA INFORMATION STORED IN A SET_SEGMENT STRUCTURE.  Putting it
   here takes advantage of what might otherwise be 32 bits of unused
   padding, and makes the set_segment struct be exactly 64 bytes in
   size (maybe advantageous for index computation (if needed), and
   perhaps for cache behaviour). */

#define SGGC_CHUNK_BITS 31    /* Bits used to record the number of chunks */

#define SET_EXTRA_INFO \
  union \
  { struct                 /* For big segments... */ \
    { unsigned big : 1;       /* 1 for a big segment with one large object  */ \
      unsigned max_chunks : SGGC_CHUNK_BITS;/* Chunks that fit in the space */ \
    } big;                                  /*  allocated; 0 if size fixed  */ \
    struct                 /* For small segments... */ \
    { unsigned big : 1;       /* 0 for a segment with several small objects */ \
      unsigned phase1 : 5;    /* Managed by the application; suitable for   */ \
      unsigned phase2 : 5;    /*  the "phases" for allocations of auxiliary */ \
      unsigned phase3 : 5;    /*  information for this segment              */ \
      unsigned kind : 8;      /* The kind of segment (equal to type if big) */ \
      unsigned n_chunks : 8;  /* Number of storage chunks allowed for object*/ \
    } small; \
  } x;


/* INCLUDE THE NON-APPLICATION-SPECIFIC HEADER. */

#include "set.h"


/* POINTER TO ARRAY OF POINTERS TO SEGMENTS.  This array of pointers
   is allocated when the GC is initialized (and possibly reallocated
   with increased size later), with the segments themselves allocated
   later, as needed. */

struct set_segment **sggc_segment;


/* MACRO FOR GETTING SEGMENT POINTER FROM SEGMENT INDEX. */

#define SET_SEGMENT(index) (sggc_segment[index])
