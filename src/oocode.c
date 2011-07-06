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
 *   oocode.c
 *   OOmnik Code implementation
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ooconfig.h"
#include "ooconcunit.h"
#include "oocode.h"

/* forward declarations */
static int
ooCode_read_usages(struct ooCode *self, xmlNode *input_node,
			 struct ooCodeUsage *parent_usage,
			 struct ooCodeUsage ***usages,
			 size_t *num_usages);

/*  Code Unit destructor */
static
int ooCodeUnit_del(struct ooCodeUnit *self)
{
    size_t i;

    for (i = 0; i < self->num_specs; i++) {
	if (self->specs[i]) {
	    if (self->specs[i]->unit)
		ooCodeUnit_del(self->specs[i]->unit);
	    free(self->specs[i]);
	}
    }
    
    if (self->specs) free(self->specs);
    free(self);

    return oo_OK;
}

/*  Code destructor */
static
int ooCode_del(struct ooCode *self)
{
    size_t i;
    struct ooCodeAttr *attr, *next_attr;
    struct ooAdaptContext *context;

    if (self->cache->num_seqs) {
	for (i = 0; i < self->cache->num_seqs; i++) {
	    if (self->cache->seqs[i])
		free(self->cache->seqs[i]);

	    if (self->cache->contexts[i]) {
		context = self->cache->contexts[i];
		if (context->affects_prepos)
		    context->affects_prepos->del(context->affects_prepos);
		if (context->affects_postpos)
		    context->affects_postpos->del(context->affects_postpos);

		if (context->affected_prepos)
		    context->affected_prepos->del(context->affected_prepos);
		if (context->affected_postpos)
		    context->affected_postpos->del(context->affected_postpos);

		free(context);
	    }
	}
	free(self->cache->seqs);
	free(self->cache->contexts);
    }

    /* usages */
    for (i = 0; i < self->num_usages; i++)
	self->usages[i]->del(self->usages[i]);
    if (self->usages)
	free(self->usages);


    /* discourse children and parents */
    for (i = 0; i < NUM_OPERS; i++) {
	attr = self->children[i];
	while (attr) {
	    next_attr = attr->next;
	    if (attr->code_name)
		free(attr->code_name);
	    free(attr);
	    attr = next_attr;
	}

	attr = self->parents[i];
	while (attr) {
	    next_attr = attr->next;
	    if (attr->code_name)
		free(attr->code_name);
	    free(attr);
	    attr = next_attr;
	}
    }


    /* code denotations */
    for (i = 0; i < self->num_denots; i++)
	if (self->denot_names[i])
	    free(self->denot_names[i]);
    if (self->denot_names)
	free(self->denot_names);
    if (self->denots)
	free(self->denots);
 
    /* implied codes */
    for (i = 0; i < self->num_implied_codes; i++)
	if (self->implied_code_names[i])
	    free(self->implied_code_names[i]);
    if (self->implied_code_names)
	free(self->implied_code_names);
    if (self->implied_codes)
	free(self->implied_codes);

    if (self->cache)
	free(self->cache);

    if (self->complex)
	ooCodeUnit_del(self->complex);

    if (self->baseclass_name)
	free(self->baseclass_name);

    if (self->name)
	free(self->name);

    free(self);

    return oo_OK;
}

/*  ooCodeUsage destructor */
static
int ooCodeUsage_del(struct ooCodeUsage *self)
{
    size_t i;

    if (self->name) free(self->name);
    if (self->conc_name) free(self->conc_name);

    /* usages */
    for (i = 0; i < self->num_usages; i++)
	self->usages[i]->del(self->usages[i]);
    if (self->usages)
	free(self->usages);

    /* derivs */
    for (i = 0; i < self->num_derivs; i++)
	self->derivs[i]->del(self->derivs[i]);
    if (self->derivs)
	free(self->derivs);



    free(self);

    return oo_OK;
}

/*  ooDeriv destructor */
static
int ooDeriv_del(struct ooDeriv *self)
{
    if (self->name) free(self->name);
    if (self->usage_name) free(self->usage_name);
    if (self->arg_code_name) free(self->arg_code_name);
    if (self->arg_code_usage_name) free(self->arg_code_usage_name);

    free(self);

    return oo_OK;
}


