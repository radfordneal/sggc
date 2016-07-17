/* Example set-app.h file. */

#define SET_OFFSET_BITS 6   /* Must be 3, 4, 5, or 6 */
#define SET_CHAINS 3

#include "set.h"

struct 
{ struct set_node set_node[SET_CHAINS];
} segments[10];

#define SET_NODE_PTR(chain,index) (&segments[index].set_node[chain])
