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
 *   oocomplex.c
 *   OOmnik Complex Concept implementation
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ooconfig.h"

#include "oocodesystem.h"
#include "oocode.h"
#include "ooconcunit.h"
#include "ooconstraint.h"
#include "ooagenda.h"
#include "oocomplex.h"
#include "oodecoder.h"
#include "ooaccumulator.h"

struct ooMindMap;

/* declarations */
static int 
ooComplex_present_linear_index(struct ooComplex*);

/*  Complex Destructor */
static
int ooComplex_del(struct ooComplex *self)
{
    free(self);
    return oo_OK;
}

static const char* 
ooComplex_str(struct ooComplex *self,
			size_t depth)
{  
    struct ooComplex *complex;
    size_t i, offset_size = sizeof(char) * OFFSET_SIZE * depth;
    char *offset = malloc(offset_size + 1);
    memset(offset, SPACE_CHAR, offset_size);
    offset[offset_size] = '\0';

    const char *gloss = "None";
    const char **operids = NULL;
    const char *opername = "???";

    if (depth == 1)
	ooComplex_present_linear_index(self);

    if (self->base->agenda)
	operids = self->base->agenda->codesystem->operids;

    /*if (self->base->terminals) 
      gloss = self->base->terminals->code->name;*/

    if (self->base->code->name)
	gloss = self->base->code->name;


    printf("%s| %s [%d:%d] (%p pres: %d gloss: \"%s\" interps: %lu, weight: %d)\n",
	   offset, self->base->code->name, 
	   self->linear_begin, self->linear_end,
	   self, self->base->is_present, 
	   gloss,
	   (unsigned long)self->num_interps,
	   self->weight);

    for (i = 0; i < NUM_OPERS; i++) {
	complex = self->specs[i];
	if (!complex) continue;
	if (operids) opername = operids[i];

	printf("%s|__%s:\n", offset, opername);
	complex->str(complex, depth + 1);
    }

    if (self->num_interps) {
	for (i = 0; i < self->num_interps; i++) {
	    printf("  INTERP: %s\n", self->interps[i]->conc->name);
	}
    }

    free(offset);
    return NULL;
}


static int
ooComplex_present_linear_index(struct ooComplex *self)
{
    struct ooComplex *c;
    size_t i;

    printf("    Linear Index: ");

    for (i = 0; i < INPUT_BUF_SIZE; i++) {
	c = self->linear_index[i];
	if (!c) continue;

	printf(" [%s %lu:%lu", 
	       c->base->code->name, 
	       (unsigned long)c->base->linear_pos,
	       (unsigned long)c->base->linear_pos + c->base->coverage);

	printf("]");
    }

    printf("\n");

    return oo_OK;
}


static size_t
ooComplex_calc_dimensions(struct ooComplex *self,
			     size_t *max_num_levels,
			     size_t *num_terminals) 
{
    struct ooComplex *c;
    size_t i, curr_max_levels = 0, child_num_levels = 0;
    int ret;

    /* explore your children */
    for (i = 0; i < NUM_OPERS; i++) {
	c = self->specs[i];
	if (!c) continue;

	child_num_levels = 0;
	ooComplex_calc_dimensions(c, &child_num_levels, &self->num_terminals);
	
	if (child_num_levels > curr_max_levels)
	    curr_max_levels = child_num_levels;

    }

    if (self->base->terminals) 
	self->num_terminals += 1;

    (*num_terminals) = self->num_terminals;

    (*max_num_levels) += curr_max_levels + 1;

    return oo_OK;
}

