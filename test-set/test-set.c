#include <stdio.h>
#include "set-app.h"

#define N_SET 4
struct set set[N_SET];

int main (void)
{ 
  set_value_t v;
  int i, j, x, o, r;
  char c; 

  for (j = 0; j<N_SEG; j++)
  { set_segment_init (&segment[j]);
  }
  for (i = 0; i<N_SET; i++) 
  { set_init (&set[i], i<SET_CHAINS ? i : SET_CHAINS-1);
  }

  for (;;)
  { 
    printf("> ");
    r = scanf(" %c %d %d %d%*[^\n]",&c,&i,&x,&o);
    if (r == -1)
    { printf("\n");
      return 0;
    }

    printf("%c %d %d %d: ",c,i,x,o);

    if (i < 0 || i >= N_SET) 
    { printf("Invalid set\n");
      continue;
    }
    if (x < 0 || x >= N_SEG) 
    { printf("Invalid segment\n");
      continue;
    }
    if (o < 0 || o >= 1<<SET_OFFSET_BITS) 
    { printf("Invalid offset\n");
      continue;
    }

    v = SET_VAL(x,o);

    switch (c) 
    { case 'c': 
      { printf("%d",set_contains(&set[i],v));
        break;
      }
      case 'a':
      { printf("%d",set_add(&set[i],v));
        break;
      }
      case 'r':
      { printf("%d",set_remove(&set[i],v));
        break;
      }
      default: 
      { printf("Unknown operation");
        break;
      }
    }
    printf("\n");

    for (i = 0; i<N_SET; i++)
    { printf("Set %d :",i);
      v = set_first (&set[i]);
      if (v == SET_NO_VALUE)
      { printf(" empty\n");
        continue;
      }
      printf (" %016llx :", (long long) set_first_bits (&set[i]));
      while (v != SET_NO_VALUE)
      { printf(" %d.%d",SET_VAL_INDEX(v),SET_VAL_OFFSET(v));
        v = set_next (&set[i], v);
      }
      printf("\n");
    }
  }
}
