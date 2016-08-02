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
   based just on number of allocations done. */


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

static sggc_cptr_t nil, a, b, c, d;


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
}

void sggc_find_object_ptrs (sggc_cptr_t cptr)
{
  if (SGGC_TYPE(cptr) == 1)
  { sggc_look_at (TYPE1(cptr)->x);
    sggc_look_at (TYPE1(cptr)->y);
  }
}


/* ALLOCATE FUNCTION FOR THIS APPLICATION.  Calls the garbage collector
   when necessary, or otherwise every 10th allocation, with every 100th
   being level 1, and every 1000th being level 2. */

static sggc_cptr_t alloc (sggc_type_t type, sggc_length_t length)
{
  static unsigned alloc_count = 0;
  sggc_cptr_t a;

  /* Do optional garbage collections according to the scheme.  Do this first,
     not after allocating the object, which would then get collected! */

  alloc_count += 1;
  if (alloc_count % 10 == 0)
  { printf("ABOUNT TO CALL sggc_collect IN ALLOC DUE TO %d ALLOCATIONS\n",
            alloc_count);
    sggc_collect (alloc_count % 1000 == 0 ? 2 : alloc_count % 100 == 0 ? 1 : 0);
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

int main (void)
{ int i, j;

  /* Initialize and allocate initial nil object, which should be represented
     as zero. */

  printf("ABOUT TO CALL sggc_init\n");
  sggc_init(8);  /* Minimum for no allocation failure is 8 */
  printf("DONE sggc_init\n");
  nil = alloc (0, 0);
  printf("ALLOCATED nil: %d\n",(int)nil);

  a = b = c = d = nil;

  for (i = 0; i < 10; i++)
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
    a = alloc (1, 1);

#   if 0  /* can be enabled to test explicit collection */
      printf("ABOUT TO CALL sggc_collect\n");
      sggc_collect(0);
      printf("DONE sggc_collect\n");
#   endif

    /* Check that the final contents are correct. */

    printf("CHECKING CONTENTS\n");

    if (SGGC_TYPE(a) != 1 || TYPE1(a)->x != nil || TYPE1(a)->y != nil
     || SGGC_TYPE(b) != 2 || TYPE2(b)->len != 10
     || SGGC_TYPE(c) != 1 || SGGC_TYPE(TYPE1(c)->x) != 1 || TYPE1(c)->y != b
     || SGGC_TYPE(d) != 2 || TYPE2(d)->len != 1 || TYPE2(d)->data[0] != 7777)
    { abort();
    }

    for (j = 0; j < TYPE2(b)->len; j++)
    { if (TYPE2(b)->data[j] != 100*i + j) 
      { abort();
      }
    }
  }

  printf("DONE TESTING\n");

  return 0;
}
