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
 *   -----------
 *   oocomplex.h
 *   OOmnik Complex Concept
 */

#ifndef OOCOMPLEX_H
#define OOCOMPLEX_H

#include "ooconstraint.h"

/* forward declarations */
struct ooCodeSpec;
struct ooAccu;
struct ooConcept;
struct ooConcUnit;

typedef struct ooInterp {
    struct ooConcept *conc;
    int weight;

} ooInterp;


typedef struct ooComplexContext {
    struct ooConstraintGroup *affects_prepos;
    struct ooConstraintGroup *affects_postpos;

    struct ooConstraintGroup *affected_prepos;
    struct ooConstraintGroup *affected_postpos;

} ooComplexContext;


/* a single syntactic solution */
typedef struct ooComplex {

    /* concept unit to be extended */
    struct ooConcUnit *base;

    /* resulting concept unit */
    struct ooConcUnit *aggregate;

    bool is_terminal;
    bool is_aggregate;

    /* concept instance */
    /*struct ooConcInstance interp_storage[INTERP_POOL_SIZE];
    struct ooConcInstance *interps[INTERP_POOL_SIZE];
    struct ooConcInstance *top_interps[INTERP_POOL_SIZE];*/

    struct ooInterp interp_storage[INTERP_POOL_SIZE];
    struct ooInterp *interps[INTERP_POOL_SIZE];
    size_t num_interps;
    size_t interp_top_count;

    /* major decision making indicator */
    int weight;

    /* linear index of _all_ subordinate descendants */
    struct ooComplex *linear_index[INPUT_BUF_SIZE];
    int linear_begin;
    int linear_end;
    size_t num_terminals;


    /* memoization of the previous solution */
    int prev_weight;
    struct ooComplex *prev_linear_index[INPUT_BUF_SIZE];
    int prev_linear_begin;
    int prev_linear_end;

    /* adaptation constraints */
    struct ooComplexContext adapt_context;
    int adapt_weight;

    /* linear delimiters: (), {} etc. */
    struct ooConcUnit *begin_delim;
    struct ooConcUnit *end_delim;

    /* fixed array of operations */
    struct ooComplex *specs[OO_NUM_OPERS];

    /* for indices */
    struct ooComplex *next;

    /* for lists */
    struct ooComplex *next_peer;

    /* status indicators */
    bool is_free;
    bool is_updated;

    /***********  public methods ***********/
    const char* (*str)(struct ooComplex *self, 
   		       size_t depth);

    int (*reset)(struct ooComplex *self);

    int (*present)(struct ooComplex *self,
		   struct ooAccu *accu,
		   output_type format);

    int (*join)(struct ooComplex *self,
	        struct ooComplex *child,
	        struct ooComplex *result,
	        struct ooCodeSpec *spec);

    int (*forget)(struct ooComplex *self, 
		  struct ooComplex *prev,
		  struct ooCodeSpec *spec);

  /*int (*find_solution)(struct ooComplex *self, 
    struct ooComplex **linear_index);*/
} ooComplex;


extern int ooComplex_init(struct ooComplex *self); 

#endif /* OOCOMPLEX_H */
