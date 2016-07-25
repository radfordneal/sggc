#define SGGC_CHUNK_SIZE 16      /* Number of bytes in a data chunk */
#define SGGC_AUX_ITEMS 0        /* Number of auxiliary data items for objects */

#define SGGC_N_TYPES 3          /* Number of object types */

typedef unsigned sggc_length_t; /* Type for holding an object length */
typedef unsigned sggc_nchunks_t;/* Type for how many chunks are in a segment */

#define SGGC_N_KINDS 3          /* Number of kinds of segments */
#define SGGC_KIND_CHUNKS { 0, 0, 0 } 

#include "sggc.h"
