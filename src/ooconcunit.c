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
 *   ------------
 *   ooconcunit.c
 *   OOmnik Concept Unit implementation
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ooconfig.h"
#include "ooconstraint.h"
#include "oocode.h"
#include "ooconcunit.h"
#include "ooagenda.h"
#include "oocomplex.h"

/* forward declarations */
static int
ooConcUnit_check_children(struct ooConcUnit *self, struct ooCode *code);

static int
ooConcUnit_check_parents(struct ooConcUnit *self, struct ooCode *code);

static int
ooConcUnit_add_child(struct ooConcUnit *self,
		     struct ooComplex  *child,
		     struct ooCodeSpec *spec);

 
/****************************** PRESENTATION **********************************/


/* string representation */
static int
ooConcUnit_str(struct ooConcUnit *self,
			size_t depth)
{   struct ooComplex *complex;
    size_t i, offset_size = sizeof(char) * OFFSET_SIZE * depth;
    char *offset = malloc(offset_size + 1);

    const char *cs_name = "None";
    const char *myclass = "None";

    memset(offset, SPACE_CHAR, offset_size);
    offset[offset_size] = '\0';

    if (!self->code) {
	printf("%s CU: %p CS: %d concid: %lu\n", 
	       offset, self, self->cs_id, (unsigned long)self->concid);
    }
    else {

	printf("%s  ** unit \"%s\" [%lu:%lu] %p\n"
               "%s  is_present: %d num_complexes: %d\n",
	       offset, self->code->name,
	       (unsigned long)self->linear_pos, 
	       (unsigned long)self->linear_pos + self->coverage, 
	       self, offset, 
	       self->is_present, 
	       self->num_complexes);

	if (self->context != NULL) {
	    if (self->context->affects_prepos != NULL) {
		self->context->affects_prepos->str(self->context->affects_prepos);
	    }
	    if (self->context->affects_postpos != NULL) {
		self->context->affects_postpos->str(self->context->affects_postpos);
	    }

	}
    }

    for (i = 0; i < self->num_complexes; i++) {
	complex = self->complexes[i];
        printf("\n%s  [complex %zu   mem: %p weight: %d]\n", 
	       offset, i, complex, complex->weight);

	complex->str(complex, depth);
    }

    printf("\n");
    free(offset);
    return oo_OK;
}


/****************************** BASIC ROUTINES **********************************/


/**
 *  transfer all complexes from top to basic storage
 *  sorted by weight
 */
static int 
ooConcUnit_sort_complexes(struct ooConcUnit *self)
{
    struct ooComplex *c, *tmp;
    int i, j;

    if (DEBUG_CONC_LEVEL_3)
	printf("\n    ... Sorting the complexes... num_complexes: %d top_count: %d\n", 
	       self->num_complexes, self->top_count);

    self->num_complexes = 0;

    for (i = 0; i < self->top_count; i++) {
	c = self->top_complexes[i];

	if (DEBUG_CONC_LEVEL_3)
	    printf("top slot %p: updated: %d weight: %d\n", 
		   c, c->is_updated, c->weight);

	if (!c->is_free) self->num_complexes++;


	for (j = 0; j < CONCUNIT_COMPLEX_POOL_SIZE; j++) {
	    if (!self->complexes[j]) {
		self->complexes[j] = c;
		/*printf("   %p -> slot %d\n", c, j);*/
		break;
	    }

	    if (c->weight > self->complexes[j]->weight) {
		tmp = self->complexes[j];
		self->complexes[j] = c;
		/*printf("   %p -> slot %d\n", c, j);*/
		c = tmp;
	    }
	}

    }



    if (DEBUG_CONC_LEVEL_3) {
	printf("\n  Total num of valid complexes after sorting: %d\n", 
	       self->num_complexes);

	for (i = 0; i < CONCUNIT_COMPLEX_POOL_SIZE; i++) {
	    c = self->complexes[i];
	    printf("  slot %p: updated: %d weight: %d\n", 
		   c, c->is_updated, c->weight);
	}
    }



    return oo_OK;
}



