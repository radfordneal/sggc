
/* NUMBER OF OFFSET BITS IN A (SEGMENT INDEX, OFFSET) PAIR. */

#define SET_OFFSET_BITS 6  /* Maximum for using 64-bit shift/mask operations */


/* CHAINS FOR LINKING SEGMENTS IN SETS.  All sets used have their own chain,
   except that the SET_FREE_NEW chain is shared amongst a collection of sets,
   one set for each possible kind of object. */

#define SET_CHAINS 5       /* Number of chains that can be used for sets */
#  define SET_FREE_NEW 0      /* Free or newly alloc'd objects, sets by kind */
#  define SET_OLD_GEN1 1      /* Objects that survived one GC */
#  define SET_OLD_GEN2 2      /* Objects that survived more than one GC */
#  define SET_OLD_TO_NEW 3    /* Objects maybe having old-to-new references */
#  define SET_TO_LOOK_AT 4    /* Objects that still need to be looked */


/* EXTRA INFORMATION STORED IN A SET_SEGMENT STRUCTURE.  This takes advantage
   of what would otherwise be 32 bits of unused padding, and makes the 
   set_segment struct be exactly 64 bytes in size (advantageous for index 
   computation and perhaps cache behaviour). */

#define SET_EXTRA_INFO \
  union \
  { struct                 /* For big segments... */ \
    { unsigned big : 1;       /* 1 for a big segment with 1 large object */ \
      unsigned max_len : SGGC_LEN_BITS; /* Maximum size for object in space */ \
    } big; \                            /*  alloc'd; 0 if object size fixed */ \
    struct                 /* For small segments... */ \
    { unsigned big : 1;       /* 0 for a segment with many small objects */ \
      unsigned unused : 15;   /* Not currently used */ \
      unsigned n_objects : 8; /* Number of objects that fit in the segment */ \
      unsigned n_chunks : 8;  /* Number of storage chunks for each object */ \
    } small; \
  } x;


/* INCLUDE THE NON-APPLICATION-SPECIFIC HEADER. */

#include "set.h"


/* POINTER TO ARRAY OF POINTERS TO SEGMENTS.  The array is allocated when the
   GC is initialized, with segments allocated as needed later. */

struct set_segment **sggc_segment;


/* MACRO FOR GETTING SEGMENT POINTER FROM SEGMENT INDEX. */

#define SET_SEGMENT(index) (sggc_segment[index])