static size_t
ooComplex_present_JSON_table_row(struct ooComplex *self,
				 struct ooAccu *accu,
				 char *parent_buf,
				 size_t parent_buf_size,
				 size_t depth,
				 size_t row_count)
{
    struct ooComplex *c;
    size_t i, curr_max_levels = 0, child_num_levels = 0;
    char *gloss;
    char output[TEMP_BUF_SIZE];
    char *tmp_buf = output;

    const char *row_end_marker = "]";
    size_t row_end_marker_size = strlen(row_end_marker);

    size_t tmp_buf_size = 0, output_size = 0;
    bool is_non_terminal = false;
    int ret;

    /*if (self->base->terminals && self->base->terminals->code)
	gloss = self->base->terminals->code->name;
    else
    gloss = "";*/

    gloss = self->base->code->name;

    /* terminal cell */
    if (self->base->terminals) {

	/* NB: no comma needed in the first row */
	if (accu->begin_row) {
	    sprintf(tmp_buf,
		    ",[{\"type\":\"term\", \"colspan\":\"%d\","
		    "\"content\":\"%s\"}",
		    depth, gloss);
	}
	else {
	    sprintf(tmp_buf,
		    "[{\"type\":\"term\", \"colspan\":\"%d\","
		    "\"content\":\"%s\"}",
		    depth, gloss);
	    accu->begin_row = true;
	    accu->begin_cell = true;
	}

	tmp_buf_size = strlen(tmp_buf);
	tmp_buf += tmp_buf_size;
	output_size += tmp_buf_size;
    }

    /* non-terminal cell  */
    for (i = 0; i < NUM_OPERS; i++) {
	c = self->specs[i];
	if (!c) continue;
	is_non_terminal = true;
	break;
    }

    if (is_non_terminal) {
	/* TODO: proper presentation of interpretations */
	if (self->num_interps)
	    gloss = self->interps[0]->conc->name;

	sprintf(tmp_buf, 
		",{\"type\": \"topic\","
		"\"rowspan\": \"%d\",\"content\":\"%s\"}", 
		self->num_terminals, gloss);

	tmp_buf_size = strlen(tmp_buf);
	tmp_buf += tmp_buf_size;
	output_size += tmp_buf_size;
    }

    /* need to append some stuff from a parent */
    if (parent_buf) {
	sprintf(tmp_buf, "%s", parent_buf);
	tmp_buf += parent_buf_size;
	output_size += parent_buf_size;
    }

    /* end row */
    if (self->base->terminals) {
	ret = accu->append(accu, (const char*)output, output_size);
	ret = accu->append(accu, row_end_marker, row_end_marker_size);
	accu->begin_cell = false;
	/* empty the buffer */
	output[0] = '\0';
	output_size = 0;
    }

    /* present your children */
    for (i = 0; i < NUM_OPERS; i++) {
	c = self->specs[i];
	if (!c) continue;
	ooComplex_present_JSON_table_row(c, 
					 accu,
					 output, output_size, 
					 depth - 1, row_count);
    }


    return oo_OK;
}


static int
ooComplex_present_JSON_list(struct ooComplex *self, 
			    struct ooAccu *accu)
{  
    struct ooConcUnit *cu;
    size_t num_terminals = 0;
    size_t max_depth = 0;
    const char *list_begin_marker = "[";
    size_t list_begin_marker_size = strlen(list_begin_marker);

    const char *list_end_marker = "]";
    size_t list_end_marker_size = strlen(list_end_marker);
    int ret;

    /* add separator */
    if (accu->begin_table)
	ret = accu->append(accu, ",", 1);
    else {
	accu->begin_table = true;
	accu->begin_row = false;
	accu->begin_cell = false;
    }

    /* table header */
    ret = accu->append(accu, list_begin_marker, list_begin_marker_size);
    if (ret != oo_OK) return ret;

    ret = ooComplex_calc_dimensions(self, &max_depth, &num_terminals);

    ret = ooComplex_present_JSON_table_row(self, 
					   accu,
					   NULL, 0,
					   max_depth, 0);

    if (ret != oo_OK) return ret;

    /* list end */
    ret = accu->append(accu, list_end_marker, list_end_marker_size);
    accu->begin_row = false;
    accu->begin_cell = false;

    if (ret != oo_OK) return ret;


    return oo_OK;
}


