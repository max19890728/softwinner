/*******************************************************************************
 * Copyright (c), 2021, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#ifndef __UX360_LIST_H__
#define __UX360_LIST_H__


#ifdef __cplusplus
extern "C" {
#endif

typedef struct link_node {
    int size;
	char *data;
	struct link_node *pre;
	struct link_node *next;
}LINK_NODE;

LINK_NODE* new_node(char *data, int size);
void add_node(LINK_NODE *list, char *data, int size);
void clear_list(LINK_NODE *list);
int get_list_length(LINK_NODE *list);
int insert_node(LINK_NODE **list, int idx, char *data, int d_size);
int remove_node(LINK_NODE **list, int idx);
int serch_node(LINK_NODE *list, int idx, char *data, int d_size);
int set_list(LINK_NODE *list, int idx, char *data, int d_size);

#ifdef __cplusplus
}   // extern "C"
#endif

#endif	//__UX360_LIST_H__