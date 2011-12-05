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
 *   --------------
 *   oocodesystem.h
 *   OOmnik Coding System 
 */

#ifndef OO_CODESYSTEM_H
#define OO_CODESYSTEM_H

#include <libxml/parser.h>

#include "ooconcept.h"
#include "oodict.h"

struct ooMindMap;

typedef enum codesystem_t { CS_DENOTATIONAL, 
			    CS_OPERATIONAL, 
			    CS_COMPLEX, 
			    CS_POSITIONAL } codesystem_t;

typedef enum atomic_codesystem_t  { ATOMIC_UTF8, 
				    ATOMIC_UTF16, 
				    ATOMIC_SINGLEBYTE } atomic_codesystem_t;


typedef struct ooConstraintType {
    char *name;
    char **values;
    size_t num_values;
} ooConstraintType;

typedef struct ooCodeSystem {

    codesystem_t type;
    char *name;
    size_t id;

    const char **operids;

    /* using direct coding by atomic bytes? */
    bool is_atomic;
    atomic_codesystem_t atomic_codesystem_type;

    bool allows_polysemy;

    bool use_visual_separators;

    char *root_elem_name;
    size_t root_elem_id;

    /* main storage */
    struct ooMindMap *mindmap;

    /* name of the XML source file */
    const char *filename;

    /* providers */
    char **provider_names;
    struct ooCodeSystem **providers;
    logic_opers providers_logic_oper;
    int num_providers;

    /* inheritors */
    char **inheritor_names;
    struct ooCodeSystem **inheritors;
    int num_inheritors;
  
    bool is_coordinated;

    /* concept storage by name */
    struct ooDict *codes;
    char **code_names;

    /* in-memory main index of codes
     * key:   code id
     * value: code object
     */
    struct ooCode **code_index;
    size_t code_index_capacity;
    size_t num_codes;

    /* for direct handling of Unicode or ASCII integers */
    NUMERIC_CODE_TYPE *numeric_denotmap;
    bool use_numeric_codes;

    /* TODO: hybrid (in-memory cache + persistence) 
       storage of complex denotations */

    /* cache of linear sequences */
    struct ooLinearCache *cache;

    struct ooConstraintType **constraints;
    size_t num_constraints;

    /* read the XML data */
    int (*read)(struct ooCodeSystem *self, xmlNode *node);

    int (*del)(struct ooCodeSystem *self);

    /* find the denotations of a Concept */
    int (*lookup)(struct ooCodeSystem *self, 
		  struct ooConcept *conc);

    int (*coordinate_codesets)(struct ooCodeSystem *self, 
			       struct ooCodeSystem *cs);

    int (*build_cache)(struct ooCodeSystem *self);

    int (*resolve_refs)(struct ooCodeSystem *self);

} ooCodeSystem;

extern int ooCodeSystem_new(struct ooCodeSystem **self); 


#endif /* OO_CODESYSTEM_H */
