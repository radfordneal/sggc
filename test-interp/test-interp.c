/* SGGC - A LIBRARY SUPPORTING SEGMENTED GENERATIONAL GARBAGE COLLECTION.
          Simple interpreter used to test SGGC

   Copyright (c) 2016 Radford M. Neal.

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


#include <stdlib.h>
#include <stdio.h>
#include "sggc-app.h"


/* INTERPRETER FOR A SIMPLE LISP-LIKE LANGUAGE.  Used to test SGGC, and
   assess its performance.

   Syntax of data and programs:

     ()             The nil object

     (a b (x y) c)  list of a, b, (x y), and c

     a, b, c, ...   Symbols: which are a single character from the set 
                      a-z, A-Z, ', ?, @, %, $, =, ., :, &, +, *, ^

     # comment      Rest of line from # on is a comment
       
   Expressions:

     (f a b c)      Evaluate f, a, b, c, then call value of f with arguments
                    a, b, c; f must not be a special symbol as listed below

     (' a)          Returns a unevaluated

     (? w a b)      Conditional expression, evaluates w, then returns result of
                       evaluating a if w is a list (not () or a symbol), and
                       otherwise returns result of evaluing b (default ())

     (@ v e)        Assignment expression, evaluates e, then changes the most
                       recent binding of symbol v to be the value of e

     (% (x y) e)    Expression that creates bindings for x and y (initially ()),
                       then returns the result of evaluating e

     ($ (x y) e)    As an expression, evaluates to itself; as a function, takes
                       arguments for x and y, creating bindings for them, and
                       returns the result of evaluating e

     (= a b)        Returns '= if the results of evaluating a and b are equal,
                       () if not

     (. a)          If a evaluates to a list, returns its first element

     (: a)          If a evaluates to a list, returns this list with the
                    first element dropped, () if the list had only one element

     (& x a)        If a evaluates to a list or (), returns the list with the
                    result of evaluating x appended to the front of this list

  Bindings for all symbols exists globally, with initial values of ().

  The interpreter repeatedly reads an expression, evaluates it, and prints
  the value returned, until end-of-file.  Changes to the global bindings
  from evaluation of one such expression may affect the evaluation of the next.
*/


/* TYPE OF A POINTER USED IN THIS APPLICATION.  Uses compressed pointers.
   The OLD_TO_NEW_CHECK macro can therefore just call sggc_old_to_new
   and TYPE is just SGGC_TYPE. */

typedef sggc_cptr_t ptr_t;

#define OLD_TO_NEW_CHECK(old,new) sggc_old_to_new_check(old,new)
#define TYPE(v) SGGC_TYPE(v)


/* TYPES FOR THIS APPLICATION. */

struct type_nil { int dummy; };              /* Nil, () */
struct type_list { ptr_t head, tail; };      /* List, not including () */
struct type_symbol { char symbol; };         /* Symbol, only 64 of them */
struct type_binding { ptr_t value, next; };  /* Binding, symbol is in aux1 */

#define TYPE_NIL 0
#define TYPE_LIST 1
#define TYPE_SYMBOL 2
#define TYPE_BINDING 3

#define LIST(v) ((struct type_list *) SGGC_DATA(v))
#define SYMBOL(v) ((struct type_symbol *) SGGC_DATA(v))
#define BINDING(v) ((struct type_binding *) SGGC_DATA(v))
#define BOUND_SYMBOL(v) ((char *) SGGC_AUX1(v))


/* GLOBAL VARIABLES. */

ptr_t nil;


/* FUNCTIONS THAT THE APPLICATION NEEDS TO PROVIDE TO THE SGGC MODULE. */

sggc_kind_t sggc_kind (sggc_type_t type, sggc_length_t length)
{
  return type;
}

sggc_nchunks_t sggc_nchunks (sggc_type_t type, sggc_length_t length)
{
  return 1;
}

char *sggc_aux1_read_only (sggc_kind_t kind)
{
  static const char spaces[SGGC_CHUNKS_IN_SMALL_SEGMENT] =
    "                                                                ";

  return kind == TYPE_BINDING ? NULL : (char *) spaces;
}

void sggc_find_root_ptrs (void)
{ 
}

void sggc_find_object_ptrs (sggc_cptr_t cptr)
{
  if (SGGC_TYPE(cptr) == TYPE_LIST)
  { if (!sggc_look_at (LIST(cptr)->head)) return;
    if (!sggc_look_at (LIST(cptr)->tail)) return;
  }

  else if (SGGC_TYPE(cptr) == TYPE_BINDING)
  { if (!sggc_look_at (BINDING(cptr)->value)) return;
    if (!sggc_look_at (BINDING(cptr)->next)) return;
  }
}


/* ALLOCATE FUNCTION FOR THIS APPLICATION.  Calls the garbage collector
   when necessary, or otherwise every 100th allocation, with every 500th
   being level 1, and every 2000th being level 2. */

static ptr_t alloc (sggc_type_t type)
{
  static unsigned alloc_count = 0;
  sggc_cptr_t a;

  /* Do optional garbage collections according to the scheme.  Do this first,
     not after allocating the object, which would then get collected! */

  alloc_count += 1;
  if (alloc_count % 100 == 0)
  { sggc_collect (alloc_count % 2000 == 0 ? 2 : alloc_count % 500 == 0 ? 1 : 0);
  }

  /* Try to allocate object, calling garbage collector if this initially
     fails. */

  a = sggc_alloc(type,1); /* length argument is ignored */
  if (a == SGGC_NO_OBJECT)
  { sggc_collect(2);
    a = sggc_alloc(type,1);
    if (a == SGGC_NO_OBJECT)
    { printf("CAN'T ALLOCATE\n");
      abort();
      exit(1);
    }
  }

  /* Initialize the object if it contains pointers. */

  if (type == TYPE_LIST)
  { LIST(a)->head = nil;
    LIST(a)->tail = nil;
  }
  else if (type == TYPE_BINDING)
  { BINDING(a)->value = nil;
    BINDING(a)->next = nil;
  }

  return a;
}


int main (void)
{
  return 0;
}
