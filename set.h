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
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

/* Must be included from the application source (eg, in a set-app.h
  file) after definitions of SET_OFFSET_BITS and SET_NODE_PTR.  Will
  define struct set_node, which can be used before set.h is included
  only when declaring a pointer to a node. */

typedef int set_offset_t;
typedef uint32_t set_index_t;
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

#define SET_HEAD 0

struct set_node 
{ set_index_t next;
  set_index_t prev;
  set_bits_t bits;
};

static inline int set_member (int set, set_pos_t pos)
{
  set_index_t index = SET_POS_INDEX(pos);
  set_offset_t offset = SET_POS_OFFSET(pos);
  return (SET_NODE_PTR(set,index)->bits >> offset) & 1;
}

static inline int set_add (int set, set_pos_t pos)
{
  set_index_t index = SET_POS_INDEX(pos);
  set_offset_t offset = SET_POS_OFFSET(pos);
  struct set_node *ptr = SET_NODE_PTR(set,index);
  set_bits_t b = ptr->bits;
  set_bits_t t = (set_pos_t)1 << offset;

  if (b & t) return 1;

  if (b == 0)
  { struct set_node *head = SET_NODE_PTR(set,SET_HEAD);
    ptr->next = head->next;
    ptr->prev = head;
    ptr->next->prev = ptr;
    head->next = ptr;
  }

  ptr->bits |= t;

  return 0;
}

static inline int set_remove (int set, set_pos_t pos)
{
  set_index_t index = SET_POS_INDEX(pos);
  set_offset_t offset = SET_POS_OFFSET(pos);
  struct set_node *ptr = SET_NODE_PTR(set,index);
  set_bits_t b = ptr->bits;
  set_bits_t t = (set_pos_t)1 << offset;

  if ((b & t) == 0) return 0;

  ptr->bits &= ~t;

  if (ptr->bits == 0)
  { ptr->next->prev = ptr->prev;
    ptr->prev->next = ptr->next;
  }

  return 1;
}


static inline set_pos_t set_next (int set, set_pos_t pos)
{
  set_index_t index = SET_POS_INDEX(pos);
  set_offset_t offset = SET_POS_OFFSET(pos);
  struct set_node *ptr = SET_NODE_PTR(set,index);
  set_bits_t b = ptr->bits >> offset;

  if (b <= 1)
  { index = ptr->next;
    if (index == 0) return 0;
    offset = -1;
  }

  do
  { offset += 1;
    b >>= 1;
  } while ((b & 1) == 0);

  return SET_POS(index,offset);
}

