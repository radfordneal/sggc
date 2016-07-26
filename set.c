/* SGGC - A LIBRARY SUPPORTING SEGMENTED GENERATIONAL GARBAGE COLLECTION.
          Facility for maintaining sets of objects - function definitions

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
    CHK_CHAIN((set)->chain); \
    if (DEBUG && (set)->first < 0 \
              && (set)->first != SET_END_OF_CHAIN) abort(); \
  } while (0)

#define CHK_SEGMENT(seg,chain) \
  do { \
    CHK_CHAIN(chain); \
    if (DEBUG && (seg)->next[chain] < 0 \
              && (seg)->next[chain] != SET_NOT_IN_CHAIN \
              && (seg)->next[chain] != SET_END_OF_CHAIN) abort(); \
    if (DEBUG && (seg)->next[chain] == SET_NOT_IN_CHAIN \
              && (seg)->bits[chain] != 0) abort(); \
  } while (0)


/* INITIALIZE A SET, AS EMPTY.  Initializes the set pointed to by 'set' to
   be an empty set, which will use 'chain' to link elements.  Note that the
   set must never contain elements with the same index as elements of any
   other set using the same chain. */

void set_init (struct set *set, int chain)
{
  CHK_CHAIN(chain);
  set->chain = chain;
  set->first = SET_END_OF_CHAIN;
}


/* INITIALIZE A SEGMENT, NOT CONTAINING ELEMENTS IN ANY CHAIN.  Must be called
   before a segment is used in a set. */

void set_segment_init (struct set_segment *seg)
{
  int j;
  for (j = 0; j < SET_CHAINS; j++)
  { seg->bits[j] = 0;
    seg->next[j] = SET_NOT_IN_CHAIN;
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

  if (seg->next[set->chain] == SET_NOT_IN_CHAIN)
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
   needed is deferred to when (if ever) it is looked at in a search, except
   that it is removed now if it is at the front of the list. */

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
  if (seg->bits[set->chain] == 0 && set->first == index)
  { set->first = seg->next[set->chain];
    seg->next[set->chain] = SET_NOT_IN_CHAIN;
  }

  return 1;
}


/* REMOVE ANY EMPTY SEGMENTS AT THE FRONT OF A SET. */

static inline void remove_empty (struct set *set)
{
  struct set_segment *seg;

  CHK_SET(set);

  while (set->first != SET_END_OF_CHAIN)
  { 
    seg = SET_SEGMENT(set->first);
    CHK_SEGMENT(seg,set->chain);

    if (seg->bits[set->chain] != 0)
    { break;
    }

    set->first = seg->next[set->chain];
    seg->next[set->chain] = SET_NOT_IN_CHAIN;
  }
}


/* FIND POSITION OF LOWEST-ORDER BIT.  The position returned is from 0 up.
   The argument must not be zero.

   Could be speeded up, but not yet. */

static inline int first_bit_pos (set_bits_t b)
{ int pos;
  pos = 0;
  while ((b & 1) == 0)
  { pos += 1;
    b >>= 1;
  }
  return pos;
}


/* FIND THE FIRST ELEMENT IN A SET.  Finds an element of 'set', the first
   in an arbitrary ordering,  Returns SET_NO_VALUE if the set is empty.

   The linked list of segments for this set is first trimmed of any at 
   the front that have no elements set, to save time in any future searches. */

set_value_t set_first (struct set *set)
{ 
  struct set_segment *seg;
  set_bits_t b;

  CHK_SET(set);

  remove_empty(set);

  if (set->first == SET_END_OF_CHAIN) 
  { return SET_NO_VALUE;
  }

  seg = SET_SEGMENT(set->first);
  CHK_SEGMENT(seg,set->chain);

  b = seg->bits[set->chain];

  return SET_VAL (set->first, first_bit_pos(b));
}


/* FIND THE NEXT ELEMENT IN A SET.  Returns the next element of 'set'
   after 'val', which must be an element of 'set.  Returns SET_NO_VALUE 
   if there are no elements past 'val'.  If the last argument is non-zero,
   'val' is also removed from 'set'.

   If the linked list has to be followed to a later segment, any
   unused segments that are skipped are deleted from the list, to save
   time in any future searches. */

set_value_t set_next (struct set *set, set_value_t val, int remove)
{
  set_index_t index = SET_VAL_INDEX(val);
  set_offset_t offset = SET_VAL_OFFSET(val);
  struct set_segment *seg = SET_SEGMENT(index);

  CHK_SET(set);
  CHK_SEGMENT(seg,set->chain);

  /* Get the bits after the one for the element we are looking after.
     Also clear the bit for 'val' if we are removing it. */

  set_bits_t b = seg->bits[set->chain] >> offset;
  if ((b & 1) == 0) abort();  /* 'val' isn't in 'set' */
  if (remove)
  { seg->bits[set->chain] &= ~ ((set_bits_t) 1 << offset);
  }
  offset += 1;
  b >>= 1;

  /* If no bits are set after the one we are starting at, go to the
     next segment, removing ones that are unused.  We may discover
     that there is no next element, and return SET_NO_VALUE. */

  if (b == 0)
  { set_index_t nindex;
    struct set_segment *nseg;

    for (;;)
    { 
      nindex = seg->next[set->chain];
      if (nindex == SET_END_OF_CHAIN) 
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
    offset = 0;
  }

  offset += first_bit_pos(b);

  return SET_VAL(index,offset);
}


/* RETURN THE BITS INDICATING MEMBERSHIP FOR THE FIRST SEGMENT OF A SET.  
   First removes empty segments at the front.  Returns zero if the set
   is empty (which will not be returned otherwise). */

set_bits_t set_first_bits (struct set *set)
{
  struct set_segment *seg;

  CHK_SET(set);
  remove_empty(set);

  if (set->first == SET_END_OF_CHAIN)
  { return 0;
  }

  return SET_SEGMENT(set->first) -> bits[set->chain];
}


/* MOVE THE FIRST SEGMENT OF A SET TO ANOTHER SET.  Adds the first
   segment of 'src' to 'dst', removing it from 'src'.  It is an error
   for 'src' to be empty, or for its first segment to be empty, or for
   'src' and 'dst' to use different chains. */

void set_move_first (struct set *src, struct set *dst)
{
  struct set_segment *seg;
  set_index_t index;

  CHK_SET(src);
  CHK_SET(dst);

  if (src->chain != dst->chain) abort();
  if (src->first == SET_END_OF_CHAIN) abort();

  index = src->first;
  seg = SET_SEGMENT(index);
  CHK_SEGMENT(seg,src->chain);
  if (seg->bits[src->chain] == 0) abort();
  src->first = seg->next[src->chain];

  seg->next[src->chain] = dst->first;
  dst->first = index;
}