static int
ooComplex_present_XML(struct ooComplex *self,
			   struct ooAccu *accu,
			   size_t depth)
{  
    struct ooConcUnit *cu;
    struct ooComplex *complex;
    size_t offset_size = sizeof(char) * OFFSET_SIZE * depth;
    char *offset = malloc(offset_size + 1); 
    memset(offset, SPACE_CHAR, offset_size);
    offset[offset_size] = '\0';
    const char **operids = NULL;
    const char *opername = "???";
    const char *gloss = "??";
    int i, ret;

    char buf[TEMP_BUF_SIZE];

    printf("XML presentation!\n");

    if (self->base->agenda) 
	operids = self->base->agenda->codesystem->operids;

    if (self->num_interps)
	gloss = self->interps[0]->conc->name;

    sprintf(buf, "%s<COMPLEX CLASS=\"%s\" WEIGHT=\"%d\""
                 " BEGIN=\"%d\" END=\"%d\" CONC=\"%s\">\n", 
	    offset, self->base->code->name, 
	    self->weight,
	    self->linear_begin,
	    self->linear_end,
	    gloss);
    ret = accu->append(accu, buf, strlen(buf));


    if (self->base->logic_oper) {
	cu = self->base->logic_oper;

	sprintf(buf, "%s  <LOGIC CLASS=\"%s\""
                 " BEGIN=\"%lu\" END=\"%lu\"/>\n", 
	    offset, cu->code->name, 
	    cu->complexes[0]->linear_begin,
	    cu->complexes[0]->linear_end);
	ret = accu->append(accu, buf, strlen(buf));
    }

    /* subordinate specs */
    for (i = 0; i < NUM_OPERS; i++) {
	complex = self->specs[i];
	if (!complex) continue;
	if (operids) opername = operids[i];

	    
	sprintf(buf, "%s  <SPEC TYPE=\"%s\">\n",
		offset, opername); 
	ret = accu->append(accu, buf, strlen(buf));

	ret = ooComplex_present_XML(complex, 
				    accu,
				    depth+2);

	sprintf(buf, "%s  </SPEC>\n", offset);
	ret = accu->append(accu, buf, strlen(buf));
    }

    sprintf(buf, "%s</COMPLEX>\n", offset);
    ret = accu->append(accu, buf, strlen(buf));

    free(offset);
    return oo_OK;
}


static int
ooComplex_topic_update(struct ooComplex *self, 
		       struct ooAccu *accu)
{
    struct ooInterp *interp;
    struct ooComplex *complex;
    struct ooMindMap *mindmap;
    struct ooTopicIngredient *ingr;
    struct ooConcept *conc;
    int i;
    
    mindmap = accu->decoder->codesystem->mindmap;

    /* walk over your concept proposition structure */
    for (i = 0; i < self->num_interps; i++) {
	interp = self->interps[i];

	printf("\n  ++ UPDATE TOPICS with \"%s\"...\n\n", 
	       interp->conc->name);

	ingr = mindmap->topic_index[interp->conc->id];
	if (!ingr) continue;

	printf("  ++ Supporting the following topics:\n");

	while (ingr) {
	    printf(" *** %s\n", ingr->topic->name);
	    accu->update_topic(accu, ingr, self);
	    ingr = ingr->next;
	}

    }

    /* subordinate specs */
    if (!self->num_interps) {
	for (i = 0; i < NUM_OPERS; i++) {
	    complex = self->specs[i];
	    if (!complex) continue;
	    ooComplex_topic_update(complex, accu);
	}
    }


    return oo_OK;
}

static int
ooComplex_present(struct ooComplex *self, 
		  struct ooAccu *accu,
		  output_type format)
{

    /* topic update */
 
    ooComplex_topic_update(self, accu);

    /* save your structure to persistent memory */
    switch (format) {
    case FORMAT_JSON:
	ooComplex_present_JSON_list(self, accu);
	break;
    case FORMAT_XML:
	ooComplex_present_XML(self, accu, 0);
	break;
    default:
	break;
    }

    return oo_OK;
}



/***** THE END OF PRESENTATION LAYER ****/




static int
ooComplex_linear_merge(struct ooComplex *self,
		       struct ooComplex *parent,
		       struct ooComplex *child)
{
    int i;

    if (parent->linear_begin != -1) {
	for (i = parent->linear_begin; i < parent->linear_end; i++) {
	    self->linear_index[i] = parent->linear_index[i];
	}
	self->linear_begin = parent->linear_begin;
	self->linear_end = parent->linear_end;
    }

    if (child->linear_begin != -1) {
	for (i = child->linear_begin; i < child->linear_end; i++) {
	    self->linear_index[i] = child->linear_index[i];
	}
	if (self->linear_begin == -1) {
	    self->linear_begin = child->linear_begin;
	    self->linear_end = child->linear_end;
	    return oo_OK;
	}
	if (child->linear_begin < self->linear_begin) 
	    self->linear_begin = child->linear_begin;

	if (child->linear_end > self->linear_end) 
	    self->linear_end = child->linear_end;
	return oo_OK;
    }
    
    return FAIL;
}


static int
ooComplex_check_linear_intersection(struct ooComplex *parent,
				    struct ooComplex *child)
{
    struct ooComplex *outer, *inner;
    size_t outer_begin = 0, outer_end = 0;
    size_t inner_begin = 0, inner_end = 0;
    int i;

