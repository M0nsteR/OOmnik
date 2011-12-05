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
 *   ---------------
 *   ooaccumulator.c
 *   OOmnik Accumulator implementation
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ooconfig.h"
#include "oocodesystem.h"
#include "oodecoder.h"
#include "ooaccumulator.h"

/*  Destructor */
static int
ooAccu_del(struct ooAccu *self)
{
    if (self->output_buf)
	free(self->output_buf);

    if (self->concept_index)
	free(self->concept_index);

    if (self->topic_index)
	free(self->topic_index);

    /* free up yourself */
    free(self);

    return oo_OK;
}

static int
ooAccu_str(struct ooAccu *self)
{
    printf("\n---- CURRENT STATE OF INTERPRETATION\n %s\n\n",
	   self->output_buf);

    return oo_OK;
}


static int
ooAccu_reset(struct ooAccu *self)
{

    /* reset agenda index with NULLs */
    /* for (i = 0; i < AGENDA_INDEX_SIZE; i++) {
	self->index[i] = NULL;
	self->tail_index[i] = NULL;
	}*/


    return oo_OK;
}


static int 
ooAccu_sort_topic_solutions(struct ooAccu *self)
{
    struct ooTopicSolution *sol, *tmp;
    int i, j;

    if (DEBUG_CONC_LEVEL_3)
	printf("\n    ... Sorting the topic solutions... num_topics: %d\n", 
	       self->num_topic_solutions);

    for (i = 0; i < self->num_topic_solutions; i++) {
	sol = self->topic_solutions[i];

	for (j = 0; j < self->num_topic_solutions; j++) {
	    if (!self->topic_rating[j]) {
		self->topic_rating[j] = sol;
		break;
	    }

	    if (sol->weight > self->topic_rating[j]->weight) {
		tmp = self->topic_rating[j];
		self->topic_rating[j] = sol;
		/*printf("   %p -> slot %d\n", c, j);*/
		sol = tmp;
	    }
	}

    }

    return oo_OK;
}


static int
ooAccu_present_conc_rating(struct ooAccu *self,
			   char *buf,
			   size_t *buf_size)
{
    struct ooConcFreq *cfreq;
    size_t curr = 0;
    int count = 0;

    sprintf(buf, "\"rating\": [");
    curr = strlen(buf);
    buf += curr;
    (*buf_size) += curr;

    cfreq = self->freq_top;
    while (cfreq) {
	count++;
	if (count > ACCU_MAX_CONCFREQS) break;
	if (count > 1) {
	    sprintf(buf, ",");
	    buf++;
	    (*buf_size)++;
	}

	sprintf(buf,"{\"name\":\"%s\",\"annot\":\"%s\",\"weight\":\"%.2f\"}",
	       cfreq->conc->name, cfreq->conc->annot, cfreq->weight);
	curr = strlen(buf);
	buf += curr;
	(*buf_size) += curr;

	cfreq = cfreq->lt;
    }

    sprintf(buf, "],");
    curr = strlen(buf);
    buf += curr;
    (*buf_size) += curr;

    return oo_OK;
}

