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
   initialization as SGGC_KIND_CHUNKS.  Entries must not be greater
   than SGGC_CHUNKS_IN_SMALL_SEGMENT. */

static int kind_chunks[SGGC_N_KINDS] = SGGC_KIND_CHUNKS;


/* READ-ONLY AUXILIARY INFORMATION.  Filled in at initialization by calling
   the application's sggc_aux1_read_only and sggc_aux2_read_only functions. */

#ifdef SGGC_AUX1_READ_ONLY
static char *kind_aux1_read_only[SGGC_N_KINDS];
#endif

#ifdef SGGC_AUX2_READ_ONLY
static char *kind_aux2_read_only[SGGC_N_KINDS];
#endif


/* BIT VECTORS FOR FULL SEGMENTS.  Computed at initialization from kind_chunks
   and SET_OFFSET_BITS. */

static set_bits_t kind_full[SGGC_N_KINDS];


/* SETS OF OBJECTS. */

static struct set unused;                     /* Segment exists, but not used */
static struct set free_or_new[SGGC_N_KINDS];  /* Free or newly allocated */
static struct set old_gen1;                   /* Survived collection once */
static struct set old_gen2;                   /* Survived collection >1 time */
static struct set old_to_new;                 /* May have old->new references */
static struct set to_look_at;                 /* Not yet looked at in sweep */


/* NEXT FREE OBJECT AND END OF FREE OBJECTS FOR EACH KIND.  The next_free[k]
   pointer is to a free object, unless it is equal to end_free[k].  These
   may be SGGC_NO_OBJECT. */

static sggc_cptr_t next_free[SGGC_N_KINDS];   /* Next free object of kind */
static sggc_cptr_t end_free[SGGC_N_KINDS];    /* Indicator when past free obj */


/* MAXIMUM NUMBER OF SEGMENTS, AND INDEX OF NEXT SEGMENT TO USE. */

static set_offset_t maximum_segments;         /* Max segments, fixed for now */
static set_offset_t next_segment;             /* Number of segments in use */


/* GLOBAL VARIABLES USED FOR LOOKING AT OLD-NEW REFERENCES. */

static int collect_level;     /* Level of current garbage collection */
static int old_to_new_check;  /* 1 if should look for old-to-new reference */


/* INITIALIZE SEGMENTED MEMORY.  Allocates space for pointers for the
   specified number of segments (currently not expandable).  Returns
   zero if successful, non-zero if allocation fails. */

