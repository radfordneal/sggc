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

char *sggc_alloc_big_segment_data (sggc_kind_t kind, sggc_length_t length)
{
  switch (kind)
  { case 0: 
    { struct type0 *p = malloc (sizeof (struct type0));
      return (char *) p;
    }
    case 1: 
    { struct type1 *p = malloc (sizeof (struct type1));
      p->x = nil;
      p->y = nil;
      return (char *) p;
    }
    case 2: 
    { struct type2 *p
               = malloc (sizeof (struct type2) + length * sizeof (int32_t));
      p->len = length;
      return (char *) p;
    }
    default: abort();
  }
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

int main (void)
{
  printf("About to call sggc_init\n");
  sggc_init(1000);
  printf("Done sggc_init\n");
  printf("Allocating nil\n");
  nil = sggc_alloc (0, 0);
  printf("Allocating a\n");
  a = sggc_alloc (1, 1);
  printf("Allocating b\n");
  b = sggc_alloc (2, 10);
  printf("Allocating a again\n");
  a = sggc_alloc (1, 1);
  printf("About to call sggc_collect\n");
  sggc_collect(0);
  printf("Done sggc_collect\n");
  return 0;
}
