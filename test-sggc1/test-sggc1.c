#include "sggc-app.h"

struct type1 { sggc_cptr_t x, y; };
struct type2 { sggc_length_t len; int32_t data[]; };

static sggc_cptr_t a, b;

sggc_kind_t sggc_kind (sggc_type_t type, sggc_length_t length)
{ 
  return type;
}

sggc_nchunks_t sggc_chunks (sggc_type_t type, sggc_length_t length)
{
  return type==1 ? 1 : (4+length) / 4;
}

void sggc_find_root_ptrs (void)
{ sggc_look_at(a);
  sggc_look_at(b);
}

void sggc_find_ptrs (sggc_cptr_t cptr)
{
  if (SGGC_TYPE(cptr) == 1)
  { struct type1 *v = (struct type1 *) SGGC_DATA(cptr);
    sggc_look_at(v->x);
    sggc_look_at(v->y);
  }
}

int main (void)
{
  sggc_init(1000);
  a = sggc_alloc (1, 1);
  b = sggc_alloc (2, 10);
  a = sggc_alloc (1, 1);
  sggc_collect(0);
  return 0;
}
