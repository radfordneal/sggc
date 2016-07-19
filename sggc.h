#include "set-app.h"

typedef set_value_t sggc_cptr;

void **sggc_data;
void **sggc_aux;
sggc_length_t **sggc_length;

void sggc_init (int max_segments);
sggc_cptr *sggc_alloc (sggc_type_t type, sggc_length_t length);
void sggc_collect (int level);