static
const char* ooCode_str(struct ooCode *self)
{
    struct ooCodeUnit *cu;
    struct ooCodeUnitAttr *cu_attr;
    struct ooDeriv *deriv;
    const char *code_name;

    const char *cs_name = "Unspec";
    size_t i;

    if (self->cs) cs_name = self->cs->name;

    printf("    CODE of CS \"%s\" %s  (id: %lu, type: %d, baseclass: %s) "
           " num_usages: %lu\n", 
	   cs_name, self->name, (unsigned long)self->id, self->type,
	   self->baseclass_name, (unsigned long)self->num_usages);

    for (i = 0; i < self->num_usages; i++)
	self->usages[i]->str(self->usages[i]);

    for (i = 0; i < self->num_denots; i++) {
	printf("  -- denot: %s\n", self->denot_names[i]);
    }

    cu = self->complex;

    while (cu) {
	printf("    nested code unit:\n");
	cu->code->str(cu->code);
	if (cu->num_specs == 0) break;
	cu_attr = cu->specs[0];
	cu = cu_attr->unit;
    }

    if (self->num_children) {
	printf("     -- children: %zu\n", self->num_children);

	for (i = 0; i < NUM_OPERS; i++) {
	    if (!self->children[i]) continue;
	    code_name = self->children[i]->code_name;
	    if (!code_name) continue;
	    printf("       child: %s\n", code_name);
	}
    }

    if (self->num_parents) {
	printf("     -- parents: %zu\n", self->num_parents);
	for (i = 0; i < NUM_OPERS; i++) {
	    if (!self->parents[i]) continue;
	    printf("      parent: %s (%zu)\n", 
		   self->parents[i]->code->name, 
		   self->parents[i]->code->id);
	}
    }

    /*if (self->cache) {
	for (i = 0; i < self->cache->num_seqs; i++) {
	}
	}*/

    return NULL;
}

static
const char* ooCodeUsage_str(struct ooCodeUsage *self)
{
    size_t i;
    printf("   -- CodeUsage: \"%s\"\n", self->conc_name);

    for (i = 0; i < self->num_derivs; i++) 
	self->derivs[i]->str(self->derivs[i]);

    if (self->num_usages)
	for (i = 0; i < self->num_usages; i++) 
	    self->usages[i]->str(self->usages[i]);


    return NULL;
}

static
const char* ooDeriv_str(struct ooDeriv *self)
{

    printf("      -- ooDeriv: \"%s\" [%s] %p\n", 
	   self->name, self->usage_name, self);
    printf("                oper: %d  arg: %s [%s]\n",
	   self->operid, self->arg_code_name, self->arg_code_usage_name);

    return NULL;
}


/* find the numeric id of the operation name */
static int
ooCode_match_operid(struct ooCode *self, 
		    const char *buf)
{
    int ret = -1, i = -1;

    const char **oper_str = self->cs->operids;

    while (*oper_str) {
	i++;
	if (!strcmp(buf, *oper_str))
	    return i;
	oper_str++;
    }
    return ret;   
}



/** 
  * set a single back reference to parent
  */
static int
ooCode_set_backref(struct ooCode *self,
		   struct ooCode *child,
		   oper_type operid,
		   struct ooCodeAttr *child_attr)
{
    int ret;
    struct ooCodeAttr *attr;

    attr = malloc(sizeof(struct ooCodeAttr));
    if (!attr) return oo_NOMEM;

    attr->operid = operid;
    attr->concid = self->id;
    attr->code_name = NULL;
    attr->code = self;
    attr->linear_order = ANYPOS;
    attr->linear_contact = child_attr->linear_contact;
    attr->stackable = child_attr->stackable;
    attr->implied_parent = child_attr->implied_parent;
    attr->next = NULL;

    /* reverse linear position */
    if (child_attr->linear_order == PREPOS) attr->linear_order = POSTPOS;
    if (child_attr->linear_order == POSTPOS) attr->linear_order = PREPOS;

    if (DEBUG_CS_LEVEL_3) {
	printf("  .. Adding parent \"%s\"  to \"%s\""
               "  (oper: %d) linear_contact: %d implied_parent: %d\n", 
	       self->name, child->name, attr->operid,
	       attr->linear_contact, attr->implied_parent);
	printf("Learning child's id: %zu\n",
	       child->id);
    }


    /* setting up parent relation */
    attr->next = child->parents[operid];
    child->parents[operid] = attr;
    child->num_parents++;

    return oo_OK;
}


struct ooCodeUsage*
ooCode_lookup_usage(struct ooCodeUsage **usages,
		    size_t num_usages,
		    const char *usage_name)
{
    struct ooCodeUsage *usage;
    size_t i;

    for (i = 0; i < num_usages; i++) {
	usage = usages[i];
	if (!usage->name) continue;
	if (!strcmp(usage->name, usage_name)) return usage;
    }

    /* nested usages */
    if (usage->num_usages)
	return ooCode_lookup_usage(usage->usages, 
				   usage->num_usages, 
				   usage_name);


    return NULL;
}


