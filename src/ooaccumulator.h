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
 *   ---------------
 *   ooaccumulator.h
 *   OOmnik Accumulator
 */

#ifndef OO_ACCUMULATOR_H
#define OO_ACCUMULATOR_H


#include "ooconcunit.h"
#include "ooconfig.h"

typedef enum accu_t { ACCU_LINEAR, ACCU_OPERATIONAL, ACCU_POSITIONAL } accu_t;


/** 
 *  Accumulator:
 *  storage of the aggregate solution
 */
typedef struct ooAccu {

    /* parent */
    struct ooDecoder *decoder;

    accu_t type;

    /* semantics of cell indices */
    bool linear_structure;

    const char *header;
    const char *footer;

    /* amount of long term memory for the output */
    char *output_buf;
    char *curr_output_buf;
    size_t output_total_space;
    size_t output_free_space;
  
    /* topic solutions */
    struct ooTopicSolution topic_solution_storage[TOPIC_POOL_SIZE];
    struct ooTopicSolution *topic_solutions[TOPIC_POOL_SIZE];
    struct ooTopicSolution *topic_rating[TOPIC_POOL_SIZE];
    struct ooTopicSolution **topic_index;
    size_t max_num_topics;
    size_t num_topic_solutions;
    size_t rating_count;

    /* temp variables */
    const char *solution;
    bool begin_table;
    bool begin_row;
    bool begin_cell;

    /***********  public methods ***********/
    int (*del)(struct ooAccu *self);
    int (*str)(struct ooAccu *self);
    int (*reset)(struct ooAccu *self);

    int (*build_topic_index)(struct ooAccu *self, 
			     struct ooMindMap *mindmap);

    int (*update_topic)(struct ooAccu *self, 
			struct ooTopicIngredient *ingr,
			struct ooComplex *complex);

    int (*present_solution)(struct ooAccu *self,
			    char *buf, 
			    size_t buf_size);


    int (*update)(struct ooAccu *self, struct ooAgenda *agenda);

    int (*append)(struct ooAccu *self,
		  const char *buf,
		  size_t buf_size);
} ooAccu;

extern int ooAccu_new(struct ooAccu **self); 

#endif /* OO_ACCUMULATOR_H */
