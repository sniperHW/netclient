#ifndef _DLIST_H
#define _DLIST_H

class dlist;
struct dnode{
	dnode():next(NULL),pre(NULL),_dlist(NULL){}
	dnode *next;
	dnode *pre;
	dlist  *_dlist;
	dnode(const dnode&);
	dnode& operator = (const dnode&);	
};

class dlist{
public:
	dlist():size(0){
		head.next = &tail;
		tail.pre = &head;
	}
	size_t Size(){
		return size;
	}

	bool Empty(){
		return size == 0;
	}

	dnode *Begin(){
		if(Empty())return &tail;
		return head.next;
	}

	dnode *End(){
		return &tail;
	}

	void Push(dnode *n){
		if(n->_dlist || n->next || n->pre) return;
		tail.pre->next = n;
		n->pre = tail.pre;
		tail.pre = n;
		n->_dlist = this;
		n->next = &tail;
		++size;
	}

	void Remove(dnode *n)
	{
		if(n->_dlist != this || (!n->pre && !n->next))
			return;
		n->pre->next = n->next;
		n->next->pre = n->pre;
		n->pre = n->next = NULL;
		n->_dlist = NULL;
		--size;
	}

	dnode* Pop(){
		if(Empty())
			return NULL;
		else
		{
			dnode *n = head.next;
			Remove(n);
			return n;
		}	
	}

private:
	dnode head;
	dnode tail;
	size_t size;
};


#endif