static int
ooConcUnit_inform_parents(struct ooConcUnit *self,
			  struct ooComplex *child)
{
    struct ooComplex *complex;
    size_t i;
    int ret;

    if (self->fixed_parent) {

	printf("\n \"%s\" must inform my FIXED PARENT: %s operid: %d\n",
	       self->code->name, self->fixed_parent->code->name,  self->fixed_operid);

	for (i = 0; i < self->fixed_parent->num_complexes; i++) {
	    complex = self->fixed_parent->complexes[i];
	    complex->specs[self->fixed_operid] = child;
	    complex->weight -= child->prev_weight;
	    complex->weight += child->weight;
	}

    }

    return oo_OK;
}


/**
 * 
 */
static int
ooConcUnit_update_solutions(struct ooConcUnit *self)
{
    struct ooComplex *complex;
    struct ooConcUnit *cu;
    size_t i;
    int ret;


    for (i = 0; i < self->num_complexes; i++) {
	complex =  self->complexes[i];
	
	if (DEBUG_CONC_LEVEL_3)
	    printf("\n ?? New aggregate's complex: %p updated: %d aggr %p\n", 
	       complex, complex->is_updated, complex->aggregate);

	if (!complex->is_updated) continue;

	if (!complex->aggregate) {
	    /* inform your parents about changes */
	    ret = ooConcUnit_inform_parents(self, complex);
	    complex->is_updated = false;
	    continue;
	}

	/* link the aggregate unit! */
	if (DEBUG_CONC_LEVEL_3)
	    printf("\n ?? New aggregate can have its children!\n\n");

	ret = ooConcUnit_check_children(complex->aggregate, complex->aggregate->code);

	ret = ooConcUnit_check_parents(complex->aggregate, complex->aggregate->code);

	complex->is_updated = false;

    }



    return oo_OK;
}


static int
ooConcUnit_add_child(struct ooConcUnit *self,
		     struct ooComplex  *child,
		     struct ooCodeSpec *spec)
{
    struct ooComplex *complex, *result_complex;
    int curr_weight = 0;
    size_t i, j, free_slot_idx;
    bool link_success = false;
    int ret;

    if (DEBUG_CONC_LEVEL_3) {
	printf("\n   ?? let's consider complex %s (%p) weight: %d\n", 
	       child->base->code->name, (void*)child, child->weight);

	printf("\n    num of complexes in parent: %lu\n", 
	       (unsigned long)self->num_complexes);
    }

    /* first of all let us drop any existing references
     * to this complex */
    for (i = 0; i < self->num_complexes; i++) {
	complex = self->complexes[i];
	ret = complex->forget(complex, child, spec);
    }

    free_slot_idx = CONCUNIT_COMPLEX_POOL_SIZE - self->top_count - 1;

    /* start comparison */
    for (i = 0; i < free_slot_idx; i++) {
	complex = self->complexes[i];
	if (complex->is_free) break;

	/* allocate memory from the pool */
	result_complex = self->complexes[free_slot_idx];

	if (DEBUG_CONC_LEVEL_3) {
	    printf("\n    -- current complex %zu: (%p) weight: %d  free_slot_idx:%d\n", 
		   i, complex, complex->weight, free_slot_idx);
	}

	/* try to merge these complexes */
	ret = complex->join(complex, child, result_complex, spec);

	if (DEBUG_CONC_LEVEL_3)
	    printf("    == Join result: %d\n", ret);

	if (ret != oo_OK) continue;

	link_success = true;

	/* join successful */
	self->top_complexes[self->top_count] = result_complex;
	self->top_count++;

	/* no more vacant slots? */
	if (self->top_count == CONCUNIT_COMPLEX_POOL_SIZE - 1)
	    break;

	free_slot_idx--;
    }

    if (link_success) return oo_OK;
    return oo_FAIL;
}

