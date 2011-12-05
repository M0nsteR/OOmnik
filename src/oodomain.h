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
 *   oodomain.h
 *   OOmnik Domain 
 */

#ifndef OO_DOMAIN_H
#define OO_DOMAIN_H

#include "ooconfig.h"

struct ooMindMap;
struct ooConcept;

/** Domain:
 *  a set of semantically related concepts
 */
typedef struct ooDomain {
    size_t numid;

    char *id;
    size_t id_size;

    char *name;
    size_t name_size;

    char *title;
    size_t title_size;

    size_t depth;

    struct ooMindMap *mindmap;

    struct ooDomain *parent;

    struct ooDomain **subdomains;
    size_t num_subdomains;

    struct ooConcept **concepts;
    size_t num_concepts;

    /***********  public methods ***********/
    int (*del)(struct ooDomain *self);
    int (*str)(struct ooDomain *self);
    int (*add_concept)(struct ooDomain *self,
		       struct ooConcept *c);

} ooDomain;

extern int ooDomain_new(struct ooDomain **self);

#endif /* OO_DOMAIN_H */
