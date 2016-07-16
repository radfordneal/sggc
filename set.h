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
   struct set_node, which may be used in set-app.h after inclusion of
   set.h. */


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


struct set_node 
{ set_index_t next;
  set_index_t prev;
  set_bits_t bits;
};

struct set
{ int chain;
  set_index_t first;  /* -1 if empty */
};

void set_init (struct set *set, int chain);
int set_is_empty (struct set *set);
int set_contains_value (struct set *set, set_value_t val);
int set_add_value (struct set *set, set_value_t val);
int set_remove_value (struct set *set, set_value_t val);
set_value_t set_first_element (struct set *set);
set_value_t set_next_element (struct set *set, set_value_t val);