static int
ooConcUnit_save_remaining_slots(struct ooConcUnit *self)
{
    struct ooComplex *complex;
    size_t i, num_valid_slots;
    
    num_valid_slots = CONCUNIT_COMPLEX_POOL_SIZE - self->top_count;
    if (DEBUG_CONC_LEVEL_3)
	printf("\n    +++ num of slots to save: %lu\n", num_valid_slots);
   
    for (i = 0; i < CONCUNIT_COMPLEX_POOL_SIZE; i++) {
	complex =  self->complexes[i];

	if (DEBUG_CONC_LEVEL_4)
	    printf(" slot %d) %p updated: %d\n", i, complex, complex->is_updated);

	if (i < num_valid_slots) {
	    self->top_complexes[self->top_count] = complex;
	    self->top_count++;
	}
	self->complexes[i] = NULL;
    }

    self->num_complexes = 0;

    return oo_OK;
}

/**
 * Our child has some updated complexes.
 *
 * Check these new complexes against the current ones
 * and join them if they provide better weight.
 */
static int
ooConcUnit_add_children(struct ooConcUnit *self,
			struct ooConcUnit *child,
			struct ooCodeSpec *spec)
{
    struct ooComplex *complex;
    size_t i;
    int ret;

    if (DEBUG_CONC_LEVEL_2)
	printf("\n    ?? CU \"%s\" (%p, pos: %lu)\n"
               "         is considering complexes from \"%s\""
	       " (%p, pos: %lu)\n",
	       self->code->name,  self, (unsigned long)self->linear_pos, 
               child->code->name, child, (unsigned long)child->linear_pos);

    self->top_count = 0;

    if (DEBUG_CONC_LEVEL_3)
	printf("\n   ++ Current number of complexes of \"%s\": %u\n",
	       self->code->name, self->num_complexes);

    for (i = 0; i < child->num_complexes; i++) {
	complex = child->complexes[i];
	/*if (!complex->is_updated) continue;*/

	if (DEBUG_CONC_LEVEL_3) {
	    printf("    -- consider child complex %d) (%p)\n", 
		   i, complex);
	    complex->str(complex, 1);
	}

	ret = ooConcUnit_add_child(self, complex, spec);
	if (ret != oo_OK) {
	    if (DEBUG_CONC_LEVEL_3)
		printf("   -- Failed to add complex %p :((\n", complex);
	    continue;
	}

	if (DEBUG_CONC_LEVEL_3)
	    printf("   ++ Successfully added complex %p!\n", complex);

	/* no more vacant slots? */
	if (self->top_count == CONCUNIT_COMPLEX_POOL_SIZE - 1)
	    break;
    }

    if (DEBUG_CONC_LEVEL_3)
	printf("\n    == Total number of updated complexes: %d\n", self->top_count);

    if (!self->top_count) return oo_FAIL;

    ooConcUnit_save_remaining_slots(self);

    ooConcUnit_sort_complexes(self);

    ooConcUnit_update_solutions(self);


    return oo_OK;
}


/**
 * find units of the same class and establish a group 
 */
static int
ooConcUnit_check_peers(struct ooConcUnit *self)
{
    struct ooConcUnit *peer;
    struct ooCodeSpec spec;
    size_t concid;
    int ret;

    concid = self->code->id;
    if (self->code->baseclass)
	concid = self->code->baseclass->id;

    if (concid >= AGENDA_INDEX_SIZE) return oo_FAIL;

    peer = self->agenda->index[concid];
    if (!peer) {
	if (DEBUG_CONC_LEVEL_4)
	    printf("  -- CU \"%s\" has no potential peers...\n", self->code->name);
	return oo_FAIL;
    }

    if (DEBUG_CONC_LEVEL_4)
	printf("\n  Checking the peers of CU \"%s\" (mem: %p)\n",
	       self->code->name, (void*)peer);

    spec.operid = OO_NEXT;

    while (peer) {
	if (DEBUG_CONC_LEVEL_4)
	    printf("  !! CU \"%s\" (mem: %p, linear_pos: %lu)\n"
                   "     is trying to link with its peer (mem: %p, linear_pos: %lu)\n",
		   self->code->name, (void*)self, (unsigned long)self->linear_pos,
		   (void*)peer, (unsigned long)peer->linear_pos);

	/* NB: only real units can form a group! */
	if (!peer->is_present) goto next_peer;

	ret = ooConcUnit_add_children(peer, self, &spec);

    next_peer:
	peer = peer->next;
    }



    return oo_OK;
}

