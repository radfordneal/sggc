/* SGGC - A LIBRARY SUPPORTING SEGMENTED GENERATIONAL GARBAGE COLLECTION.
          Test program #1 - main program

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


/* This test program uses only big segments, and no auxiliary data.
   Optional garbage collections are done according to a simple scheme
   based just on number of allocations done.  It is run with its first
   program argument giving the maximum number of segments (default 11,
   the minimum for not running out of space), and its second giving
   the number of iterations of the test loop (default 15). */


#include <stdlib.h>
#include <stdio.h>
#include "sggc-app.h"


/* TYPES FOR THIS APPLICATION.  Type 0 is a "nil" type.  Type 1 is a 
   typical "dotted pair" type.  Type 3 is a typical numeric vector type. */

struct type0 { int dummy; };
struct type1 { sggc_cptr_t x, y; };
struct type2 { sggc_length_t len; int32_t data[]; };

#define TYPE1(v) ((struct type1 *) SGGC_DATA(v))
#define TYPE2(v) ((struct type2 *) SGGC_DATA(v))


/* VARIABLES THAT ARE ROOTS FOR THE GARBAGE COLLECTOR. */

static sggc_cptr_t nil, a, b, c, d, e;


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
  { if (!sggc_look_at (TYPE1(cptr)->x)) return;
    if (!sggc_look_at (TYPE1(cptr)->y)) return;
  }
}


/* ALLOCATE FUNCTION FOR THIS APPLICATION.  Calls the garbage collector
   when necessary, or otherwise every 8th allocation, with every 24th
   being level 1, and every 48th being level 2. */

static sggc_cptr_t alloc (sggc_type_t type, sggc_length_t length)
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

int main (int argc, char **argv)
{ 
  int segs = argc<2 ? 11 /* min for no failure */ : atoi(argv[1]);
  int iters = argc<3 ? 15 : atoi(argv[2]);
  int i, j;

  printf ("TEST-SGGC1: segs = %d, iters = %d\n\n", segs, iters);

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
    a = alloc (1, 1);

    printf("ALLOCATING b, setting contents to 100*i .. 100*i+9\n");
    b = alloc (2, 10);
    for (j = 0; j < TYPE2(b)->len; j++)
    { TYPE2(b)->data[j] = 100*i + j;
    }

    printf("ALLOCATING c, setting its contents to a and b\n");
    c = alloc (1, 1);
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
      sggc_old_to_new_check(e,a);
      TYPE1(e)->y = b;
      sggc_old_to_new_check(e,b);
    }
    a = alloc (1, 1);
    if (i == 8)
    { printf("AND KEEP REFERENCE TO NEW a IN e->x\n");
      TYPE1(e)->x = a;
      sggc_old_to_new_check(e,a);
    }

    /* Check that the contents are correct. */

    printf("CHECKING CONTENTS\n");

    if (SGGC_TYPE(a) != 1 || TYPE1(a)->x != nil || TYPE1(a)->y != nil
     || SGGC_TYPE(b) != 2 || TYPE2(b)->len != 10
     || SGGC_TYPE(c) != 1 || SGGC_TYPE(TYPE1(c)->x) != 1 || TYPE1(c)->y != b
     || SGGC_TYPE(d) != 2 || TYPE2(d)->len != 1 || TYPE2(d)->data[0] != 7777)
    { abort();
    }

    if (i < 2)
    { if (e != nil) abort();
    }
    else if (i < 6)
    { if (SGGC_TYPE(e)!=1 || TYPE1(e)->x != nil || TYPE1(e)->y != nil) abort();
    }
    else
    { if (SGGC_TYPE(e) != 1 || SGGC_TYPE(TYPE1(e)->x) != 1 
                            || SGGC_TYPE(TYPE1(e)->y) != 2) abort();
      for (j = 0; j < TYPE2(TYPE1(e)->y)->len; j++)
      { if (TYPE2(TYPE1(e)->y)->data[j] != 100*6 + j) abort();
      }
    }

    for (j = 0; j < TYPE2(b)->len; j++)
    { if (TYPE2(b)->data[j] != 100*i + j) abort();
    }
  }

  printf("DONE TESTING\n");

  return 0;
}
