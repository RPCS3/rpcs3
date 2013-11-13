/*
* Copyright (c) 2011-2013 by naehrwert
* This file is released under the GPLv2.
*/

#ifndef _LIST_H_
#define _LIST_H_

#include "types.h"

#define LIST_FOREACH(iter, list) for(lnode_t *iter = list->head; iter != NULL; iter = iter->next)

typedef struct _lnode
{
	void *value;
	struct _lnode *next;
} lnode_t;

typedef struct _list
{
	lnode_t *head;
	scetool::u32 count;
} list_t;

list_t *list_create();
void list_destroy(list_t *l);
BOOL list_isempty(list_t *l);
scetool::u32 list_count(list_t *l);
BOOL list_push(list_t *l, void *value);
void *list_pop(list_t *l);
BOOL list_add_back(list_t *l, void *value);
void *list_get(list_t *l, scetool::u32 idx);
lnode_t *list_get_node(list_t *l, scetool::u32 idx);
BOOL list_remove_node(list_t *l, lnode_t *node);
BOOL list_remove_value(list_t *l, void *value);

#endif
