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
 *   ----------
 *   ooagenda.h
 *   OOmnik Agenda
 */

#ifndef OO_AGENDA_H
#define OO_AGENDA_H


#include "ooconcunit.h"
#include "ooconfig.h"

typedef enum agenda_t { AGENDA_LINEAR, 
			AGENDA_OPERATIONAL, 
			AGENDA_POSITIONAL } agenda_t;


/** 
 *  Solution Agenda:
 *  operational short-term memory 
 */
typedef struct ooAgenda {

    /* codes and rules */
    struct ooCodeSystem *codesystem;

    /* main accumulating storage */
    struct ooAccu *accu;

    agenda_t type;

    /* semantics of cell indices */
    bool linear_structure;

    /** concept unit storage: a fixed pool of memory */
    struct ooConcUnit storage[AGENDA_CONCUNIT_STORAGE_SIZE];
    size_t storage_space_used;

    /** operational pool of complexes */
    struct ooConcUnit complexes[AGENDA_COMPLEX_POOL_SIZE];
    size_t num_complexes;

    /* concept unit index */
    struct ooConcUnit *index[AGENDA_INDEX_SIZE];
    struct ooConcUnit *tail_index[AGENDA_INDEX_SIZE];
    size_t last_idx_pos;

    /* complex ratings */
    /*struct ooComplex **complex_rating[AGENDA_COMPLEX_RATING_SIZE];
      size_t complex_rating_size;*/

    /* all possible combinations */
    struct ooComplex *linear_index[INPUT_BUF_SIZE];
    size_t linear_index_size;

    /* a single solution */
    struct ooComplex *best_complex;

    /* indices for another types of codes */
    struct ooConcUnit *delimiter_index[INPUT_BUF_SIZE];
    struct ooConcUnit *logic_oper_index[INPUT_BUF_SIZE];

    /* positional tail */
    struct ooConcUnit *linear_last;

    bool has_garbage;

    /***********  public methods ***********/
    int (*del)(struct ooAgenda *self);
    int (*str)(struct ooAgenda *self);

    int (*reset)(struct ooAgenda *self);

    struct ooConcUnit* (*alloc_unit)(struct ooAgenda *self);

    int (*register_unit)(struct ooAgenda *self, 
			 struct ooConcUnit *cu,
			 struct ooCode *code);

    int (*update)(struct ooAgenda *self, 
		  struct ooAgenda *segm_agenda);

    /* present solution */
    int (*present_solution)(struct ooAgenda *self,
			    struct ooComplex *c, 
			    int linear_begin,
			    int linear_end);

    /* private attrs */
} ooAgenda;

extern int ooAgenda_new(struct ooAgenda **self); 

#endif /* OO_AGENDA_H */