static int
ooCode_resolve_refs(struct ooCode *self)
{
    struct ooCode *child_code, *ref;
    struct ooCodeAttr *attr, *child_attr;
    struct ooCodeUsage *usage;
    struct ooDeriv *deriv;
    struct ooMindMap *mindmap;
    struct ooConcept *conc;
    int operid, ret, i;

    mindmap = self->cs->mindmap;

    /* baseclass reference */
    if (self->baseclass_name) {
	ref = (struct ooCode*)self->cs->codes->get(self->cs->codes, 
						   self->baseclass_name);
	if (!ref) {
	    if (DEBUG_CS_LEVEL_2)
		printf(" -- no such baseclass: \"%s\" :(\n", 
		       self->baseclass_name);
	    }
	else self->baseclass = ref;
    }

    /* usages */
    for (i = 0; i < self->num_usages; i++) {
	usage = self->usages[i];

	/*printf("\nRESOLVE USAGE REF: %s\n", usage->conc_name);*/

	if (!usage->conc_name) continue;
	conc = mindmap->lookup(mindmap, (const char*)usage->conc_name);
	if (!conc) {
	  /*printf("\n -- usage reference not resolved: %s :((\n", usage->conc_name);*/
	  continue;
	}
	usage->conc = conc;
   }

    for (operid = 0; operid < NUM_OPERS; operid++) {
	/* usage references */
	deriv = self->deriv_matches[operid];

	while (deriv) {
	    ref = (struct ooCode*)self->cs->codes->get(self->cs->codes, 
						   deriv->name);
	    if (!ref) {
		printf("\n -- deriv reference not resolved: %s :((\n", deriv->name);
		goto next_deriv;
	    }

	    deriv->code = ref;
	    deriv->code_usage = ooCode_lookup_usage(ref->usages, 
						    ref->num_usages, 
						    (const char*)deriv->usage_name);

	    if (!deriv->code_usage) {
		printf("\n  -- no such CodeUsage found: %s (deriv: %s) :((\n", 
		       deriv->usage_name, deriv->name);
		goto next_deriv;
	    }

    next_deriv:
	    deriv = deriv->next;
	}

	/* children to parents */
	child_attr = self->children[operid];
	if (!child_attr) continue;
	    
	while (child_attr) {
	    
	    if (DEBUG_CS_LEVEL_4)
		printf("  BACKREF TO PARENT: %s FROM CHILD ATTR " 
                       " CODE NAME: %s LINEAR ORDER: %d\n", 
		       self->name,
		       child_attr->code_name, child_attr->linear_order);

	    if (!child_attr->code_name) goto next_attr;
	    child_code = (struct ooCode*)self->cs->codes->get(self->cs->codes, 
							      child_attr->code_name);
	    if (!child_code) break;

	    /* replace the temporary code with the real one */
	    /* child_attr->code->del(child_attr->code);*/

	    child_attr->code = child_code;
	    child_attr->concid = child_code->id; 

	    ret = ooCode_set_backref(self, child_code, operid, child_attr);
	next_attr:
	    child_attr = child_attr->next;
	}

    }
 

    return oo_OK;
}
    




/* reading XML */


static int
ooCode_read_position_constraints(struct ooCode *self, 
				 xmlNode *input_node,
				 struct ooAdaptContext *context,
				 adaptation_roles role)
{
    xmlNode *cur_node;
    struct ooConstraintGroup *cg;
    int ret;

    for (cur_node = input_node; cur_node; cur_node = cur_node->next) {
	if (cur_node->type != XML_ELEMENT_NODE) continue;

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"prepos"))) {
	    ret = ooConstraintGroup_init(&cg);
	    if (ret != oo_OK) return ret;
	    ret = cg->read(cg, self, cur_node->children);
	    if (ret != oo_OK) return ret;
	    if (role == AFFECTS) context->affects_prepos = cg;
	    else context->affected_prepos = cg; 
	}
	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"postpos"))) {
	    ret = ooConstraintGroup_init(&cg);
	    if (ret != oo_OK) return ret;
	    ret = cg->read(cg, self, cur_node->children);
	    if (ret != oo_OK) return ret;
	    if (role == AFFECTS) context->affects_postpos = cg;
	    else context->affected_postpos = cg; 
	}
    }
    return oo_OK;
}




static int
ooCode_read_cache_unit_context(struct ooCode *self, 
				     xmlNode *input_node,
				     struct ooAdaptContext *context)
{
    xmlNode *cur_node;
    int ret;

    for (cur_node = input_node; cur_node; cur_node = cur_node->next) {
	if (cur_node->type != XML_ELEMENT_NODE) continue;

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"affects"))) {
	    ret = ooCode_read_position_constraints(self, 
				 cur_node->children, context, AFFECTS);
	}
	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"affected"))) {
	    ret = ooCode_read_position_constraints(self, 
						   cur_node->children, 
						   context, AFFECTED);
	}
    }
    return oo_OK;
}