    if (DEBUG_COMPLEX_LEVEL_3)
	printf("\n   ?? Checking linear intersections of complexes...\n");

    /* parent->str(parent, 1);
       printf("\nAND\n");
       child->str(child, 1);
    */

    if (parent->linear_begin == -1) return oo_OK;
    if (child->linear_begin == -1) return oo_OK;
    if (child->linear_begin >= parent->linear_end) return oo_OK;
    if (child->linear_end <= parent->linear_begin) return oo_OK;

    /* potential intersection */
    if (DEBUG_COMPLEX_LEVEL_3)
	printf("  NB:  Overlapping outer boundaries!\n");


    /* find the preceeding complex */
    if (parent->linear_begin < child->linear_begin) {
	outer = parent;
	inner = child;
    }
    else {
	outer = child;
	inner = parent;
    }

    for (i = outer->linear_begin; i < outer->linear_end; i++) {

	if (outer->linear_index[i]) {
	    outer_begin = outer->linear_index[i]->linear_begin;
	    outer_end = outer->linear_index[i]->linear_end;
	}

	if (inner->linear_index[i]) {
	    inner_begin = inner->linear_index[i]->linear_begin;
	    inner_end = inner->linear_index[i]->linear_end;
	}


	if (outer->linear_index[i])
	    if (i >= inner_begin && i < inner_end) goto intersection;

	if (inner->linear_index[i])
	    if (i >= outer_begin && i < outer_end) goto intersection;
    }
    

    return oo_OK;

intersection:
    /* Houston, we have a problem: linear intersection! ))) 
     * let's check if it's legitimate here?
     * In some cases atomic intersection is OK:
     * eg. Rus. ПЕК-ТИ -> ПЕЧЬ 
     */
    if (DEBUG_COMPLEX_LEVEL_3) {
	printf("  Linear conflict at pos: %d!!!...",
	       i);
    }
    
    /*if (c->base->common_unit && p->base->common_unit &&
      c->base->common_unit == p->base->common_unit)
      continue;*/
    return FAIL;
}

static int
ooComplex_check_logic_opers(struct ooComplex *self, struct ooComplex *aggr)
{
    struct ooComplex *complex;
    struct ooConcUnit *result, *cu;
    size_t i, endpos;

    if (DEBUG_COMPLEX_LEVEL_3)
	printf(" ?? Checking logical operators...\n");

    /* create an aggregator */
    result = self->base->agenda->alloc_unit(self->base->agenda);
    if (!result) return oo_NOMEM;

    result->make_instance(result, self->base->code, NULL);

    complex = result->complexes[0];
    complex->reset(complex);
    complex->weight = aggr->weight + 1;
    complex->linear_begin = aggr->linear_begin;
    complex->linear_end = aggr->linear_end;

    result->linear_pos = complex->linear_begin;
    result->coverage = complex->linear_end - complex->linear_begin;

    complex->is_free = false;
    complex->is_updated = true;
    complex->specs[AGGREGATES] = aggr;

    /* make a copy of linear index */
    for (i = complex->linear_begin; i < complex->linear_end; i++) 
	complex->linear_index[i] = aggr->linear_index[i];

    result->num_complexes = 1;
    aggr->aggregate = result;


    /* NB: 
     * our basic assumption is that
     * logical opers are linearly located
     * in-between two peers */    
    endpos = aggr->specs[NEXT_IN_GROUP]->linear_begin;

    if (self->linear_end < endpos) {
	for (i = self->linear_end; i < endpos; i++) {
	    cu = self->base->agenda->logic_oper_index[i];
	    if (!cu) continue;
	    /* check linear intersection */
	    if (cu->linear_pos + cu->coverage > endpos) break;

	    if (DEBUG_COMPLEX_LEVEL_3) {
		printf("   ++ Logical marker found!\n");
		cu->str(cu, 1);
	    }

	    result->is_group = true;
	    result->logic_oper = cu;

	    /* TODO: consider syntax within operators.. cases like "and/or" */
	    complex->weight += cu->complexes[0]->weight + OPER_SUCCESS_BONUS;
	    break; /* NB: one oper is enough */

	}
    }





    return oo_OK;
}

static int
ooComplex_check_delimiters(struct ooComplex *aggr)
{

    /*printf("   ?? Checking linear delimiters...\n"); */



    return oo_OK;
}




