/**
 *   Copyright (c) 2011 by Dima Dmitriev <http://www.oomnik.ru>
 *
 *   This file is part of the OOmnik Conceptual Processor, 
 *   and as such it is subject to the license stated
 *   in the LICENSE file which you have received 
 *   as part of this distribution.
 *
 *   oodict.h
 *   OOmnik Dictionary
 */

#ifndef OODICT_H
#define OODICT_H

#include "ooconfig.h"
#include "ooarray.h"

typedef size_t (*oo_hash_func)(const char *key);

typedef struct ooDictItem
{
    char *key;
    void *data;

} ooDictItem;

typedef struct ooDict
{
    /******** public attributes ********/
    int (*init)(struct ooDict *self);
    int (*del)(struct ooDict *self);

    /* get data */
    void* (*get)(struct ooDict *self,
                 const char *key);
    /*
     * set data
     * return true, if key already exists, false otherwise
     */
    int (*set)(struct ooDict *self,
                const char *key,
                void *data);

    /* if exists */
    bool (*key_exists)(struct ooDict *self,
                       const char    *key);
    /* delete */
    int (*remove)(struct ooDict *self,
                    const char *key);

    /* Resize the hash */
    int (*resize)(struct ooDict *self,
		  unsigned int new_size);

    /* set hash function */
    oo_hash_func (*set_hash)(struct ooDict *self,
			     oo_hash_func new_hash);

    oo_compar_func (*set_compare)(struct ooDict *self,
                                  oo_compar_func new_compar_func);

    /******** private attributes ********/

    struct ooArray *hash;

    oo_hash_func hash_func;

} ooDict;


/* constructor */
extern int ooDict_new(struct ooDict **self);

#endif /* OODICT_H */