static int
ooCode_read_cache_unit(struct ooCode *self, 
		       xmlNode *input_node)
{   char *value, *seq, **seqs;
    xmlNode *cur_node;
    struct ooAdaptContext **contexts, *context;
    bool gotcha = false;
    int ret;

    value = (char*)xmlGetProp(input_node,  (const xmlChar *)"seq");
    if (!value)
	value = (char*)xmlGetProp(input_node,  (const xmlChar *)"codeseq");

    if (!value) return oo_FAIL;

    seqs = realloc(self->cache->seqs, (sizeof(char*) *
				    (self->cache->num_seqs  + 1)));
    if (!seqs) {
	xmlFree(value);
	return oo_NOMEM;
    }

    seq = malloc(strlen(value) + 1);
    if (!seq) {
	xmlFree(value);
	return oo_NOMEM;
    }
    strcpy(seq, value);
    xmlFree(value);

    seqs[self->cache->num_seqs] = seq;
    self->cache->seqs = (const char**)seqs;
    self->is_cached = true;

    /* reading the context conditions */
    for (cur_node = input_node->children; cur_node; cur_node = cur_node->next) {
	if (cur_node->type != XML_ELEMENT_NODE) continue;

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"context"))) {
	    context = malloc(sizeof(struct ooAdaptContext));
	    if (!context) return oo_NOMEM;
	    context->affects_prepos = NULL;
	    context->affects_postpos = NULL;

	    context->affected_prepos = NULL;
	    context->affected_postpos = NULL;

	    contexts = realloc(self->cache->contexts, 
			       (sizeof(struct ooAdaptContext*) *
				(self->cache->num_seqs + 1)));
	    if (!contexts) return oo_NOMEM;

	    ret = ooCode_read_cache_unit_context(self,
						 cur_node->children, context);

	    if (ret != oo_OK) return ret;

	    contexts[self->cache->num_seqs] = context;
	    self->cache->contexts = contexts;
	    gotcha = true;
	}
    }

    /* no context was found.. make a NULL reference in index */
    if (!gotcha) {
	contexts = realloc(self->cache->contexts, 
			   (sizeof(struct ooAdaptContext*) *
			    (self->cache->num_seqs + 1)));
	if (!contexts) return oo_NOMEM;
	contexts[self->cache->num_seqs] = NULL;
	self->cache->contexts = contexts;
    }

    self->cache->num_seqs++;

    return oo_OK;
}



static int
ooCode_read_specs(struct ooCode *self, 
		       xmlNode *input_node)
{
    xmlNode *cur_node = NULL;
    oper_type operid;
    char *oper_name, *name, *value;
    struct ooCodeAttr *attr, **specs;
    struct ooCode *child;
    linear_type linear_order;
    bool linear_contact, stackable, implied_parent;
    int ret;

    for (cur_node = input_node; cur_node; cur_node = cur_node->next) {
	if (cur_node->type != XML_ELEMENT_NODE) continue;

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"spec"))) {
	    /* default linear order */
	    linear_order = ANYPOS;

	    oper_name = (char*)xmlGetProp(cur_node,  (const xmlChar *)"oper");
	    if (!oper_name) continue;
	    operid = ooCode_match_operid(self, (const char*)oper_name);
	    xmlFree(oper_name);

	    name = (char*)xmlGetProp(cur_node,  (const xmlChar *)"operand");
	    if (!name) continue;
	    
	    value = (char*)xmlGetProp(cur_node,  (const xmlChar *)"linear_order");
	    if (value) {
		if (!strcmp(value, "prepos")) linear_order = PREPOS;
		if (!strcmp(value, "postpos")) linear_order = POSTPOS;
		xmlFree(value);
	    }

	    linear_contact = false;

	    value = (char*)xmlGetProp(cur_node,  (const xmlChar *)"linear_contact");
	    if (value) {
		if (!strcmp(value, "1")) linear_contact = true;
		xmlFree(value);
	    }

	    stackable = true;

	    value = (char*)xmlGetProp(cur_node,  (const xmlChar *)"stackable");
	    if (value) {
		if (!strcmp(value, "0")) stackable = false;
		xmlFree(value);
	    }

	    implied_parent = false;
	    value = (char*)xmlGetProp(cur_node,  (const xmlChar *)"implied_parent");
	    if (value) {
		if (!strcmp(value, "1")) implied_parent = true;
		xmlFree(value);
	    }


	    attr = malloc(sizeof(struct ooCodeAttr));
	    if (!attr) {
		xmlFree(name);
		return oo_NOMEM;
	    }
	    attr->operid = operid;
	    attr->concid = 0;  /* real value to be assigned at a later stage */
	    attr->linear_order = linear_order;
	    attr->linear_contact = linear_contact;
	    attr->stackable = stackable;
	    attr->implied_parent = implied_parent;
	    attr->next = NULL;
	    attr->code = NULL;

	    attr->code_name = malloc(strlen(name) + 1);
	    if (!attr->code_name) {
		free(attr);
		xmlFree(name);
		return oo_NOMEM;
	    }
	    strcpy(attr->code_name, name);
	    xmlFree(name);

	    attr->next = self->children[operid];
	    self->children[operid] = attr;
	    self->num_children++;

	}
    }

    return oo_OK;
}


