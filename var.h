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


#ifndef VAR_H
#define VAR_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

enum {
    V_INT,
    V_FLOAT,
    V_STRING,
	V_DATA,
    V_LIST,
    V_ARRAY
};

typedef struct var {
    uint8_t type;
    char 	*name;
	int		size;
    union {
		struct {
        int     itg;
        double  dbl;
        char    *str;
        void    *list;
		void	*data;
		};
		void *v;
    } value;
} var_t;

#define list_next(_l) _l = _l->next
#define list_head(_l, __l) _l = __l->head
#define list_tail(_l, __l) _l = __l

typedef struct var_list {
    struct var_list *next,*prev,*head;
    void *item;
} var_list_t;

/* var */

void json_build(var_list_t *list);
int parse_json(char *map, var_list_t **list);
int parse_json_file(const char *f, var_list_t **list);

#define var_int(x) (int)(x->value.itg)
#define var_str(x) (char *)(x->value.str)
#define var_double(x) (double)(x->value.dbl)
#define var_list(x) (var_list_t *)(x->value.list)
#define var_array(x) (var_list_t *)(x->value.list)
#define var_data(x) (void *)(x->value.data)

var_t *var_new(uint8_t type, char *name, void *value);
void var_free(var_t *var);
void var_add(var_list_t **vstack, var_t *var);
void var_del(var_list_t **list, var_t *item, int (*compare)(void *, void*));
var_t *var_get(var_list_t *list, const char *name);
var_t *var_array_get(var_list_t *list, int idx);
void var_print(var_t *var);
void var_print_list(var_list_t *list);
int var_array_size(var_list_t *list);

char *typestr(var_t *x) ;
#endif
