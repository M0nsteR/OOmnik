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
 *   Code contributors:
 *         * Jenia Krylov <info@e-krylov.ru>
 *
 *   ----------
 *   ooagenda.h
 *   OOmnik Agenda implementation
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ooconfig.h"

#include "ooagenda.h"
#include "oocode.h"
#include "ooconcunit.h"
#include "oocodesystem.h"
#include "oodecoder.h"
#include "ooaccumulator.h"


static int
ooAgenda_present_linear_solution(struct ooAgenda *self,
				 struct ooComplex *c, 
				 int linear_begin,
				 int linear_end);

/*  Destructor */
static int
ooAgenda_del(struct ooAgenda *self)
{
    /* free up yourself */
    free(self);
    return oo_OK;
}

static int
ooAgenda_str(ooAgenda *self)
{
    struct ooConcUnit *cu;
    char *conc_name = "Unrec";
    struct ooComplex *complex, *best_complex = NULL;

    int curr_weight = 0;
    size_t i, j;
    bool gotcha;

    printf("\n---- CURRENT STATE OF \"%s\" AGENDA\n"
	   "    ==== Conceptual index (storage_space_used: %zu last_idx_pos: %zu):\n", 
	   self->codesystem->name, self->storage_space_used, self->last_idx_pos);

    /*cu = self->index[0];
      cu->str(cu, 0); */

    for (i = 0; i < self->last_idx_pos; i++) {
	cu = self->index[i];
	if (!cu) continue;

	if (self->linear_structure || 
	    CS_POSITIONAL == self->codesystem->type) {
	    printf("\n  [pos: %d]\n", i);
	}
	else {
	    /* lookup for the codesystem name */
	    if (self->codesystem->code_index[i])
		conc_name = self->codesystem->code_index[i]->name;

	    printf("\n  [concept %d: \"%s\"]\n", i, conc_name);
	}

	while (cu) {
	    cu->str(cu, 1);
	    cu = cu->next;
	}

    }

    /* LOGICAL MARKERS */
    printf("\n   ==== Logical marker index:\n");
    gotcha = false;
    for (i = 0; i < INPUT_BUF_SIZE; i++) {
	cu = self->logic_oper_index[i];
	if (!cu) continue;
	gotcha = true;
	printf("    [pos: %d]\n", i);
	while (cu) {
	    cu->str(cu, 1);
	    cu = cu->next;
	}
    }
    if (!gotcha) printf("     -- No logical markers found.\n");


    /* DELIMITERS */
    printf("\n   ==== Delimiter index:\n");
    gotcha = false;
    for (i = 0; i < INPUT_BUF_SIZE; i++) {
	cu = self->delimiter_index[i];
	if (!cu) continue;
	gotcha = true;
	printf("    [pos: %d]\n", i);
	while (cu) {
	    cu->str(cu, 1);
	    cu = cu->next;
	}
    }
    if (!gotcha) printf("    -- No delimiters found!\n");

    for (i = 0; i < INPUT_BUF_SIZE; i++) {
	complex = self->linear_index[i];
	if (!complex) continue;
	while (complex) {
	    complex->str(complex, 1);
	    complex = complex->next;
	}
    }

    if (self->codesystem) {
	printf("\n---- THE END OF \"%s\" AGENDA\n\n", 
	       self->codesystem->name);
    }

    /*self->best_complex->str(self->best_complex, 1);*/

    /*
    if (self->best_complex != NULL) {
	printf("*** THE BEST SOLUTION:\n");
	self->next_solution(self, 0);
	}*/

    return oo_OK;
}



static int
ooAgenda_sort_solutions(struct ooAgenda *self)
{
    struct ooConcUnit *cu;
    struct ooComplex *complex, *c, *neighbour = NULL, *best_neighbour = NULL;
    int weight = 0, linear_end = -1;
    size_t i, j;

    /*cu = self->alloc_head;*/

    /*while (cu) {*/
    for (i = 0; i < self->storage_space_used; i++) {
	cu = &self->storage[i];
	if (!cu) continue;

	/* find the best linear coverage */
	for (j = 0; j < cu->num_complexes; j++) {
	    complex = cu->complexes[j];

	}
    }

    return oo_OK;
}