static int
ooConcUnit_check_linear_order(struct ooConcUnit *self, 
			      struct ooConcUnit *child,
			      linear_type linear_order)
{
    size_t i, j;
    int distance = 0;

    if (DEBUG_CONC_LEVEL_4) {
	printf("    ?? Checking linear constraints between parent \"%s\" and child \"%s\":\n"
               "       linear_order: %d parent linear_pos: %lu child linear_pos: %lu\n", 
	       self->code->name, child->code->name,
	       linear_order, (unsigned long)self->linear_pos, (unsigned long)child->linear_pos);
    }

    if (linear_order == OO_PRE_POS) {
	if (self->linear_pos <= child->linear_pos) return oo_FAIL;

	if (child->linear_pos + child->coverage > self->linear_pos) return oo_FAIL;

	/* check max distance between the units */
	distance = self->linear_pos - child->linear_pos - child->coverage;

	if (DEBUG_CONC_LEVEL_4) 
	    printf("   distance between parent and child: %d\n", distance);

	if (distance > MAX_CONTACT_DISTANCE) return oo_FAIL;
    }

    if (linear_order == OO_POST_POS) {
	if (self->linear_pos > child->linear_pos) return oo_FAIL;
    }


    return oo_OK;
}

static int
ooConcUnit_join_predicted(struct ooConcUnit *self, 
			  struct ooConcUnit *pred_cu,
			  struct ooCode *code)
{
    size_t i, j;
    struct ooConcUnit *child;
    struct ooComplex **specs;
    struct ooCodeSpec *code_spec;
    int ret;

    if (DEBUG_CONC_LEVEL_2) {
	printf("   ... trying to occupy the position of a predicted unit \"%s\"...\n", 
	       code->name);
	code->str(code);
    }

    /* NB: there's only one complex in the predicted unit */
    specs = pred_cu->complexes[0]->specs;

    /* check every child */
    for (i = 0; i < OO_NUM_OPERS; i++) {
	if (!specs[i]) continue;

	child = specs[i]->base;
	code_spec = code->children[i];
	if (!code_spec) continue;

	if (DEBUG_CONC_LEVEL_3)
	    printf("    LINEAR ORDER CONSTRAINT: %d\n", code_spec->linear_order);
	    
	if (code_spec->linear_order == OO_PRE_POS) {
	    ret = ooConcUnit_check_linear_order(self, child, code_spec->linear_order);

	    if (DEBUG_CONC_LEVEL_3)
		printf("  == linear order check verdict: %d\n", ret);

	    if (ret != oo_OK) continue;
	}

	ooConcUnit_add_children(self, child, code_spec);
    }
    return oo_OK;
}


static int
ooConcUnit_check_children(struct ooConcUnit *self, 
			  struct ooCode *code)
{
    struct ooConcUnit *cu;
    size_t concid;
    int ret;

    if (DEBUG_CONC_LEVEL_3)
	printf("\n    ?? Anybody waiting for the parent \"%s\" (%d)?\n", 
	   code->name, code->id);

    concid = self->code->id;
    if (self->code->baseclass)
	concid = self->code->baseclass->id;

    if (concid >= AGENDA_INDEX_SIZE) return oo_FAIL;

    cu = self->agenda->index[concid];

    if (DEBUG_CONC_LEVEL_3)
	if (!cu) printf("      -- No predictions found.\n");

