/*
 * Copyright 2018 Robotic Eyes GmbH
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.*
 */
#include <stdlib.h>

#include "list.h"
#include "util.h"

struct list *list_create()
{
    struct list *l = (struct list *) malloc (sizeof (struct list));
    l->head = NULL;
    l->tail = NULL;
    return l;
}

void list_insert (struct list *l, void *data)
{
    if (l->head == NULL)
    {
        l->head = (struct node *) malloc (sizeof (struct node));
        l->head->data = data;
        l->head->prev = NULL;
        l->head->next = NULL;
        l->tail = l->head;
    }
    else
    {
        struct node *new =  malloc (sizeof (struct node));

        new->data = data;
        new->next = NULL;
        new->prev = l->tail;
        l->tail->next = new;
        l->tail = new;
    }
}

void list_delete_node (struct list *l, struct node *N)
{
    if (!l || !l->head) return;

    struct node *cur = l->head;

    if (cur == N)
    {
        l->head = cur->next;
        FREE (cur);
        return;
    }
    cur = cur->next;

    while (cur != N && cur != NULL)
        cur = cur->next;

    // not found
    if (cur == NULL)
        return;

    // found as last
    if (cur == l->tail)
    {
        l->tail = cur->prev;
        cur->prev->next = NULL;
        FREE (N);
        return;
    }

    // middle
    cur->prev->next = cur->next;
    cur->next->prev = cur->prev;
    FREE (N);
}

void list_destroy (struct list *l)
{
    if (!l) return;
    struct node *cur = l->head;
    while (cur != NULL)
    {
        struct node *next = cur->next;
        FREE (cur);
        cur = next;
    }
    FREE (l);
}