/**
 * remove any previous references to this child complex
 */
static int
ooComplex_forget(struct ooComplex *self,
		 struct ooComplex *child,
		 struct ooCodeAttr *attr)
{
    int i, j, new_begin_pos, new_end_pos;
    bool is_dropped = false;

    if (DEBUG_COMPLEX_LEVEL_3) 
	printf("    ?? Any previous references to %p? If so, forget them...\n", child); 

    if (!self->specs[attr->operid]) return FAIL;

    if (self->specs[attr->operid] != child) return FAIL;

    self->specs[attr->operid] = NULL;

    if (DEBUG_COMPLEX_LEVEL_3) 
	printf("   ++ Dropped child at operid %d ..." 
               "  Clearing up the linear index...\n", 
	       attr->operid); 

    /* remove all references from the linear index */
    for (i = self->linear_begin; i < self->linear_end; i++) {
	if (self->linear_index[i] == child->prev_linear_index[i]) {
	    self->linear_index[i] = NULL;
	}
    }

    /* update linear boundaries */
    for (i = self->linear_begin; i < self->linear_end; i++) {
	if (self->linear_index[i]) {
	    self->linear_begin = i;
	    break;
	}
    }

    /*printf("DROP: %d\n", self->linear_end);*/

    for (i = self->linear_end - 1; i > 0; i--) {
	if (self->linear_index[i]) {
	    self->linear_end = i + self->linear_index[i]->base->coverage;
	    break;
	}
    }



    return oo_OK;
}

/**
 * try to perform conceptual binding of usages
 */
static int
ooComplex_discriminate_usage(struct ooComplex *self,
			     struct ooComplex *child_complex,
			     oper_type operid,
			     struct ooComplex *result)
{
    struct ooDeriv *deriv;
    struct ooInterp *interp;
    int i;

    /* child must know about all fixed bindings */

    /* derivation check */
    deriv = child_complex->base->code->deriv_matches[operid];

    while (deriv) {
	deriv->str(deriv);

	if (!strcmp(deriv->arg_code_name, self->base->code->name)) {

	    if (deriv->code)
		result->base->code = deriv->code;

	    if (result->num_interps < INTERP_POOL_SIZE) {
		interp = result->interps[result->num_interps];
		if (!deriv->code_usage) continue;
		if (!deriv->code_usage->conc) continue;
		/* valid reference to specific usage */
		interp->conc = deriv->code_usage->conc;
		result->num_interps++;
	    }
	}
	deriv = deriv->next;
    }

    return oo_OK;
}



/**
 * try to merge two syntactic branches
 */
static int
ooComplex_join(struct ooComplex *self,
	       struct ooComplex *child_complex,
	       struct ooComplex *aggr_complex,
	       struct ooCodeAttr *attr)
{   struct ooComplex *complex;
    struct ooConcUnit *result;

    char **operids = NULL, *opername = "???";
    size_t i;
    int ret;

    oper_type operid = attr->operid;

    if (DEBUG_COMPLEX_LEVEL_1) {
	if (self->base->agenda) {
	    operids = self->base->agenda->codesystem->operids;
	    if (operids) {
		opername = operids[operid];
	    }
	}
    }

    if (DEBUG_COMPLEX_LEVEL_3)
	printf("    ... Let's try to merge existing complex \"%s\" %p (weight: %d)\n"
	       "        and child \"%s\" %p (weight: %d)  OPER: %s\n",
	       self->base->code->name, self, self->weight,
	       child_complex->base->code->name, child_complex, 
	       child_complex->weight, opername);

    /* only one child of a kind is allowed */
    if (self->specs[operid])
	return oo_FAIL;

    if (ooComplex_check_linear_intersection(self, child_complex)) {
	if (DEBUG_COMPLEX_LEVEL_3)
	    printf("    -- Intersection conflict! :(((\n");
	return oo_FAIL;
    }

/*    if (ooComplex_check_linear_order(self, child_complex, attr->linear_order)) {
	if (DEBUG_COMPLEX_LEVEL_3)
	    printf("    -- Linear order conflict! :(((\n");
	return FAIL;
    }
*/

    /* TODO: check linear _adaptation_ conditions! */



    /* resetting the aggregated complex */
    ret = aggr_complex->reset(aggr_complex);

