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


#include <stdlib.h>
#include "set-app.h"


/* INITIALIZE A SET, AS EMPTY.  Initializes the set pointed to by 'set' to
   be an empty set, which will use 'chain' to link elements.  Note that the
   set must never contain elements with the same index as elements of any
   other set using the same chain. */

void set_init (struct set *set, int chain)
{
  set->chain = chain;
  set->first = -1;
}


/* CHECK WHETHER A SET CONTAINS A SPECIFIED VALUE.  Returns 1 if 'val' is
   an element of 'set', 0 if not. */

int set_contains (struct set *set, set_value_t val)
{
  set_index_t index = SET_VAL_INDEX(val);
  set_offset_t offset = SET_VAL_OFFSET(val);

  return (SET_SEGMENT(index)->bits[set->chain] >> offset) & 1;
}


/* ADD A VALUE TO A SET.  Changes 'set' so that it contains 'val' as
   an element.  Returns 1 if 'set' had already contained 'val', 0 if not. */

int set_add (struct set *set, set_value_t val)
{
  set_index_t index = SET_VAL_INDEX(val);
  set_offset_t offset = SET_VAL_OFFSET(val);
  struct set_segment *ptr = SET_SEGMENT(index);
  set_bits_t b = ptr->bits[set->chain];
  set_bits_t t = (set_value_t)1 << offset;

  if (b & t) return 1;

  if (ptr->next[set->chain] == -1)
  { ptr->next[set->chain] = set->first;
    set->first = index;
  }

  ptr->bits[set->chain] |= t;

  return 0;
}


/* REMOVE A VALUE FROM A SET.  Changes 'set' so that it does not contain
   'val' as an element.  Returns 1 if 'set' had previously contained 'val',
   0 if not. */

int set_remove (struct set *set, set_value_t val)
{
  set_index_t index = SET_VAL_INDEX(val);
  set_offset_t offset = SET_VAL_OFFSET(val);
  struct set_segment *ptr = SET_SEGMENT(index);
  set_bits_t b = ptr->bits[set->chain];
  set_bits_t t = (set_value_t)1 << offset;

  if ((b & t) == 0) return 0;

  ptr->bits[set->chain] &= ~t;

  return 1;
}


/* FIND THE FIRST ELEMENT IN A SET.  Finds an element of 'set', the first
   in an arbitrary ordering,  Returns SET_NO_VALUE if the set is empty. */

set_value_t set_first (struct set *set)
{ 
  struct set_segment *ptr;

  for (;;)
  { 
    if (set->first == -1) return SET_NO_VALUE;
    ptr = SET_SEGMENT(set->first);
    if (ptr->bits[set->chain] != 0) break;
    set->first = ptr->next[set->chain];
    ptr->next[set->chain] = -1;
  }

  set_value_t val = SET_VAL(set->first,0);
  return (ptr->bits[set->chain] & 1) ? val : set_next (set, val);
}


/* FIND THE NEXT ELEMENT IN A SET.  Returns the next element of 'set'
   after 'val', or SET_NO_VALUE if 'val' was the last value. */

set_value_t set_next (struct set *set, set_value_t val)
{
  set_index_t index = SET_VAL_INDEX(val);
  set_offset_t offset = SET_VAL_OFFSET(val);
  struct set_segment *ptr = SET_SEGMENT(index);
  set_bits_t b = (ptr->bits[set->chain] >> offset) >> 1;

  if (b == 0)
  { set_index_t nindex;
    struct set_segment *nptr;
    for (;;)
    { nindex = ptr->next[set->chain];
      if (nindex == -1) return SET_NO_VALUE;
      nptr = SET_SEGMENT(nindex);
      b = nptr->bits[set->chain];
      if (b != 0) break;
      ptr->next[set->chain] = nptr->next[set->chain];
    }
    index = nindex;
    offset = -1;
  }

  for (;;)
  { offset += 1;
    if (b & 1) break;
    b >>= 1;
  } 

  return SET_VAL(index,offset);
}