static int
ooAccu_present_solution(struct ooAccu *self,
			char *buf,
			size_t max_buf_size)
{
    struct ooTopicSolution *topsol;

    char ingr_buf[TEMP_BUF_SIZE];
    char *tmp_buf = ingr_buf;
    size_t tmp_buf_size = 0;

    size_t curr = 0, out_size = 0;
    int i;

    ooAccu_sort_topic_solutions(self);

    /* TODO: select format */

    sprintf(buf, "{");
    buf++;
    out_size++;

    ooAccu_present_conc_rating(self, buf, &curr);
    buf += curr;
    out_size += curr;

    if (self->num_topic_solutions) {
	sprintf(buf, "\"topics\": [");
	curr = strlen(buf);
	buf += curr;
	out_size += curr;
    }

    for (i = 0; i < self->num_topic_solutions; i++) {
	topsol = self->topic_rating[i];
	if (!topsol) continue;

	if (i == TOPIC_SHOW_LIMIT) break;

	if (i) {   /* we need a separator here */
	    sprintf(buf, ",");
	    buf++;
	    out_size++;
	}

	topsol->present(topsol, buf);
	curr = strlen(buf);
	buf += curr;
	out_size += curr;
    }

    if (self->num_topic_solutions) {
	sprintf(buf, "],");
	curr = strlen(buf);
	buf += curr;
	out_size += curr;
    }

    sprintf(buf, "\"concepts\": [");
    curr = strlen(buf);
    buf += curr;
    out_size += curr;
    
    sprintf(buf, self->output_buf);
    curr = strlen(buf);
    buf += curr;
    out_size += curr;

    sprintf(buf, "]}");



    return oo_OK;
}


static int
ooAccu_build_indices(struct ooAccu    *self, 
		     struct ooMindMap *mindmap)
{
    int i;

    /* topic index */
    self->topic_index = malloc(sizeof(struct ooTopicSolution*) *
			       mindmap->num_topics);
    if (!self->topic_index) return oo_NOMEM;

    for (i = 0; i < mindmap->num_topics; i++)
	self->topic_index[i] = NULL;
    
    self->max_num_topics = mindmap->topics;

    /* concept index */
    self->concept_index = malloc(sizeof(struct ooConcFreq*) *
			       mindmap->num_concepts);
    self->num_concepts = mindmap->num_concepts;
    for (i = 0; i < self->num_concepts; i++)
	self->concept_index[i] = NULL;

    return oo_OK;
}

static int
ooAccu_update_topic(struct ooAccu *self, 
		    struct ooTopicIngredient *ingr)
{
    struct ooTopicSolution *topsol;
    float curr_weight;

    /*printf("Updating Accumulator's topics with \"%s\"...\n", 
      ingr->topic->name, ingr->topic->id);*/

    if (ingr->topic->id > self->max_num_topics) return oo_FAIL;

    topsol = self->topic_index[ingr->topic->id];
    
    if (!topsol) {

	/* printf("First support of topic \"%s\"", ingr->topic->name);*/

	/* TODO: take memory of the least popular topic */
	if (self->num_topic_solutions >= TOPIC_POOL_SIZE)
	    return oo_FAIL;

	topsol = &self->topic_solution_storage[self->num_topic_solutions++];
	topsol->topic = ingr->topic;
	self->topic_index[ingr->topic->id] = topsol;
    }

    curr_weight = ingr->complexity * ingr->relevance;

    if (ingr->id < NUM_TOPIC_INGREDIENTS)
	topsol->ingredients[ingr->id] += curr_weight;

    topsol->weight += curr_weight;

    return oo_OK;
}

static int
ooAccu_update_conc_rating(struct ooAccu *self, 
			  struct ooConcept *conc)
{
    struct ooConcFreq *cfreq, *curr_cfreq;
    
    if (DEBUG_CONC_LEVEL_3)
	printf("\n ** Updating global conceptual index"
               " with \"%s\" (complexity: %.2f)\n",
	       conc->name, conc->complexity);

    cfreq = self->concept_index[conc->numid];
    if (!cfreq) {
	if (self->num_concfreqs >= ACCU_CONCFREQ_STORAGE_SIZE)
	    return oo_FAIL;
	cfreq = &self->freq_storage[self->num_concfreqs];
	self->num_concfreqs++;
	cfreq->conc = conc;
	cfreq->weight = conc->complexity;
	cfreq->gt = NULL;
	cfreq->lt = NULL;

	self->concept_index[conc->numid] = cfreq;

	/* first item in rating */
	if (!self->freq_top) {
	    self->freq_top = cfreq;
	    self->freq_tail = cfreq;
	    return oo_OK;
	}

	curr_cfreq = self->freq_tail;
    }
    else {

	cfreq->weight += conc->complexity;

	/* temporarily remove yourself from the list*/
	if (cfreq->gt) 
	    cfreq->gt->lt = cfreq->lt;

	curr_cfreq = cfreq->gt;
    }

