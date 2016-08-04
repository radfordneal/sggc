/* SGGC - A LIBRARY SUPPORTING SEGMENTED GENERATIONAL GARBAGE COLLECTION.
          Header for sggc's use of the facility for maintaining sets of objects

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


/* CHAINS FOR LINKING SEGMENTS IN SETS.  All sets used have their own
   chain, except that the SET_UNUSED_FREE_NEW chain is shared amongst 
   a collection of sets, one set for each possible kind of object. */

#define SET_CHAINS 5       /* Number of chains that can be used for sets */
#  define SET_UNUSED_FREE_NEW 0  /* Unused, free or newly allocated objects */
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

/* Extra information possibly defined by the application.  Should be as
   bit fields with more than 7 bits total. */

#ifndef SGGC_EXTRA_INFO
#define SGGC_EXTRA_INFO
#endif

/* Phases for allocations of auxilary information, allowing overlap when
   an item takes up more than one chunk.  Will cause expansion of the
   segment structure if SGGC_AUX_ITEMS is greater than two. */

#if SGGC_AUX_ITEMS > 0
#define SGGC_AUX_PHASES unsigned char phase[SGGC_AUX_ITEMS];
#else
#define SGGC_AUX_PHASES 
#endif

#define SET_EXTRA_INFO \
  union \
  { struct                 /* For big segments... */ \
    { unsigned big : 1;       /* 1 for a big segment with one large object  */ \
      unsigned max_chunks : SGGC_CHUNK_BITS;/* Chunks that fit in the space */ \
    } big;                                  /*  allocated; 0 if size fixed  */ \
    struct                 /* For small segments... */ \
    { unsigned big : 1;       /* 0 for a segment with several small objects */ \
      SGGC_EXTRA_INFO         /* Extra app information for small segments   */ \
      SGGC_AUX_PHASES         /* Phases for auxiliary storage allocations   */ \
      unsigned char kind;     /* The kind of segment (equal to type if big) */ \
    } small; \
  } x;


/* INCLUDE THE NON-APPLICATION-SPECIFIC HEADER. */

#include "set.h"


/* POINTER TO ARRAY OF POINTERS TO SEGMENTS.  This array of pointers
   is allocated when the GC is initialized, with the segments themselves
   allocated later, as needed. */

struct set_segment **sggc_segment;


/* MACRO FOR GETTING SEGMENT POINTER FROM SEGMENT INDEX. */

#define SET_SEGMENT(index) (sggc_segment[index])
