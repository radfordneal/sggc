/* SGGC - A LIBRARY SUPPORTING SEGMENTED GENERATIONAL GARBAGE COLLECTION.
          Facility for maintaining sets of objects

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


/* This header file must be included from the set-app.h file provided
   by the application using the set facility, after the definition of
   SET_OFFSET_BITS and SET_CHAINS.  It will define struct set and
   struct set_segment, which may be used in set-app.h after inclusion of
   set.h. */


#include <stdint.h>

typedef int set_offset_t;
typedef int set_index_t;
typedef uint32_t set_value_t;

#define SET_VAL(index,offset) \
  (((set_value_t)(index) << SET_OFFSET_BITS) | (offset))
#define SET_VAL_INDEX(val) \
  ((val) >> SET_OFFSET_BITS)
#define SET_VAL_OFFSET(val) \
  ((val) & (((set_value_t)1 << SET_OFFSET_BITS) - 1))

#if SET_OFFSET_BITS == 3
  typedef uint8_t set_bits_t;
#elif SET_OFFSET_BITS == 4
  typedef uint16_t set_bits_t;
#elif SET_OFFSET_BITS == 5
  typedef uint32_t set_bits_t;
#elif SET_OFFSET_BITS == 6
  typedef uint64_t set_bits_t;
#endif


#define SET_NO_VALUE SET_VAL(~0,0)

#define SET_NOT_IN_CHAIN -1
#define SET_END_OF_CHAIN -2

struct set_segment
{ set_bits_t bits[SET_CHAINS];   /* Bits indicating membership in sets */
  set_index_t next[SET_CHAINS];  /* Next segment in chain, SET_NOT_IN_CHAIN, */
};                               /*   or SET_END_OF_CHAIN                    */
struct set
{ int chain;                     /* Chain used for this set */
  set_index_t first;             /* First segment, or SET_END_OF_CHAIN */
};

void set_init (struct set *set, int chain);
void set_segment_init (struct set_segment *seg);
int set_contains (struct set *set, set_value_t val);
int set_add (struct set *set, set_value_t val);
int set_remove (struct set *set, set_value_t val);
set_value_t set_first (struct set *set);
set_value_t set_next (struct set *set, set_value_t val);