static int
ooCode_read_implied_codes(struct ooCode *self, 
			  xmlNode *input_node)
{
    xmlNode *cur_node = NULL;
    struct ooCode **implied_codes;
    char **codenames;
    char *codename;

    for (cur_node = input_node; cur_node; cur_node = cur_node->next) {
	if (cur_node->type != XML_ELEMENT_NODE) continue;

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"implied_code"))) {
	    codename = (char*)xmlGetProp(cur_node,  (const xmlChar *)"name");
	    if (!codename) continue;

	    codenames = realloc(self->implied_code_names,
				sizeof(char*) * 
				(self->num_implied_codes + 1));
	    if (!codenames) {
		xmlFree(codename);
		return oo_NOMEM;
	    }

	    codenames[self->num_implied_codes] = \
		malloc(strlen(codename) + 1);

	    if (!codenames[self->num_implied_codes]) {
		free(codenames);
		xmlFree(codename);
		return oo_NOMEM;
	    }
	    strcpy(codenames[self->num_implied_codes], codename);
	    xmlFree(codename);


	    implied_codes = realloc(self->implied_codes,
				    sizeof(struct ooCode*) *
				    (self->num_implied_codes + 1));
	    if (!implied_codes) {
		free(codenames[self->num_implied_codes]);
		free(codenames);
		return oo_NOMEM;
	    }
	    implied_codes[self->num_implied_codes] = NULL;
	    self->implied_codes = implied_codes;

	    self->num_implied_codes++;
	    self->implied_code_names = codenames;
	}
    }
    return oo_OK;
}

static struct ooCodeUnit*
ooCode_read_complex_unit(struct ooCode *self,
			 xmlNode *input_node)
{
    xmlNode *cur_node, *attr_node, *aggr_node;
    char *code_name, *oper_name;
    int operid;
    struct ooCode *code;
    struct ooCodeUnit *unit, *aggr_unit;
    struct ooCodeUnitAttr *cu_attr, **specs;
    int ret;

    code_name = (char*)xmlGetProp(input_node,  (const xmlChar *)"name");
    if (!code_name) return NULL;

    /* get a reference to the real code by its name */
    code = (struct ooCode*)self->cs->codes->get(self->cs->codes, code_name);
    if (!code) {
	printf("No such code found :(  %s\n", code_name);
	xmlFree(code_name);
	return NULL;
    }
    xmlFree(code_name);
    
    unit = malloc(sizeof(struct ooCodeUnit));
    if (!unit) return NULL;

    unit->code = code;
    unit->specs = NULL;
    unit->num_specs = 0;

    /* any aggregated units? */
    for (cur_node = input_node->children; cur_node; cur_node = cur_node->next) {
	if (cur_node->type != XML_ELEMENT_NODE) continue;

	if ((xmlStrcmp(cur_node->name, (const xmlChar *)"specs"))) continue;

	for (attr_node = cur_node->children; attr_node; attr_node = attr_node->next) {
	    if (attr_node->type != XML_ELEMENT_NODE) continue;
	    if ((xmlStrcmp(attr_node->name, (const xmlChar *)"spec"))) continue;

	    /* what kind of operation is this? */
	    oper_name = (char*)xmlGetProp(attr_node,  (const xmlChar *)"oper");
	    if (!oper_name) continue;
	    operid = ooCode_match_operid(self, (const char*)oper_name);
	    xmlFree(oper_name);
	    if (operid == -1) continue;

	    for (aggr_node = attr_node->children; aggr_node; aggr_node = aggr_node->next) {
		if (aggr_node->type != XML_ELEMENT_NODE) continue;
		if ((xmlStrcmp(aggr_node->name, (const xmlChar *)"unit"))) continue;

		aggr_unit = ooCode_read_complex_unit(self, aggr_node);
		if (!aggr_unit) continue;

		specs = realloc(unit->specs, (sizeof(struct ooCodeAttr*) *
					((self->num_children)  + 1)));
		if (!specs) continue;

		cu_attr = malloc(sizeof(struct ooCodeUnitAttr));
		if (!cu_attr) continue;

		cu_attr->operid = operid;
		cu_attr->unit = aggr_unit;

		specs[unit->num_specs] = cu_attr;
		unit->specs = specs;
		unit->num_specs++;
	    }

	}

    }

    return unit;
}

