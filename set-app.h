#define SET_OFFSET_BITS 6   /* Must be 3, 4, 5, or 6 */

struct 
{ struct set_node *set_node;
} *segments;

#define SET_NODE_PTR(set,index) (&segments[index].set_node[set])

#include "set.h"


