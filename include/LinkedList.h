#include <stddef.h>

#ifndef __LIKED_LIST_H
#define __LIKED_LIST_H

typedef struct list_node {
    struct list_node *next;
} list_node_t;

#ifdef _WIN32
#define EXPORTFUNC __declspec(dllexport)
#else
#define EXPORTFUNC
#endif

EXPORTFUNC list_node_t *list_get_tail(list_node_t **head);

EXPORTFUNC list_node_t *list_append(list_node_t **head, list_node_t *item);

EXPORTFUNC list_node_t *list_pop(list_node_t **head);

EXPORTFUNC list_node_t *list_remove(list_node_t **head, list_node_t *item, size_t size_of_value);

EXPORTFUNC void list_reverse(list_node_t **head);

#endif //__LIKED_LIST_H