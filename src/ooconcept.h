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
 *   ooconcept.h
 *   OOmnik Concept class
 */

#ifndef OOCONCEPT_H
#define OOCONCEPT_H

#include <db.h>
#include <libxml/parser.h>

#include "ooconfig.h"

struct ooMindMap;

typedef enum logic_opers { LOGIC_AND, LOGIC_OR } logic_opers;

typedef enum conc_type { ATOM,     /* atomic system unit: byte */
                         STATIC,   /* entity */
                         DYNAMIC,  /* method */
                         GROUP,      /* AND  */
                         SELECT_ONE, /* OR */
                         SELECT_REST /* NOT */
                       } conc_type;

typedef enum oper_type { NONE, IS_SUBCLASS, AGGREGATES, 
                         HAS_ATTR, 
                         TAKES_ARG, DENOTES, RUNS, 
			 PRECEEDS, NEXT_IN_GROUP,
			 START_REVERSE_OPERS,
			 SUBCLASSED_BY, CONSTITUTES, IS_ATTR_OF, IS_ARG, DENOTED_BY, 
			 FOLLOWS, PREV_IN_GROUP,
			 IS_RUN, NUM_OPERS } oper_type;

typedef enum linear_type { ANYPOS, PREPOS, POSTPOS } linear_type;

/* Path:
   a list of points to get from to the destination concept */
typedef struct ooPath {
    grade_t prob;
    attr_size_t *points;
    size_t num_points;
} ooPath;


/* Concept Attribute aka Feature */
typedef struct ooAttr {
    oper_type operid;

    /* child concept id for lazy evaluation:
       type and value pointers are NULL
     */
    mindmap_size_t concid;
    bool is_complete;

    /* attribute type Concept:
     * a BluePrint Concept that defines the range of possible values */
    struct ooConcept *baseclass;

    /* value:
     * a subclass of a BluePrint Concept */
    struct ooConcept *value;

    /* is it affirmed or denied? */
    bool is_affirmed;

    /* the level of likelyhood of presence of this attribute
       in the concept's instance:
       this value varies reflecting the personal experience
       that depends on empirical evidence and opinions of others */
    grade_t relevance;
} ooAttr;


/* destination guideline */
typedef struct ooGuide {
    /* child concept that leads to the destination */
    mindmap_size_t concid;
    /* path length */
    mindmap_size_t len;
} ooGuide;

/* Reference Node */
typedef struct ooRefNode {
    mindmap_size_t concid;     /* final destination */
    /* the payload: a list of direct child concepts 
     * that all lead to destination */
    struct ooGuide *guides;
    size_t num_guides;
    struct ooRefNode *next;
} ooRefNode;

/* List of References */
typedef struct ooRefList { 
    struct ooRefNode *first;
} ooRefList;


/** A Concept is a named Container of Attributes:
 *  a piece of memory that is shared 
 *  by many Concept Units (aka instances)
 *  for the purposes of effective storage of common features 
 */
typedef struct ooConcept {
    conc_type type;
    mindmap_size_t id;

    /* unique name representation in atomic codes */
    char *name;
    size_t name_size;  /* for packing */

    /* any comments, explanations, examples of usage etc. go here */
    char *description;

    /** packed binary string representation 
     * of the concept for the DB storage 
     */
    char *bytecode;
    size_t bytecode_size;

    /*****************
     * public methods 
     *****************/
    int (*init)(struct ooConcept *self);
    int (*del)(struct ooConcept *self);
    const char* (*str)(struct ooConcept *self);

    int (*read)(struct ooConcept *self, 
		xmlNode *input_node);

    int (*resolve_refs)(struct ooConcept *self, 
			struct ooMindMap *mindmap);

    /* read your attrs from the MindMap DB:
       perform lazy evaluation,
       ie. expand the immediate attributes only! */
    int (*unpack)(struct ooConcept *self);

    /* serialize yourself into a binary string:
       calculate the size of resulting string in bytes */
    int (*pack)(struct ooConcept *self, pack_type pt);

    /* save yourself and your children in a MindMap DB */
    int (*put)(struct ooConcept *self, DB *dbp);

    /*** attribute management ***/

    int (*newattr)(struct ooConcept *self,  
                          oper_type  operid,
		   struct ooConcept *conc, 
                               bool  is_affirmed,
                     unsigned short  relevance);

    int (*delattr)( struct ooConcept *self,  
                      mindmap_size_t  concid);

    int (*setattr)(struct ooConcept *self, 
                     mindmap_size_t  concid, 
                   struct ooConcept *value);

    struct ooAttr* (*getattr)(struct ooConcept *self,
		         mindmap_size_t  concid);

    struct ooGuide* (*lookup)(struct ooConcept *self,
                                mindmap_size_t  concid);

    /***********************
     * private attributes
     ***********************/

    bool _is_modified;     /**< tracking changes */
    bool _is_visited;      /**< avoiding loops */

    /* Concept Attributes: 
       array of pointers to ooAttr */
    struct ooAttr **_attrs;
    attr_size_t  _num_attrs;

    /* Concept Timeline for methods: 
       "panta rei, panta xorei, ouden menei..."  */
    struct ooTimeline *_timeline;

    /* TODO: shall I keep track of my instances ?*/

    /* a hybrid search engine for instances and nested attributes:
     *  1. simple RefList to hold no more 
     *     than LINEAR_SEARCH_ITEMS_MAX values,
     *  2. BTree balanced search index
     */

    struct ooSearchEngine *_search;

} ooConcept;

/** \fn
 * Allocate memory and initialize an instance of an ooConcept
 * @param self a double pointer used to assign
 *             an external reference
 */
extern int ooConcept_init(struct ooConcept **self);

#endif