static int
ooAgenda_next_solution(struct ooAgenda *self, 
		       size_t sol_id)
{
    size_t i, j, sol_count = 0;
    int weight = 0;
    size_t cur_id = 0;
    struct ooComplex *c, *local_c, **local_index;
    bool gotcha;
    int ret;

    if (!self->best_complex) return FAIL;

    /* terminals covered by the best solution */
    local_index = self->best_complex->linear_index;

    for (i = 0; i < INPUT_BUF_SIZE; i++) {
	local_c = local_index[i];
	if (local_c) {
	    /*local_c->present(local_c, FORMAT_LINEAR_XML);*/
	    i = local_c->linear_end;
	    continue;
	}

	c = self->linear_index[i];
	if (!c) continue;
	gotcha = false;

	/* find the next complex 
         * from our best solution */
	for (j = i; j < INPUT_BUF_SIZE; j++) {
	    local_c = local_index[j];
	    if (local_c == NULL) continue;
	    break;
	}

	/*
	if (!gotcha)
	c->present(c, FORMAT_LINEAR_XML);*/
	
    }

    return oo_OK;
}




static int
ooAgenda_find_best_complex(struct ooAgenda *self,
			   int linear_begin,
			   int linear_end)
{
    struct ooComplex *c, *best = NULL;
    int i, weight = 0;

    /* find the best linear coverage */
    for (i = linear_begin; i < linear_end; i++) {
	c = self->linear_index[i];
	if (!c) continue;

	while (c) {
	    if (c->linear_begin == -1 || 
		c->linear_end == -1) goto next_complex;

	    if (c->linear_end >= linear_end) goto next_complex;
	
	    if (!c->base->is_present) goto next_complex;

	    if (c->weight == weight) {
		if (best) {
		    if (c->base->code->id <
			best->base->code->id) {
			best = c;
		    }
		} else 
		    best = c;
	    }
	    /* higher weight */
	    if (c->weight > weight) {
		best = c;
		weight = c->weight;
	    }
	next_complex:
	    c = c->next;
	}

    }

    
    if (best) {
	printf("\n  NEXT best complex:\n");
	best->str(best, 0);

	ooAgenda_present_linear_solution(self, 
					 best,
					 linear_begin, linear_end);
    }

    return oo_OK;
}



static int
ooAgenda_present_linear_solution(struct ooAgenda *self,
				 struct ooComplex *c, 
				 int linear_begin,
				 int linear_end)
{
    output_type format = self->accu->decoder->format;
    int ret, i;

    if (!c) {
	printf("start finding from %d to %d...\n",linear_begin, linear_end );

	return ooAgenda_find_best_complex(self,
					  linear_begin, linear_end);
    }

    if (c->linear_begin > linear_begin)
	ret = ooAgenda_find_best_complex(self,
					 linear_begin, 
					 c->linear_begin);

    /* TODO: check the linearly nested isolated complexes */

    c->present(c, self->accu, format);

    if (c->linear_end < linear_end)
	ret = ooAgenda_find_best_complex(self, 
					 c->linear_end, linear_end);
    return oo_OK;
}



static int
ooAgenda_build_linear_index(struct ooAgenda *self)
{
    struct ooConcUnit *cu;
    struct ooComplex *complex, *c, *neighbour = NULL, *best_neighbour = NULL;
    int weight = 0, linear_end = -1;
    size_t i, j;


    for (i = 0; i < self->storage_space_used; i++) {
	cu = &self->storage[i];
	if (!cu) continue;

	/* find the best linear coverage */
	for (j = 0; j < cu->num_complexes; j++) {
	    complex = cu->complexes[j];

	    if (complex->linear_begin == -1 || 
                complex->linear_end == -1) continue;

	    /* if complexes have the same weight 
             *  we take the one of the higher class */
	    if (complex->weight == weight) {
		if (self->best_complex) {
		    if (complex->base->code->id <
			self->best_complex->base->code->id) {
			self->best_complex = complex;
		    }
		} else {
		    self->best_complex = complex;
		}
	    }

	    /* higher weight */
	    if (complex->weight > weight) {
		self->best_complex = complex;
		weight = complex->weight;
	    }

	    complex->next = self->linear_index[complex->linear_begin];
	    self->linear_index[complex->linear_begin] = complex;
	}

	/*cu = cu->next_alloc;*/

    }


    return oo_OK;
}