    while (cu) {
	if (!cu->code) goto next_cu;
	/* not a prediction? */
	if (cu->is_present) goto next_cu;

	/* predicted unit found */
	if (DEBUG_CONC_LEVEL_3) {
	    printf("    ++ PREDICTION of  %s (%d) found!\n",
		   code->name, code->id);
	    cu->str(cu, 1);
	}

	ooConcUnit_join_predicted(self, cu, code);
    next_cu:
	cu = cu->next;
    }

    /* check inheritance */
    if (code->num_children && code->children[OO_IS_SUBCLASS] != NULL)
	    return ooConcUnit_check_children(self, code->children[OO_IS_SUBCLASS]->code);

    return oo_FAIL;
}


static int
ooConcUnit_check_parent(struct ooConcUnit *self, 
			oo_oper_type operid, 
			struct ooCodeSpec *child_spec)
{
    struct ooConcUnit *parent;
    struct ooComplex *complex;
    struct ooCodeSpec *spec;
    struct ooCode *parent_code;
    size_t i, j, concid;
    int ret;

    if (!child_spec->code) return oo_FAIL;
    parent_code = child_spec->code;

    concid = parent_code->id;
    if (parent_code->baseclass)
	concid = parent_code->baseclass->id;

    if (concid >= AGENDA_INDEX_SIZE) return oo_FAIL;

    parent = self->agenda->index[concid];
    spec = parent_code->children[operid];

    /* existing parents */
    while (parent) {
	if (!parent->is_present) goto next_parent;

	if (spec && spec->linear_order != OO_ANY_POS) {

	    ret = ooConcUnit_check_linear_order(parent, self, spec->linear_order);
	    if (DEBUG_CONC_LEVEL_4) 
		printf("    == linear order check result: %d\n", ret);

	    if (ret != oo_OK) goto next_parent;
	}
	ooConcUnit_add_children(parent, self, spec);
    next_parent:
	parent = parent->next;
    }
	
    /* create new parent unit anyway */
    /* STR */

    parent = self->agenda->alloc_unit(self->agenda);
    if (!parent) return oo_NOMEM;
    parent->make_instance(parent, parent_code, NULL);


    /* fast linking with child's complexes */
    for (i = 0; i < self->num_complexes; i++) {
	complex = parent->complexes[i];
	complex->reset(complex);

	complex->weight = self->complexes[i]->weight;
	complex->linear_begin = self->complexes[i]->linear_begin;
	complex->linear_end = self->complexes[i]->linear_end;
	complex->is_free = false;

	complex->specs[operid] = self->complexes[i];
	
	for (j = complex->linear_begin; j < complex->linear_end; j++) 
	    complex->linear_index[j] = self->complexes[i]->linear_index[j];

	parent->num_complexes++;
    }

    if (DEBUG_CONC_LEVEL_3)
	printf("\n    !! Predicting empty parent CU: %s (mem: %p)\n", 
	       parent_code->name, parent);

    /* implied parent linking */
    if (child_spec->implied_parent) {
	if (DEBUG_CONC_LEVEL_3)
	    printf("\n    !! Parent \"%s\" is implied and needs to be linked!\n", 
		   parent_code->name);

	parent->linear_pos = self->linear_pos;
	parent->coverage = self->coverage;
	parent->is_present = true;
	ret = parent->link(parent);
    }

    /* TODO: ellipsed parent linking */



    self->agenda->register_unit(self->agenda, parent, parent_code);

    return oo_OK;
}


