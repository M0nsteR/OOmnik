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
 *   ---------
 *   oocache.h
 *   OOmnik Linear Cache
 */

#ifndef OO_CACHE_H
#define OO_CACHE_H

#include "ooconfig.h"

#include "oosegmentizer.h"
#include "oocode.h"
#include "oodict.h"


/** specifies a matching reference to 
 * the code with particular context 
 */
typedef struct ooCodeMatch {
    struct ooCode *code;
    struct ooAdaptContext *context;
    struct ooCodeMatch *next;
} ooCodeMatch;


/* Cache Tail */
typedef struct ooLinearCacheTail {
    size_t *units;
    size_t num_units;
    size_t coverage;

    struct ooCodeMatch *code_match;
} ooTail;

/* Cache Cell: Matrix atom */
typedef struct ooLinearCacheCell {
    struct ooLinearCacheTail **tails;
    size_t num_tails;
    size_t max_tail_len;
} ooLinearCacheCell;


/**
 * OOmnik Linear Cache object stores the linear sequences of codes
 * mapped directly to their denotations
 */
typedef struct ooLinearCache {

    struct ooCodeSystem *cs;

    char *provider_name;
    struct ooCodeSystem *provider;

    struct ooLinearCacheCell **matrix;
    size_t matrix_size;
    size_t matrix_depth;
    size_t max_unrec_chars;

    size_t num_cells;
    bool trust_separators;

    size_t *row_sizes;

    struct ooDict *codes;
    struct ooDict *code_list_sizes;
    unsigned char **codeseqs;
    size_t num_codes;

    /* string representation */
    char *repr;

    /***********  public methods ***********/
    int (*del)(struct ooLinearCache *self);
    const char* (*str)(struct ooLinearCache *self);

    /* add a new item to cache */
    int (*set)(struct ooLinearCache *self, 
	       struct ooCode *code);

    /* allocate memory for the matrix  */
    int (*build_matrix)(struct ooLinearCache *self);

    /* insert real values into matrix */
    int (*populate_matrix)(struct ooLinearCache *self);

    /* ask Cache about the meaning 
     * of a linear sequence of concepts */
    int (*lookup)(struct ooLinearCache *self, 
		  struct ooSegmentizer *segm,
		  ooATOM *input,
		  size_t input_len,
		  size_t batch_id,
		  struct ooAgenda *agenda);

} ooLinearCache;


extern int ooLinearCache_new(struct ooLinearCache **self); 

#endif
