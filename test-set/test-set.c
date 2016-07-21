/* SGGC - A LIBRARY SUPPORTING SEGMENTED GENERATIONAL GARBAGE COLLECTION.
          Facility for maintaining sets of objects - test program

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

#include <stdio.h>
#include "set-app.h"

#define N_SET 3
struct set set[N_SET];

int main (void)
{ 
  int i, j, x, o, r;
  char s[1000];
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
    c = ' ';
    i = x = o = -1;
    r = scanf(" %c %d %d %d",&c,&i,&x,&o);
    if (r == -1)
    { printf("\n");
      return 0;
    }

    s[0] = 0;
    scanf("%[^\n]",s);

    if (c == 'm')
    { if (r != 3)
      { printf("Wrong number of arguments\n");
      }
      printf("%c %d %d %s\n",c,i,x,s);
    }
    else
    { if (r != 4)
      { printf("Wrong number of arguments\n");
      }
      printf("%c %d %d %d %s\n",c,i,x,o,s);
    }

    if (i < 0 || i >= N_SET) 
    { printf("Invalid set\n");
      continue;
    }

    if (c == 'm')
    { if (x < 0 || x >= N_SET) 
      { printf("Invalid set\n");
        continue;
      }
    }
    else
    { if (x < 0 || x >= N_SEG) 
      { printf("Invalid segment\n");
        continue;
      }
      if (o < 0 || o >= 1<<SET_OFFSET_BITS) 
      { printf("Invalid offset\n");
        continue;
      }
    }

    switch (c) 
    { case 'c': 
      { printf("result: %d\n",set_contains(&set[i],SET_VAL(x,o)));
        break;
      }
      case 'a':
      { printf("result: %d\n",set_add(&set[i],SET_VAL(x,o)));
        break;
      }
      case 'r':
      { printf("result: %d\n",set_remove(&set[i],SET_VAL(x,o)));
        break;
      }
      case 'm':
      { set_move_first (&set[i], &set[x]);
        break;
      }
      default: 
      { printf("Unknown operation");
        break;
      }
    }

    for (i = 0; i<N_SET; i++)
    { set_value_t v;
      printf("Set %d :",i);
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
