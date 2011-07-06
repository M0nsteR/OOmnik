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
 *   OOmnik Topic implementation
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "oomindmap.h"
#include "ootopic.h"
#include "ooconfig.h"

/*  Destructor */
static int
ooTopic_del(struct ooTopic *self)
{
    
    free(self->name);

    /* free up yourself */
    free(self);
    return oo_OK;
}

static int
ooTopicSolution_del(struct ooTopicSolution *self)
{

    /* free up yourself */
    free(self);
    return oo_OK;
}

static int
ooTopic_str(struct ooTopic *self)
{
    printf("\n---- CURRENT STATE OF INTERPRETATION\n");

    return oo_OK;
}

static int
ooTopicSolution_str(struct ooTopicSolution *self)
{


    return oo_OK;
}


static int
ooTopic_read_ingredients(struct ooTopic *self, 
			 xmlNode *input_node)
{
    xmlNode *cur_node = NULL;
    char *value;
    struct ooTopicIngredient *ingr;
    size_t ingr_size;
    struct ooTopicIngredient **ingrs;
    int ret;

    for (cur_node = input_node; cur_node; cur_node = cur_node->next) {
	if (cur_node->type != XML_ELEMENT_NODE) continue;

	/* concept ingredient */
	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"concept"))) {

	    value = (char*)xmlGetProp(cur_node,  (const xmlChar *)"name");
	    if (!value) continue;

	    ingr = malloc(sizeof(struct ooTopicIngredient));
	    if (!ingr) {
		xmlFree(value);
		return oo_NOMEM;
	    }

	    ingr->name = malloc(strlen(value) + 1);
	    if (!ingr->name) {
		free(ingr);
		xmlFree(value);
		return oo_NOMEM;
	    }
	    strcpy(ingr->name, value);
	    xmlFree(value);

	    /* default value: maximal relevance */
	    ingr->relevance = 1.0;

	    value = (char*)xmlGetProp(cur_node,  (const xmlChar *)"relev");
	    if (value) {
		ingr->relevance = atof(value);
		xmlFree(value);
	    }

	    value = (char*)xmlGetProp(cur_node,  (const xmlChar *)"relevance");
	    if (value) {
		ingr->relevance = atof(value);
		xmlFree(value);
	    }

	    ingr->topic = self;
	    ingr->conc = NULL;
	    ingr->next = NULL;
	    ingr->complexity = DEFAULT_CONCEPT_COMPLEXITY;

	    /*printf("TOPIC INGREDIENT: %p %s relevance: %.2f\n", 
	      ingr->name, ingr->name, ingr->relevance);*/

	    ingr_size = (self->num_ingredients + 1) * 
		sizeof(struct ooTopicIngredient*);
	    ingrs = realloc(self->ingredients, ingr_size);
	    if (!ingrs) {
		free(ingr->name);
		free(ingr);
		return oo_NOMEM;
	    }
	    
	    ingrs[self->num_ingredients] = ingr;
	    self->ingredients = ingrs;
	    self->num_ingredients++;
	}

    }

    return oo_OK;
}


static int
ooTopic_resolve_refs(struct ooTopic *self, 
		     struct ooMindMap *mindmap)
{
    struct ooTopicIngredient *ingr;
    struct ooConcept *conc;

    size_t i;

    printf("Resolving refs of topic \"%s\"...\n",
	self->name);

    for (i = 0; i < self->num_ingredients; i++) {
	ingr = self->ingredients[i];

	conc = mindmap->lookup(mindmap, (const char*)ingr->name);

	if (conc) {
	    printf("Updating topic index with concept %d\n",
		conc->id);

	    if (conc->id > mindmap->num_concepts) return oo_FAIL;

	    ingr->conc = conc;
	    ingr->next = mindmap->topic_index[conc->id];
	    mindmap->topic_index[conc->id] = ingr;

	}

    }

    return oo_OK;
}


static int
ooTopic_read_XML(struct ooTopic *self, 
		xmlNode *input_node)
{
    xmlNode *cur_node = NULL;
    char *value;
    int ret;

    value = (char*)xmlGetProp(input_node,  (const xmlChar *)"name");
    if (value) {
	self->name = malloc(strlen(value) + 1);
	if (!self->name) {
	    xmlFree(value);
	    return oo_NOMEM;
	}
	strcpy(self->name, value);
	xmlFree(value);
    }

    printf("...Reading topic \"%s\"...\n", self->name);

    for (cur_node = input_node->children; cur_node; cur_node = cur_node->next) {
	if (cur_node->type != XML_ELEMENT_NODE) continue;

	/* concept ingredients */
	if ((!xmlStrcmp(cur_node->name, (const xmlChar *)"concepts"))) {
	    ret = ooTopic_read_ingredients(self, cur_node->children);
	    continue;
	}

    }

    return oo_OK;
}

/* ooTopic Initializer */
extern int 
ooTopic_new(struct ooTopic **topic)
{
    struct ooTopic *self = malloc(sizeof(struct ooTopic));
    if (!self) return oo_NOMEM;

    self->id = 0;
    self->name = NULL;

    self->ingredients = NULL;
    self->num_ingredients = 0;

    self->topics = NULL;
    self->num_topics = 0;

    /* bind your methods */
    self->del = ooTopic_del;
    self->str = ooTopic_str;
    self->read = ooTopic_read_XML;
    self->resolve_refs = ooTopic_resolve_refs;

    *topic = self;
    return oo_OK;
}

/* ooTopic Solution Initializer */
extern int 
ooTopicSolution_init(struct ooTopicSolution *self)
{
    self->topic = NULL;
    self->weight = 0;

    /* bind your methods */
    self->del = ooTopicSolution_del;
    self->str = ooTopicSolution_str;

    return oo_OK;
}
