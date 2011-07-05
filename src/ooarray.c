/**
 *   Copyright (c) 2011 by Dmitri Dmitriev
 *   All rights reserved.
 *
 *   This file is part of the OOmnik Conceptual Processor, 
 *   and as such it is subject to the license stated
 *   in the LICENSE file which you have received 
 *   as part of this distribution.
 *
 *   Project homepage:
 *   <http://www.oomnik.ru>
 *
 *   Initial author and maintainer:
 *         Dmitri Dmitriev aka M0nsteR <dmitri@globbie.net>
 *
 *   Code contributors:
 *         * Jenia Krylov <info@e-krylov.ru>
 *
 *   ---------
 *   ooarray.c
 *   OOmnik Dynamic Array implementation
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "ooarray.h"

static oo_compar_func local_function;

int compare_array_items(const void *a1,
                        const void *a2)
{
    const void **item_1, **item_2;
    item_1 = (const void**)a1;
    item_2 = (const void**)a2;
    return local_function(*item_1, *item_2);
}


static int 
ooArray_del(struct ooArray *self)
{
    self->clear(self);
    return oo_OK;
}

static int
ooArray_set_item(struct ooArray *self,
		 void    *data,
		 size_t pos)
{
    if (pos >= self->size)
        return oo_FAIL;

    self->data[pos] = data;
    return oo_OK;
}

static
void* ooArray_get_item(struct ooArray *self,
                       size_t pos)
{
    if (pos >= self->size)
        return NULL;

    return self->data[pos];
}

static int
ooArray_clear(struct ooArray *self)
{
    free(self->data);

    self->data  = NULL;
    self->size = 0;
    return oo_OK;
}

static int
ooArray_add(struct ooArray *self,
	    void    *val,
	    size_t pos)
{
    unsigned int i;
    void **data;

    if (pos >= self->size)
        return NULL;

    data = (void**)realloc(self->data, 
			   sizeof(void*) * (self->size + 1));

    if (!data) return NULL;

    for (i = pos + 1; i < self->size + 1; i++)
        data[i] = data[i - 1];

    data[pos] = val;

    self->data = data;
    self->size++;

    return oo_OK;
}

static
void  ooArray_push(struct ooArray *self,
                   void    *val)
{
    ooArray_add(self, val, self->size);
}

static
struct ooArray* 
ooArray_get_subsequence(struct ooArray *self,
			size_t start,
			size_t length)
{
    int ret;

    struct ooArray *res;
    ret = ooArray_new(&res);
    if (ret != oo_OK) return ret;

    if (start > length)
        return res;

    if (start + length > self->size)
        length = self->size - start;


    res->data = (void**)malloc(sizeof(void*) * length);
    memcpy(res->data, self->data + start, sizeof(void*) * length);
    res->size = length;

    return res;
}


static int
ooArray_remove(struct ooArray *self,
                     size_t pos)
{
    unsigned int i;
    void *res;

    if (pos >= self->size)
        return oo_FAIL;

    self->size--;

    res = self->data[pos];
    for (i = pos; i < self->size; ++i)
        self->data[i] = self->data[i + 1];

    self->data = (void**)realloc(self->data, sizeof(void*) * self->size);

    return oo_OK;
}

static
void* 
ooArray_pop(struct ooArray *self)
{
    return ooArray_remove(self, self->size - 1);
}

static void 
ooArray_resize(struct ooArray *self,
	       size_t new_size)
{
    unsigned int size = self->size;
    self->size = new_size;
    self->data  = (void**)realloc(self->data, self->size * sizeof(void*));
    if (size < new_size)
        memset(self->data + size, 0, (new_size - size) * sizeof(void*));
}

static void 
ooArray_sort(struct ooArray *self,
	     oo_compar_func function)
{
    local_function = function;
    qsort(self->data, self->size, sizeof(void*), compare_array_items);
}

static void* 
ooArray_find(struct ooArray *self,
	     void *data,
	     oo_compar_func function)
{
    size_t i;

    for (i = 0; i < self->size; ++i)
        if (function(data, self->data[i]) == 0)
            return self->data[i];

    return NULL;
}


static int 
ooArray_init(struct ooArray *self)
{
    self->del             = ooArray_del;
    self->add             = ooArray_add;
    self->push            = ooArray_push;
    self->remove          = ooArray_remove;
    self->pop             = ooArray_pop;
    self->set_item        = ooArray_set_item;
    self->get_item        = ooArray_get_item;
    self->clear           = ooArray_clear;
    self->get_subsequence = ooArray_get_subsequence;
    self->resize          = ooArray_resize;
    self->sort            = ooArray_sort;
    self->find            = ooArray_find;

    self->data  = NULL;
    self->size = 0;

    return oo_OK;
}


/* constructor */
extern int
ooArray_new(struct ooArray **array)
{
    struct ooArray *self = malloc(sizeof(struct ooArray));
    if (!self) return oo_NOMEM;

    ooArray_init(self);

    *array = self;
    return oo_OK;
}
