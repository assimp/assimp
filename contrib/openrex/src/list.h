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
#pragma once

/**
 * \file
 * \brief Simple linked list storing an arbitrary payload
 */

struct node
{
    void *data;
    struct node *prev;
    struct node *next;
};

struct list
{
    struct node *head;
    struct node *tail;
};

struct list *list_create();
void list_destroy (struct list *l);

void list_insert (struct list *l, void *data);
void list_delete_node (struct list *l, struct node *N);
