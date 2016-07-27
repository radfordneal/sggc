/* SGGC - A LIBRARY SUPPORTING SEGMENTED GENERATIONAL GARBAGE COLLECTION.
          Garbage collection routines.

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
#include "sggc-app.h"


/* NUMBERS OF CHUNKS ALLOWED FOR AN OBJECT IN KINDS OF SEGMENTS.  Zero
   means that this kind of segment is big, containing one object of
   size found using sggc_chunks.  The application must define the
   initialization as SGGC_KIND_CHUNKS. */

static int kind_chunks[SGGC_N_KINDS] = SGGC_KIND_CHUNKS;


/* SETS OF OBJECTS. */

static struct set free_or_new[SGGC_N_KINDS];  /* Free or newly allocated */
static struct set old_gen1;                   /* Survived collection once */
static struct set old_gen2;                   /* Survived collection >1 time */
static struct set old_to_new;                 /* May have old->new references */
static struct set to_look_at;                 /* Not yet looked at in sweep */


/* MAXIMUM NUMBER OF SEGMENTS, AND INDEX OF NEXT SEGMENT TO USE. */

static set_offset_t max_segments;
static set_offset_t next_segment;


/* FLAGS FOR CHECKING FOR OLD-NEW REFERENCES. */

static int has_gen0;   /* Set if has gen2->gen0 references */
static int has_gen1;   /* Set if has gen2->gen1 referendes */


/* INITIALIZE SEGMENTED MEMORY.  Allocates space for pointers for the
   specified number of segments (currently not expandable).  Returns
   zero if successful, non-zero if allocation fails. */

int sggc_init (int n_segments)
{
  int i;

  /* Allocate space for pointers to segment descriptors, data, and
     possibly auxiliary information for segments.  The information for
     a segment these point to is allocated later, when the segment is
     actually needed. */

  sggc_segment = malloc (n_segments * sizeof *sggc_segment);
  if (sggc_segment == NULL)
  { return 1;
  }

  sggc_data = malloc (n_segments * sizeof *sggc_data);
  if (sggc_data == NULL)
  { return 1;
  }

# if SGGC_AUX_ITEMS > 0
  { for (i = 0; i<SGGC_AUX_ITEMS; i++)
    { sggc_aux[i] = malloc (n_segments * sizeof *sggc_aux[i]);
      if (sggc_aux[i] == NULL)
      { return 1;
      }
    }
  }
# endif

  /* Allocate space for types of segments. */

  sggc_type = malloc (n_segments * sizeof *sggc_type);
  if (sggc_type == NULL)
  { return 1;
  }

  /* Initialize sets of objects, as empty. */

  for (i = 0; i<SGGC_N_KINDS; i++) 
  { set_init(&free_or_new[i],SET_FREE_OR_NEW);
  }
  set_init(&old_gen1,SET_OLD_GEN1);
  set_init(&old_gen2,SET_OLD_GEN2);
  set_init(&old_to_new,SET_OLD_TO_NEW);
  set_init(&to_look_at,SET_TO_LOOK_AT);

  /* Record maximum, and initialize to no segments in use. */

  max_segments = n_segments;
  next_segment = 0;

  return 0;
}


/* ALLOCATE AN OBJECT OF SPECIFIED TYPE AND LENGTH.  The length is
   (possibly) used in determining the kind for the object, as
   determined by the sggc_kind function provided by the application.
   The value returned is SGGC_NO_OBJECT if allocation fails, even
   after a full garbage collection is done. */

sggc_cptr_t sggc_alloc (sggc_type_t type, sggc_length_t length)
{
  int collected_already = 0;
  unsigned kind = sggc_kind(type,length);
  sggc_cptr_t v;

  for (;;)
  {
    v = set_first (&free_or_new[kind]);

    if (v == SGGC_NO_OBJECT)
    { if (next_segment == max_segments)
      { if (collected_already) 
        { return SGGC_NO_OBJECT;
        }
        sggc_collect(2);
        collected_already = 1;
        continue;
      }
      v = SET_VAL(next_segment,0);
      if (kind_chunks[kind] == 0)
      { SET_SEGMENT(v) -> x.big.big = 1;
        SET_SEGMENT(v) -> x.big.max_chunks = 0;
      }
      else
      { SET_SEGMENT(v) -> x.small.big = 0;
        SET_SEGMENT(v) -> x.small.kind = kind;
        SET_SEGMENT(v) -> x.small.nchunks = sggc_nchunks (type, length);
      }
      set_add (&free_or_new[kind], v);
      next_segment += 1;
    }
  }

  for (;;)
  {
    if (kind_chunks[kind] == 0)
    { char *data = sggc_alloc_big_segment_data (kind, length);
      if (data == NULL)
      { if (collected_already) 
        { return SGGC_NO_OBJECT;
        }
        sggc_collect(2);
        collected_already = 1;
        continue;
      }
      sggc_data [SET_VAL_INDEX(v)] = data;
      return v;
    }
    else
    { /* not done yet */
      return SGGC_NO_OBJECT;
    }
  }
}


