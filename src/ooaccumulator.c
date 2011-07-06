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
ooAccu_present_solution(struct ooAccu *self,
			char *buf,
			size_t max_buf_size)
{
    struct ooTopicSolution *topsol;
    size_t curr = 0, out_size = 0;
    int i;

    /* TODO: select format */

    sprintf(buf, "{");
    curr = 1;
    buf++;
    out_size++;

    if (self->num_topic_solutions) {
	sprintf(buf, "\"topics\": [");
	curr = strlen(buf);
	buf += curr;
	out_size += curr;
    }

    for (i = 0; i < self->num_topic_solutions; i++) {
	topsol = self->topic_solutions[i];
	if (i == 0)
	    sprintf(buf, "{\"name\":\"%s\", \"weight\":\"%.2f\"}",
		    topsol->topic->name, topsol->weight);
	else
	    sprintf(buf, ",{\"name\":\"%s\", \"weight\":\"%.2f\"}",
		    topsol->topic->name, topsol->weight);
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
ooAccu_build_topic_index(struct ooAccu    *self, 
			 struct ooMindMap *mindmap)
{
    int i;
    
    self->topic_index = malloc(sizeof(struct ooTopicSolution*) *
			       mindmap->num_topics);
    if (!self->topic_index) return oo_NOMEM;
    for (i = 0; i < mindmap->num_topics; i++)
	self->topic_index[i] = NULL;
    
    self->max_num_topics = mindmap->topics;

    return oo_OK;
}


static int
ooAccu_update_topic(struct ooAccu *self, 
		    struct ooTopicIngredient *ingr,
		    struct ooCompex *complex)
{
    struct ooTopicSolution *topsol;

    printf("Updating Accumulator's topics with \"%s\"...\n", ingr->topic->name);

    if (ingr->topic->id > self->max_num_topics) return oo_FAIL;

    topsol = self->topic_index[ingr->topic->id];
    
    if (!topsol) {

	printf("First support of topic \"%s\"", ingr->topic->name);

	/* TODO: take memory of the least popular topic */
	if (self->num_topic_solutions > TOPIC_POOL_SIZE)
	    return oo_FAIL;

	topsol = &self->topic_solution_storage[self->num_topic_solutions++];
	topsol->topic = ingr->topic;
	self->topic_index[ingr->topic->id] = topsol;
    }

    topsol->weight += ingr->complexity * ingr->relevance;


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
    if (buf_size > self->output_free_space) return oo_NOMEM;

    memcpy(self->curr_output_buf, buf, buf_size);
    self->curr_output_buf += buf_size;
    self->curr_output_buf[0] = '\0';

    self->output_free_space -= buf_size;

    return oo_OK;
}




/* ooAccu Initializer */
extern int 
ooAccu_new(struct ooAccu **accu)
{
    struct ooTopicSolution *topsol;
    size_t i;

    struct ooAccu *self = malloc(sizeof(struct ooAccu));
    if (!self) return oo_NOMEM;

    self->decoder = NULL;

    const char *header = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
	                 "<STATEMENT>\n<COMPLEXES>\n";
    const char *footer = "\n</COMPLEXES>\n</STATEMENT>\n";

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

    /* initialize the pool of topic solutions */
    for (i = 0; i < TOPIC_POOL_SIZE; i++) {
	topsol = &self->topic_solution_storage[i];
	ooTopicSolution_init(topsol);
	self->topic_solutions[i] = topsol;
    }

    self->solution = NULL;
    self->begin_table = false;
    self->begin_row = false;
    self->begin_cell = false;

    /* bind your methods */
    self->del = ooAccu_del;
    self->reset = ooAccu_reset;
    self->str = ooAccu_str;

    self->build_topic_index = ooAccu_build_topic_index;
    self->update_topic = ooAccu_update_topic;
    self->present_solution = ooAccu_present_solution;
    self->update = ooAccu_update;
    self->append = ooAccu_append;

    *accu = self;
    return oo_OK;
}
