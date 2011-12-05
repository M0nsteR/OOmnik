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
 *   OOmnik Decoder implementation
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ooconfig.h"
#include "oodecoder.h"
#include "oocodesystem.h"
#include "oocode.h"
#include "oocache.h"
#include "oosegmentizer.h"
#include "ooaccumulator.h"

/*  destructor */
static int
ooDecoder_del(struct ooDecoder *self)
{   int ret;

  /*printf("Freeing Decoder \"%s\"\n", self->codesystem->name);*/

    ret = self->segm->del(self->segm);
    ret = self->agenda->del(self->agenda);
    ret = self->accu->del(self->accu);

    /* free up yourself */
    free(self);

    return oo_OK;
}

static const char* 
ooDecoder_str(struct ooDecoder *self)
{
    struct ooDecoder *dec;
    size_t i;

    if (self->codesystem == NULL) {
	if (DEBUG_LEVEL_3) printf("  -- Non-initialized Decoder :(\n");
	return NULL;
    }

    if (DEBUG_LEVEL_3) printf("  == Here is a Decoder responsible for \"%s\"!\n",
                        self->codesystem->name);

    if (self->codesystem->cache != NULL) {
	self->codesystem->cache->str(self->codesystem->cache);
	return NULL;
    }

    if (DEBUG_LEVEL_3) printf("  ++ Subdecoders of %s: %zu\n",
			      self->codesystem->name,
			      self->segm->num_decoders);

    for (i = 0; i < self->segm->num_decoders; i++) {
	dec = self->segm->decoders[i];
	dec->str(dec);
    }

    return L"";
}

static int 
ooDecoder_set_parent(struct ooDecoder *self,  
			 struct ooDecoder *parent)
{   int ret;
    size_t i, parent_size;
    struct ooDecoder **parents;

    parent_size = (self->num_parents + 1) * sizeof(struct ooDecoder*);
    parents = realloc(self->parents, parent_size);
    if (parents == NULL) return FAIL;

    parents[self->num_parents] = parent;
    self->parents = parents;
    self->num_parents++;

    return oo_OK;
}

static struct ooDecoder* 
ooDecoder_get_parent(struct ooDecoder *self,  
		     char *name)
{   size_t i;
    struct ooDecoder *dec;

    if (self->num_parents == 0) return NULL;
    
    for (i = 0; i < self->num_parents; i++) {
	dec = self->parents[i];
	if (!strcmp(self->codesystem->name, 
		    dec->codesystem->name)) {
	    return dec;
	}
    }

    return NULL;
}

static int 
ooDecoder_add_providers(struct ooDecoder *self,  
			  struct ooCodeSystem *cs)
{   int i, ret;
    size_t dec_size;
    struct ooDecoder *dec;
    struct ooDecoder **decs;
    struct ooCodeSystem *provider;

    if (DEBUG_LEVEL_1)
	printf("\n  ** Decoder \"%s\": adding providers...\n",
	       self->codesystem->name);

    self->segm->decoders = malloc(sizeof(struct ooDecoder*) * cs->num_providers);
    if (self->segm->decoders == NULL) return oo_NOMEM;

    self->segm->num_decoders = cs->num_providers;
    self->segm->decoders_logic_oper = cs->providers_logic_oper;

    for (i = 0; i < cs->num_providers; i++) {
	provider = cs->providers[i];

	if (DEBUG_LEVEL_3) 
	    printf("  ** %zu) \"%s\" is adding subordinate Decoder \"%s\"...\n", 
				  i, cs->name, provider->name);

	/* TODO check the loops */
	/*dec = ooDecoder_get_parent(self, cs_name);
	if (dec != NULL) {
	    if (DEBUG_LEVEL_3) printf("  ++ Looping reference found!\n");
	    dec->loops = NULL;
	    continue;
	    }*/

	ret = ooDecoder_new(&dec);
	if (ret != oo_OK) {
	    if (DEBUG_LEVEL_1)
		printf("  -- Failed to initialize a Decoder for \"%s\" :(\n", 
		       provider->name);
	    continue;
	}

	dec->oomnik = self->oomnik;
	ret = dec->set_codesystem(dec, provider);

	if (ret != oo_OK) {
	    if (DEBUG_LEVEL_1)
		printf("  -- Failed to add CS \"%s\" :(\n", provider->name);
	    dec->del(dec);
	    continue;
	}

	/* add a decoder to our segmentizer */
	self->segm->decoders[i] = dec;
    }