/* give a complex to the caller */
static struct ooComplex* 
ooAgenda_alloc_complex(struct ooAgenda *self)
{
    struct ooComplex *c;

    if (self->num_complexes == AGENDA_COMPLEX_POOL_SIZE) {
	fprintf(stderr, "Agenda's pool of complexes exceeded...\n");
	return NULL;
    }

    c = &self->complexes[self->num_complexes];
    ooComplex_init(c);

    self->num_complexes++;

    return c;
} 


static int
ooAgenda_register_unit(struct ooAgenda *self,
		       struct ooConcUnit *cu,
		       struct ooCode *code)
{
    struct ooComplex *complex;
    size_t concid, i;
    int ret;

    if (CODE_SEPARATOR == cu->code->type) {
	if (DEBUG_CONC_LEVEL_3)
	    printf("   ... Register a separator...\n");

	i = cu->linear_pos;
	if (!self->delimiter_index[i]) {
	    self->delimiter_index[i] = cu;
	}
	else {
	    self->delimiter_index[i]->next = cu;
	    self->delimiter_index[i] = cu;
	}

	return oo_OK;
    }

    if (CODE_GROUP_MARKER == cu->code->type) {
	if (DEBUG_CONC_LEVEL_3)
	    printf("   ... Register a grouping marker...\n");

	i = cu->linear_pos;
	if (!self->logic_oper_index[i]) {
	    self->logic_oper_index[i] = cu;
	}
	else {
	    self->logic_oper_index[i]->next = cu;
	    self->logic_oper_index[i] = cu;
	}

	return oo_OK;
    }

    /* positional agenda */
    if (CS_POSITIONAL == self->codesystem->type) {
	i = cu->linear_pos;
	if (!self->index[i]) {
	    self->index[i] = cu;
	    self->tail_index[i] = cu;
	}
	else {
	    self->tail_index[i]->next = cu;
	    self->tail_index[i] = cu;
	}

	if (i >= self->last_idx_pos)
	    self->last_idx_pos = i + 1;

	self->linear_last = cu;

	return oo_OK;
    }


    /* inheritance */
    /*if (code->num_children) {
	if (code->children[IS_SUBCLASS] != NULL) {
	    ooAgenda_register_unit(self, cu, 
		      code->children[IS_SUBCLASS]->code);
	}
	}*/

    /*concid = code->id;*/

    /* operational agenda */

    /* check the presence of upper aggregates */
    for (i = 0; i < cu->num_complexes; i++) {
	complex = cu->complexes[i];
	if (!complex->aggregate) continue;
	ret = ooAgenda_register_unit(self, complex->aggregate, 
                 complex->aggregate->code);
    }



    concid = cu->code->id;
    if (cu->code->baseclass)
	concid = cu->code->baseclass->id;

    if (!self->index[concid]) {
	self->index[concid] = cu;
	self->tail_index[concid] = cu;
    }
    else {
	self->tail_index[concid]->next = cu;
	self->tail_index[concid] = cu;
    }

    /* update the index sentinel value */
    if (concid >= self->last_idx_pos) {
	self->last_idx_pos = concid + 1;
    }

    return oo_OK;
}


static struct ooConcUnit*
ooAgenda_add_code_complex(struct ooAgenda *self,
			  struct ooCodeUnit *code_unit,
			  struct ooConcUnit *terminal,
			  struct ooConcUnit *common_unit)
{
    struct ooConcUnit *cu, *child;
    struct ooCodeUnit *child_code_unit;
    struct ooComplex *complex, *child_complex;
    size_t i, j, concid, operid, pos;
    int ret;

    if (DEBUG_AGENDA_LEVEL_4)
	printf(" .. Adding complex code..\n");

    cu = self->alloc_unit(self);
    if (cu == NULL) return NULL;

