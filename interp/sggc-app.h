/* SGGC - A LIBRARY SUPPORTING SEGMENTED GENERATIONAL GARBAGE COLLECTION.
          Simple interpreter - sggc application header file

   Copyright (c) 2016, 2017 Radford M. Neal.

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


#define SGGC_CHUNK_SIZE 8       /* Number of bytes in a data chunk */

#define SGGC_N_TYPES 4          /* Number of object types */

typedef unsigned sggc_length_t; /* Type for holding an object length */
typedef unsigned sggc_nchunks_t;/* Type for how many chunks are in a segment */

#define SGGC_N_KINDS 4          /* Number of kinds of segments */

#define SGGC_KIND_CHUNKS { 0, 1, 1, 1 }
#define SGGC_KIND_TYPES { 0, 1, 2, 3 }

#ifdef UNCOLLECTED_NIL
#define SGGC_KIND_UNCOLLECTED { 1, 0, 0, 0 } /* Make () an uncollected object */
#elif UNCOLLECTED_NIL_SYMS
#define SGGC_KIND_UNCOLLECTED { 1, 0, 1, 0 } /* () and symbols are uncollected*/
#endif

#define SGGC_AUX1_SIZE 1        /* Size of auxiliary information 1 */
#define SGGC_AUX1_BLOCK_SIZE 4  /* Number of blocks in aux1 allocations */
#define SGGC_AUX1_READ_ONLY     /* Some auxiliary information 1 is read-only */

#define sggc_kind(type,length) (type) /* One kind for each type */

#define sggc_nchunks(type,length) 1  /* Should never be used, no big segments */

/* Include the generic SGGC header file. */

#include "sggc.h"
