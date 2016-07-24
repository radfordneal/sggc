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

int sggc_kind_chunks[SGGC_N_KINDS] = SGGC_KIND_CHUNKS;


/* SETS OF OBJECTS. */

static struct set free_or_new[SGGC_N_KINDS];  /* Free or newly allocated */
static struct set old_gen1;                   /* Survived collection once */
static struct set old_gen2;                   /* Survived collection >1 time */
static struct set old_to_new;                 /* May have old->new references */
static struct set to_look_at;                 /* Not yet looked at in sweep */


/* INDEX OF NEXT SEGMENT TO USE. */

static int next_segment;


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

  /* Initialize to no segments in use. */

  next_segment = 0;

  return 0;
}


sggc_cptr_t sggc_alloc (sggc_type_t type, sggc_length_t length)
{
  int kind = sggc_kind(type,length);

  if (sggc_kind_chunks[kind] == 0)
  { 
    return SET_NO_VALUE;
  }
  else
  { return SET_NO_VALUE;
  }
}


void sggc_collect (int level)
{
}