static int
ooConcUnit_check_parents(struct ooConcUnit *self, struct ooCode *code)
{   size_t i;
    struct ooCode *parent_code;
    struct ooCodeSpec *spec;
    bool gotcha = false;
    int ret;

    if (DEBUG_CONC_LEVEL_3)
	printf("\n   .. Checking the parents of the CU \"%s\"...",
	       self->code->name);

    /* inheritance goes first */
    if (code->num_children) {
	if (code->children[OO_IS_SUBCLASS]) {
	    if (DEBUG_CONC_LEVEL_3) {
		printf("\n    NB: Before checking the parents of \"%s\"\n"
		       "          we\'ll examine its superclass \"%s\"!\n", 
		       code->name, code->children[OO_IS_SUBCLASS]->code->name);
	    }
	    ooConcUnit_check_parents(self, code->children[OO_IS_SUBCLASS]->code);


	}
    }
    
    if (DEBUG_CONC_LEVEL_3) {
	printf("\n    ?? parent codes of \"%s\":\n      ", code->name);

	/* list parent codes */
	for (i = 0; i < OO_NUM_OPERS; i++) {
	    if (i == OO_IS_SUBCLASS) continue;
	    if (!code->parents[i]) continue;
	    spec = code->parents[i];

	    /* no need to check the implied parent */
	    /*if (spec->implied_parent) continue;*/

	    while (spec) {
		printf("\"%s\"   ", spec->code->name);
		spec = spec->next;
		gotcha = true;
	    }
	}
	if (!gotcha) 
	    printf("-- \"%s\" has no parents.\n",  code->name);
	else
	    printf("\n");
    }

    /* check parent codes */
    for (i = 0; i < OO_NUM_OPERS; i++) {
	if (i == OO_IS_SUBCLASS) continue;
	if (!code->parents[i]) continue;

	spec = code->parents[i];
	/* no need to check the implied parent */
	/*if (spec->implied_parent) continue;*/
	
	while (spec) {
	    ooConcUnit_check_parent(self, i, spec);
	    spec = spec->next;
	}
    }

    return oo_OK;
}


static int
ooConcUnit_positional_link(struct ooConcUnit *self)
{   
    size_t i;
    struct ooComplex *c, *prev_c;

    if (DEBUG_CONC_LEVEL_1)
	printf("    !! Positional linking of CU \"%s\" (linear_pos: %lu)\n",
	       self->code->name, self->linear_pos);

    /* try to become the first element */
    if (!self->agenda->linear_last) {

	/* TODO: remove specific concid! */
	/* zero numeric value can only take the final position */
	if (self->concid == 1)
	    return oo_OK;

	self->agenda->linear_last = self;
	return oo_OK;
    }

    /* establish link */
    /*
    printf(" Need to link with %p!\n", self->agenda->linear_last);
    self->str(self, 1);
    */

    c = self->complexes[0];
    prev_c = self->agenda->linear_last->complexes[0];

    /*c->specs[PRECEEDS] = prev_c;*/
    c->weight += prev_c->weight;
    c->linear_begin = prev_c->linear_begin;
    c->linear_end = self->linear_pos + self->coverage;

    return oo_OK;
}


static int
ooConcUnit_link(struct ooConcUnit *self)
{
    struct ooCode *code;
    int ret;

    code = self->code;
    if (!code) return oo_FAIL;

    if (code->baseclass)
	code = code->baseclass;

    /* do not try to link unrecognized units */
    if (code->id == 0) {
	if (DEBUG_CONC_LEVEL_1) 
	    printf("Not linking unrecognized code...\n");
	return oo_FAIL;
    }

    if (DEBUG_CONC_LEVEL_1)
	printf("    !! Linking CU \"%s\" (linear_pos: %lu)"
	       " with the current state of agenda...\n",
	       code->name, self->linear_pos);

    /* positional linking needed */
    if (self->code->cs->type == CS_POSITIONAL)
	return ooConcUnit_positional_link(self);



    /* check delimiters */
    if (self->code->type == CODE_SEPARATOR) {
	if (DEBUG_CONC_LEVEL_3)
	    printf("   ... Separator found...\n");

	return oo_OK;
    }


    /* check selection markers: AND/OR */
    if (self->code->type == CODE_GROUP_MARKER) {
	if (DEBUG_CONC_LEVEL_3)
	    printf("   ... Grouping marker found...\n");

	return oo_OK;
    }


    /* check negation */

    /* peers as group elements */
    /*if (code->allows_grouping)
      ret = ooConcUnit_check_peers(self);*/


    if (code->num_children) {
	ret = ooConcUnit_check_children(self, code);
	if (ret == oo_OK) return ret;
    }


    ret = ooConcUnit_check_parents(self, code);

    /*ret = ooConcUnit_check_linear_neighbour(self);*/

    /* TODO:
      if linking was successful with substantial coverage achieved,
      clean up the resources.. remove some of the CUs  */

    return oo_OK;
}