static int
ooCode_read_complex(struct ooCode *self, 
			 xmlNode *input_node)
{
    xmlNode *cur_node = NULL;
    struct ooConcUnit *cu;
    int ret;

    if (DEBUG_CS_LEVEL_3)
	printf("\n *** Reading Code's Complex...\n");

    for (cur_node = input_node; cur_node; cur_node = cur_node->next) {
	if (cur_node->type != XML_ELEMENT_NODE) continue;

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"unit"))) {
	    self->complex = ooCode_read_complex_unit(self, cur_node);
	    
	    if (DEBUG_CS_LEVEL_3)
		self->str(self);
	}
    }
    return oo_OK;
}

static int
ooCode_read_cache(struct ooCode *self, 
		  xmlNode *input_node)
{
    xmlNode *cur_node = NULL;
    int ret;

    for (cur_node = input_node; cur_node; cur_node = cur_node->next) {
	if (cur_node->type != XML_ELEMENT_NODE) continue;

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"units"))) {
	    ret = ooCode_read_cache_unit(self, cur_node);
	}
   }
    return oo_OK;
}

static int
ooCode_read_usage_derivs(struct ooCode *self, 
			 xmlNode *input_node,
			 struct ooCodeUsage *usage)
{
    xmlNode *cur_node;
    char *value;
    struct ooDeriv *deriv;
    size_t derivs_size;

    int ret;

    for (cur_node = input_node; cur_node; cur_node = cur_node->next) {
	if (cur_node->type != XML_ELEMENT_NODE) continue;

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"deriv"))) {
	    value = (char*)xmlGetProp(cur_node,  (const xmlChar *)"name");
	    if (!value) {
		fprintf(stderr, "Deriv with no name in %s [%s]...\n", 
			self->name, usage->name);
		continue;
	    }

	    /* new derivation */
	    ret = ooDeriv_init(&deriv);
	    deriv->name = malloc(strlen(value) + 1);
	    if (!deriv->name) {
		deriv->del(deriv);
		return oo_NOMEM;
	    }
	    strcpy(deriv->name, value);
	    xmlFree(value);

	    /* operation id */
	    value = (char*)xmlGetProp(cur_node,  (const xmlChar *)"oper");
	    if (!value) {
		fprintf(stderr, "Deriv has no operid in %s [%s]...\n", 
			self->name, deriv->name);
		deriv->del(deriv);
		continue;
	    }

	    deriv->operid = ooCode_match_operid(self, (const char*)value);
	    xmlFree(value);

	    if (deriv->operid < 0) {
		fprintf(stderr, "Deriv has incorrect operid \"%s\" in %s [%s]...\n", 
			value, self->name, deriv->name);
		deriv->del(deriv);
		continue;
	    }

	    /* usage name */
	    value = (char*)xmlGetProp(cur_node,  (const xmlChar *)"usage_name");
	    if (!value) {
		fprintf(stderr, "Deriv has no usage_name in %s [%s]...\n", 
			self->name, deriv->name);
		deriv->del(deriv);
		continue;
	    }
	    deriv->usage_name = malloc(strlen(value) + 1);
	    if (!deriv->usage_name) {
		deriv->del(deriv);
		xmlFree(value);
		return oo_NOMEM;
	    }
	    strcpy(deriv->usage_name, value);
	    xmlFree(value);

	    /* argument name */
	    value = (char*)xmlGetProp(cur_node,  (const xmlChar *)"arg_name");
	    if (!value) {
		fprintf(stderr, " warning: deriv has no arg_name in %s [%s]...\n", 
			self->name, deriv->name);
	    }
	    else {
		deriv->arg_code_name = malloc(strlen(value) + 1);
		if (!deriv->arg_code_name) {
		    deriv->del(deriv);
		    xmlFree(value);
		    return oo_NOMEM;
		}
		strcpy(deriv->arg_code_name, value);
		xmlFree(value);
	    }

	    /* argument usage name */
	    value = (char*)xmlGetProp(cur_node,  (const xmlChar *)"arg_usage_name");
	    if (!value) {
		fprintf(stderr, "Deriv has no arg_usage_name in %s [%s]...\n", 
			self->name, deriv->name);
		deriv->del(deriv);
		continue;
	    }
	    deriv->arg_code_usage_name = malloc(strlen(value) + 1);
	    if (!deriv->arg_code_usage_name) {
		deriv->del(deriv);
		xmlFree(value);
		return oo_NOMEM;
	    }
	    strcpy(deriv->arg_code_usage_name, value);
	    xmlFree(value);

	    /* register new derivation */
	    derivs_size = (usage->num_derivs + 1) * sizeof(struct ooDeriv*);
	    usage->derivs = realloc(usage->derivs, derivs_size);
	    if (!usage->derivs) {
		deriv->del(deriv);
		return oo_NOMEM;
	    }
	    usage->derivs[usage->num_derivs] = deriv;
	    usage->num_derivs++;

	    if (deriv->operid < 1) continue;

	    /* add deriv to search engine */
	    deriv->next = self->deriv_matches[deriv->operid];
	    self->deriv_matches[deriv->operid] = deriv;
	}

    }
    return oo_OK;
}



