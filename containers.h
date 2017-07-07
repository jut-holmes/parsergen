
#ifndef __CONTAINERS_H_
#define __CONTAINERS_H_

#include <string.h>

//
// linked list
//
template<class _Typ> class _list
{
public:
	template<class __Typ> class _listnode
	{
	public:
		_listnode(__Typ _val) : val(_val) {prev=next=0;}
		_listnode() {prev=next=0;}
		__Typ	val;
		_listnode<__Typ>* prev;
		_listnode<__Typ>* next;
	};
	typedef _listnode<_Typ>* iterator;
	_list() {_front=_back=0;nsize=0;}
	_list(_list<_Typ>& l) {_front=_back=0;nsize=0;*this=l;}
	~_list() {clear();}
	_list<_Typ>& operator=(const _list<_Typ>& l) {
		for (_list<_Typ>::iterator it=l._front;it;it=it->next)
            push_back(it->val);
		return *this;
	}
	iterator push_back(_Typ val) {
		_listnode<_Typ>* n = new _listnode<_Typ>(val);
		if (!_front) _front=_back=n;
		else {n->prev=_back;_back=_back->next=n;}
		nsize++;
		return n;
	}
	iterator push_front(_Typ val) {
		_listnode<_Typ>* n = new _listnode<_Typ>(val);
		if (!_front) _front=_back=n;
		else {n->next=_front;_front=_front->prev=n;}
		nsize++;
		return n;
	}
	void pop_front() {
		if (_front->next) _front->next->prev=0;
		_listnode<_Typ>* tmp = _front;
		_front=_front->next;
		nsize--;
		delete tmp;
	}
	void pop_back() {
		if (_back->prev) _back->prev->next=0;
		_listnode<_Typ>* tmp = _back;
		_back=_back->prev;
		nsize--;
		delete tmp;
	}
	int size() {return nsize;}
	_Typ& back() {return _back->val;}
	_Typ& front() {return _front->val;}
	iterator first() {return _front;}
	iterator last() {return _back;}
	iterator erase(iterator it) {
		_listnode<_Typ>* ret = it->prev;
		if (it->prev) it->prev->next = it->next;
		if (it->next) it->next->prev = it->prev;
		if (it == _front) _front=it->next;
		if (it == _back) _back=it->prev;
		delete it;
		return ret;
	}
	void clear() {
		for (iterator it=_front;it;) {
			_listnode<_Typ>* tmp = it;
			it=it->next;
			delete tmp;
		}
		_front=_back=0;nsize=0;
	}
protected:
	int nsize;
	_listnode<_Typ>* _front;
	_listnode<_Typ>* _back;
};

//
// vector
//
template<class _Typ> class _vector
{
public:
	_vector() {v=0;nsize=0;ncapacity=0;}
	_vector(const _vector<_Typ>& vec) {
		v=0;nsize=0;ncapacity=0;
		*this=vec;
	}
	~_vector() {clear();}
	void push_back(_Typ val) {
		reserve((nsize+1)*2);
		v[nsize++] = val;
	}
	void pop_back() {
		if (nsize>0) nsize--;
	}
	typedef _Typ* iterator;
	void reserve(int ncap) {
		if (ncapacity < ncap) {
			int noldcap=ncapacity;
			ncapacity=ncap;
			if (ncapacity<8) ncapacity=8;
			_Typ* vn = new _Typ[ncapacity]();
			if (v) {for (int i=0;i<nsize;i++)vn[i]=v[i];delete[]v;}
//			for (int i=noldcap;i<ncapacity;i++) vn[i]=_Typ();
			v=vn;
		}
	}
	void resize(int size) {
		if (size==nsize) return;
		if (size>nsize)	reserve(size);
		nsize=size;
	}
	_vector<_Typ>& operator=(const _vector<_Typ>& vec) {
		clear();
		nsize=vec.nsize;
		ncapacity=vec.ncapacity;
		v=new _Typ[ncapacity]();
		for (int i=0;i<nsize;i++) v[i]=vec.v[i];
		return *this;
	}
	iterator begin() {return v;}
	iterator end() {return v+nsize;}
	int size() {return nsize;}
	int capacity() {return ncapacity;}
	_Typ& back() {return v[nsize-1];}
	_Typ& front() {return v[0];}
	void clear() {if(v)delete[]v;v=0;nsize=ncapacity=0;}
	_Typ& operator[](int idx) {return v[idx];}
protected:
	_Typ* v;
	int nsize;
	int ncapacity;
};

