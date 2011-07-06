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
 *   ootopic.h
 *   OOmnik Decoder implementation
 */

#ifndef OO_DECODER_H
#define OO_DECODER_H

#include "ooconfig.h"

#include "oomnik.h"
#include "ooconcunit.h"
#include "ooaccumulator.h"
#include "ooagenda.h"

/**
 * OOmnik Decoder: a controller of decoding process
 * for a particular Coding System 
 */
typedef struct ooDecoder {

    /* main controller */
    struct OOmnik *oomnik;

    /* coding system */
    struct ooCodeSystem *codesystem;

    /* refs to upper decoders to help check the loops */
    struct ooDecoder **parents;
    size_t num_parents;

    /* looping references
     * as in: integer -> symbol -> expression -> integer */
    struct ooDecoder *loops;

    bool is_root;

    /* inheritors */
    struct ooDecoder **inheritors;
    size_t num_inheritors;

    /* solution accumulating memory */
    struct ooAccu *accu;

    /* solution operational memory */
    struct ooAgenda *agenda;

    /* producers of terminal concepts */
    struct ooSegmentizer *segm;
    struct ooSegmentizer *cache_segm;

    /* ratings */
    /* shows overall performance as compared to alternatives */
    int  performance;
    /* shall we use it any longer? */
    bool is_activated;

    /* current solution */
    struct ooConcUnit *solution;


    /* input atoms */
    ooATOM *input;
    size_t input_len;

    size_t task_id;

    size_t term_count;
    size_t num_parsed_atoms;
    size_t num_terminals;

    bool used_by_cache;
    bool is_atomic;

    output_type format;

    /***********  public methods ***********/
    int (*del)(struct ooDecoder *self);
    const char* (*str)(struct ooDecoder *self);

    int (*set_codesystem)(struct ooDecoder *self, struct ooCodeSystem *cs);

    /* main job */
    int (*process)(struct ooDecoder *self, const char *input);
    int (*decode)(struct ooDecoder *self);

} ooDecoder;

extern int ooDecoder_new(struct ooDecoder **self); 

#endif /* OO_DECODER_H */
