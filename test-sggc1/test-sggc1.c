#include "sggc-app.h"

int sggc_kind (sggc_type_t type, sggc_length_t length)
{ 
  return type;
}

int sggc_chunks (sggc_type_t type, sggc_length_t length)
{
  return type==1 ? 1 : (2+length) / 2;
}

int main (void)
{
  sggc_cptr_t a, b;
  sggc_init(1000);
  a = sggc_alloc (1, 1);
  b = sggc_alloc (2, 10);
  return 0;
}