    /* copying all the specs of the base */
    for (i = 0; i < NUM_OPERS; i++)
	aggr_complex->specs[i] = self->specs[i];

    /* adopt a new child */
    aggr_complex->specs[operid] = child_complex;

    /* update the linear index of terms */
    ooComplex_linear_merge(aggr_complex, self, child_complex);

    aggr_complex->is_updated = true;
    aggr_complex->is_free = false;

    /* memoization */
    aggr_complex->prev_weight = self->weight;

    aggr_complex->weight = self->weight + child_complex->weight;

    /* extra bonuses */
    if (operid != NEXT_IN_GROUP)
	aggr_complex->weight += OPER_SUCCESS_BONUS;


     /* TODO: evaluate the discourse propositions, establish conceptual paths */
    /*if (DEBUG_COMPLEX_LEVEL_2) {*/

    if (operid != NEXT_IN_GROUP) {
	printf("  ... Evaluating conceptual bindings between "
           " \"%s\" and its child \"%s\"...\n", 
	   self->base->code->name, 
	   child_complex->base->code->name);
	ret = ooComplex_discriminate_usage(self, child_complex, 
					   operid, aggr_complex);
    }



    if (DEBUG_COMPLEX_LEVEL_2) {
	printf("\n    ++ Join complete! New weight: %d  complex: %p\n", 
	       aggr_complex->weight, aggr_complex);
	/*ooComplex_present_linear_index(aggr_complex);*/
    }


    ret = ooComplex_check_delimiters(aggr_complex);

    /* peers need to check the logic opers */
    if (operid == NEXT_IN_GROUP) 
	return ooComplex_check_logic_opers(self, aggr_complex);


    /* any upper units to be created? */
    if (!attr->stackable) return oo_OK;

    /* create an aggregator */
    result = self->base->agenda->alloc_unit(self->base->agenda);
    if (!result) return oo_NOMEM;

    result->make_instance(result, aggr_complex->base->code, NULL);

    complex = result->complexes[0];
    complex->reset(complex);
    complex->weight = aggr_complex->weight;
    complex->linear_begin = aggr_complex->linear_begin;
    complex->linear_end = aggr_complex->linear_end;

    result->linear_pos = complex->linear_begin;
    result->coverage = complex->linear_end - complex->linear_begin;

    complex->is_aggregate = true;
    complex->is_free = false;
    complex->is_updated = true;
    complex->specs[AGGREGATES] = aggr_complex;

    
    for (i = complex->linear_begin; i < complex->linear_end; i++) 
	complex->linear_index[i] = aggr_complex->linear_index[i];

    result->num_complexes = 1;

    aggr_complex->aggregate = result;

    return oo_OK;
}


/*  Complex Reset */
static int 
ooComplex_reset(struct ooComplex *self)
{
    struct ooInterp *interp;
    int i;
    
    self->aggregate = NULL;

    self->is_terminal = false;
    self->is_aggregate = false;

    self->is_free = true;
    self->is_updated = false;

    self->next = NULL;
    self->next_peer = NULL;

    self->weight = 0;
    self->linear_begin = -1;
    self->linear_end = -1;
    self->num_terminals = 0;

    self->begin_delim = NULL;
    self->end_delim = NULL;

    for (i = 0; i < NUM_OPERS; i++)
	self->specs[i] = NULL;

    self->adapt_context.affects_prepos = NULL;
    self->adapt_context.affects_postpos = NULL;
    self->adapt_context.affected_prepos = NULL;
    self->adapt_context.affected_postpos = NULL;

    self->adapt_weight = 0;

    for (i = 0; i < INTERP_POOL_SIZE; i++) {
	interp = &self->interp_storage[i];
	interp->conc = NULL;
	interp->weight = 0;
	self->interps[i] = interp;
    }
    self->num_interps = 0;

    /* memoization of previous solution */
    self->prev_weight = 0;
    self->prev_linear_begin = -1;
    self->prev_linear_end = -1;

    return oo_OK;
}


/*  Complex Initializer */
int ooComplex_init(ooComplex *self)
{
    size_t i;

    self->str = ooComplex_str;
    self->reset = ooComplex_reset;
    self->join = ooComplex_join;
    self->forget = ooComplex_forget;
    self->present = ooComplex_present;
    self->reset(self);

    for (i = 0; i < INPUT_BUF_SIZE; i++)
	self->linear_index[i] = NULL;

    return oo_OK;
}