/* DO A GARBAGE COLLECTION AT THE SPECIFIED LEVEL.  Levels are 0 for
   collecting only newly allocated objects, 1 for also collecting
   objects that have survived one collection, and 2 for collecting all
   objects.

   This procedure is called automatically when sggc_alloc would
   otherwise fail, but should also be called by the application
   according to its heuristics for when this would be desirable. */

void sggc_collect (int level)
{ 
  sggc_cptr_t v;

  if (set_first(&to_look_at) != SET_NO_VALUE) abort();

  /* Put objects in the old generations being collected in the free_or_new set.

     This could be greatly sped up with some special facility in the set
     module that would do it a segment at a time. */

  if (level == 2)
  { for (v = set_first(&old_gen2); v!=SET_NO_VALUE; v = set_next(&old_gen2,v,0))
    { set_add (&free_or_new[SGGC_KIND(v)], v);
    }
  }

  if (level >= 1)
  { for (v = set_first(&old_gen1); v!=SET_NO_VALUE; v = set_next(&old_gen1,v,0))
    { set_add (&free_or_new[SGGC_KIND(v)], v);
    }
  }

  /* Handle old-to-new references. */

  v = set_first(&old_to_new);
  while (v != SET_NO_VALUE)
  { int remove;
    if (set_contains (&old_gen1, v))
    { has_gen0 = has_gen1 = 1;  /* we don't care */
      sggc_find_object_ptrs (v);
      remove = 1;
    }
    else if (level == 0)
    { has_gen0 = has_gen1 = 0;  /* look for both generation 0 and generaton 1 */
      sggc_find_object_ptrs (v);
      remove = has_gen0 | has_gen1;
    }
    else /* level > 0 */
    { has_gen0 = 0; has_gen1 = 1;  /* look only for generation 0 */
      sggc_find_object_ptrs (v);
      remove = has_gen0;
    }
    v = set_next (&old_to_new, v, remove);
  }

  /* Get the application to take root pointers out of the free_or_new set,
     and put them in the to_look_at set. */

  sggc_find_root_ptrs();

  /* Keep looking at objects in the to_look_at set, putting them in
     the correct old generation, and getting the application to find
     any pointers they contain (which may add to the to_look_at set),
     until there are no more in the set. */

  for (;;)
  { 
    v = set_first (&to_look_at);
    if (v == SGGC_NO_OBJECT)
    { break;
    }

    (void) set_remove (&to_look_at, v);

    if (level > 0 && set_remove (&old_gen1, v))
    { set_add (&old_gen2, v);
    }
    else if (level < 2 || !set_contains (&old_gen2, v))
    { set_add (&old_gen1, v); 
    }

    sggc_find_object_ptrs (v);
  }
}


/* TELL THE GARBAGE COLLECTOR THAT AN OBJECT NEEDS TO BE LOOKED AT.
   Called by the application from within its sggc_find_root_ptrs or
   sggc_find_object_ptrs functions, to signal to the garbage collector
   that it has found a pointer to an object that must also be looked at,
   and also that it is not free. */

void sggc_look_at (sggc_cptr_t ptr)
{
  if (ptr != SGGC_NO_OBJECT)
  { if ((has_gen0 & has_gen1) == 0)
    { if (set_contains (&old_gen1, ptr))
      { has_gen1 = 1;
      }
      else if (has_gen0 == 0 && !set_contains (&old_gen2, ptr))
      { has_gen0 = 1;
      }
    }
    if (set_remove(&free_or_new[SGGC_KIND(ptr)],ptr))
    { set_add (&to_look_at, ptr);
    }
  }
}


/* RECORD AN OLD-TO-NEW REFERENCE.  Must be called by the application
   when about to store a reference to to_ptr in the object from_ptr,
   unless from_ptr has just been allocated (with no allocations or
   explicit garbage collections since then), or unless from_ptr has
   been verified to be in the youngest generation by using
   sggc_youngest_generation (with no allocations or explicit garbage
   collections have been done since then). */

void sggc_old_to_new_check (sggc_cptr_t from_ptr, sggc_cptr_t to_ptr)
{
  if (set_contains (&old_to_new, from_ptr))
  { return;
  }

  if (set_contains (&old_gen2, from_ptr))
  { if (set_contains (&old_gen2, to_ptr))
    { return;
    }
  }
  else if (set_contains (&old_gen1, from_ptr))
  { if (set_contains (&old_gen1, to_ptr) || set_contains (&old_gen2, to_ptr))
    { return;
    }
  }
  else
  { return;
  }

  set_add (&old_to_new, from_ptr);
}


/* CHECK WHETHER AN OBJECT IS IN THE YOUNGEST GENERATION.  If it is,
   the application can skip the old-to-new check.  Note that an object
   may cease to be in the youngest generation when an allocation or
   explicit garbage collection is done. */

int sggc_youngest_generation (sggc_cptr_t from_ptr)
{
  return set_contains (&old_to_new, from_ptr);
}