static int
ooConcUnit_make_instance(struct ooConcUnit *self, 
			 struct ooCode *myclass,
			 struct ooConcUnit *myclass_terminal)
{
    struct ooComplex *complex;
    struct ooCodeUsage *usage;

    size_t i, pos, coverage;
    int ret;

    self->code = myclass;
    self->concid = myclass->id;

    self->terminals = myclass_terminal;

    /* enough for the predicted units */
    if (!myclass_terminal) return oo_OK;

    pos = myclass_terminal->linear_pos;

    coverage = myclass_terminal->coverage;
    self->coverage = coverage;
    self->linear_pos = pos;

    self->start_term_pos = myclass_terminal->start_term_pos;
    self->num_terminals = myclass_terminal->num_terminals;

    /* pass on the adaptation constraints */
    self->context = myclass_terminal->context;

    /* if (self->context) {
        complex->adapt_context->affects_prepos = self->context->affects_prepos;
	complex->adapt_context->affects_postpos = self->context->affects_postpos;
	complex->adapt_context->affected_prepos = self->context->affected_prepos;
	complex->adapt_context->affected_postpos = self->context->affected_postpos;
    }
    */

    /* terminal complex */
    complex = self->complexes[0];
    ret = complex->reset(complex);

    complex->is_free = false;

    /* give more weight to the sequences with no garbage in-between */
    if (myclass_terminal->is_sparse)
	complex->weight = coverage;
    else
	complex->weight = coverage * coverage * coverage;

    if (myclass->verif_level > 0) 
	complex->weight *= myclass->verif_level * CODE_VERIFICATION_BONUS;

    complex->linear_begin = pos;
    complex->linear_end = pos + coverage;
    complex->linear_index[pos] = complex;

    /* add interps */
    for (i = 0; i < myclass->num_usages; i++) {
	usage = myclass->usages[i];

	if (!usage->conc) continue;
	if (complex->num_interps == INTERP_POOL_SIZE) break;

	complex->interps[complex->num_interps]->conc = usage->conc;
	complex->num_interps++;
    }
    self->num_complexes = 1;

    return oo_OK;
}


/*  ooConcUnit Erazer: setting zero values */
static int 
ooConcUnit_reset(struct ooConcUnit *self)
{
    size_t i;
    struct ooComplex *complex;
    struct ooConcUnit **cu;

    self->linear_pos = 0;
    self->is_present = false;
    self->is_implied = false;

    self->coverage = 0;
    self->concid = 0;
    self->code = NULL;
    self->context = NULL;

    self->is_group = false;
    self->group_closed = false;
    self->logic_oper = NULL;

    self->fixed_parent = NULL;
    self->fixed_operid = 0;

    self->terminals = NULL;
    self->num_terminals = 0;
    self->start_term_pos = 0;
    self->is_sparse = false;

    self->is_group = false;
    self->common_unit = NULL;

    self->next = NULL;
    self->prev = NULL;

    self->num_complexes = 0;
    self->recursion_level = 0;

    return oo_OK;
}


/*  ooConcUnit Initializer */
extern int 
ooConcUnit_init(struct ooConcUnit *self)
{
    size_t i;
    struct ooComplex *complex;
    int ret;

    /* bind your methods */
    self->str = ooConcUnit_str;
    self->link = ooConcUnit_link;
    self->add_children = ooConcUnit_add_children;
    self->make_instance = ooConcUnit_make_instance;
    self->reset = ooConcUnit_reset;

    /* initialize the pool of complex solutions */
    for (i = 0; i < CONCUNIT_COMPLEX_POOL_SIZE; i++) {
	complex = &self->complex_storage[i];
	ooComplex_init(complex);
	complex->base = self;
	self->complexes[i] = complex;
    }

    self->reset(self);

    return oo_OK;
}


