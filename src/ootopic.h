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
 *   OOmnik Topic 
 */

#ifndef OO_TOPIC_H
#define OO_TOPIC_H

#include "ooconfig.h"

struct ooTopic;
struct ooMindMap;

/** Topic Ingredient:
 *  a set of concepts
 */
typedef struct ooTopicIngredient {
    size_t id;
    char *name;
    struct ooTopic *topic;
    struct ooConcept *conc;
    float relevance;
    float complexity;

    struct ooTopicIngredient *next;

} ooTopicIngredient;


/** Topic:
 *  a set of conceptual ingredients
 *  that allows further subdivision.
 */
typedef struct ooTopic {
    size_t id;
    char *name;

    struct ooTopicIngredient **ingredients;
    size_t num_ingredients;

    struct ooTopic **topics;
    size_t num_topics;

    /***********  public methods ***********/
    int (*del)(struct ooTopic *self);
    int (*str)(struct ooTopic *self);
    int (*read)(struct ooTopic *self,
		xmlNode *input_node);
    int (*resolve_refs)(struct ooTopic *self,
			struct ooMindMap *mindmap);
} ooTopic;


/** 
 * Topic Solution
 */
struct ooTopicSolution {

    struct ooTopic *topic;

    float ingredients[NUM_TOPIC_INGREDIENTS];

    float weight;

    /***********  public methods ***********/
    int (*del)(struct ooTopicSolution *self);
    int (*str)(struct ooTopicSolution *self);
    int (*present)(struct ooTopicSolution *self, char *buf);
};


extern int ooTopic_new(struct ooTopic **self);

extern int ooTopicSolution_init(struct ooTopicSolution *self);

#endif /* OO_TOPIC_H */
