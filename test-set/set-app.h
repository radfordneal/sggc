/* Example set-app.h file. */

#define SET_OFFSET_BITS 6   /* Must be 3, 4, 5, or 6 */
#define SET_CHAINS 3

#include "set.h"

#define N_SEG 10

struct set_segment segment[N_SEG];

#define SET_SEGMENT(index) (&segment[index])
