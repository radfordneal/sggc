/* SGGC - A LIBRARY SUPPORTING SEGMENTED GENERATIONAL GARBAGE COLLECTION.
          Test program #1 - main program

   Copyright (c) 2016, 2017 Radford M. Neal.

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


/* This test program uses compressed pointers, big segments only, and
   no auxiliary data.  Optional garbage collections are done according
   to a simple scheme based just on number of allocations done.  It is
   run with its first program argument giving the maximum number of
   segments (default 11, the minimum for not running out of space),
   and its second giving the number of iterations of the test loop
   (default 15). */


#include <stdlib.h>
#include <stdio.h>
#include "sggc-app.h"


/* TYPE OF A POINTER USED IN THIS APPLICATION.  Uses compressed pointers.
   The OLD_TO_NEW_CHECK macro can therefore just call sggc_old_to_new,
   YOUNGEST just call sggc_youngest_generation, and TYPE is just SGGC_TYPE. */

typedef sggc_cptr_t ptr_t;

#define OLD_TO_NEW_CHECK(from,to) sggc_old_to_new_check(from,to)
#define YOUNGEST(v) sggc_youngest_generation(v)
#define TYPE(v) SGGC_TYPE(v)


/* TYPES FOR THIS APPLICATION.  Type 0 is a "nil" type.  Type 1 is a 
   typical "dotted pair" type.  Type 2 is a typical numeric vector type. */

struct type0 { int dummy; };
struct type1 { ptr_t x, y; };
struct type2 { sggc_length_t len; int32_t data[]; };

#define TYPE1(v) ((struct type1 *) SGGC_DATA(v))
#define TYPE2(v) ((struct type2 *) SGGC_DATA(v))

#define LENGTH(v) (TYPE2(v)->len)  /* only works for type 2 */


/* VARIABLES THAT ARE ROOTS FOR THE GARBAGE COLLECTOR. */

static ptr_t nil, a, b, c, d, e;


/* FUNCTIONS THAT THE APPLICATION NEEDS TO PROVIDE TO THE SGGC MODULE. */

sggc_kind_t sggc_kind (sggc_type_t type, sggc_length_t length)
{ 
  return type;
}

sggc_nchunks_t sggc_nchunks (sggc_type_t type, sggc_length_t length)
{
  return type != 2 ? 1 : (4+length) / 4;
}

void sggc_find_root_ptrs (void)
{ sggc_look_at(nil);
  sggc_look_at(a);
  sggc_look_at(b);
  sggc_look_at(c);
  sggc_look_at(d);
  sggc_look_at(e);
}

void sggc_find_object_ptrs (sggc_cptr_t cptr)
{
  if (SGGC_TYPE(cptr) == 1)
  { sggc_look_at (TYPE1(cptr)->x);
    sggc_look_at (TYPE1(cptr)->y);
  }
}


/* ALLOCATE FUNCTION FOR THIS APPLICATION.  Calls the garbage collector
   when necessary, or otherwise every 8th allocation, with every 24th
   being level 1, and every 48th being level 2. */

static ptr_t alloc (sggc_type_t type, sggc_length_t length)
{
  static unsigned alloc_count = 0;
  sggc_cptr_t a;

  /* Do optional garbage collections according to the scheme.  Do this first,
     not after allocating the object, which would then get collected! */

  alloc_count += 1;
  if (alloc_count % 8 == 0)
  { printf("ABOUT TO CALL sggc_collect IN ALLOC DUE TO %d ALLOCATIONS\n",
            alloc_count);
    sggc_collect (alloc_count % 48 == 0 ? 2 : alloc_count % 24 == 0 ? 1 : 0);
  }

  /* Try to allocate object, calling garbage collector if this initially 
     fails. */

  a = sggc_alloc(type,length);
  if (a == SGGC_NO_OBJECT)
  { printf("ABOUT TO CALL sggc_collect IN ALLOC BECAUSE ALLOC FAILED\n");
    sggc_collect(2);
    a = sggc_alloc(type,length);
    if (a == SGGC_NO_OBJECT)
    { printf("CAN'T ALLOCATE\n");
      abort();
      exit(1);
    }
  }

  /* Initialize the object (essential for objects containing pointers). */

  switch (type)
  { case 1: 
    { TYPE1(a)->x = TYPE1(a)->y = nil;
      break;
    }
    case 2:
    { TYPE2(a)->len = length;
      break;
    }
    default:
    { break;
    }
  }

  printf("ALLOC RETURNING %x\n",(unsigned)a);
  return a;
}


