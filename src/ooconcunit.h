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
 *   ------------
 *   ooconcunit.h
 *   OOmnik Concept Unit
 */

#ifndef OO_CONCUNIT_H
#define OO_CONCUNIT_H

#include "ooconcept.h"
#include "oocode.h"
#include "oocomplex.h"
#include "ooconfig.h"

/* Concept Unit: storage of computed attributes 
   and hypothetical operations */

typedef struct ooConcUnit {
    size_t cs_id;
    mindmap_size_t concid;

    /* linear position in the sequence of atoms */
    size_t linear_pos;
    bool is_present;
    bool is_implied;

    /* number of atoms covered by this unit */
    size_t coverage;

    /* reference to Code Class */
    struct ooCode *code;

    /* reference to an operation that produced this concept value */
    struct ooComplex *oper;

    /* adaptation context */
    struct ooAdaptContext *context;

    /* complex units */
    struct ooConcUnit *fixed_parent;
    oo_oper_type fixed_operid;

    /* reference to terminal units */
    struct ooConcUnit *terminals;
    size_t start_term_pos;
    size_t num_terminals;

    /* contains inner breaks */
    bool is_sparse;

    /* syntax groupings */
    bool is_group;
    bool group_closed;
    struct ooConcUnit *logic_oper;

    /* reference to common CU for siblings 
     * (=units sharing the same atomic coverage) */
    struct ooConcUnit *common_unit;

    /* main storage and handler */
    struct ooAgenda *agenda;

    /* recursion level */
    size_t recursion_level;

    /* syntax solutions sorted by weight:
     * max weight -> min good enough weight
     */
    struct ooComplex complex_storage[CONCUNIT_COMPLEX_POOL_SIZE];
    struct ooComplex *complexes[CONCUNIT_COMPLEX_POOL_SIZE];
    struct ooComplex *top_complexes[CONCUNIT_COMPLEX_POOL_SIZE];
    size_t num_complexes;
    size_t top_count;

    /* for lists */
    struct ooConcUnit *next;
    struct ooConcUnit *prev;

    struct ooConcUnit *next_alloc;

    /***********  public methods ***********/
    int (*str)(struct ooConcUnit *self,
		    size_t depth);
    int (*reset)(struct ooConcUnit *self);

    int (*present)(struct ooConcUnit *self, 
		   char *buf,
		   size_t *chunk_size,
		   const char *input,
		   size_t start_pos,
		   output_type format);

    int (*make_instance)(struct ooConcUnit *self, 
			 struct ooCode *myclass,
			 struct ooConcUnit *terminal);

    /* establish links with your peers and parents */
    int (*link)(struct ooConcUnit *self);

    /* adding complexes from child */
    int (*add_children)(struct ooConcUnit *self,
			struct ooConcUnit *child,
			struct ooCodeSpec *spec);
} ooConcUnit;

extern int ooConcUnit_init(struct ooConcUnit *self);

#endif /* OO_CONCUNIT_H */
