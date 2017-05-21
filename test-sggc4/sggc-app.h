/* SGGC - A LIBRARY SUPPORTING SEGMENTED GENERATIONAL GARBAGE COLLECTION.
          Test program #4 - sggc application header file

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


#define SGGC_CHUNK_SIZE 16      /* Number of bytes in a data chunk */

#define SGGC_N_TYPES 3          /* Number of object types */

typedef unsigned sggc_length_t; /* Type for holding an object length */
typedef unsigned sggc_nchunks_t;/* Type for how many chunks are in a segment */

#define SGGC_N_KINDS 3          /* Number of kinds of segments */

#define SGGC_KIND_CHUNKS { 0, 0, 0 }  /* Sizes of chunks by kind, 0 for 'big' */

#define SGGC_AUX1_SIZE 4        /* Size of auxiliary information */
#define SGGC_AUX1_BLOCK_SIZE 4  /* Number of blocks in aux1 allocations */

/* Special alloc/free for testing; defines sggc_mem_alloc_zero,
   sggc_mem_alloc, and sggc_mem_free. */

#include "test-calloc.h"

/* Include the generic SGGC header file. */

#include "sggc.h"