    cu->make_instance(cu, code_unit->code, terminal);
    cu->is_present = true;

    /* - all units in the complex code will share 
         the reference to their top parent 
       - the top parent makes a reference to itself */
    if (!common_unit) {
	cu->common_unit = cu;
	common_unit = cu;
    }
    else cu->common_unit = common_unit;

    /* nested children should be linked and registered first */
    if (!code_unit->num_specs) goto agenda_register;

    child_code_unit = code_unit->specs[0]->unit;
    child = ooAgenda_add_code_complex(self, child_code_unit, terminal, common_unit);
    if (!child) goto agenda_register;

    operid = code_unit->specs[0]->operid;

    child->fixed_parent = cu;
    child->fixed_operid = operid;

    if (DEBUG_AGENDA_LEVEL_4) {
	printf("\n   ++ CHILD of \"%s\" IN A COMPLEX CODE:\n", 
	       code_unit->code->name);
	child->str(child, 1);
    }

    pos = cu->linear_pos;
    cu->num_complexes = 0;

    /* establish fixed links between complexes */
    for (i = 0; i < child->num_complexes; i++) {
	complex = cu->complexes[i];
	child_complex = child->complexes[i];

	complex->base = cu;
	complex->linear_begin = pos;
	complex->linear_end = pos + cu->coverage;
	complex->linear_index[pos] = complex;
	complex->weight = child_complex->weight;

	if (child_complex->linear_begin < complex->linear_begin) 
	    complex->linear_begin = child_complex->linear_begin;
	if (child_complex->linear_end > complex->linear_end) 
	    complex->linear_end = child_complex->linear_end;

	/* merge linear indices */
	for (j = child_complex->linear_begin; j < child_complex->linear_end; j++)
	    complex->linear_index[j] = child_complex->linear_index[j];

	complex->specs[operid] = child_complex;
	complex->is_free = false;

	cu->num_complexes++;
	/*cu->str(cu, 1);*/
    }

    if (DEBUG_AGENDA_LEVEL_4) {
	printf("    == Shared CU after adding its child:\n");
	cu->str(cu,1);
    }

agenda_register:
    ret = cu->link(cu);

    ret = ooAgenda_register_unit(self, cu, cu->code);

    return cu;
}


static int 
ooAgenda_explore_code_denots(struct ooAgenda *self,
			     struct ooConcUnit *cu)
{
    size_t i, j;
    struct ooCode *denot;
    struct ooConcUnit *denot_cu;
    bool gotcha = false;
    int ret;

    if (!cu->code) return FAIL;

    if (DEBUG_AGENDA_LEVEL_3)
	printf("\n  ?? exploring denots of CU \"%s\" (id: %lu):\n",
	       cu->code->name, (unsigned long)cu->concid);

    if (DEBUG_AGENDA_LEVEL_3) {
	for (i = 0; i < cu->code->num_denots; i++) {
	    printf("  [%s]", cu->code->denot_names[i]);
	    gotcha = true;
	}
	printf("\n");
	if (!gotcha) 
	    printf("   ... \%s\" has no denots.\n",
		   cu->code->name);
    }

    /* make instances of denots */
    for (i = 0; i < cu->code->num_denots; i++) {
	denot = cu->code->denots[i];
	if (!denot) continue;

	if (DEBUG_AGENDA_LEVEL_3) {
	    printf("\n   !! linking code %lu) %s (%lu)\n",
		   (unsigned long)i, denot->name, 
		   (unsigned long)denot->id);
	    denot->str(denot);
	}

	if (denot->complex) {
	    ret = ooAgenda_add_code_complex(self, denot->complex, cu, NULL);
	    continue;
	}

	/* single concept unit */
	denot_cu = self->alloc_unit(self);
	if (!denot_cu) return oo_NOMEM;

	denot_cu->make_instance(denot_cu, denot, cu);
	denot_cu->is_present = true;

	/* find syntactic solutions */
	ret = denot_cu->link(denot_cu);

	
	ret = ooAgenda_register_unit(self, denot_cu, denot_cu->code);


    }

    return oo_OK;
}