    return oo_OK;
}


static int
ooDecoder_set_codesystem(struct ooDecoder *self, 
			 struct ooCodeSystem *cs)
{
    size_t i;
    int ret;

    if (DEBUG_LEVEL_1)
	printf("   !! Fresh Decoder: setting the CodeSystem \"%s\"...\n",
	       cs->name);

    self->codesystem = cs;
    if (cs->is_atomic) {
	self->segm->is_atomic = true;
	self->is_atomic = true;
	return oo_OK;
    }

    if (cs->cache) {
	ret = ooDecoder_add_providers(self, cs->cache->provider);

	self->agenda->codesystem = cs;
	self->segm->agenda->codesystem = cs->cache->provider;
	self->segm->num_solutions = cs->cache->provider->num_providers;
	return oo_OK;
    }


    ret = ooDecoder_add_providers(self, cs);
    if (ret != oo_OK) return ret;

    self->agenda->codesystem = cs;
    self->segm->agenda->codesystem = cs;
    self->segm->num_solutions = cs->num_providers;

    if (self->is_root) {
	ret = self->accu->build_indices(self->accu, cs->mindmap);
	if (ret != oo_OK) return ret;
    }

    /* TODO: agenda's exact index size must be set here! */


    return oo_OK;
}


static int 
ooDecoder_save_result(struct ooDecoder *self)
{   int ret, weight;
    size_t i, chunk_size = 0;
    struct ooComplex *c, *best_complex;

    /*if (DEBUG_LEVEL_1)*/
	printf("\n   ... Saving the decoded results of \"%s\"...\n",
	   self->input);





    return oo_OK;
}


/***************************   ANALYSIS ***************************/

/**  Concept Unit analysis:
 *   - check cache;
 *   - if there are no cached results
 *     add a batch of units to Agenda 
 */
static int
ooDecoder_analyze_units(struct ooDecoder *self)
{
    struct ooAgenda *agenda;
    int num_remaining_solutions;
    int ret;

    if (DEBUG_LEVEL_3)
	printf("\n **** CS \"%s\": analyzing units "
	       " from Segmentizer (atomic: %d next_sol: %d num_sols: %d)\n", 
	       self->codesystem->name, self->is_atomic,
	       self->segm->next_solution_id, self->segm->num_solutions);

    /* root decoder saves the best interpretation */
    if (self->is_root) {

	agenda = self->segm->agenda;
	agenda->accu = self->accu;

	/*printf("\n\n\nFINAL STATE OF AGENDA (last idx: %d):\n",
	  agenda->last_idx_pos + 1);*/

	ret = agenda->present_solution(agenda, NULL,
				       0, agenda->last_idx_pos + 1);
	if (ret != oo_OK) return ret;

	num_remaining_solutions = self->segm->num_solutions -\
	    self->segm->next_solution_id;
	if (DEBUG_LEVEL_2)
	    printf("\n  == Alternative solutions left: %d\n", 
		   num_remaining_solutions);

	/*if (num_remaining_solutions)
	  self->decode(self);*/

	return oo_OK;
    }

    /* atomic codes stay the same */
    if (self->segm->is_atomic) 	return oo_OK;

    /* no valuable results in Segmentizer's agenda */
    if (!self->segm->agenda->last_idx_pos) return oo_NO_RESULTS;
    
    return self->agenda->update(self->agenda,
				self->segm->agenda);
}