//
// non-exclusive hashmap, allows duplicates
//
template<class _Key, class _Val> struct _multimap
{
public:
	template<class __Key, class __Val> struct node
	{
		__Key key;
		__Val value;
		node* prev;
		node* next;
		node* prev_walk;
		node* next_walk;
	};
	typedef node<_Key,_Val>* iterator;
	_multimap() {head=tail=0;nsize=0;memset(buckets,0,sizeof(node<_Key,_Val>*)*37);}
	~_multimap() {node<_Key,_Val>* it=head; while(it){node<_Key,_Val>* tmp=it->next_walk; delete it;it=tmp;};head=tail=0;}
	iterator add(_Key key, _Val val)
	{
		int bucket = ((unsigned int)key) % 37;
		node<_Key,_Val>* b = new node<_Key,_Val>;
		b->key= key;
		b->value = val;
		b->prev=b->next=0;
		b->prev_walk=b->next_walk=0;
		if (!head) {head=b;tail=b;} else {b->prev_walk=tail;tail->next_walk=b;tail=b;}
		if (!buckets[bucket])
			buckets[bucket] = b;
		else {
			node<_Key,_Val>* p=buckets[bucket];
			while (p->next) {p=p->next;}
			p->next = b;
		}
		nsize++;
		return b;
	}
	iterator find(_Key key)
	{
		int bucket = ((unsigned int)key) % 37;
		node<_Key,_Val>* p=buckets[bucket];
		if (p) {do {if (p->key==key) return p; p=p->next;} while(p);}
		return 0;
	}
	_Val& operator[](_Key key) {
		iterator it=find(key);
		if (it) return it->value;
		return add(key, _Val())->value;
	}
	iterator first(){return head;}
	int size() {return nsize;}
	void erase(iterator it)
	{
		if (!it) return;
		if (it->prev_walk) it->prev_walk->next_walk=it->next_walk;
		if (it->next_walk) it->next_walk->prev_walk=it->prev_walk;
		if (it == head) head=it->val.next_walk;
		if (it == tail) tail=it->val.prev_walk;
		delete it;
	}
protected:
	int nsize;
	node<_Key,_Val>*	buckets[37];
	node<_Key,_Val>* head;
	node<_Key,_Val>* tail;
};

//
// fast set
//
#ifdef MINIMAL_SET
#define fsSetBit(bit) (map[(bit >> 3)] |= (1 << (bit % 8)))
#define fsIsSet(bit) (map[(bit >> 3)] & (1 << (bit % 8)))
#define fsAllocSize (elem_count+7)/8	
#else
#define fsSetBit(bit) (map[bit]=1)
#define fsIsSet(bit) (map[bit])
#define fsAllocSize (elem_count)
#endif
struct _fastset
{
	struct _fastsetnode
	{
		int id;
		_fastsetnode* next;
	};
	_fastset(int elem_count=256) {
		alloc_size=fsAllocSize;
		map=new unsigned char[alloc_size];
		memset(map,0,alloc_size);
		head=tail=0;
	}
	~_fastset() {clear(); delete[] map;}
	void set(int elem) {
		if (fsIsSet(elem)) return;
		fsSetBit(elem);
		_fastsetnode* n=new _fastsetnode;
		n->id = elem;
		n->next=0;
		if (!head) {
			head=tail=n;
		} else {
			tail->next=n;
			tail=n;
		}
	}
	void clear() {
		memset(map,0,alloc_size);
		_fastsetnode* it=head;
		while(it){_fastsetnode* tmp=it->next;delete it;it=tmp;}
		head=tail=0;
	}
	_fastsetnode* first() {return (it_ptr=head);}
	_fastsetnode* next(){return (it_ptr=it_ptr->next);}

	int alloc_size;
	unsigned char* map;
	_fastsetnode* head;
	_fastsetnode* tail;
	_fastsetnode* it_ptr;
};

