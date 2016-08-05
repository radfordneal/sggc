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
#define SGGC_DEBUG 0
#endif


/* NUMBERS OF CHUNKS ALLOWED FOR AN OBJECT IN KINDS OF SEGMENTS.  Zero
   means that this kind of segment is big, containing one object of
   size found using sggc_chunks.  The application must define the
   initialization as SGGC_KIND_CHUNKS.  Entries Must not be greater
   than SGGC_CHUNKS_IN_SMALL_SEGMENT. */

static int kind_chunks[SGGC_N_KINDS] = SGGC_KIND_CHUNKS;


/* BIT VECTORS FOR FULL SEGMENTS.  Computed at initialization from kind_chunks
   and SET_OFFSET_BITS. */

static set_bits_t kind_full[SGGC_N_KINDS];


/* SIZES AND READ-ONLY FLAGS FOR AUXILIARY INFORMATION. */

#if SGGC_AUX_ITEMS >= 1
static int aux_sz1[SGGC_N_KINDS] = SGGC_AUX1_SZ;
static int aux_ro1[SGGC_N_KINDS]
#ifdef SGGC_AUX1_RO
 = SGGC_AUX1_RO
#endif
;
#endif

#if SGGC_AUX_ITEMS >= 2
static int aux_sz2[SGGC_N_KINDS] = SGGC_AUX2_SZ;
static int aux_ro2[SGGC_N_KINDS]
#ifdef SGGC_AUX2_RO
 = SGGC_AUX2_RO
#endif
;
#endif


/* SETS OF OBJECTS. */

static struct set unused;                     /* Has space, but not in use */
static struct set free_or_new[SGGC_N_KINDS];  /* Free or newly allocated */
static struct set old_gen1;                   /* Survived collection once */
static struct set old_gen2;                   /* Survived collection >1 time */
static struct set old_to_new;                 /* May have old->new references */
static struct set to_look_at;                 /* Not yet looked at in sweep */


/* NEXT FREE OBJECT AND END OF FREE OBJECTS FOR EACH KIND. */

static sggc_cptr_t next_free[SGGC_N_KINDS];
static sggc_cptr_t end_free[SGGC_N_KINDS];


/* MAXIMUM NUMBER OF SEGMENTS, AND INDEX OF NEXT SEGMENT TO USE. */

static set_offset_t max_segments;             /* Max segments, fixed for now */
static set_offset_t next_segment;             /* Number of segments in use */


/* GLOBAL VARIABLES USED FOR LOOKING AT OLD-NEW REFERENCES. */

static int collect_level;     /* Level of current garbage collection */
static int old_to_new_check;  /* 1 if should look for old-to-new reference */


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
      for (j = 0;
           j + kind_chunks[k] <= SGGC_CHUNKS_IN_SMALL_SEGMENT; 
           j += kind_chunks[k])
      { kind_full[k] |= (set_bits_t)1 << j;
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
    end_free[k] = SGGC_NO_OBJECT;
  }

  /* Record maximum segments, and initialize to no segments in use. */

  max_segments = n_segments;
  next_segment = 0;

  return 0;
}


/* ALLOCATE AN OBJECT OF SPECIFIED TYPE AND LENGTH.  The length is
   (possibly) used in determining the kind for the object, as
   determined by the application's sggc_kind function, and the number
   of chunks of storage it requires, as determined by the
   application's sggc_nchunks function.  

   The value returned is SGGC_NO_OBJECT if allocation fails, but note
   that it might succeed if retried after garbage collection is done
   (but this is left to the application to do if it wants to). */

