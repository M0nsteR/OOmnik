/**
 *   Copyright (c) 2011 by Dima Dmitriev <http://www.oomnik.ru>
 *
 *   This file is part of the OOmnik Conceptual Processor, 
 *   and as such it is subject to the license stated
 *   in the LICENSE file which you have received 
 *   as part of this distribution.
 *
 *   ooarray.h
 *   OOmnik Dynamic Array 
 */

#ifndef OOARRAY_H
#define OOARRAY_H

#include "ooconfig.h"

typedef struct ooArray
{
    /******** public attributes ********/

    /* constructor */
    int (*init)(struct ooArray *self);

    /* destructor */
    int (*del)(struct ooArray *self);

    /* add a value at specific position */
    void (*add)(struct ooArray *self,
                void           *val,
                size_t        pos);

    /* add a value to the end of the array */
    void (*push)(struct ooArray *self,
                 void           *val);

    /* delete an item */
    int (*remove)(struct ooArray *self,
		  size_t pos);

    /* delete the last value */
    void* (*pop)(struct ooArray *self);

    /* set the item of the array (check the position) */
    void* (*set_item)(struct ooArray *self,
                      void *data,
                      size_t pos);

    /* get the element of array (check the position) */
    void* (*get_item)(struct ooArray *self,
                      size_t pos);

    /* delete all items */
    void (*clear)(struct ooArray *self);

    /* returns the subarray (copy the part of the array) */
    struct ooArray* (*get_subsequence)(struct ooArray *self,
                                       size_t start,
                                       size_t length);
    /* set the new size */
    void (*resize)(struct ooArray *self,
                   size_t        new_size);

    /* sort the array */
    void (*sort)(struct ooArray *self,
                 oo_compar_func  function);

    /* find the element */
    void* (*find)(struct ooArray *self,
                  void *data,
                  oo_compar_func function);

    /******** private attributes ********/

    /* pointers to the data */
    void **data;

    /* array size */
    size_t size;

} ooArray;

/* array constructor */
int ooArray_new(struct ooArray **self);

#endif /* OOARRAY_H */
