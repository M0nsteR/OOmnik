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
 *   oolist.c
 *   OOmnik List implementation
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "oolist.h"

static
int ooList_del(struct ooList *self)
{
    self->clear(self);

    return true;
}

static int 
ooList_add(struct ooList *self,
	   void *data,
	   struct ooListItem *prev)
{
    struct ooListItem *item;
    
    item = malloc(sizeof(struct ooListItem));
    if (!item) return oo_NOMEM;

    item->next = NULL;
    item->previous = NULL;
    item->data = data;

    self->size++;

    if (!self->head) {
        self->head = item;
        self->tail = item;
        return oo_OK;
    }

    if (!prev) {
        item->next = self->head;
        self->head->previous = item;
        self->head = item;
        return oo_OK;
    }

    if (prev->next)
        prev->next->previous = item;

    if (prev == self->tail)
        self->tail = item;

    item->next     = prev->next;
    item->previous = prev;
    prev->next     = item;

    return oo_OK;
}

static
struct ooListItem* 
ooList_get_item(struct ooList *self,
		size_t pos)
{
    size_t tail_len;
    size_t cur_len;

    if (pos == 0)
        return self->head;

    if (pos == self->size - 1)
        return self->tail;

    if (pos > self->size - 1)
        return NULL;

    tail_len = self->size - pos - 1;

    if (self->cur) {
	if (self->cur_pos == pos)
            return self->cur;

        cur_len = abs(pos - self->cur_pos);

        if (cur_len < pos && cur_len < tail_len) {
            if (pos < self->cur_pos) {
                while (self->cur_pos != pos) {
                    self->cur = self->cur->previous;
                    self->cur_pos--;
                }
            }
            else {
                while (self->cur_pos != pos) {
                    self->cur = self->cur->next;
                    self->cur_pos++;
                }
            }
            return self->cur;
        }

    }

    if (pos > tail_len) {
        self->cur = self->tail;
        self->cur_pos = self->size - 1;

        while (self->cur_pos != pos) {
            self->cur = self->cur->previous;
            self->cur_pos--;
        }
        return self->cur;
    }

    self->cur = self->head;
    self->cur_pos = 0;

    while (self->cur_pos != pos) {
        self->cur = self->cur->next;
        self->cur_pos++;
    }
    return self->cur;

}

static void* 
ooList_remove(struct ooList *self,
	      struct ooListItem *item)
{
    void *data = item->data;

    self->size--;

    if (item == self->cur) {
        if (self->cur->next)
            self->cur = self->cur->next;
        else self->cur = self->cur->previous;
    }

    if (item == self->head) {
        if (item->next)
            item->next->previous = NULL;
        self->head = self->head->next;
    }
    else if (item == self->tail) {
        item->previous->next = NULL;
        self->tail = self->tail->previous;
    }
    else    {
        item->previous->next = item->next;
        item->next->previous = item->previous;
    }

    free(item);

    return data;
}

static
int ooList_clear(struct ooList *self)
{
    struct ooListItem *cur = self->head;
    struct ooListItem *tmp;

    if (!cur)
        return oo_FAIL;

    while (cur) {
        tmp = cur;
        cur = cur->next;
        free(tmp);
    }

    self->head  = NULL;
    self->tail  = NULL;
    self->size = 0;
    self->cur   = NULL;

    return oo_OK;
}

static
struct ooListItem* ooList_next_item(struct ooList *self,
                              struct ooListItem *item)
{
    if (item == self->tail)
        return NULL;
    return item->next;
}

static
struct ooListItem* ooList_prev_item(struct ooList      *self,
                              struct ooListItem *item)
{
    if (item == self->head)
        return NULL;
    return item->previous;
}

static
ooList* ooList_get_subsequence(struct ooList      *self,
                               struct ooListItem *start,
                               struct ooListItem *end,
                               size_t     length)
{
    int ret;
    struct ooList *result;

    ret = ooList_new(&result);
    if (ret != oo_OK) return ret;

    result->head  = start;
    result->size = length;

    if (end)
        result->tail = end->previous;
    else result->tail = self->tail;

    return result;
}

static int
ooList_copy(struct ooList *self,
	    struct ooList **copy)
{
    struct ooList *new_list;
    struct ooListItem *curr, *new_curr;
    int ret;

    ret = ooList_new(&new_list);
    if (ret != oo_OK) return ret;

    curr = self->head;

    while (curr) {
        new_curr = malloc(sizeof(struct ooListItem));
	if (!new_curr) return oo_NOMEM;
 
        new_curr->data = curr->data;
        new_list->add(new_list, new_curr, new_list->tail);
        curr = self->next_item(self, curr);
    }

    *copy = new_list;
    return oo_OK;
}

static struct ooListItem* 
ooList_find(struct ooList *self,
	    void          *data,
	    oo_compar_func func)
{
    struct ooListItem *cur = self->head;

    while (func(data, cur->data) != 0) {
        cur = self->next_item(self, cur);
        if (!cur)
            break;
    }

    return cur;
}


int ooList_init(struct ooList *self)
{
    self->del             = ooList_del;
    self->add             = ooList_add;
    self->clear           = ooList_clear;
    self->remove          = ooList_remove;
    self->get_item        = ooList_get_item;
    self->next_item       = ooList_next_item;
    self->prev_item       = ooList_prev_item;
    self->get_subsequence = ooList_get_subsequence;
    self->copy            = ooList_copy;
    self->find            = ooList_find;
    self->head            = NULL;
    self->size            = 0;
    self->tail            = self->head;
    self->cur             = NULL;

    return oo_OK;
}


/* constructor */
extern int
ooList_new(struct ooList **list)
{
    size_t i;
    struct ooList *l;

    struct ooList *self = malloc(sizeof(struct ooList));
    if (!self) return oo_NOMEM;

    ooList_init(self);

    *list = self;

    return oo_OK;
}
