/*
    Copyright (C) <2012>  <huangweilook@21cn.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/*
 *侵入式的单向列表
*/
#ifndef _LLIST_H
#define _LLIST_H

#include <stdint.h>
#include <stdlib.h>

struct lnode
{
    lnode *next;
};


struct llist
{
private:
	int   size;
    lnode *head;
    lnode *tail;
public:
	llist():size(0),head(NULL),tail(NULL){
	}

	lnode *Head(){
		return head;
	}

	lnode *Tail(){
		return tail;
	}

	void push_back(lnode *node){
		if(node->next) return;
		node->next = NULL;
		if(0 == size)
			head = tail = node;
		else
		{
			tail->next = node;
			tail = node;
		}
		++size;
	}


	void push_front(lnode *node)
	{
		if(node->next) return;
		node->next = NULL;
		if(0 == size)
			head = tail = node;
		else
		{
			node->next = head;
			head = node;
		}
		++size;
	}

	lnode* llist_pop()
	{
		if(0 == size)
			return NULL;
		lnode *node = head;
		head = head->next;
		if(NULL == head)
			tail = NULL;
		--size;
		node->next = NULL;
		return node;
	}

	bool isEmpty(){
		return size == 0;
	}

	int Size(){
		return size;
	}
};

#endif