/* MAIN TEST PROGRAM. */

static int freed1 (sggc_cptr_t v)
{ printf("CALLED_FOR_NEWLY_FREE: Object %x of type 1 being freed at end\n",
          (unsigned)v);
  return 0;
}

static int freed2 (sggc_cptr_t v)
{ printf("CALLED_FOR_NEWLY_FREE: Object %x of type 2 won't be freed at end\n",
          (unsigned)v);
  return 1;
}

static void in_use (sggc_cptr_t v, sggc_nchunks_t nch)
{ printf("CALLED_FOR_OBJECT_IN_USE: Object %x with %d chunks\n",
          (unsigned)v, (int) nch);
}


int main (int argc, char **argv)
{ 
  int segs = argc<2 ? 11 /* min for no failure */ : atoi(argv[1]);
  int iters = argc<3 ? 15 : atoi(argv[2]);

# include "test-common.h"

  sggc_call_for_newly_freed_object (1, freed1);
  sggc_call_for_newly_freed_object (2, freed2);
  sggc_call_for_object_in_use (in_use);

  printf("\nCOLLECTING EVERYTHING, EXCEPT TYPE 2 AND nil\n\n");
  a = b = c = d = e = nil;
  sggc_collect(2);

  printf("\nSGGC INFO\n\n");
  printf("Counts... Gen0: %u, Gen1: %d, Gen2: %d, Uncollected: %d\n",
          sggc_info.gen0_count, sggc_info.gen1_count, 
          sggc_info.gen2_count, sggc_info.uncol_count);
  printf("Big chunks... Gen0: %u, Gen1: %d, Gen2: %d, Uncollected: %d\n",
   (unsigned) sggc_info.gen0_big_chunks, (unsigned) sggc_info.gen1_big_chunks, 
   (unsigned) sggc_info.gen2_big_chunks, (unsigned) sggc_info.uncol_big_chunks);
  printf("Number of segments: %u,  Total memory usage: %llu bytes\n",
          sggc_info.n_segments, (unsigned long long) sggc_info.total_mem_usage);

  sggc_call_for_newly_freed_object (1, freed1);
  sggc_call_for_newly_freed_object (2, 0);

  printf("\nCOLLECTING EVERYTHING\n\n");
  nil = a = b = c = d = e = SGGC_NO_OBJECT;
  sggc_collect(2);

  printf("\nSGGC INFO\n\n");
  printf("Counts... Gen0: %u, Gen1: %d, Gen2: %d, Uncollected: %d\n",
          sggc_info.gen0_count, sggc_info.gen1_count, 
          sggc_info.gen2_count, sggc_info.uncol_count);
  printf("Big chunks... Gen0: %u, Gen1: %d, Gen2: %d, Uncollected: %d\n",
   (unsigned) sggc_info.gen0_big_chunks, (unsigned) sggc_info.gen1_big_chunks, 
   (unsigned) sggc_info.gen2_big_chunks, (unsigned) sggc_info.uncol_big_chunks);
  printf("Number of segments: %u,  Total memory usage: %llu bytes\n",
          sggc_info.n_segments, (unsigned long long) sggc_info.total_mem_usage);

  printf("\nEND TESTING\n");

  return 0;
}