static int
ooAgenda_denotational_update(struct ooAgenda *self,
		struct ooAgenda *segm_agenda)
{
    size_t i, start_term_pos = 0;
    struct ooConcUnit *src_cu, *dest_cu = NULL;
    const char *code_name = "None", *denot_name = "None";
    struct ooCode *code = NULL;
    struct ooCodeSystem *cs;

    cs = self->codesystem;

    /* code mappings aka denotations */
    for (i = 0; i < segm_agenda->last_idx_pos; i++) {
	src_cu = segm_agenda->index[i];
	if (!src_cu) continue;

	dest_cu = self->alloc_unit(self);
	if (!dest_cu) return oo_NOMEM;

	dest_cu->cs_id = cs->id;
	dest_cu->terminals = src_cu;
	dest_cu->num_terminals = 1;
	dest_cu->is_present = true;
	dest_cu->linear_pos = src_cu->linear_pos;
	dest_cu->start_term_pos = start_term_pos;
	dest_cu->coverage = src_cu->coverage;
	dest_cu->next = NULL;

	start_term_pos++;

	if (cs->use_numeric_codes) {
	    dest_cu->concid = (size_t)cs->numeric_denotmap[src_cu->concid];
	}
	else {
	    code = cs->code_index[src_cu->concid];
	    if (code) {
		dest_cu->concid = (size_t)code->id;
		dest_cu->code = code;
		/*printf("AFTER DENOT:\n");
		  code->str(code);*/
	    }
	}

	if (DEBUG_LEVEL_3) {
	    if (code) {
		denot_name = code->name;
	    } else {
		denot_name = cs->code_names[dest_cu->concid];
	    }
	    code_name = "None";
	    if (code) code_name = code->name;
	    printf("  ++ SRC: %s (%zu)  =>  DENOT: %s (%zu)\n", 
	      code_name, src_cu->concid, denot_name, dest_cu->concid);
	}

        /* update agenda */
	self->index[i] = dest_cu;
	/*self->agenda->coverage += dest_cu->coverage;*/
	self->last_idx_pos = i + dest_cu->coverage;
    }

    return oo_OK;
}

static int
ooAgenda_complex_update(struct ooAgenda *self,
			struct ooAgenda *segm_agenda)
{   size_t i;
    struct ooConcUnit *cu;
    int ret;

    if (DEBUG_AGENDA_LEVEL_1)
	printf("\n  !! \"%s\" Agenda is adding terminal units...\n",
	       self->codesystem->name);

    /* at least some unrecognizable matter is found */
    if (self->last_idx_pos == 0)
	self->last_idx_pos = 1;

    /* TODO: choose an algorithm to add units 
     * - max greed?
     * - batched */

    for (i = 0; i < segm_agenda->last_idx_pos; i++) {
	/* get a list of concept units at a given linear position */
	cu = segm_agenda->index[i];
	if (!cu) continue;

	/* a list of valid concepts */
	while (cu) {
	    /* unrecognized codes are considered later */
	    if (!cu->concid) goto next_cu;

	    if (DEBUG_AGENDA_LEVEL_2)
		printf("\n  ++ adding CU \"%s\" "
                       " (linear_pos: %lu, coverage: %lu)\n", 
		       cu->code->name, 
		       (unsigned long)cu->linear_pos,
		       (unsigned long)cu->coverage);

	    ret = ooAgenda_explore_code_denots(self, cu);
	next_cu:
	    cu = cu->next;
	}

	if (DEBUG_AGENDA_LEVEL_4) {
	    printf("\n\n   *** AFTER adding units at linear_pos %lu:\n", 
	       (unsigned long)i);
	    self->str(self);
	}

    }

    if (DEBUG_AGENDA_LEVEL_1)
	printf("\n  !! \"%s\" Agenda has finished adding terminal units!\n",
	       self->codesystem->name);


    /* TODO: any unrecognized sequences left? 
          apply correction */

    if (DEBUG_AGENDA_LEVEL_1)
	self->str(self);

