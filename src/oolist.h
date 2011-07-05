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
 *   --------
 *   oolist.h
 *   OOmnik List
 */

#ifndef OOLIST_H
#define OOLIST_H

#include "ooconfig.h"

typedef struct ooListItem
{
    void *data;
    struct ooListItem *next;
    struct ooListItem *previous;

} ooListItem;


typedef struct ooList
{
    /* constructor */
    int (*init)(struct ooList *self);

    /* destructor */
    int (*del)(struct ooList *self);

    /* add a new item (if prev == NULL, then push front) */
    int (*add)(struct ooList *self,
                void *data,
                struct ooListItem *prev);

    /* get an element by it's position */
    struct ooListItem* (*get_item)(struct ooList *self, 
				   size_t pos);

    /* remove the item */
    void* (*remove)(struct ooList *self, 
		    struct ooListItem *item);

    /* clear the list */
    void (*clear)(struct ooList *self);

    /* find an item */
    struct ooListItem* (*find)(struct ooList *self,
			       void *data,
			       oo_compar_func func);

    /* copy the list */
    int (*copy)(struct ooList *self, 
		struct ooList **copy);

    /* returns the next element */
    struct ooListItem* (*next_item)(struct ooList *self, 
				    struct ooListItem *item);

    /* returns the previous element */
    struct ooListItem* (*prev_item)(struct ooList *self, 
				    struct ooListItem *item);

    /* get subsequence */
    struct ooList* (*get_subsequence)(struct ooList *self,
                                      struct ooListItem *start,
                                      struct ooListItem *end,
                                      size_t length);
    /* pointer to the head */
    struct ooListItem *head;
    struct ooListItem *tail;

    /* list size */
    size_t size;

    /* last used item - cash */
    struct ooListItem *cur;
    size_t     cur_pos;
} ooList;


/* constructor */
int ooList_new(struct ooList **self);

#endif  /* OOLIST_H */
