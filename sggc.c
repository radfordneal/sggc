/* SGGC - A LIBRARY SUPPORTING SEGMENTED GENERATIONAL GARBAGE COLLECTION.
          Segmented garbage collection - function definitions

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


#include <stdio.h>
#include <stdlib.h>
#include "sggc-app.h"


/* DEBUGGING FLAG.  Set to 1 to enable debug output.  May be set by a compiler
   flag, in which case it isn't overridden here. */

#ifndef SGGC_DEBUG
#define SGGC_DEBUG 1
#endif


/* NUMBERS OF CHUNKS ALLOWED FOR AN OBJECT IN KINDS OF SEGMENTS.  Zero
   means that this kind of segment is big, containing one object of
   size found using sggc_chunks.  The application must define the
   initialization as SGGC_KIND_CHUNKS.  Must not be greater than
   SGGC_CHUNKS_IN_SMALL_SEGMENT. */

static int kind_chunks[SGGC_N_KINDS] = SGGC_KIND_CHUNKS;


/* BIT VECTORS FOR FULL SEGMENTS.  Computed at initialization from kind_chunks
   and SET_OFFSET_BITS. */

static set_bits_t kind_full[SGGC_N_KINDS];


/* SETS OF OBJECTS. */

static struct set unused;                     /* Has space, but not in use */
static struct set free_or_new[SGGC_N_KINDS];  /* Free or newly allocated */
static struct set old_gen1;                   /* Survived collection once */
static struct set old_gen2;                   /* Survived collection >1 time */
static struct set old_to_new;                 /* May have old->new references */
static struct set to_look_at;                 /* Not yet looked at in sweep */


/* NEXT FREE OBJECT OF EACH KIND. */

static sggc_cptr_t next_free[SGGC_N_KINDS];


/* MAXIMUM NUMBER OF SEGMENTS, AND INDEX OF NEXT SEGMENT TO USE. */

static set_offset_t max_segments;             /* Max segments, fixed for now */
static set_offset_t next_segment;             /* Number of segments in use */


/* FLAGS FOR CHECKING FOR OLD-NEW REFERENCES. */

static int has_gen0;                          /* Has gen2->gen0 references? */
static int has_gen1;                          /* Has gen2->gen1 references? */


/* INITIALIZE SEGMENTED MEMORY.  Allocates space for pointers for the
   specified number of segments (currently not expandable).  Returns
   zero if successful, non-zero if allocation fails. */

int sggc_init (int n_segments)
{
  int i, j, k;

  /* Allocate space for pointers to segment descriptors, data, and
     possibly auxiliary information for segments.  Information for
     segments these point to is allocated later, when the segment is
     actually needed. */

  sggc_segment = malloc (n_segments * sizeof *sggc_segment);
  if (sggc_segment == NULL)
  { return 1;
  }

  sggc_data = malloc (n_segments * sizeof *sggc_data);
  if (sggc_data == NULL)
  { return 2;
  }

# if SGGC_AUX_ITEMS > 0
  { for (i = 0; i < SGGC_AUX_ITEMS; i++)
    { sggc_aux[i] = malloc (n_segments * sizeof *sggc_aux[i]);
      if (sggc_aux[i] == NULL)
      { return 3;
      }
    }
  }
# endif

  /* Allocate space for types of segments. */

  sggc_type = malloc (n_segments * sizeof *sggc_type);
  if (sggc_type == NULL)
  { return 4;
  }

  /* Initialize bit vectors that indicate when segments of different kinds
     are full. */

  for (k = 0; k < SGGC_N_KINDS; k++)
  { if (kind_chunks[k] == 0) /* big segment */
    { kind_full[k] = 1;
    }
    else /* small segment */
    { if (kind_chunks[k] > SGGC_CHUNKS_IN_SMALL_SEGMENT) abort();
      kind_full[k] = 0;
      for (j = 0; j < SGGC_CHUNKS_IN_SMALL_SEGMENT; j += kind_chunks[i])
      { kind_full[i] |= (set_bits_t)1 << j;
      }
    }
  }

  /* Initialize sets of objects, as empty. */

  set_init(&unused,SET_UNUSED_FREE_NEW);
  for (k = 0; k < SGGC_N_KINDS; k++) 
  { set_init(&free_or_new[k],SET_UNUSED_FREE_NEW);
  }
  set_init(&old_gen1,SET_OLD_GEN1);
  set_init(&old_gen2,SET_OLD_GEN2);
  set_init(&old_to_new,SET_OLD_TO_NEW);
  set_init(&to_look_at,SET_TO_LOOK_AT);

  /* Initialize to no free objects of each kind. */

  for (k = 0; k < SGGC_N_KINDS; k++)
  { next_free[k] = SGGC_NO_OBJECT;
  }

  /* Record maximum segments, and initialize to no segments in use. */

  max_segments = n_segments;
  next_segment = 0;

  return 0;
}