    ret = ooAgenda_build_linear_index(self);

   
    if (self->best_complex && self->accu->decoder->is_root) {
	printf("\n\n Start writing solution from pos %d\n", 
	       self->best_complex->linear_begin);
	ret = ooAgenda_present_linear_solution(self, 
					       self->best_complex,
					       0, 
					       segm_agenda->last_idx_pos + 1);
    }

    if (self->best_complex)
	self->accu->solution = (const char*)self->accu->output_buf;
 

    return oo_OK;

}




static int
ooAgenda_update(struct ooAgenda *self,
		struct ooAgenda *segm_agenda)
{   size_t i, coverage;
    struct ooConcUnit *cu;
    int ret;

    if (segm_agenda->best_complex) {
	self->accu->solution = segm_agenda->accu->solution;
    }

    switch (self->codesystem->type) {
    case AGENDA_OPERATIONAL:
	return ooAgenda_complex_update(self, segm_agenda);
    case AGENDA_POSITIONAL:
	return ooAgenda_complex_update(self, segm_agenda);
    default:
	return ooAgenda_denotational_update(self, segm_agenda);
    }

}


static int
ooAgenda_reset(struct ooAgenda *self)
{
    struct ooConcUnit *cu;
    size_t i;
    int ret;

    /* reset agenda index with NULLs */
    for (i = 0; i < AGENDA_INDEX_SIZE; i++) {
	self->index[i] = NULL;
	self->tail_index[i] = NULL;
    }

    /* initialize linear index */
    for (i = 0; i < INPUT_BUF_SIZE; i++) {
	self->linear_index[i] = NULL;
	self->delimiter_index[i] = NULL;
	self->logic_oper_index[i] = NULL;
    }

    
    /* initialize storage units */
    /*while (self->alloc_head) {
        cu = self->alloc_head;
        self->alloc_head = cu->next_alloc;
	cu->next_alloc = self->free_head;
	self->free_head = cu;
	}*/

    self->linear_last = NULL;
    self->storage_space_used = 0;
    self->last_idx_pos = 0;
    self->num_complexes = 0;

    return oo_OK;
}


/* give a unit to the caller */
static struct ooConcUnit* 
ooAgenda_alloc_unit(struct ooAgenda *self)
{
    struct ooConcUnit *cu;

    /*if (!self->free_head) {*/

    if (self->storage_space_used == AGENDA_CONCUNIT_STORAGE_SIZE) {
	fprintf(stderr, "Agenda's storage_size exceeded...\n");
	return NULL;
    }

    /*    cu = self->free_head;
    self->free_head = cu->next_alloc;
    ooConcUnit_init(cu);

    cu->next_alloc = self->alloc_head;
    self->alloc_head = cu;
    */

    cu = &self->storage[self->storage_space_used];
    ooConcUnit_init(cu);
    cu->agenda = self;

    self->storage_space_used++;

    return cu;
} 


/* ooAgenda Initializer */
extern int 
ooAgenda_new(struct ooAgenda **agenda)
{
    size_t i;
    struct ooConcUnit *cu, *prev_cu;
    int ret;
    struct ooAgenda *self = malloc(sizeof(struct ooAgenda));
    if (!self) return oo_NOMEM;

    self->codesystem = NULL;
    self->accu = NULL;

    self->storage_space_used = 0;
    self->num_complexes = 0;

    /* initialize index and queue refs */
    for (i = 0; i < AGENDA_INDEX_SIZE; i++) {
	self->index[i] = NULL;
	self->tail_index[i] = NULL;
    }
    self->last_idx_pos = 0;

    /* initialize linear index */
    for (i = 0; i < INPUT_BUF_SIZE; i++) {
	self->linear_index[i] = NULL;
    }
    self->best_complex = NULL;

    self->linear_structure = false;
    self->linear_last = NULL;

    /* bind your methods */
    self->del = ooAgenda_del;
    self->str = ooAgenda_str;
    self->alloc_unit = ooAgenda_alloc_unit;
    self->update = ooAgenda_update;
    self->register_unit = ooAgenda_register_unit;
    self->reset = ooAgenda_reset;
    self->present_solution = ooAgenda_present_linear_solution;

    *agenda = self;
    return oo_OK;
}


