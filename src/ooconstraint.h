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
 *   ooconstraint.h
 *   OOmnik  ConstraintGroup
 */

#ifndef OO_CONSTRAINT_H
#define OO_CONSTRAINT_H

#include <libxml/parser.h>

#include "oocodesystem.h"
#include "ooconfig.h"

/*typedef enum logic_opers { LOGIC_AND, LOGIC_OR } logic_opers;*/
typedef enum adaptation_roles { OO_AFFECTS, OO_AFFECTED } adaptation_roles;

typedef struct ooConstraint {
    int constraint_type;
    unsigned int weight;
    int value;

    struct ooConstraint *next;
} ooConstraint;



/**
 *  A Group of Constraints:
 *  contains _one_ logical operator 
 *  that is applied to its operands
 */
typedef struct ooConstraintGroup
{
    bool is_affirmed;
    logic_opers logic_oper;

    struct ooConstraint *atomic_constraints;
    struct ooConstraintGroup *children;
    struct ooConstraintGroup *next;

    /***********  public methods ***********/
    int (*del)(struct ooConstraintGroup *self);
    const char* (*str)(struct ooConstraintGroup *self);

    int (*read)(struct ooConstraintGroup *self,
		struct ooCode *code,
		xmlNode *input_node);


} ooConstraintGroup;

extern int ooConstraintGroup_new(struct ooConstraintGroup **self); 
#endif