//
// hashing container
//
template<class _Key, class _Node> struct _hashcont
{
public:
	typedef _Node* iterator;
	_hashcont() {
		head=tail=0;
		nsize=0;
	}
	_hashcont(const _hashcont<_Key,_Node>& hs) {
		head=tail=0;
		nsize=0;
		*this = hs;
	}
	~_hashcont() {clear();}
	iterator add(_Key key)
	{
		int bucket = ((unsigned int)key) % 37;
		for (_list<_Node>::iterator it=buckets[bucket].first(); it; it=it->next)
			if (it->val.key==key) return &it->val;
		_Node s(key);
		_list<_Node>::iterator nn = buckets[bucket].push_back(s);
		if (!head) {
			head=&nn->val;
			tail=&nn->val;
		} else {
			tail->next_walk=&nn->val;
			nn->val.prev_walk=tail;
			tail=&nn->val;
		}
		nsize++;
		return &nn->val;
	}
	void erase(_Key key)
	{
		int bucket = ((unsigned int)key) % 37;
		_list<_Node>::iterator it = buckets[bucket].first();
		for (; it; it=it->next) if (it->val.key==key) break;
		if (!it) return;
		if (it->val.prev_walk) it->val.prev_walk->next_walk=it->val.next_walk;
		if (it->val.next_walk) it->val.next_walk->prev_walk=it->val.prev_walk;
		if (&it->val == head) head=it->val.next_walk;
		if (&it->val == tail) tail=it->val.prev_walk;
		buckets[bucket].erase(it);
	}
	virtual _hashcont<_Key,_Node>& operator=(const _hashcont<_Key,_Node>& hs) {
		for (iterator it=hs.first(); it; it=it->next_walk) add(it->key);
		return *this;
	}
	iterator find(_Key key)
	{
		int bucket = ((unsigned int)key) % 37;
		for (_list<_Node>::iterator it=buckets[bucket].first(); it; it=it->next)
			if (it->val.key==key) return &it->val;
		return 0;
	}
	iterator first() const {return head;}
	bool compare(const _hashcont<_Key,_Node>& s)
	{
		iterator s1 = first();
		iterator s2 = s.first();
		for (;s1 && s2; s1=s1->next_walk,s2=s2->next_walk)
			if (s1->key != s2->key)
				return false;
		if (s1 || s2) return false;
		return true;
	}
	void clear() {
		for (int i=0;i<37;i++) buckets[i].clear();
		head=tail=0; nsize=0;
	}
	int size() {return nsize;}
protected:
	int nsize;
	_list<_Node> buckets[37];
	_Node* head;
	_Node* tail;
};

//
// _set<>
//
template<class _Key> class _setnode
{
public:
	_setnode() : next_walk(0), prev_walk(0) {}
	_setnode(_Key _key) : key(_key), prev_walk(0), next_walk(0) {}
	_Key key;
	_setnode* prev_walk;
	_setnode* next_walk;
};
template<class _Key> class _hashset : public _hashcont<_Key,_setnode<_Key> >
{
public:
	_hashset() {}
	_hashset(const _hashset<_Key>& hs) {*this=hs;}
};

//
// _map<>
//
template<class _Key, class _Value> struct _mapnode
{
	_mapnode() : next_walk(0), prev_walk(0) {}
	_mapnode(_Key _key) : key(_key), prev_walk(0), next_walk(0) {}
	_Key key;
	_Value value;
	_mapnode* prev_walk;
	_mapnode* next_walk;
};
template<class _Key, class _Value> struct _hashmap : public _hashcont<_Key,_mapnode<_Key,_Value> >
{
	_hashmap() : _hashcont<_Key,_mapnode<_Key,_Value> >() {}
	_hashmap(const _hashmap<_Key,_Value>& hs) : _hashcont<_Key,_mapnode<_Key,_Value> >() {*this = hs;}
	_Value& operator[](_Key key) {
		_hashcont<_Key,_mapnode<_Key,_Value> >::iterator it=find(key);
		if (it) return it->value;
		it=_hashcont<_Key,_mapnode<_Key,_Value> >::add(key);
		it->value = _Value();
		return it->value;
	}
	bool contains(_Key key) {
		_hashcont<_Key,_mapnode<_Key,_Value> >::iterator it=find(key);
		return (it != NULL);
	}
	_hashcont<_Key,_mapnode<_Key,_Value> >::iterator add(_Key key, _Value value)
	{
		_hashcont<_Key,_mapnode<_Key,_Value> >::iterator it = _hashcont<_Key,_mapnode<_Key,_Value> >::add(key);
		it->value = value;
		return it;
	}
	virtual _hashmap<_Key,_Value>& operator=(const _hashmap<_Key,_Value>& hs) {
		for (_hashmap<_Key,_Value>::iterator it=hs.first(); it; it=it->next_walk)
			add(it->key, it->value);
		return *this;
	}
};

#endif
