/*
 * Copyright (C) 2013 rafael villordo.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#include <assert.h>
#include "var.h"

char *typestr(var_t *x) 
{
	char *r = "NULL";
	switch(x->type) {
		case V_ARRAY: r = "array"; break;
		case V_LIST:r = "list"; break;
		case V_INT:r = "int"; break;
		case V_FLOAT:r = "float"; break;
		case V_STRING:r = "string"; break;
		case V_DATA:r = "data"; break;
		default:
			break;
	}
	return (r);
}
int var_compare(void *v1, void *v2)
{
    return ((v1 == v2));
}

var_t *var_new(uint8_t type, char *name, void *value)
{
    var_t *v = malloc(sizeof(var_t));
	v->type = type;
	v->name = malloc(strlen(name));
    strcpy(v->name, name);
	v->name[strlen(name)]=0;
    switch(type) {
        case V_ARRAY:
        case V_LIST:
            v->value.list=value;
            break;
        case V_INT:
            v->value.itg=(int)value;
            break;
        case V_FLOAT:
            v->value.dbl=*(double *)value;
            break;
        case V_STRING:
            v->value.str=strdup((char *)value);
            break;
		case V_DATA:
			v->value.data = value;
			break;
    }
    return v;
}

void var_free(var_t *var)
{
	var_list_t *l;
	if(var->name)
		free(var->name);
	if(var->type==V_STRING) {
		if(var->value.str)
			free(var->value.str);
	} else if(var->type==V_LIST) {
		for(l = var_list(var); l; l = l->next) {
    	    var_free((var_t *)l->item);
		}
	}
	free(var);
}

void var_add(var_list_t **vstack, var_t *var)
{
    var_list_t *n = malloc(sizeof(var_list_t));
	assert(n != NULL);
    n->item=var;
    if((*vstack) == NULL) {
        n->prev=n->next=NULL;
		n->head=n;
        *vstack = n;
    } else {
		n->prev = NULL;
        n->next = *vstack;
        n->head = (*vstack)->head;
        (*vstack)->prev=n;
        *vstack = n;
    }
}

void var_del(var_list_t **list, var_t *item, int (*compare)(void *, void*))
{
    var_list_t *l;
    var_list_t *prev=NULL,
                *next = NULL;

    if(compare == NULL)
        compare = var_compare;

    for(l = (*list); l; l = l->next) {
        if(compare(l->item, item)) {
            if(prev) {
                prev->next = l->next;
            } else {
                if((next = l->next)) {
                    if(l->prev == NULL)
                        next->prev = NULL;
					else next->prev = l->prev;
                    (*list) = next;
                } else {
                    (*list) = NULL;
				}
            }
            free(l);
            break;
        }
        prev = l;
    }
}

var_t *var_get(var_list_t *list, const char *name)
{
    var_list_t *v;
	if(list==NULL)
		return (NULL);
    for(v=list->head; v; v=v->prev) { 
       if(!strcmp(((var_t *)(v->item))->name, name)){
            return v->item;
	   }
    }
    return (NULL);
}

int var_list_size(var_list_t *list)
{
	int size=0;
    var_list_t *v;
    for(v=list->head;v!=NULL;v=v->prev)
		size++;
    return (size?(size):0);
}

int var_array_size(var_list_t *list)
{
	int size=0;
    var_list_t *v;
    for(v=list->head;v!=NULL;v=v->prev)
		size++;
    return (size?(size):0);
}

var_t *var_array_get(var_list_t *list, int idx)
{
    var_list_t *v;
    for(v=list->head; v; v=v->prev) {
        if(!idx--) { 
            return (v->item);
		}
    }
    return (NULL);
}

void var_print(var_t *var)
{
	printf("VAR(%s, ", var->name);
	 switch(var->type) {
        case V_ARRAY:
            printf("%s, %p) [ ", typestr(var), var->value.list);
			var_print_list(var->value.list);
			printf("]\n");
        case V_LIST:
            printf("%s, %p) ", typestr(var), var->value.list);
			var_print_list(var->value.list);
            break;
        case V_INT:
            printf("%s, %d)", typestr(var), *(int *)&var->value.itg);
            break;
        case V_FLOAT:
            printf("%s, %f)", typestr(var), *(double *)&var->value.dbl);
            break;
        case V_STRING:
            printf("%s, %s)", typestr(var), (char *)var->value.str);
            break;
    }
	 printf("\n");
}

void var_print_list(var_list_t *list)
{
    var_list_t *v;
	var_t *item;
    for(v=list; v; v=v->next) {
		item = (var_t*)v->item;
		var_print(item);
		if(item->type==V_LIST) {
			var_print_list((var_list_t*)item->value.list);
		}
    }
}


