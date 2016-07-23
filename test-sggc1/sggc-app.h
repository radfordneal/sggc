#define SGGC_CHUNK_SIZE 16    /* Number of bytes in a data chunk */
#define SGGC_AUX_ITEMS 0      /* Number of auxiliary data items for objects */

typedef int sggc_length_t;    /* Type for holding an object length */
typedef char sggc_type_t;     /* Type for holding an object type */

#define SGGC_N_KINDS 2
#define SGGC_KIND(type,length) 

#include "sggc.h"
