/* SGGC - A LIBRARY SUPPORTING SEGMENTED GENERATIONAL GARBAGE COLLECTION.
          Common part of test programs

   Copyright (c) 2016, 2017, 2018 Radford M. Neal.

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


/* COMMON PART OF TEST PROGRAMS.  Defines the sequence of allocations and
   settings of objects, of trace output, and of checks for correctness. */

{ int i, j;

  printf ("STARTING TEST: segs = %d, iters = %d\n\n", segs, iters);

  /* Initialize and allocate initial nil object, which should be represented
     as zero. */

  printf("ABOUT TO CALL sggc_init\n");
  sggc_init(segs);
  printf("DONE sggc_init\n");
  printf("ALLOCATING nil\n");
  nil = alloc (0, 0);

  a = b = c = d = e = nil;

  for (i = 1; i <= iters; i++)
  { 
    printf("\nITERATION %d\n",i);

    /* Do some allocations, and set data fields. */

    printf("ALLOCATING a, leaving contents as nil\n");
    a = alloc (1, 2);

    printf("ALLOCATING b, setting contents to 100*i .. 100*i+9\n");
    b = alloc (2, 10);
    for (j = 0; j < LENGTH(b); j++)
    { TYPE2(b)->data[j] = 100*i + j;
    }

    printf("ALLOCATING c, setting its contents to a and b\n");
    c = alloc (1, 2);
    TYPE1(c)->x = a;
    TYPE1(c)->y = b;

    printf("ALLOCATING d, setting contents to 7777\n");
    d = alloc (2, 1);
    TYPE2(d)->data[0] = 7777;

    printf("ALLOCATING a AGAIN, leaving contents as nil\n");
    if (i == 2)
    { printf("BUT KEEPING REFERENCE TO OLD a IN e\n");
      e = a;
    }
    else if (i == 6)
    { printf("BUT KEEPING REFERENCES TO OLD a IN e->x AND TO b IN e->y\n");
      TYPE1(e)->x = a;
      OLD_TO_NEW_CHECK(e,a);
      TYPE1(e)->y = b;
      OLD_TO_NEW_CHECK(e,b);
    }
    a = alloc (1, 2);
    if (i == 8)
    { printf("AND KEEP REFERENCE TO NEW a IN e->x\n");
      TYPE1(e)->x = a;
      OLD_TO_NEW_CHECK(e,a);
    }

    /* Check that the contents are correct. */

    printf("CHECKING CONTENTS\n");

    if (TYPE(nil) != 0) abort();
    if (TYPE(a) != 1 || TYPE1(a)->x != nil || TYPE1(a)->y != nil) abort();
    if (TYPE(b) != 2 || LENGTH(b) != 10) abort();
    if (TYPE(c) != 1 || TYPE(TYPE1(c)->x) != 1 || TYPE1(c)->y != b) abort();
    if (TYPE(d) != 2 || LENGTH(d) != 1 || TYPE2(d)->data[0] != 7777) abort();

    if (i < 2)
    { if (e != nil) abort();
    }
    else if (i < 6)
    { if (TYPE(e)!=1 || TYPE1(e)->x != nil || TYPE1(e)->y != nil) abort();
    }
    else
    { if (TYPE(e) != 1 || TYPE(TYPE1(e)->x) != 1 
                       || TYPE(TYPE1(e)->y) != 2) abort();
      for (j = 0; j < LENGTH(TYPE1(e)->y); j++)
      { if (TYPE2(TYPE1(e)->y)->data[j] != 100*6 + j) abort();
      }
    }

    for (j = 0; j < LENGTH(b); j++)
    { if (TYPE2(b)->data[j] != 100*i + j) abort();
    }
  }

  printf("DONE MAIN PART OF TEST\n");

  printf("\nSGGC INFO\n\n");
  printf("Counts... Gen0: %u, Gen1: %d, Gen2: %d, Uncollected: %d\n",
          sggc_info.gen0_count, sggc_info.gen1_count, 
          sggc_info.gen2_count, sggc_info.uncol_count);
  printf("Big chunks... Gen0: %u, Gen1: %d, Gen2: %d, Uncollected: %d\n",
   (unsigned) sggc_info.gen0_big_chunks, (unsigned) sggc_info.gen1_big_chunks, 
   (unsigned) sggc_info.gen2_big_chunks, (unsigned) sggc_info.uncol_big_chunks);
  printf("Number of segments: %u,  Total memory usage: %llu bytes\n",
          sggc_info.n_segments, (unsigned long long) sggc_info.total_mem_usage);
  printf("Number of allocations: %llu,  At time of last GC: %llu\n",
          (unsigned long long) sggc_info.allocations, 
          (unsigned long long) sggc_info.allocations_at_last_gc);
  printf("GC counts: %u %u %u,  Since last lev 2: %u %u,  Since lev 1/2: %u\n",
          (unsigned) sggc_info.gc_count[0],
          (unsigned) sggc_info.gc_count[1], 
          (unsigned) sggc_info.gc_count[2],
          (unsigned) sggc_info.gc_since_lev2[0],
          (unsigned) sggc_info.gc_since_lev2[1], 
          (unsigned) sggc_info.gc_since_lev12);

#ifdef SGGC_TRACE_CPTR
  if (sggc_trace_cptr == SGGC_NO_OBJECT)
  { printf ("\nSGGC_TRACE_CPTR: none\n");
  }
  else
  { printf (
     "\nSGGC_TRACE_CPTR: %x (%u), sggc_trace_cptr_count: %u, in use: %d\n",
        sggc_trace_cptr, sggc_trace_cptr, 
        sggc_trace_cptr_count, sggc_trace_cptr_in_use);
  }
#endif

  printf("\nFINAL YOUNGEST:  nil %d, a %d, b %d, c %d, d %d, e %d\n",
         YOUNGEST(nil), YOUNGEST(a), YOUNGEST(b), 
         YOUNGEST(c), YOUNGEST(d), YOUNGEST(e));
}
