#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "Device/US363/Util/ux360_list.h"

//new
LINK_NODE* new_node(char *data, int size) {
	LINK_NODE *node = NULL;
	char *buf = NULL;

	node = (LINK_NODE*)malloc(sizeof(LINK_NODE));
	if(node == NULL) {
		printf("new_node() malloc node error!\n");
		return NULL;
	}
	else {
		if(size > 0) {
			buf = (char*)malloc(size);
			if(buf != NULL) {
				memcpy(buf, data, size);
				node->data = buf;
                node->size = size;
			}
			else
				printf("new_node() malloc buf error!\n");
		}
		node->pre  = NULL;
		node->next = NULL;
	}
	return node;
}

//add
void add_node(LINK_NODE *list, char *data, int size) {
	LINK_NODE *node = NULL, *list_p = NULL;

	node = new_node(data, size);
	if(node == NULL) return;
	
	if(list == NULL)
		list = node;
	else {
		list_p = list;
		while(list_p->next != NULL) {
			list_p = list_p->next;
		}
		list_p->next = node;
		node->pre = list_p;
	}
}

//clear
void clear_list(LINK_NODE *list) {
	LINK_NODE *p1, *p2;

	p1 = list;
	while(p1 != NULL) {
		p2 = p1->next;
		if(p1->data != NULL)
			free(p1->data);
		free(p1);
		p1 = p2;
	}
}

//get size
int get_list_length(LINK_NODE *list) {
	int size = 0;
	LINK_NODE *node;

	node = list;
	while(node != NULL) {
		size++;
		node = node->next;
	}
	return size;
}

//insert, insert one node after idx
int insert_node(LINK_NODE **list, int idx, char *data, int d_size) {
	int cnt=0;
	int size = get_list_length(*list);
	LINK_NODE *node, *pre, *node2;

	if(idx < 0) {
		printf("insert_node() idx error!\n");
		return -1;
	}
	
	if(idx >= size) {
		add_node(*list, data, d_size);
	}
	else {
		node2 = new_node(data, d_size);
		
		node = *list;
		while(node != NULL && cnt != idx) {
			cnt++;
			node = node->next;
		}
		pre = node->pre;

		if(idx == 0) {
			*list = node2;
			node2->pre = NULL;
			node2->next = node;

			node->pre = node2;
		}
		else {
			node2->pre = pre;
			node2->next = node;

			pre->next = node2;
			node->pre = node2;
		}
	}
	return 0;
}

//remove idx
int remove_node(LINK_NODE **list, int idx) {
	int cnt=0;
	int size = get_list_length(*list);
	LINK_NODE *node, *pre, *next;

	if(idx >= size || idx < 0) {
		printf("remove_node() idx over size!\n");
		return -1;
	}

	node = *list;
	while(node != NULL && cnt != idx) {
		cnt++;
		node = node->next;
	}
	pre = node->pre;
	next = node->next;	

	if(idx == 0) {
		*list = next;
		next->pre = NULL;
	}
	else {
		pre->next = next;
		next->pre = pre;	
	}
	
	if(node->data != NULL)
		free(&node->data);
	free(node);
	return 0;
}

//swap
int swap_node(LINK_NODE **list, int idx1, int idx2) {
	int cnt=0;
	int size = get_list_length(*list);
	LINK_NODE *node1, *pre1, *next1;
	LINK_NODE *node2, *pre2, *next2;
	
	if(idx1 >= size || idx1 < 0 || idx2 >= size  || idx2 < 0) {
		printf("swap_node() idx over size!(%d, %d)\n", idx1, idx2);
		return -1;
	}
	if(idx1 == idx2) return 0;
	
	cnt = 0;
	node1 = *list;
	while(node1 != NULL && cnt != idx1) {
		cnt++;
		node1 = node1->next;
	}
	pre1 = node1->pre;
	next1 = node1->next;	

	cnt = 0;
	node2 = *list;
	while(node2 != NULL && cnt != idx2) {
		cnt++;
		node2 = node2->next;
	}
	pre2 = node2->pre;
	next2 = node2->next;	
	
	node1->pre  = pre2;
	node1->next = next2;
	node2->pre  = pre1;
	node2->next = next1;
	return 0;
}

