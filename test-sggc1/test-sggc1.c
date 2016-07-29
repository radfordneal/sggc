#include <stdlib.h>
#include <stdio.h>
#include "sggc-app.h"

struct type0 { int dummy; };
struct type1 { sggc_cptr_t x, y; };
struct type2 { sggc_length_t len; int32_t data[]; };

static sggc_cptr_t nil, a, b;

sggc_kind_t sggc_kind (sggc_type_t type, sggc_length_t length)
{ 
  return type;
}

sggc_nchunks_t sggc_nchunks (sggc_type_t type, sggc_length_t length)
{
  return type<2 ? 1 : (4+length) / 4;
}

void sggc_find_root_ptrs (void)
{ sggc_look_at(nil);
  sggc_look_at(a);
  sggc_look_at(b);
}

void sggc_find_object_ptrs (sggc_cptr_t cptr)
{
  if (SGGC_TYPE(cptr) == 1)
  { struct type1 *v = (struct type1 *) SGGC_DATA(cptr);
    sggc_look_at(v->x);
    sggc_look_at(v->y);
  }
}

static sggc_cptr_t alloc (sggc_type_t type, sggc_length_t length)
{
  sggc_cptr_t a;
  a = sggc_alloc(type,length);
  if (a == SGGC_NO_OBJECT)
  { printf("ABOUT TO CALL sggc_collect IN ALLOC\n");
    sggc_collect(2);
    a = sggc_alloc(type,length);
    if (a == SGGC_NO_OBJECT)
    { printf("CAN'T ALLOCATE\n");
      abort();
      exit(1);
    }
  }
  switch (type)
  { case 1: 
    { struct type1 *p = (struct type1 *) SGGC_DATA(a);
      p->x = p->y = nil;
      break;
    }
    case 2:
    { struct type2 *p = (struct type2 *) SGGC_DATA(a);
      p->len = length;
      break;
    }
    default:
    { break;
    }
  }
  printf("ALLOC RETURNING %x\n",(unsigned)a);
  return a;
}

int main (void)
{ int i;
  printf("ABOUT TO CALL sggc_init\n");
  sggc_init(4);
  printf("DONE sggc_init\n");
  printf("ALLOCATING nil\n");
  nil = alloc (0, 0);
  for (i = 0; i<10; i++)
  { printf("\nITERATION %d\n",i);
    printf("ALLOCATING a\n");
    a = alloc (1, 1);
    printf("ALLOCATING b\n");
    b = alloc (2, 10);
    printf("ALLOCATING a AGAIN\n");
    a = alloc (1, 1);
    printf("ABOUT TO CALL sggc_collect\n");
    sggc_collect(0);
    printf("DONE sggc_collect\n");
  }
  return 0;
}