/*  find solutions */
static int
ooDecoder_decode(struct ooDecoder *self)
{
    struct ooLinearCache *cache = self->codesystem->cache;
    struct ooConcUnit *cu;
    int ret;

    if (DEBUG_LEVEL_1)
	printf("  ** Decoder \"%s\" at work. task id: %lu input: %s\n", 
	       self->codesystem->name, 
	       (unsigned long)self->task_id,
	       self->input);

    /* clean up the agenda */
    self->agenda->reset(self->agenda);
    self->segm->reset(self->segm);

    /* linear cache is available */
    if (cache) {
	ret = cache->lookup(cache,
			    self->segm,
			    self->input, 
			    self->input_len,
			    self->task_id,
			    self->agenda);

	self->num_parsed_atoms = self->segm->num_parsed_atoms;
	self->num_terminals = self->segm->num_terminals;
	return oo_OK;
    }

    self->segm->input = self->input;
    self->segm->input_len = self->input_len;
    self->segm->task_id = self->task_id;

    if (DEBUG_LEVEL_3)
	printf("  -- No cache available... Segmentizing input: %zu\n", 
	   self->input_len);

    ret = self->segm->segmentize(self->segm);
    if (ret != oo_OK) return ret;

    ret = ooDecoder_analyze_units(self);
    if (ret != oo_OK) return ret;

    /*if (self->is_root)
	printf("ROOT DECODER: terms: %d  segm: %d symbols!\n", 
	       self->num_terminals,
	       self->segm->num_terminals);*/

    self->num_parsed_atoms = self->segm->num_parsed_atoms;
    self->num_terminals = self->segm->num_terminals;

    return oo_OK;
}


static int
ooDecoder_process_string(struct ooDecoder *self,
			 const char *input)
{   
    char buf[INPUT_BUF_SIZE + 1] = { 0 };
    const unsigned char *c;
    size_t task_id = 0, last_pos = 0, i = 0, offset = 0;
    int ret = oo_OK;

    c = input;
    if (!c) return oo_FAIL;

    /* walking over the input string 
     * filling the window buffer
     */
    while (*c) {

	if (i < INPUT_BUF_SIZE) {
            buf[i++] = *c++;
	    continue;
	}

	/* buffer is full */
	buf[INPUT_BUF_SIZE] = '\0';

	self->input = buf;
	self->input_len = INPUT_BUF_SIZE;

	/* decode the complete buffer */
	ret = self->decode(self);
	if (ret != oo_OK) return ret;

	last_pos = self->num_parsed_atoms;
	self->term_count += self->num_terminals;

	/*printf("AFTER BATCH %d: term count: %d\n", 
	  task_id, self->term_count);*/
	task_id++;

	/* some unrecognized atoms are left in the remainder */
	if (last_pos < i) { 

	    offset = i - last_pos;
	    if (offset < MAX_UNREC_ATOMS) { /*printf("offset: %u\n", offset);*/
		c -= offset;
	    }
	    /*self->input_offset += last_pos;*/
	    i = 0;
	    continue;
	}
	/* restart buffer */
	/*self->input_offset += i;*/
	i = 0;
    }

    buf[i] = '\0';

    /* scan the remainder if any */
    if (i) {
	self->input = buf;
	self->input_len = i;

	/* decode the last buffer */
	ret = self->decode(self);
    }

    return ret;
}

/**
 *  ooDecoder Initializer 
 */
extern int
ooDecoder_new(struct ooDecoder **dec)
{
    int ret;
    struct ooDecoder *self = malloc(sizeof(struct ooDecoder));
    if (!self) return oo_NOMEM;

    ret = ooAccu_new(&self->accu);
    if (ret != oo_OK) return ret;
    self->accu->decoder = self;

    ret = ooAgenda_new(&self->agenda);
    if (ret != oo_OK) return ret;
    self->agenda->accu = self->accu;

    ret = ooSegmentizer_new(&self->segm);
    if (ret != oo_OK) return ret;
    self->segm->parent_decoder = self;

    self->cache_segm = NULL;
    self->oomnik = NULL;
    self->codesystem = NULL;
    self->loops = NULL;
    self->is_root = false;

    self->performance = 0;
    self->is_activated = true;

    self->input = NULL;
    self->input_len = 0;
    self->task_id = 0;
    self->num_parsed_atoms = 0;
    self->num_terminals = 0;
    self->term_count = 0;


    self->parents = NULL;
    self->num_parents = 0;

    self->inheritors = NULL;
    self->num_inheritors = 0;
    self->solution = NULL;

    self->is_atomic = false;
    self->used_by_cache = false;

    self->format = FORMAT_XML;

    /* bind your methods */
    self->del = ooDecoder_del;
    self->str = ooDecoder_str;

    self->decode = ooDecoder_decode;
    self->process = ooDecoder_process_string;
    self->set_codesystem = ooDecoder_set_codesystem;
    *dec = self;
    return oo_OK;
}


