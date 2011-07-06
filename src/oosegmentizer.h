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
 *   oosegmentizer.h
 *   OOmnik Segmentizer
 */

#ifndef OO_SEGMENTIZER_H
#define OO_SEGMENTIZER_H

#include "ooconcunit.h"
#include "oodecoder.h"
#include "ooagenda.h"
#include "ooconfig.h"

typedef struct ooSegmentizer {

    /* parent decoder */
    struct ooDecoder *parent_decoder;

    /* providers */
    struct ooDecoder **decoders;
    int num_decoders;
    logic_opers decoders_logic_oper;

    /* segmentation solutions */
    struct ooAgenda *agenda;

    /* terms encoded by atoms (ie. bytes)? */
    bool is_atomic;

    /* array of atomic decoding functions */
    int (**atomic_decoders)(struct ooSegmentizer *self);
    int num_atomic_decoders;
    int pref_atomic_decoder;

    int next_solution_id;
    int num_solutions;

    /* input atoms */
    ooATOM *input;
    size_t input_len;
    size_t task_id;

    size_t num_parsed_atoms;
    size_t num_terminals;

    /***********  public methods ***********/
    int (*del)(struct ooSegmentizer *self);
    int (*str)(struct ooSegmentizer *self);
    int (*reset)(struct ooSegmentizer *self);

    int (*add_decoder)(struct ooSegmentizer *self, struct ooDecoder *dec);

    /* main job: build a maze of segments */
    int (*segmentize)(struct ooSegmentizer *self);
} ooSegmentizer;

extern int ooSegmentizer_new(struct ooSegmentizer **self); 

#endif /* OO_SEGMENTIZER_H */