sggc_cptr_t sggc_alloc (sggc_type_t type, sggc_length_t length)
{
  sggc_kind_t kind = sggc_kind(type,length);
  sggc_cptr_t v;

  if (SGGC_DEBUG) 
  { printf("sggc_alloc: type %u, length %u\n",(unsigned)type,(unsigned)length);
  }

  /* Find a segment for this object to go in (and offset within it). The 
     object will be in free_or_new, but not between next_free and end_free
     (for small segments), and so won't be taken again (at least before the
     next collection).  If the segment is new, it's data will be NULL. */

  if (kind_chunks[kind] == 0) /* uses big segments */
  { v = set_first (&unused, 0);
    if (v != SGGC_NO_OBJECT)
    { if (SGGC_DEBUG) printf("sggc_alloc: found %x in unused\n",(unsigned)v);
      set_move_first (&unused, &free_or_new[kind]);
    }
  }
  else /* uses small segments */
  { v = next_free[kind]; 
    if (v != end_free[kind])
    { next_free[kind] = set_next (&free_or_new[kind], v, 0);
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

    set_segment_init (SET_SEGMENT(next_segment));
    sggc_data[next_segment] = NULL;
    
    v = SET_VAL(next_segment,0);
    if (SGGC_DEBUG) 
    { printf("sggc_alloc: created %x in new segment\n", (unsigned)v);
    }

    next_segment += 1;
  }

  /* Set up type and data for a new segment. */

  if (sggc_data[SET_VAL_INDEX(v)] == NULL)
  {
    int index = SET_VAL_INDEX(v);
    struct set_segment *seg = SET_SEGMENT(index);
    char *data, *aux1, *aux2;

    sggc_type[index] = type;

    if (kind_chunks[kind] == 0) /* big segment */
    { 
      sggc_nchunks_t nch = sggc_nchunks (type, length);

      seg->x.big.big = 1;
      seg->x.big.max_chunks = (nch >> SGGC_CHUNK_BITS) == 0 ? nch : 0;

      data = malloc ((size_t) SGGC_CHUNK_SIZE * nch);
      if (SGGC_DEBUG) 
      { printf ("sggc_alloc: called malloc for %x (big %d, %d chunks): %p\n", 
                 v, kind, (int)nch, data);
      }
    }
    else /* small segment */
    { 
      seg->x.small.big = 0;
      seg->x.small.kind = kind;

      data = malloc ((size_t) SGGC_CHUNK_SIZE * SGGC_CHUNKS_IN_SMALL_SEGMENT);
      if (SGGC_DEBUG) 
      { printf ("sggc_alloc: called malloc for %x (small %d, %d chunks): %p\n", 
                 v, kind, (int)SGGC_CHUNKS_IN_SMALL_SEGMENT, data);
      }
    }

    sggc_data[index] = data;

    if (data == NULL) 
    { return SGGC_NO_OBJECT;
    }

#   if SGGC_AUX_ITEMS >= 1
      aux1 = sggc_alloc_segment_aux1 (kind);
      if (aux1 == NULL)
      { return SGGC_NO_OBJECT;
      }
      sggc_aux1[index] = aux1;
#   endif

#   if SGGC_AUX_ITEMS >= 2
      aux2 = sggc_alloc_segment_aux2 (kind);
      if (aux2 == NULL)
      { return SGGC_NO_OBJECT;
      }
      sggc_aux2[index] = aux2;
#   endif

    if (kind_chunks[kind] == 0) /* big segment */
    {
      set_add (&free_or_new[kind], v);
    }
    else /* small segment */
    { 
      int none_free = next_free[kind] == end_free[kind];

      if (none_free) 
      { end_free[kind] = set_first (&free_or_new[kind], 0);
      }
  
      set_add (&free_or_new[kind], v);
  
      set_assign_segment_bits (&free_or_new[kind], v, kind_full[kind]);
      if (SGGC_DEBUG)
      { printf("sggc_alloc: new segment has bits %016llx\n", 
                (unsigned long long) set_segment_bits (&free_or_new[kind], v));
      }

      if (none_free)
      { next_free[kind] = set_next (&free_or_new[kind], v, 0);
      }
    }
  }

  if (1) /* can be enabled for consistency checks */
  { if (set_contains (&old_gen1, v)) abort();
    if (set_contains (&old_gen2, v)) abort();
    if (set_contains (&old_to_new, v)) abort();
    if (set_contains (&to_look_at, v)) abort();
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

static void collect_debug (void)
{ int k;
  printf(
  "  unused: %d, old_gen1: %d, old_gen2: %d, old_to_new: %d, to_look_at: %d\n",
       set_n_elements(&unused), 
       set_n_elements(&old_gen1), 
       set_n_elements(&old_gen2), 
       set_n_elements(&old_to_new),
       set_n_elements(&to_look_at));
  printf("  free_or_new");
  for (k = 0; k < SGGC_N_KINDS; k++) 
  { printf(" [%d]: %3d ",k,set_n_elements(&free_or_new[k]));
  }
  printf("\n");
  printf("    next_free");
  for (k = 0; k < SGGC_N_KINDS; k++) 
  { if (next_free[k] == SGGC_NO_OBJECT)
    { printf(" [%d]: --- ",k);
    }
    else
    { printf(" [%d]: %3llx ",k,(unsigned long long)next_free[k]);
    }
  }
  printf("\n");
  printf("     end_free");
  for (k = 0; k < SGGC_N_KINDS; k++) 
  { if (end_free[k] == SGGC_NO_OBJECT)
    { printf(" [%d]: --- ",k);
    }
    else
    { printf(" [%d]: %3llx ",k,(unsigned long long)end_free[k]);
    }
  }
  printf("\n");
}

void sggc_collect (int level)
{ 
  sggc_cptr_t v;
  int k;

  if (SGGC_DEBUG) printf("sggc_collect: level %d\n",level);
  if (SGGC_DEBUG) collect_debug();

  if (set_first(&to_look_at, 0) != SET_NO_VALUE) abort();

  /* Put objects in the old generations being collected in the free_or_new set.

     This could be greatly sped up with some special facility in the set
     module that would do it a segment at a time. */

  if (level == 2)
  { for (v = set_first(&old_gen2, 0); 
         v != SET_NO_VALUE; 
         v = set_next(&old_gen2,v,0))
    { set_add (&free_or_new[SGGC_KIND(v)], v);
      if (SGGC_DEBUG) 
      { printf("sggc_collect: put %x from old_gen2 in free\n",(unsigned)v);
      }
    }
  }

  if (level >= 1)
  { for (v = set_first(&old_gen1, 0);
         v != SET_NO_VALUE; 
         v = set_next(&old_gen1,v,0))
    { set_add (&free_or_new[SGGC_KIND(v)], v);
      if (SGGC_DEBUG) 
      { printf("sggc_collect: put %x from old_gen1 in free\n",(unsigned)v);
      }
    }
  }

  /* Handle old-to-new references. */

  collect_level = level;
  v = set_first(&old_to_new, 0);

  while (v != SET_NO_VALUE)
  { int remove;
    if (SGGC_DEBUG) 
    { printf ("sggc_collect: old->new for %x (gen%d)\n", (unsigned)v,
        set_contains(&old_gen2,v) ? 2 : set_contains(&old_gen1,v) ? 1 : 0);
    }
    if (set_contains (&old_gen2, v)) /* v is in old generation 2 */
    { old_to_new_check = 2;
      sggc_find_object_ptrs (v);
      remove = old_to_new_check;
    }
    else /* v is in old generation 1 */
    { if (level == 0)
      { old_to_new_check = 0;
        sggc_find_object_ptrs (v);
        remove = 1;
      }
      else
      { old_to_new_check = 1;
        sggc_find_object_ptrs (v);
        remove = old_to_new_check;
      }
    }
    if (SGGC_DEBUG) 
    { if (remove) 
      { printf("sggc_collect: old->new for %x no longer needed\n",(unsigned)v);
      }
      else 
      { printf("sggc_collect: old->new for %x still needed\n",(unsigned)v);
      }
    }
    v = set_next (&old_to_new, v, remove);
  }

  old_to_new_check = 0;

  /* Get the application to take root pointers out of the free_or_new set,
     and put them in the to_look_at set. */

  sggc_find_root_ptrs();

  /* Keep looking at objects in the to_look_at set, putting them in
     the correct old generation, and getting the application to find
     any pointers they contain (which may add to the to_look_at set),
     until there are no more in the set. */

# ifdef SGGC_AFTER_COLLECT
  int phase = 1;
# endif

  do
  { 
    while ((v = set_first (&to_look_at, 1)) != SGGC_NO_OBJECT)
    {
      if (SGGC_DEBUG) printf("sggc_collect: looking at %x\n",(unsigned)v);
  
      if (level > 0 && set_remove (&old_gen1, v))
      { set_add (&old_gen2, v);
        if (SGGC_DEBUG) printf("sggc_collect: %x now old_gen2\n",(unsigned)v);
      }
      else if (level < 2 || !set_contains (&old_gen2, v))
      { set_add (&old_gen1, v); 
        if (SGGC_DEBUG) printf("sggc_collect: %x now old_gen1\n",(unsigned)v);
      }
  
      sggc_find_object_ptrs (v);
    }

#   ifdef SGGC_AFTER_COLLECT
    sggc_after_collect (phase++);
#   endif

  } while (set_first (&to_look_at, 0) != SGGC_NO_OBJECT);

  /* Remove objects that are still in the free_or_new set from the old 
     generations that were collected.

     This could be greatly sped up with some special facility in the set
     module that would do it a segment at a time. */

  if (level == 2)
  { v = set_first (&old_gen2, 0); 
    while (v != SET_NO_VALUE)
    { int remove = set_contains (&free_or_new[SGGC_KIND(v)], v);
      if (SGGC_DEBUG && remove) 
      { printf("sggc_collect: %x in old_gen2 now free\n",(unsigned)v);
      }
      v = set_next (&old_gen2, v, remove);
    }
  }

  if (level >= 1)
  { v = set_first (&old_gen1, 0); 
    while (v != SET_NO_VALUE)
    { int remove = set_contains (&free_or_new[SGGC_KIND(v)], v);
      if (SGGC_DEBUG && remove) 
      { printf("sggc_collect: %x in old_gen1 now free\n",(unsigned)v);
      }
      v = set_next (&old_gen1, v, remove);
    }
  }

  /* Move big segments to the 'unused' set, while freeing their storage. 
     Note that all big kinds are equal to their types, so we stop the
     loop at SGGC_N_TYPES. */

  for (k = 0; k < SGGC_N_TYPES; k++)
  { if (kind_chunks[k] == 0)
    { while ((v = set_first (&free_or_new[k], 0)) != SGGC_NO_OBJECT)
      { if (SGGC_DEBUG) 
        { printf ("sggc_collect: calling free for %x: %p\n", v, SGGC_DATA(v));
        }
        free (SGGC_DATA(v));
        sggc_data [SET_VAL_INDEX(v)] = NULL;
        if (SGGC_DEBUG) 
        { printf("sggc_collect: putting %x in unused\n",(unsigned)v);
        }
        set_move_first (&free_or_new[k], &unused);
      }
    }
  }

  /* Set up new next_free and end_free pointers, to use all of free_or_new. */

  for (k = 0; k < SGGC_N_KINDS; k++)
  { if (kind_chunks[k] != 0)
    { next_free[k] = set_first (&free_or_new[k], 0);
      end_free[k] = SGGC_NO_OBJECT;
    }
  }

  if (SGGC_DEBUG) printf("sggc_collect: done\n");
  if (SGGC_DEBUG) collect_debug();
}


/* TELL THE GARBAGE COLLECTOR THAT AN OBJECT NEEDS TO BE LOOKED AT.
   Called by the application from within its sggc_find_root_ptrs or
   sggc_find_object_ptrs functions, to signal to the garbage collector
   that it has found a pointer to an object for the GC to look at.
   The caller should continue calling sggc_look_at for all root pointers,
   and for all pointers in an object, except that the rest of the 
   pointers in an object should be skipped if sggc_look_at returns 0.

   The principal use of this is to mark objects as in use.  If the
   object is presently in the free_or_new set, it is removed, and put
   in the set of objects to be looked at.  (If it is not in
   free_or_new, it has already been looked at, and so needn't be
   looked at again.)

   This procedure is also used as part of the old-to-new scheme to
   check whether an object in the old-to-new set still needs to be
   there, as well as sometimes marking the objects it points to. */

int sggc_look_at (sggc_cptr_t ptr)
{
  if (SGGC_DEBUG) 
  { printf ("sggc_look_at: %x %d\n", (unsigned)ptr ,old_to_new_check);
  }

  if (ptr != SGGC_NO_OBJECT)
  { if (old_to_new_check != 0)
    { if (collect_level == 0)
      { if (!set_contains (&old_gen2, ptr))
        { old_to_new_check = 0;
        }
      }
      else if (collect_level == 1 && old_to_new_check == 2)
      { if (set_chain_contains (SET_UNUSED_FREE_NEW, ptr))
        { old_to_new_check = 0;
        }
      }
      else
      { if (set_chain_contains (SET_UNUSED_FREE_NEW, ptr))
        { old_to_new_check = 0;
          return 0;
        }
        return 1;
      }
    }
    if (set_remove (&free_or_new[SGGC_KIND(ptr)], ptr))
    { set_add (&to_look_at, ptr);
      if (SGGC_DEBUG) printf("sggc_look_at: will look at %x\n",(unsigned)ptr);
    }
  }

  return 1;
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


/* TEST WHETHER AN OBJECT IS NOT (YET) MARKED AS IN USE. */

int sggc_not_marked (sggc_cptr_t ptr)
{
  return !set_chain_contains (SET_UNUSED_FREE_NEW, ptr);
}
