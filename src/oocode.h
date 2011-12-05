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
 *   --------
 *   oocode.h
 *   OOmnik Code
 */

#ifndef OO_CODE_H
#define OO_CODE_H

#include "ooconfig.h"

#include "ooconcept.h"
#include "ooconstraint.h"
#include "ooconcunit.h"
#include "oocodesystem.h"

typedef enum code_type { CODE_NONE,
                         CODE_STATIC,
                         CODE_ROLE_MARKER,
			 CODE_GROUP_MARKER, 
			 CODE_RELATION_MARKER, 
			 CODE_ALTERN_MARKER, 
			 CODE_SEPARATOR } code_type;

/* forward declaration */
struct ooCode;

typedef struct ooCodeDeriv {
    char *name;
    char *usage_name;
    struct ooCode *code;
    struct ooCodeUsage *code_usage;

    /* topic or spec? */
    bool used_as_topic;
    oo_oper_type operid;

    char *arg_code_name;
    char *arg_code_usage_name;
    struct ooCode *arg_code;
    struct ooCodeUsage *arg_code_usage;

    /* for search indices */
    struct ooCodeDeriv *next;

    int (*del)(struct ooCodeDeriv *self);
    const char* (*str)(struct ooCodeDeriv *self);
    

} ooCodeDeriv;

typedef struct ooCodeUsage {

    char *name;
    char *conc_name;
    struct ooConcept *conc;

    struct ooCodeUsage *parent;
    struct ooCode *code;

    /* TODO: point of similarity */

    /* nested usages */
    struct ooCodeUsage **usages;
    size_t num_usages;

    struct ooCodeDeriv **derivs;
    size_t num_derivs;

    int (*del)(struct ooCodeUsage *self);
    const char* (*str)(struct ooCodeUsage *self);

} ooCodeUsage;


typedef struct ooAdaptContext {
    struct ooConstraintGroup *affects_prepos;
    struct ooConstraintGroup *affects_postpos;

    struct ooConstraintGroup *affected_prepos;
    struct ooConstraintGroup *affected_postpos;

} ooAdaptContext;



typedef struct ooCodeCache {
    char **seqs;
    size_t num_seqs;

    /* each sequence of cached terminals 
     * is valid in its context only */
    struct ooAdaptContext **contexts;

} ooCodeCache;


/* Code Specifier  */
typedef struct ooCodeSpec {
    oo_oper_type operid;
    mindmap_size_t concid;
    
    char *code_name;
    struct ooCode *code;
    
    linear_type linear_order;
    bool linear_contact;
    bool stackable;
    bool implied_parent;

    /* logic of the list */
    logic_opers group_logic;
    struct ooCodeSpec *next;

    /* inner lists */
    struct ooCodeSpec *specs;
  
} ooCodeSpec;


typedef struct ooCode {
    size_t id;
    char *name;
    code_type type;

    struct ooCodeSystem *cs;
  
    /* char *concref;
      char *gloss;
      char *semclass; */

    /* inherit syntactic properties from the baseclass */
    char *baseclass_name;
    struct ooCode *baseclass;

    /* human verification of code's correctness */
    int verif_level;

    struct ooCodeCache *cache;
    bool is_cached;

    /* semantic usages */
    struct ooCodeUsage **usages;
    size_t num_usages;

    /* next level code denotation */
    struct ooCode **denots;
    char **denot_names;
    size_t num_denots;

    /* automatic implications */
    struct ooCode **implied_codes;
    char **implied_code_names;
    size_t num_implied_codes;

    /* shared code */
    struct ooCodeUnit *shared;

    /* syntax */
    struct ooCodeSpec *parents[OO_NUM_OPERS];
    size_t num_parents;

    struct ooCodeSpec *children[OO_NUM_OPERS];
    logic_opers spec_group_logic;
    size_t num_children;

    /* units that can be grouped */
    bool allows_grouping;

    bool stackable;

    /* group closing markers */
    bool closes_group;

    /* linear delimiters like (), {} etc. */
    bool has_linear_delimiters;

    /* derivation search engine:
     * lookup by oper_id and code reference 
     */
    struct ooCodeDeriv *deriv_matches[OO_NUM_OPERS];

    /***********  public methods ***********/
    int (*del)(struct ooCode *self);
    const char* (*str)(struct ooCode *self);

    /* read the XML data */
    int (*read)(struct ooCode *self, xmlNode *node);

    /* resolve references */
    int (*resolve_refs)(struct ooCode *self);

} ooCode;

typedef struct ooCodeUnitSpec {
    oo_oper_type operid;
    struct ooCodeUnit *unit;
} ooCodeUnitSpec;

typedef struct ooCodeUnit {
    struct ooCode *code;
    struct ooCodeUnitSpec **specs;
    size_t num_specs;

} ooCodeUnit;

extern int ooCode_new(struct ooCode **self); 
#endif
