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
 *   ----------
 *   oodomain.c
 *   OOmnik Domain implementation
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <libxml/parser.h>

#include "oodomain.h"
#include "ooconcept.h"
#include "ooconfig.h"

/*  Destructor */
static int
ooDomain_del(struct ooDomain *self)
{
    int i;

    if (self->subdomains)
	free(self->subdomains);

    if (self->concepts)
	free(self->concepts);

    free(self->id);
    free(self->name);
    free(self->title);

    /* free up yourself */
    free(self);
    return oo_OK;
}

static int
ooDomain_str(struct ooDomain *self)
{
    struct ooConcept *conc;
    struct ooDomain *subdomain;
    int i;
    printf("\nDomain \"%s : %s\" (depth: %d)\n",
	   self->title, self->name, (unsigned int)self->depth);

    if (self->parent)
	printf("  parent -> \"%s\"\n", self->parent->title);

    if (self->num_concepts) {
	printf(" -- concepts:\n");
	for (i = 0; i < self->num_concepts; i++) {
	    conc = self->concepts[i];
	    printf(" %s ", conc->name);
	}
	printf("\n");
    }

    if (self->num_subdomains) {
	printf(" -- subdomains:\n");
	for (i = 0; i < self->num_subdomains; i++) {
	    subdomain = self->subdomains[i];
	    printf("  o  %s\n", subdomain->title);
	}
    }

    return oo_OK;
}

static int
ooDomain_add_concept(struct ooDomain *self,
		     struct ooConcept *c)
{
    size_t concepts_size;
    struct ooConcept **concepts;

    concepts_size = (self->num_concepts + 1) * sizeof(struct ooConcept*);
    concepts = realloc(self->concepts, concepts_size);
    if (!concepts) return oo_NOMEM;

    c->domain = self;

    concepts[self->num_concepts] = c;
    self->concepts = concepts;
    self->num_concepts++;

    return oo_OK;
}

/* ooDomain Initializer */
extern int 
ooDomain_new(struct ooDomain **domain)
{
    struct ooDomain *self = malloc(sizeof(struct ooDomain));
    if (!self) return oo_NOMEM;

    self->numid = 0;

    self->id = NULL;
    self->id_size = 0;

    self->name = NULL;
    self->name_size = 0;

    self->title = NULL;
    self->title_size = 0;

    self->depth = 0;

    self->mindmap = NULL;
    self->parent = NULL;
   
    self->subdomains = NULL;
    self->num_subdomains = 0;

    self->concepts = NULL;
    self->num_concepts = 0;

    /* bind your methods */
    self->del = ooDomain_del;
    self->str = ooDomain_str;
    self->add_concept = ooDomain_add_concept;

    *domain = self;
    return oo_OK;
}