static int
ooCode_read_usage(struct ooCode *self, 
		  xmlNode *input_node,
		  struct ooCodeUsage *usage)
{
    xmlNode *cur_node = NULL;
    char *value;
    int ret;

    for (cur_node = input_node; cur_node; cur_node = cur_node->next) {
	if (cur_node->type != XML_ELEMENT_NODE) continue;

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"usages"))) {
	    ret = ooCode_read_usages(self, cur_node->children, 
				     usage,
				     &usage->usages, &usage->num_usages);
	}

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"derivs"))) {
	    ret = ooCode_read_usage_derivs(self, cur_node->children, 
					   usage);
	}




    }
    return oo_OK;
}


static int
ooCode_read_usages(struct ooCode *self, 
		   xmlNode *input_node,
		   struct ooCodeUsage *parent_usage,
		   struct ooCodeUsage ***usages,
		   size_t *num_usages)
{
    xmlNode *cur_node = NULL;
    struct ooCodeUsage *usage;
    struct ooCodeUsage **curr_usages = NULL;
    size_t usages_size = 0;
    char *value;
    int ret;

    for (cur_node = input_node; cur_node; cur_node = cur_node->next) {
	if (cur_node->type != XML_ELEMENT_NODE) continue;

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"usage"))) {
	    value = (char*)xmlGetProp(cur_node,  (const xmlChar *)"concref");
	    if (!value) {
		fprintf(stderr, "No concref in usage of \"%s\"...\n", self->name);
		return oo_FAIL;
	    }

	    ret = ooCodeUsage_new(&usage);
	    if (ret != oo_OK) {
		xmlFree(value);
		return ret;
	    }
	    usage->parent = parent_usage;
	    usage->code = self;

	    usage->conc_name = malloc(strlen(value) + 1);
	    if (!usage->conc_name) {
		usage->del(usage);
		xmlFree(value);
		return oo_NOMEM;
	    }
	    strcpy(usage->conc_name, value);
	    xmlFree(value);

	    value = (char*)xmlGetProp(cur_node,  (const xmlChar *)"name");
	    if (value) {
		usage->name = malloc(strlen(value) + 1);
		if (!usage->name) {
		    usage->del(usage);
		    return oo_NOMEM;
		}
		strcpy(usage->name, value);
		xmlFree(value);
	    }

	    /* add new usage */
	    usages_size = ((*num_usages) + 1) * sizeof(struct ooCodeUsage*);
	    curr_usages = realloc((*usages), usages_size);
	    if (!curr_usages) {
		usage->del(usage);
		return oo_NOMEM;
	    }
	    curr_usages[(*num_usages)] = usage;
	    (*usages) = curr_usages;
	    (*num_usages)++;
	    
	    /* explore inner elements */
	    ooCode_read_usage(self, cur_node->children, usage);
	}
    }
    return oo_OK;
}

static int
ooCode_read_denots(struct ooCode *self, 
		   xmlNode *input_node)
{
    xmlNode *cur_node = NULL;
    char *value;
    struct ooCode *denot;
    struct ooCode **denots;
    int ret;

    for (cur_node = input_node; cur_node; cur_node = cur_node->next) {
	if (cur_node->type != XML_ELEMENT_NODE) continue;

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"denot"))) {
	    value = (char*)xmlGetProp(cur_node,  (const xmlChar *)"name");
	    if (!value) continue;

	    self->denot_names = realloc(self->denot_names,
					(self->num_denots + 1) * sizeof(char*));
	    if (!self->denot_names) {
		xmlFree(value);
		return oo_NOMEM;
	    }

	    self->denots = realloc(self->denots,
				   (self->num_denots + 1) * sizeof(struct ooCode*));
	    if (!self->denots) {
		xmlFree(value);
		free(self->denot_names);
		return oo_NOMEM;
	    }

	    self->denots[self->num_denots] = NULL;
	    self->denot_names[self->num_denots] = malloc(strlen(value) + 1);
	    if (!self->denot_names[self->num_denots]) {
		xmlFree(value);
		free(self->denot_names);
		free(self->denots);
		return oo_NOMEM;
	    }

	    strcpy(self->denot_names[self->num_denots], value);
	    xmlFree(value);
	    self->num_denots++;
	}
    }
    return oo_OK;
}




