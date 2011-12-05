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
 *   ooconstraint.c
 *   OOmnik  ConstraintGroup implementation
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <libxml/parser.h>

#include "ooconfig.h"
#include "ooconcunit.h"
#include "oocode.h"

/*  destructor */
static
int ooConstraintGroup_del(struct ooConstraintGroup *self)
{
    struct ooConstraintGroup *cg, *next_cg;
    struct ooConstraint *c, *next_c;

    c = self->atomic_constraints;
    while (c) {
	next_c = c->next;
	free(c);
	c = next_c;
    }

    cg = self->children;
    while (cg) {
	next_cg = cg;
	cg->del(cg);
	cg = next_cg->next;
    }

    cg = self->next;
    while (cg) {
	next_cg = cg;
	cg->del(cg);
	cg = next_cg->next;
    }

    free(self);
    return oo_OK;
}

/*
static int 
ooConstraintGroup_add_constraint(struct ooConstraintGroup *cg,
				     int constraint_type,
				     linear_type direction,
				     int weight,
				     int value)
{
    return oo_OK;
}
*/


static const char* 
ooConstraintGroup_str(struct ooConstraintGroup *self)
{
    struct ooConstraintGroup *peer, *child;
    struct ooConstraint *c;

    const char *logic_oper_name = "AND";

    if (self->logic_oper) logic_oper_name = "OR";

    printf("\n    Constraints (logic %s):\n", logic_oper_name);

    c = self->atomic_constraints;
    while (c) {
	printf("    -- atomic constraint: type: %d value: %d weight: %d\n", 
	       c->constraint_type, c->value, c->weight);
	c = c->next;
    }

    child = self->children;
    while (child != NULL) {
	printf("CHILD: ");
	child->str(child);
	child = child->next;
    }

    peer = self->next;
    while (peer != NULL) {
	printf("PEER: ");
	peer->str(peer);
	peer = peer->next;
    }
    return "";
}




static int
ooConstraint_read_atomic_constraint(struct ooConstraint *self,
				    struct ooCode *code,
				    xmlNode *input_node)
{
    xmlAttr *attr_node;
    struct ooConstraintType *ct = NULL;
    char *value = NULL;
    size_t i;

    self->constraint_type = -1;
    self->next = NULL;
    self->weight = 0;
    self->value = -1;

    for (attr_node = input_node->properties; attr_node; attr_node = attr_node->next) {
	if (attr_node->type != XML_ATTRIBUTE_NODE) continue;

	/*printf("ATTR: \"%s\" VALUE: \"%s\"\n",
	  attr_node->name, attr_node->children->content);*/

	if ((!xmlStrcmp(attr_node->name, (const xmlChar *)"weight")))
	    self->weight = atoi( (char*)attr_node->children->content);

	if ((!xmlStrcmp(attr_node->name, (const xmlChar *)"name"))) {
	    /* find out the constraint id */
	    for (i = 0; i < code->cs->num_constraints; i++) {
		ct = code->cs->constraints[i];
		if (!strcmp(ct->name, (char*)attr_node->children->content)) {
		    self->constraint_type = i;
		    break;
		} else ct = NULL;
	    }
	}

	if ((!xmlStrcmp(attr_node->name, (const xmlChar *)"value")))
	    value = (char*)attr_node->children->content;
    }

    /* the value string must find its numeric id */
    if (value && ct) {
	for (i = 0; i < ct->num_values; i++) {
	    if (!strcmp(value, ct->values[i]))
		self->value = i;
	}
    }

    /*if (self->value != -1) 
	printf("VALUE ID: %d\n", self->value);
    else
    printf(" VALUE WITH NO ID: %s\n", value); */

    return oo_OK;
}

static int
ooConstraintGroup_read_children(struct ooConstraintGroup *self,
			   struct ooCode *code,
			   xmlNode *input_node)
{
    xmlNode *cur_node;
    struct ooConstraintGroup *cg = NULL;
    struct ooConstraint *c = NULL;
    char *value = NULL;
    int ret;

    for (cur_node = input_node; cur_node; cur_node = cur_node->next) {
	if (cur_node->type != XML_ELEMENT_NODE) continue;
	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"cgroup"))) {
	    ret = ooConstraintGroup_new(&cg);
	    if (ret != oo_OK) return ret;

	    value = (char*)xmlGetProp(cur_node,  (const xmlChar *)"oper");
	    if (value) {
		if (!strcmp(value, "OR")) cg->logic_oper = LOGIC_OR;
		xmlFree(value);
	    }
	    ret = ooConstraintGroup_read_children(cg, code, cur_node->children);
	    cg->next = self->children;
	    self->children = cg;
	}

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"constraint"))) {
	    c = malloc(sizeof(struct ooConstraint));
	    if (!c) return oo_NOMEM;

	    ret = ooConstraint_read_atomic_constraint(c, code, cur_node);
	    c->next = self->atomic_constraints;
	    self->atomic_constraints = c;
	}

    }

    return oo_OK;
}


/* the root constraint group reads elements from XML */
static int
ooConstraintGroup_read_XML(struct ooConstraintGroup *self,
			   struct ooCode *code,
			   xmlNode *input_node)
{
    xmlNode *cur_node;
    char *value;
    int ret = FAIL;

    for (cur_node = input_node; cur_node; cur_node = cur_node->next) {
	if (cur_node->type != XML_ELEMENT_NODE) continue;
	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"cgroup"))) {
	    value = (char*)xmlGetProp(cur_node,  (const xmlChar *)"oper");
	    if (value) {
		if (!strcmp(value, "OR")) self->logic_oper = LOGIC_OR;
		xmlFree(value);
	    }
	    ret = ooConstraintGroup_read_children(self, code, cur_node->children);
	}
    }

    return ret;
}



/*  ooConstraintGroup Initializer */
extern int 
ooConstraintGroup_new(struct ooConstraintGroup **cg)
{   
    struct ooConstraintGroup *self = malloc(sizeof(struct ooConstraintGroup));
    if (!self) return oo_NOMEM;

    self->is_affirmed = true;
    self->logic_oper = LOGIC_AND;

    self->atomic_constraints = NULL;
    self->children = NULL;
    self->next = NULL;

    /* public methods */
    self->del = ooConstraintGroup_del;
    self->str = ooConstraintGroup_str;

    self->read = ooConstraintGroup_read_XML;

    *cg = self;
    return oo_OK;
}


