#define SGGC_CHUNK_SIZE 16    /* Number of bytes in a data chunk */
#define SGGC_AUX_ITEMS 0      /* Number of auxiliary data items for objects */

#define SGGC_N_TYPES 2        /* Number of object types */
typedef char sggc_type_t;     /* Type for holding an object type */

typedef int sggc_length_t;    /* Type for holding an object length */

#define SGGC_N_KINDS 2        /* Number of segment types */
#define SGGC_KIND_CHUNKS { 0, 0 } 

#include "sggc.h"