int sggc_init (int max_segments)
{
  int i, j, k;

  /* Allocate space for pointers to segment descriptors, data, and
     possibly auxiliary information for segments.  Information for
     segments these point to is allocated later, when the segment is
     actually needed. */

  sggc_segment = malloc (max_segments * sizeof *sggc_segment);
  if (sggc_segment == NULL)
  { return 1;
  }

  sggc_data = malloc (max_segments * sizeof *sggc_data);
  if (sggc_data == NULL)
  { return 2;
  }

# ifdef SGGC_AUX1_SIZE
    sggc_aux1 = malloc (max_segments * sizeof *sggc_aux1);
    if (sggc_aux1 == NULL)
    { return 3;
    }
# endif

# ifdef SGGC_AUX2_SIZE
    sggc_aux2 = malloc (max_segments * sizeof *sggc_aux2);
    if (sggc_aux2 == NULL)
    { return 4;
    }
# endif

  /* Allocate space for holding the types of segments. */

  sggc_type = malloc (max_segments * sizeof *sggc_type);
  if (sggc_type == NULL)
  { return 5;
  }

  /* Initialize bit vectors that indicate when segments of different
     kinds are full, and are also used to initialize segments as full.
     Along the way, check that all big kinds correspond to types. */

  for (k = 0; k < SGGC_N_KINDS; k++)
  { if (kind_chunks[k] == 0) /* big segment */
    { if (k >= SGGC_N_TYPES) abort();
      kind_full[k] = 1;
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

  /* Initialize tables of read-only auxiliary information. */

# ifdef SGGC_AUX1_READ_ONLY
    for (k = 0; k < SGGC_N_KINDS; k++)
    { kind_aux1_read_only[k] = sggc_aux1_read_only(k);
    }
# endif

# ifdef SGGC_AUX2_READ_ONLY
    for (k = 0; k < SGGC_N_KINDS; k++)
    { kind_aux2_read_only[k] = sggc_aux2_read_only(k);
    }
# endif

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

  maximum_segments = max_segments;
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
  if (SGGC_DEBUG) 
  { printf("sggc_alloc: type %u, length %u\n",(unsigned)type,(unsigned)length);
  }

  sggc_kind_t kind = sggc_kind(type,length);
  sggc_index_t index;
  sggc_cptr_t v;
  int from_free;

  /* Look for an existing segment for this object to go in (and offset
     within).  For a small segment, the object found will be in
     free_or_new, but will not be between next_free and end_free, and
     so won't be taken again (at least before the next collection). */

  from_free = 0;
  if (kind_chunks[kind] == 0) /* uses big segments */
  { v = set_first (&unused, 1);
    if (v != SGGC_NO_OBJECT)
    { if (SGGC_DEBUG) printf("sggc_alloc: found %x in unused\n",(unsigned)v);
      set_add (&free_or_new[kind], v);
    }
  }
  else /* uses small segments */
  { v = next_free[kind]; 
    if (v == end_free[kind])
    { v = SGGC_NO_OBJECT;
    }
    else
    { next_free[kind] = set_next (&free_or_new[kind], v, 0);
      if (SGGC_DEBUG) printf("sggc_alloc: found %x in next_free\n",(unsigned)v);
      from_free = 1;
    }
  }

  /* Create a new segment for this object, if none found above.  Also set
     'index' to the new or old segment being used. */

  if (v != SGGC_NO_OBJECT)
  { index = SET_VAL_INDEX(v);
  }
  else
  { 
    if (next_segment == maximum_segments)
    { return SGGC_NO_OBJECT;
    }

    sggc_segment[next_segment] = malloc (sizeof **sggc_segment);
    if (sggc_segment[next_segment] == NULL)
    { return SGGC_NO_OBJECT;
    }

    index = next_segment; 
    next_segment += 1;

    set_segment_init (SET_SEGMENT(index));
    sggc_data[index] = NULL;
#   ifdef SGGC_AUX1_SIZE
      sggc_aux1[index] = NULL;
#   endif
#   ifdef SGGC_AUX2_SIZE
      sggc_aux2[index] = NULL;
#   endif
    
    v = SET_VAL(index,0);
    if (SGGC_DEBUG) 
    { printf("sggc_alloc: created %x in new segment\n", (unsigned)v);
    }
  }

  /* Set up type and flags for the segment, or (if enabled) check that they 
     are already what they should be. */

  struct set_segment *seg = SET_SEGMENT(index);

  if (from_free) /* should be a small segment of the right kind already */
  { if (1) /* can be enabled for consistency checks */
    { if (sggc_type[index] != type) abort();
      if (seg->x.small.big) abort();
      if (seg->x.small.kind != kind) abort();
    }
  }
  else
  { sggc_type[index] = type;
    if (kind_chunks[kind] == 0) /* big segment */
    { seg->x.big.big = 1;
    }
    else /* small segment */
    { seg->x.small.big = 0;
      seg->x.small.kind = kind;
    }
  }

  /* Allocate data for the segment, if not already there. */

  if (sggc_data[index] == NULL)
  {
    char *data, *aux1, *aux2;

    if (seg->x.big.big) /* big segment */
    { 
      sggc_nchunks_t nch = sggc_nchunks (type, length);
      seg->x.big.max_chunks = (nch >> SGGC_CHUNK_BITS) == 0 ? nch : 0;

      data = malloc ((size_t) SGGC_CHUNK_SIZE * nch);
      if (SGGC_DEBUG) 
      { printf ("sggc_alloc: called malloc for %x (big %d, %d chunks):: %p\n", 
                 v, kind, (int)nch, data);
      }
    }
    else /* small segment */
    { 
      data = malloc ((size_t) SGGC_CHUNK_SIZE * SGGC_CHUNKS_IN_SMALL_SEGMENT);
      if (SGGC_DEBUG) 
      { printf ("sggc_alloc: called malloc for %x (small %d, %d chunks):: %p\n",
                 v, kind, (int)SGGC_CHUNKS_IN_SMALL_SEGMENT, data);
      }
    }

    sggc_data[index] = data;
    if (data == NULL) 
    { goto fail;
    }

#   ifdef SGGC_AUX1_SIZE
#     ifdef SGGC_AUX1_READ_ONLY
        aux1 = kind_aux1_read_only[kind];
        if (SGGC_DEBUG)
        { if (aux1!=NULL) printf("sggc_alloc: used read-only aux1 for %x\n", v);
        }
#     else
        aux1 = NULL;
#     endif
      if (aux1 == NULL)
      { aux1 = malloc ((size_t) SGGC_AUX1_SIZE * SGGC_CHUNKS_IN_SMALL_SEGMENT);
        if (SGGC_DEBUG)
        { printf("sggc_alloc: called malloc for aux1 for %x:: %p\n", v, aux1);
        }
        if (aux1 == NULL)
        { goto fail;
        }
      }
      sggc_aux1[index] = aux1;
#   endif

#   ifdef SGGC_AUX2_SIZE
#     ifdef SGGC_AUX2_READ_ONLY
        aux2 = kind_aux2_read_only[kind];
        if (SGGC_DEBUG)
        { if (aux2!=NULL) printf("sggc_alloc: used read-only aux2 for %x\n", v);
        }
#     else
        aux2 = NULL;
#     endif
      if (aux2 == NULL)
      { aux2 = malloc ((size_t) SGGC_AUX2_SIZE * SGGC_CHUNKS_IN_SMALL_SEGMENT);
        if (SGGC_DEBUG)
        { printf("sggc_alloc: called malloc for aux2 for %x:: %p\n", v, aux2);
        }
        if (aux2 == NULL)
        { goto fail;
        }
      }
      sggc_aux2[index] = aux2;
#   endif
  }

  /* Add the object to the free_or_new set.  For small segments not
     from free_or_new, we also put the segment into free_or_new, and
     use if as the current set of free objects if there were none
     before. */

  if (from_free || kind_chunks[kind] == 0) /* big, or already in new_or_free */
  {
    set_add (&free_or_new[kind], v);
  }
  else /* small segment not already in new_or_free */
  { 
    if (next_free[kind]!=end_free[kind]) abort(); /* should be none free now */

    end_free[kind] = set_first (&free_or_new[kind], 0);

    set_add (&free_or_new[kind], v);
  
    set_assign_segment_bits (&free_or_new[kind], v, kind_full[kind]);
    if (SGGC_DEBUG)
    { printf("sggc_alloc: new segment has bits %016llx\n", 
              (unsigned long long) set_segment_bits (&free_or_new[kind], v));
    }

    next_free[kind] = set_next (&free_or_new[kind], v, 0);
  }

  if (1) /* can be enabled for consistency checks */
  { if (set_contains (&old_gen1, v)) abort();
    if (set_contains (&old_gen2, v)) abort();
    if (set_contains (&old_to_new, v)) abort();
    if (set_contains (&to_look_at, v)) abort();
  }

  return v;

fail: /* segment obtained, but couldn't allocate data / aux info for segment */

  if (sggc_data[index] != NULL) 
  { free(sggc_data[index]);
    sggc_data[index] = NULL;
  }
# ifdef SGGC_AUX1_SIZE
    if (sggc_aux1[index] != NULL)
    {
#     ifdef SGGC_AUX1_READ_ONLY
      if (sggc_aux1[index] != kind_aux1_read_only[kind])
#     endif
      { free(sggc_aux1[index]);
        sggc_aux1[index] = NULL;
      }
    }
# endif
# ifdef SGGC_AUX2_SIZE
    if (sggc_aux2[index] != NULL)
    {
#     ifdef SGGC_AUX2_READ_ONLY
      if (sggc_aux2[index] != kind_aux2_read_only[kind])
#     endif
      { free(sggc_aux2[index]);
        sggc_aux2[index] = NULL;
      }
    }
# endif

  set_add (&unused, v);

  return SGGC_NO_OBJECT;
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
  int rep = 1;
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
    sggc_after_collect (level, rep++);
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

  /* Move big segments to the 'unused' set, while freeing their data
     storage.  Auxiliary information is not freed, but pointers to
     read-only information are cleared, since it might not apply when
     the segment is re-used as a different kind.

     Note that all big kinds are equal to their types, so we stop the
     loop at SGGC_N_TYPES. */

  for (k = 0; k < SGGC_N_TYPES; k++)
  { if (kind_chunks[k] == 0)
    { while ((v = set_first (&free_or_new[k], 1)) != SGGC_NO_OBJECT)
      { set_index_t index = SET_VAL_INDEX(v);
        if (SGGC_DEBUG) 
        { printf ("sggc_collect: calling free for %x:: %p\n", v, SGGC_DATA(v));
        }
        free (sggc_data[index]);
        sggc_data [index] = NULL;
#       ifdef SGGC_AUX1_READ_ONLY
          if (kind_aux1_read_only[k])
          { sggc_aux1[index] = NULL;
          }
#       endif
#       ifdef SGGC_AUX2_READ_ONLY
          if (kind_aux2_read_only[k])
          { sggc_aux2[index] = NULL;
          }
#       endif
        if (SGGC_DEBUG) 
        { printf("sggc_collect: putting %x in unused\n",(unsigned)v);
        }
        set_add (&unused, v);
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
  { printf ("sggc_look_at: %x %d\n", (unsigned)ptr, old_to_new_check);
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
  return set_chain_contains (SET_UNUSED_FREE_NEW, from_ptr);
}


/* TEST WHETHER AN OBJECT IS NOT (YET) MARKED AS IN USE. */

int sggc_not_marked (sggc_cptr_t ptr)
{
  return set_chain_contains (SET_UNUSED_FREE_NEW, ptr);
}