/* ALLOCATE AN OBJECT OF SPECIFIED TYPE AND LENGTH.  The length is
   (possibly) used in determining the kind for the object, as
   determined by the application's sggc_kind function, and is also
   needed if the application's sggc_alloc_big_segment_data or
   sggc_alloc_big_segment_aux functions are used.  The value returned
   is SGGC_NO_OBJECT if allocation fails, but note that it might
   succeed if retried after garbage collection is done (but this is
   left to the application to do if it wants to). */

sggc_cptr_t sggc_alloc (sggc_type_t type, sggc_length_t length)
{
  sggc_kind_t kind = sggc_kind(type,length);
  sggc_cptr_t v;
  int i;

  if (SGGC_DEBUG) 
  { printf("sggc_alloc: type %u, length %u\n",(unsigned)type,(unsigned)length);
  }

  /* Find a segment for this object to go in (and offset within it). The 
     object will be in free_or_new, but before next_free (for small segment), 
     and so won't be  taken again (at least before next collection). */

  if (kind_chunks[kind] == 0) /* uses big segments */
  { v = set_first (&unused);
    if (v != SGGC_NO_OBJECT)
    { if (SGGC_DEBUG) printf("sggc_alloc: found %x in unused\n",(unsigned)v);
      sggc_type[SET_VAL_INDEX(v)] = type;
      set_move_first (&unused, &free_or_new[kind]);
    }
  }
  else /* uses small segments */
  { v = next_free[kind]; 
    if (v != SGGC_NO_OBJECT)
    { next_free[kind] = set_next (&free_or_new[kind], v, 1);
      if (SGGC_DEBUG) printf("sggc_alloc: found %x in next_free\n",(unsigned)v);
    }
  }

  if (v == SGGC_NO_OBJECT)
  { 
    if (next_segment == max_segments)
    { return SGGC_NO_OBJECT;
    }
    sggc_segment[next_segment] = malloc (sizeof **sggc_segment);
    if (sggc_segment[next_segment] == NULL)
    { return SGGC_NO_OBJECT;
    }
    v = SET_VAL(next_segment,0);
    if (SGGC_DEBUG) 
    { printf("sggc_alloc: created %x in new segment\n", (unsigned)v);
    }

    struct set_segment *seg = SET_SEGMENT (next_segment);
    set_segment_init (seg);
    sggc_type[next_segment] = type;
    next_segment += 1;

    if (kind_chunks[kind] == 0) /* big segment */
    { seg->x.big.big = 1;
      seg->x.big.max_chunks = 0;  /* for now */
    }
    else /* small segment */
    { char *data;
      data = malloc ((size_t) SGGC_CHUNK_SIZE * SGGC_CHUNKS_IN_SMALL_SEGMENT);
      if (data == NULL) 
      { return SGGC_NO_OBJECT;
      }
      seg->x.small.big = 0;
      seg->x.small.kind = kind;
    }

    set_add (&free_or_new[kind], v);
    /* NEED MORE HERE */
  }

  /* Allocate space for data for a big segment. */

  if (kind_chunks[kind] == 0)
  { sggc_nchunks_t nch = sggc_nchunks (type, length);
    char *data = malloc ((size_t)nch * SGGC_CHUNK_SIZE);
    if (SGGC_DEBUG) printf ("called malloc for %x (%d chunks): %p\n", 
                             v, (int)nch, data);
    if (data == NULL)
    { return SGGC_NO_OBJECT;
    }
    sggc_data [SET_VAL_INDEX(v)] = data;
  }

  return v;
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
  int k;

  if (SGGC_DEBUG) printf("Collecting at level %d\n",level);

  if (set_first(&to_look_at) != SET_NO_VALUE) abort();

  /* Put objects in the old generations being collected in the free_or_new set.

     This could be greatly sped up with some special facility in the set
     module that would do it a segment at a time. */

  if (level == 2)
  { for (v = set_first(&old_gen2); v!=SET_NO_VALUE; v = set_next(&old_gen2,v,0))
    { set_add (&free_or_new[SGGC_KIND(v)], v);
      if (SGGC_DEBUG) printf("Put %x from old_gen2 in free\n",(unsigned)v);
    }
  }

  if (level >= 1)
  { for (v = set_first(&old_gen1); v!=SET_NO_VALUE; v = set_next(&old_gen1,v,0))
    { set_add (&free_or_new[SGGC_KIND(v)], v);
      if (SGGC_DEBUG) printf("Put %x from old_gen1 in free\n",(unsigned)v);
    }
  }

  /* Handle old-to-new references. */

  v = set_first(&old_to_new);
  while (v != SET_NO_VALUE)
  { int remove;
    if (SGGC_DEBUG) printf("Followed old->new in %x\n",(unsigned)v);
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

  has_gen0 = has_gen1 = 1;  /* we don't care */

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
    if (SGGC_DEBUG) printf("Looking at %x\n",(unsigned)v);

    if (level > 0 && set_remove (&old_gen1, v))
    { set_add (&old_gen2, v);
      if (SGGC_DEBUG) printf("Now %x is in old_gen2\n",(unsigned)v);
    }
    else if (level < 2 || !set_contains (&old_gen2, v))
    { set_add (&old_gen1, v); 
      if (SGGC_DEBUG) printf("Now %x is in old_gen1\n",(unsigned)v);
    }
    else
    { if (SGGC_DEBUG) printf("No generation change for %x (%d %d)\n",
        (unsigned)v, set_contains(&old_gen1,v), set_contains(&old_gen2,v));
    }

    sggc_find_object_ptrs (v);
  }

  /* Move big segments to the 'unused' set, while freeing their storage. */

  for (k = 0; k < SGGC_N_TYPES; k++)
  { if (kind_chunks[k] == 0)
    { for (;;)
      { v = set_first (&free_or_new[k]);
        if (v == SGGC_NO_OBJECT) 
        { break;
        }
        if (SGGC_DEBUG) printf("Putting %x in unused\n",(unsigned)v);
        (void) set_remove (&free_or_new[k], v);
        (void) set_add (&unused, v);
        if (SGGC_DEBUG) printf ("calling free for %x: %p\n", v, SGGC_DATA(v));
        free (SGGC_DATA(v));
        sggc_data [SET_VAL_INDEX(v)] = NULL;
      }
    }
  }
}


/* TELL THE GARBAGE COLLECTOR THAT AN OBJECT NEEDS TO BE LOOKED AT.
   Called by the application from within its sggc_find_root_ptrs or
   sggc_find_object_ptrs functions, to signal to the garbage collector
   that it has found a pointer to an object that must also be looked
   at, and also that it is not free.  If the object is presently in
   the free_or_new set, it will be removed, and put in the set of
   objects to be looked at.  (If it is not in free_or_new, it has
   already been looked at, and so needn't be looked at again.)

   The global has_gen0 or has_gen1 flag is set to 1 if ptr is in the
   corresponding generation. */

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
      if (SGGC_DEBUG) printf("Will look at %x\n",(unsigned)ptr);
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