    while (curr_cfreq) {
	/* superior node found */
	if (cfreq->weight <= curr_cfreq->weight) {
	    cfreq->gt = curr_cfreq;
	    cfreq->lt = curr_cfreq->lt;
	    curr_cfreq->lt = cfreq;
	    if (self->freq_tail == curr_cfreq) 
		self->freq_tail = cfreq;
	    return oo_OK;
	}

	/* top reached */
	if (!curr_cfreq->gt) {
	    cfreq->lt = curr_cfreq;
	    cfreq->gt = NULL;
	    curr_cfreq->gt = cfreq;
	    self->freq_top = cfreq;
	    return oo_OK;
	}

	curr_cfreq = curr_cfreq->gt;
    }

    return oo_OK;
}


static int
ooAccu_update(struct ooAccu   *self, 
	      struct ooAgenda *agenda)
{

    /* reset agenda index with NULLs */
    /* for (i = 0; i < AGENDA_INDEX_SIZE; i++) {
	self->index[i] = NULL;
	self->tail_index[i] = NULL;
	}*/

    printf("Adding agenda's best hypotheses to the accumulating storage..");




    return oo_OK;
}

static int
ooAccu_append(struct ooAccu *self,
	      const char    *buf,
	      size_t        buf_size)
{
    /*printf("\"%s\"'s ACCU appending: %s\n\n", 
      self->decoder->codesystem->name, buf);*/

    if (buf_size > self->output_free_space) return oo_NOMEM;

    memcpy(self->curr_output_buf, buf, buf_size);
    self->curr_output_buf += buf_size;
    self->curr_output_buf[0] = '\0';

    self->output_free_space -= buf_size;

    return oo_OK;
}


/* ooAccu Initializer */
static int
ooAccu_init(struct ooAccu *self)
{
    struct ooTopicSolution *topsol;
    int i;

    self->decoder = NULL;

    /* allocate enough memory for the output */
    self->output_buf = malloc(OUTPUT_BUF_SIZE);
    if (!self->output_buf) return oo_NOMEM;
    self->output_buf[0] = '\0';

    self->output_total_space = OUTPUT_BUF_SIZE;
    self->output_free_space = OUTPUT_BUF_SIZE;
    self->curr_output_buf = self->output_buf;

    /* topics */
    self->num_topic_solutions = 0;
    self->rating_count = 0;
    self->max_num_topics;

    self->num_concfreqs = 0;
    self->freq_top = NULL;

    self->concept_index = NULL;
    self->num_concepts = 0;

    self->topic_index = NULL;

    /* initialize the pool of topic solutions */
    for (i = 0; i < TOPIC_POOL_SIZE; i++) {
	topsol = &self->topic_solution_storage[i];
	ooTopicSolution_init(topsol);
	self->topic_solutions[i] = topsol;
	self->topic_rating[i] = NULL;
    }

    self->solution = NULL;
    self->begin_table = false;
    self->begin_row = false;
    self->begin_cell = false;

    /* bind your methods */
    self->del = ooAccu_del;
    self->str = ooAccu_str;
    self->init = ooAccu_init;

    self->build_indices = ooAccu_build_indices;
    self->update_conc_rating = ooAccu_update_conc_rating;
    self->update_topic = ooAccu_update_topic;
    self->present_solution = ooAccu_present_solution;
    self->update = ooAccu_update;
    self->append = ooAccu_append;


    return oo_OK;
}

extern int 
ooAccu_new(struct ooAccu **accu)
{
    struct ooAccu *self = malloc(sizeof(struct ooAccu));
    if (!self) return oo_NOMEM;

    ooAccu_init(self);

    *accu = self;
    return oo_OK;
}
