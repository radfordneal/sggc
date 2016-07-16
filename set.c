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
   set must always be disjoint from any other set using 'chain'. */

void set_init (struct set *set, int chain)
{
  set->chain = chain;
  set->first = -1;
}


/* CHECK WHETHER A SET IS EMPTY.  Returns 1 if 'set' is empty, 0 if not. */

int set_is_empty (struct set *set)
{
  return set->first == -1;
}


/* CHECK WHETHER A SET CONTAINS A SPECIFIED VALUE.  Returns 1 if 'val' is
   an element of 'set', 0 if not. */

int set_contains_value (struct set *set, set_value_t val)
{
  set_index_t index = SET_VAL_INDEX(val);

  set_offset_t offset = SET_VAL_OFFSET(val);
  return (SET_NODE_PTR(set->chain,index)->bits >> offset) & 1;
}


/* ADD A VALUE TO A SET.  Changes 'set' so that it contains 'val' as
   an element.  Returns 1 if 'set' had already contained 'val', 0 if not. */

int set_add_value (struct set *set, set_value_t val)
{
  set_index_t index = SET_VAL_INDEX(val);
  set_offset_t offset = SET_VAL_OFFSET(val);
  struct set_node *ptr = SET_NODE_PTR(set->chain,index);
  set_bits_t b = ptr->bits;
  set_bits_t t = (set_value_t)1 << offset;

  if (b & t) return 1;

  if (b == 0)
  { if (set->first == -1)
    { set->first = index;
      ptr->next = index;
      ptr->prev = index;
    }
    else
    { struct set_node *head = SET_NODE_PTR(set->chain,set->first);
      ptr->next = head->next;
      ptr->prev = set->first;
      SET_NODE_PTR(set->chain,ptr->next)->prev = index;
      head->next = index;
    }
  }

  ptr->bits |= t;

  return 0;
}


/* REMOVE A VALUE FROM A SET.  Changes 'set' so that it does not contain
   'val' as an element.  Returns 1 if 'set' had previously contained 'val',
   0 if not. */

int set_remove_value (struct set *set, set_value_t val)
{
  set_index_t index = SET_VAL_INDEX(val);
  set_offset_t offset = SET_VAL_OFFSET(val);
  struct set_node *ptr = SET_NODE_PTR(set->chain,index);
  set_bits_t b = ptr->bits;
  set_bits_t t = (set_value_t)1 << offset;

  if ((b & t) == 0) return 0;

  ptr->bits &= ~t;

  if (ptr->bits == 0)
  { if (ptr->next == index) 
    { set->first = -1;
    }
    else
    { if (set->first == index) 
      { set->first = ptr->next;
      }
      SET_NODE_PTR(set->chain,ptr->next)->prev = ptr->prev;
      SET_NODE_PTR(set->chain,ptr->prev)->next = ptr->next;
    }
  }

  return 1;
}


/* FIND THE FIRST ELEMENT IN A SET.  Finds an element of 'set', the first
   in an arbitrary ordering (which will, however, remain consistent). 
   Returns SET_NO_VALUE if the set is empty. */

set_value_t set_first_element (struct set *set)
{ 
  if (set_is_empty(set)) return SET_NO_VALUE;

  struct set_node *ptr = SET_NODE_PTR(set->chain,set->first);
  set_value_t val = SET_VAL (set->first, 0);
  return ptr->bits & 1 ? val : set_next_element (set, val);
}


/* FIND THE NEXT ELEMENT IN A SET.  Retuerns the next element of 'set'
   after 'val' (in the arbitrary ordering used), or SET_NO_VALUE if
   'val' was the last value. */

set_value_t set_next_element (struct set *set, set_value_t val)
{
  set_index_t index = SET_VAL_INDEX(val);
  set_offset_t offset = SET_VAL_OFFSET(val);
  struct set_node *ptr = SET_NODE_PTR(set->chain,index);
  set_bits_t b = (ptr->bits >> offset) >> 1;

  while (b == 0)
  { index = ptr->next;
    if (index == set->first) return SET_NO_VALUE;
    ptr = SET_NODE_PTR(set->chain,index);
    b = ptr->bits;
    offset = -1;
  }

  for (;;)
  { offset += 1;
    if (b & 1) break;
    b >>= 1;
  } 

  return SET_VAL(index,offset);
}