//serch idx
int serch_node(LINK_NODE *list, int idx, char *data, int d_size) {
	int cnt=0;
	int length = get_list_length(list);
	LINK_NODE *node;

	if(idx >= length || idx < 0) {
		printf("serch_node() idx over length!\n");
		return -1;
	}
		
	node = list;
	while(node != NULL && cnt != idx) {
		cnt++;
		node = node->next;
	}
	if(node == NULL) {
		printf("serch_node() node is NULL!\n");
		return -1;
	}
	else {		  	 
        if(d_size >= node->size) {
            if(node->size > 0)
                memcpy(data, node->data, node->size);
        }
        else {
        	printf("serch_node() d_size too small!\n");
            return -1;
        }
	}
	return node->size;
}

//set list
int set_list(LINK_NODE *list, int idx, char *data, int d_size) {
	int cnt=0;
	int size = get_list_length(list);
	LINK_NODE *node;

	if(idx >= size || idx < 0) {
		printf("set_list() idx over size!\n");
		return -1;
	}
	
	node = list;
	while(node != NULL && cnt != idx) {
		cnt++;
		node = node->next;
	}
	if(node == NULL) {
		printf("set_list() node is NULL!\n");
		return -1;
	}
	else {		  	 
        node->size = d_size;
		if(node->size > 0)
			memcpy(node->data, data, node->size);
	}
	return 0;
}


