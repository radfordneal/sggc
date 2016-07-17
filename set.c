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


/* DEBUGGING CHECKS.  These check validity of data, calling abort if
   the check fails.  Enabled only if DEBUG is defined to be non-zero. */

#define DEBUG 1

#define CHK_CHAIN(chain) \
  do { \
    if (DEBUG && ((chain) < 0 || (chain) >= SET_CHAINS)) abort(); \
  } while (0)

#define CHK_SET(set) \
  do { \
    CHK_CHAIN(set->chain); \
    if (DEBUG && (set->first < 0 && set->first != -2)) abort(); \
  } while (0)

#define CHK_SEGMENT(seg,chain) \
  do { \
    CHK_CHAIN(chain); \
    if (DEBUG && (seg->next[chain] < -2)) abort(); \
    if (DEBUG && (seg->next[chain] == -1 && seg->bits[chain] != 0)) abort(); \
  } while (0)


/* INITIALIZE A SET, AS EMPTY.  Initializes the set pointed to by 'set' to
   be an empty set, which will use 'chain' to link elements.  Note that the
   set must never contain elements with the same index as elements of any
   other set using the same chain. */

void set_init (struct set *set, int chain)
{
  CHK_CHAIN(chain);
  set->chain = chain;
  set->first = -2;
}


/* INITIALIZE A SEGMENT, IN WHICH ALL CHAINS ARE EMPTY.  Must be called
   before a segment is used in a set. */

void set_segment_init (struct set_segment *seg)
{
  int j;
  for (j = 0; j < SET_CHAINS; j++)
  { seg->bits[j] = 0;
    seg->next[j] = -1;
  }
}


/* CHECK WHETHER A SET CONTAINS A SPECIFIED VALUE.  Returns 1 if 'val' is
   an element of 'set', 0 if not. 

   This is implemented by just looking at the right bit in the bits for
   the set's chain, within the segment structure for this value's index. */

int set_contains (struct set *set, set_value_t val)
{
  set_index_t index = SET_VAL_INDEX(val);
  set_offset_t offset = SET_VAL_OFFSET(val);
  struct set_segment *seg = SET_SEGMENT(index);

  CHK_SET(set);
  CHK_SEGMENT(seg,set->chain);

  return (seg->bits[set->chain] >> offset) & 1;
}


/* ADD A VALUE TO A SET.  Changes 'set' so that it contains 'val' as
   an element.  Returns 1 if 'set' had already contained 'val', 0 if not. 

   This is implemented by setting the right bit in the bits for the set's 
   chain, within the segment structure for this value's index. This segment
   is then added to the linked list of segments for this set if it is not 
   there already. */

int set_add (struct set *set, set_value_t val)
{
  set_index_t index = SET_VAL_INDEX(val);
  set_offset_t offset = SET_VAL_OFFSET(val);
  struct set_segment *seg = SET_SEGMENT(index);

  CHK_SET(set);
  CHK_SEGMENT(seg,set->chain);

  set_bits_t b = seg->bits[set->chain];
  set_bits_t t = (set_bits_t)1 << offset;

  if (b & t) return 1;

  if (seg->next[set->chain] == -1)
  { seg->next[set->chain] = set->first;
    set->first = index;
  }

  seg->bits[set->chain] |= t;

  return 0;
}


/* REMOVE A VALUE FROM A SET.  Changes 'set' so that it does not contain
   'val' as an element.  Returns 1 if 'set' had previously contained 'val',
   0 if not.

   This is implemented by clearing the right bit in the bits for the set's 
   chain, within the segment structure for this value's index.  Removing this
   segment from the linked list of segments for this set if it is no longer
   needed is deferred to when (if ever) it is looked at in a search. */

int set_remove (struct set *set, set_value_t val)
{
  set_index_t index = SET_VAL_INDEX(val);
  set_offset_t offset = SET_VAL_OFFSET(val);
  struct set_segment *seg = SET_SEGMENT(index);

  CHK_SET(set);
  CHK_SEGMENT(seg,set->chain);

  set_bits_t b = seg->bits[set->chain];
  set_bits_t t = (set_bits_t)1 << offset;

  if ((b & t) == 0) return 0;

  seg->bits[set->chain] &= ~t;

  return 1;
}


/* FIND THE FIRST ELEMENT IN A SET.  Finds an element of 'set', the first
   in an arbitrary ordering,  Returns SET_NO_VALUE if the set is empty.

   The linked list of segments for this set is first trimmed of any at 
   the front that have no elements set, to save time in any future searches.
   Once a segment with an element is found, a value is returned directly 
   if the first bit is set, and otherwise set_next is called.  (Note that 
   set_next can't do it all because it can't delete segments at the front 
   of the list.) */

set_value_t set_first (struct set *set)
{ 
  struct set_segment *seg;

  CHK_SET(set);

  /* Loop until a segment with an element is found, or it is discovered
     that the set is empty, while removing unused segments at the front. */

  for (;;)
  { 
    if (set->first == -2) 
    { return SET_NO_VALUE;
    }

    seg = SET_SEGMENT(set->first);
    CHK_SEGMENT(seg,set->chain);

    if (seg->bits[set->chain] != 0)
    { break;
    }

    set->first = seg->next[set->chain];
    seg->next[set->chain] = -1;
  }

  /* Return the value directly if it is for the first bit; otherwise
     use set_next to find the value. */

  set_value_t val = SET_VAL(set->first,0);
  return (seg->bits[set->chain] & 1) ? val : set_next (set, val);
}


/* FIND THE NEXT ELEMENT IN A SET.  Returns the next element of 'set'
   after 'val', or SET_NO_VALUE if there are no elements past 'val'.

   If the linked list has to be followed to a later segment, any
   unused segments that are skipped are deleted from the list, to save
   time in any future searches. */

set_value_t set_next (struct set *set, set_value_t val)
{
  set_index_t index = SET_VAL_INDEX(val);
  set_offset_t offset = SET_VAL_OFFSET(val);
  struct set_segment *seg = SET_SEGMENT(index);

  CHK_SET(set);
  CHK_SEGMENT(seg,set->chain);

  /* Get the bits after the one for the element we are looking after.
     Note that shifting by the number of bits in an operand is undefined,
     and so must be avoided. */

  set_bits_t b = (seg->bits[set->chain] >> offset) >> 1;

  /* If no bits are set after the one we are starting at, go to the
     next segment, removing ones that are unused.  We may discover
     that there is no next element, and return SET_NO_VALUE. */

  if (b == 0)
  { set_index_t nindex;
    struct set_segment *nseg;

    for (;;)
    { 
      nindex = seg->next[set->chain];
      if (nindex == -2) 
      { return SET_NO_VALUE;
      }

      nseg = SET_SEGMENT(nindex);
      CHK_SEGMENT(nseg,set->chain);

      b = nseg->bits[set->chain];
      if (b != 0) 
      { break;
      }

      seg->next[set->chain] = nseg->next[set->chain];
    }

    index = nindex;
    offset = -1;
  }

  /* Find the first bit in b that is set, incrementing offset to indicate
     the position of this bit in the original set of bits. 

     This could be speeded up, but not yet. */

  for (;;)
  { offset += 1;
    if (b & 1) break;
    b >>= 1;
  } 

  return SET_VAL(index,offset);
}