static int
ooCode_read_XML(struct ooCode *self, 
		xmlNode *input_node)
{
    xmlNode *cur_node = NULL;
    char **names;
    char *value;
    int ret;

    for (cur_node = input_node; cur_node; cur_node = cur_node->next) {
	if (cur_node->type != XML_ELEMENT_NODE) continue;

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"baseclass"))) {
	    value = (char*)xmlGetProp(cur_node,  (const xmlChar *)"name");
	    if (value) {
		self->baseclass_name = malloc(strlen(value) + 1);
		if (!self->baseclass_name) {
		    xmlFree(value);
		    return oo_NOMEM;
		}
		strcpy(self->baseclass_name, value);
		xmlFree(value);
	    }
	    continue;
	}

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"cache"))) {
	    ret = ooCode_read_cache(self, cur_node->children);
	    continue;
	}

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"denots"))) {
	    ret = ooCode_read_denots(self, cur_node->children);
	    continue;
	}

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"complex"))) {
	    ret = ooCode_read_complex(self, cur_node->children);
	    continue;
	}

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"specs"))) {
	    ret = ooCode_read_specs(self, cur_node->children);
	    continue;
	}

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"implied_codes"))) {
	    ret = ooCode_read_implied_codes(self, cur_node->children);
	    continue;
	    }

	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"usages"))) {
	    ret = ooCode_read_usages(self, cur_node->children, 
				     NULL, 
				     &self->usages, &self->num_usages);
	    /*self->str(self);*/
	}
    }

    return oo_OK;
}




/*  ooDeriv initializer */
int ooDeriv_init(struct ooDeriv **deriv)
{   
    int i;
    struct ooDeriv *self = malloc(sizeof(struct ooDeriv));
    if (!self) return oo_NOMEM;

    self->name = NULL;
    self->usage_name = NULL;

    self->operid = -1;

    self->code = NULL;
    self->code_usage = NULL;

    /* spec by default */
    self->used_as_topic = false;

    self->arg_code_name = NULL;
    self->arg_code_usage_name = NULL;
    self->arg_code = NULL;
    self->arg_code_usage = NULL;
    self->next = NULL;

    self->del = ooDeriv_del;
    self->str = ooDeriv_str;

    *deriv = self;
    return oo_OK;    
}
 


/*  ooCodeUsage initializer */
int ooCodeUsage_new(struct ooCodeUsage **usage)
{   
    int i;
    struct ooCodeUsage *self = malloc(sizeof(struct ooCodeUsage));
    if (!self) return oo_NOMEM;

    self->name = NULL;
    self->conc_name = NULL;
    self->conc = NULL;

    self->usages = NULL;
    self->num_usages = 0;

    self->derivs = NULL;
    self->num_derivs = 0;

    self->del = ooCodeUsage_del;
    self->str = ooCodeUsage_str;

    *usage = self;
    return oo_OK;    
}
 

/*  ooCode initializer */
int ooCode_new(struct ooCode **code)
{   
    int i;
    struct ooCode *self = malloc(sizeof(struct ooCode));
    if (!self) return oo_NOMEM;

    self->id = 0;
    self->name = NULL;
    self->type = CODE_STATIC;

    /*self->concref = NULL;
    self->gloss = NULL;
    self->semclass = NULL;*/

    self->cs = NULL;

    self->complex = NULL;

    self->baseclass_name = NULL;
    self->baseclass = NULL;
    
    self->usages = NULL;
    self->num_usages = 0;

    self->cache = malloc(sizeof(struct ooCodeCache));
    if (!self->cache) {
	free(self);
	return oo_NOMEM;
    }
    self->cache->seqs = NULL;
    self->cache->num_seqs = 0;
    self->cache->contexts = NULL;

    self->is_cached = false;

    self->implied_codes = NULL;
    self->implied_code_names = NULL;
    self->num_implied_codes = 0;

    for (i = 0; i < NUM_OPERS; i++) {
	self->parents[i] = NULL;
	self->children[i] = NULL;
	self->deriv_matches[i] = NULL;
    }
    self->num_parents = 0;
    self->num_children = 0;

    self->allows_grouping = true;

    self->denots = NULL;
    self->denot_names = NULL;
    self->num_denots = 0;

    /* public methods */
    self->del = ooCode_del;
    self->str = ooCode_str;
    self->read = ooCode_read_XML;
    self->resolve_refs = ooCode_resolve_refs;

    *code = self;
    return oo_OK;
}