/*
#define LIST_TYPE_INTEGER			0
#define LIST_TYPE_STRING			1
#define LIST_TYPE_DOUBLE			2
#define LIST_TYPE_STRUCT_GSENSOR	20

#define LINK_NODE_STR_DATA_MAX	64

typedef struct gsensor_val_struct {
	float pan;
	float titl;
	unsigned long long time;
}GSENSOR_VAL;

void set_gsensor_val(GSENSOR_VAL *gsensor, float pan, float titl, unsigned long long time) {
	gsensor->pan  = pan;
	gsensor->titl = titl;
	gsensor->time = time;
}

//show all data
void show_list(int type, LINK_NODE *list) {
	int data_int;
	double data_double;
	char data_str[LINK_NODE_STR_DATA_MAX];
	int size = get_list_length(list);
	LINK_NODE *node;
	GSENSOR_VAL data_gsensor;

	switch(type) {
	case LIST_TYPE_INTEGER:
		node = list;
		while(node != NULL) {
			memcpy(&data_int, node->data, node->size);
			node = node->next;
		}
		break;
	case LIST_TYPE_STRING:
		node = list;
		while(node != NULL) {
			memcpy(&data_str[0], node->data, node->size);
			node = node->next;
		}
		break;
	case LIST_TYPE_DOUBLE:
		node = list;
		while(node != NULL) {
			memcpy(&data_double, node->data, node->size);
			node = node->next;
		}
		break;
	case LIST_TYPE_STRUCT_GSENSOR:
		node = list;
		while(node != NULL) {
			memcpy(&data_gsensor, node->data, node->size);
			node = node->next;
		}
		break;
	}
}

void list_int_test_func(void) {
	int size=0, idx=-1, data=-1;
	LINK_NODE *list = NULL;

	data = 1;
	list = new_node((char*)&data, sizeof(data));
	data = 2;
	add_node(list, (char*)&data, sizeof(data));
	data = 3;
	add_node(list, (char*)&data, sizeof(data));
	data = 4;
	add_node(list, (char*)&data, sizeof(data));
	data = 5;
	add_node(list, (char*)&data, sizeof(data));

	LINK_NODE **list_p = &list;
	data = 6;
	insert_node(list_p, 0, (char*)&data, sizeof(data));

	size = get_list_length(list);
	show_list(LIST_TYPE_INTEGER, list, sizeof(data));

	remove_node(list_p, 2);
	size = get_list_length(list);
	show_list(LIST_TYPE_INTEGER, list, sizeof(data));

	serch_node(list, 2, (char*)&data, sizeof(data));

	data = 10;
	set_list(list, 2, (char*)&data, sizeof(data));
	data = 20;
	set_list(list, 3, (char*)&data, sizeof(data));
	show_list(LIST_TYPE_INTEGER, list, sizeof(data));

	clear_list(list);
}

void list_str_test_func(void) {
	int size=0, idx=-1;
	char str[LINK_NODE_STR_DATA_MAX];
	LINK_NODE *list = NULL;

	sprintf(str, "No.1\0");
	list = new_node(&str[0], sizeof(str));
	sprintf(str, "No.2\0");
	add_node(list, &str[0], sizeof(str));
	sprintf(str, "No.3\0");
	add_node(list, &str[0], sizeof(str));
	sprintf(str, "No.4\0");
	add_node(list, &str[0], sizeof(str));
	sprintf(str, "No.5\0");
	add_node(list, &str[0], sizeof(str));

	LINK_NODE **list_p = &list;
	sprintf(str, "No.6\0");
	insert_node(list_p, 0, &str[0], sizeof(str));

	size = get_list_length(list);
	show_list(LIST_TYPE_STRING, list, sizeof(str));

	remove_node(list_p, 2);
	size = get_list_length(list);
	show_list(LIST_TYPE_STRING, list, sizeof(str));

	serch_node(list, 2, &str[0], sizeof(str));

	sprintf(str, "No.10\0");
	set_list(list, 2, &str[0], sizeof(str));
	sprintf(str, "No.20\0");
	set_list(list, 3, &str[0], sizeof(str));
	show_list(LIST_TYPE_STRING, list, sizeof(str));

	clear_list(list);
}

void list_double_test_func(void) {
	int size=0, idx=-1;
	double data;
	LINK_NODE *list = NULL;

	data = 1.123;
	list = new_node((char*)&data, sizeof(data));
	data = 2.123;
	add_node(list, (char*)&data, sizeof(data));
	data = 3.123;
	add_node(list, (char*)&data, sizeof(data));
	data = 4.123;
	add_node(list, (char*)&data, sizeof(data));
	data = 5.123;
	add_node(list, (char*)&data, sizeof(data));

	LINK_NODE **list_p = &list;
	data = 6.123;
	insert_node(list_p, 0, (char*)&data, sizeof(data));

	size = get_list_length(list);
	show_list(LIST_TYPE_DOUBLE, list, sizeof(data));

	remove_node(list_p, 2);
	size = get_list_length(list);
	show_list(LIST_TYPE_DOUBLE, list, sizeof(data));

	serch_node(list, 2, (char*)&data, sizeof(data));

	data = 10.123;
	set_list(list, 2, (char*)&data, sizeof(data));
	data = 20.123;
    set_list(list, 3, (char*)&data, sizeof(data));
	show_list(LIST_TYPE_DOUBLE, list, sizeof(data));

	clear_list(list);
}

void list_struct_test_func(void) {
	int size=0, idx=-1;
	LINK_NODE *list = NULL;
	GSENSOR_VAL gsensor;

	set_gsensor_val(&gsensor, 1.0, 1.1, 20200306);
	list = new_node((char*)&gsensor, sizeof(gsensor));
	set_gsensor_val(&gsensor, 2.0, 2.1, 20200306);
	add_node(list, (char*)&gsensor, sizeof(gsensor));
	set_gsensor_val(&gsensor, 3.0, 3.1, 20200306);
	add_node(list, (char*)&gsensor, sizeof(gsensor));
	set_gsensor_val(&gsensor, 4.0, 4.1, 20200306);
	add_node(list, (char*)&gsensor, sizeof(gsensor));
	set_gsensor_val(&gsensor, 5.0, 5.1, 20200306);
	add_node(list, (char*)&gsensor, sizeof(gsensor));

	LINK_NODE **list_p = &list;
	set_gsensor_val(&gsensor, 6.0, 6.1, 20200306);
	insert_node(list_p, 0, (char*)&gsensor, sizeof(gsensor));

	size = get_list_length(list);
	show_list(LIST_TYPE_STRUCT_GSENSOR, list, sizeof(gsensor));

	remove_node(list_p, 2);
	size = get_list_length(list);
	show_list(LIST_TYPE_STRUCT_GSENSOR, list, sizeof(gsensor));

	serch_node(list, 2, (char*)&gsensor, sizeof(gsensor));

	set_gsensor_val(&gsensor, 10.0, 10.1, 20200306);
	set_list(list, 2, (char*)&gsensor, sizeof(gsensor));
	set_gsensor_val(&gsensor, 20.0, 20.1, 20200306);
	set_list(list, 3, (char*)&gsensor, sizeof(gsensor));
	show_list(LIST_TYPE_STRUCT_GSENSOR, list, sizeof(gsensor));

	clear_list(list);
}*/