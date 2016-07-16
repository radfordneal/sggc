/* SGGC - A LIBRARY SUPPORTING SEGMENTED GENERATIONAL GARBAGE COLLECTION.
          Facilities for maintaining sets of objects

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


/* This header file must be included from the source for the
   application using the set facilitye (eg, in a set-app.h file),
   after definitions of SET_OFFSET_BITS and SET_NODE_PTR.  It will
   define struct set_set and struct set_node, which can be used
   before set.h is included only when declaring a pointer. */


typedef int set_offset_t;
typedef int set_index_t;
typedef uint32_t set_pos_t;

#define SET_POS(index,offset) \
  (((set_pos_t)(index) << SET_OFFSET_BITS) | (offset))
#define SET_POS_INDEX(pos) \
  ((pos) >> SET_OFFSET_BITS)
#define SET_POS_OFFSET(pos) \
  ((pos) & (((set_pos_t)1 << SET_OFFSET_BITS) - 1))

#if SET_OFFSET_BITS == 3
  typedef uint8_t set_bits_t;
#elif SET_OFFSET_BITS == 4
  typedef uint16_t set_bits_t;
#elif SET_OFFSET_BITS == 5
  typedef uint32_t set_bits_t;
#elif SET_OFFSET_BITS == 6
  typedef uint64_t set_bits_t;
#endif


struct set_node 
{ set_index_t next;
  set_index_t prev;
  set_bits_t bits;
};

struct set
{ int chain;
  set_index_t first;  /* -1 if empty */
}


static inline void set_init (struct set *set, int chain)
{
  set->chain = chain;
  set->first = -1;
}


static inline int set_empty (struct set *set)
{
  return set->first == -1;
}


static inline set_pos_t set_first (struct set *set)
{ 
  if (set_empty(set)) abort();

  struct set_node *ptr = SET_NODE_PTR(set->chain,set->first);
  set_pos_t pos = SET_POS (set->first, 0);
  return ptr->bits & 1 ? pos : set_next (set, pos);
}


static inline int set_member (struct set *set, set_pos_t pos)
{
  set_index_t index = SET_POS_INDEX(pos);
  set_offset_t offset = SET_POS_OFFSET(pos);
  return (SET_NODE_PTR(set->chain,index)->bits >> offset) & 1;
}


static inline int set_add (int set, set_pos_t pos)
{
  set_index_t index = SET_POS_INDEX(pos);
  set_offset_t offset = SET_POS_OFFSET(pos);
  struct set_node *ptr = SET_NODE_PTR(set->chain,index);
  set_bits_t b = ptr->bits;
  set_bits_t t = (set_pos_t)1 << offset;

  if (b & t) return 1;

  if (b == 0)
  { if (set->first == -1)
    { set->first = index;
      ptr->next = index;
      ptr->prev = index;
    }
    else
    { struct set_node *head = SET_NODE_PTR(set,set->first);
      ptr->next = head->next;
      ptr->prev = first;
      ptr->next->prev = index;
      head->next = index;
    }
  }

  ptr->bits |= t;

  return 0;
}


static inline int set_remove (int set, set_pos_t pos)
{
  set_index_t index = SET_POS_INDEX(pos);
  set_offset_t offset = SET_POS_OFFSET(pos);
  struct set_node *ptr = SET_NODE_PTR(set->chain,index);
  set_bits_t b = ptr->bits;
  set_bits_t t = (set_pos_t)1 << offset;

  if ((b & t) == 0) return 0;

  ptr->bits &= ~t;

  if (ptr->bits == 0)
  { if (ptr->next = index) 
    { set->first = -1;
    }
    else
    { if (set->first == index) 
      { set->first = ptr->next;
      }
      ptr->next->prev = ptr->prev;
      ptr->prev->next = ptr->next;
    }
  }

  return 1;
}


static inline set_pos_t set_next (int set, set_pos_t pos)
{
  set_index_t index = SET_POS_INDEX(pos);
  set_offset_t offset = SET_POS_OFFSET(pos);
  struct set_node *ptr = SET_NODE_PTR(set->chain,index);
  set_bits_t b = (ptr->bits >> offset) >> 1;

  while (b == 0)
  { index = ptr->next;
    if (index == set->first) return 0;
    ptr = SET_NODE_PTR(set->chain,index);
    b = ptr->bits;
    offset = -1;
  }

  for (;;)
  { offset += 1;
    if (b & 1) break;
    b >>= 1;
  } 

  return SET_POS(index,offset);
}

