
#pragma warning(disable:4311)

//#include <conio.h>
#include "pgen.h"

int nNextNodeId = 1;

extern int nnext_partition;
bool divide_partitions(_vdfa& vdfa, _vdfa& newdfa, _partition* start, _partition* accept, taccept& oldaccept, taccept& newaccept);

struct State
{
	int id;
	char marked;
	nodeset states;
	transmap trans;
	State* next;
};
typedef _list<State*> _statelist;

typedef _vector<_vector<Node*> > vtran;

void get_move(Node* node, vtran& tm, int* accept_id)
{
	if (node->accept_id != -1 && (node->accept_id < *accept_id || *accept_id == -1))
		*accept_id = node->accept_id;
	for (transmap::iterator it = node->state_trans.first(); it; it=it->next_walk)
		tm[it->key].push_back(it->value);
}

void add_dfa(_vdfa& dfa, Node* nfa, taccept& accept, int& offset)
{
	taccept naccept;
	_vdfa ndfa;
	offset = 0;

	// find highest referenced state in dfa
	offset = dfa.size();

	// build dfa from nfa
	build_dfa(nfa, ndfa, naccept);

	// increase size of existing dfa to accomodate new states
	dfa.reserve(dfa.size() + ndfa.size() + offset);

	// used for adding new states to dfa
	_vector<unsigned short> newstate;
	newstate.resize(256);

	// combine new dfa with existing dfa
	for (int state=0;state<ndfa.size(); state++)
	{
		// add each row of new dfa to existing, adding offset to target states
		int targ_state = state + offset;
		dfa.push_back(newstate);
		int sz = dfa.size();
		for (int i=0;i<256;i++)
		{
			int targ = ndfa[state][i];
			dfa[targ_state][i] = targ?targ+offset:0;
		}
	}

	// combine new accept states with existing accept states
	for (taccept::iterator it = naccept.first(); it; it=it->next_walk)
	{
		int state = it->key;
		int acc_id = it->value;
		accept[state + offset] = it->value;
	}
}

//
// pre-calculate transitions for nodes
//
void calc_moves(Node* node, transmap& tm, int visited_id, int* accept_id)
{
/*	if (!(ccur++ % 11))
	{
		printf("%c", 8);
		printf("%c", ctable[ccur % 4]);
	}
*/
	if (!node)
		return;

	if (node->type == ACCEPT)
	{
		if (*accept_id == -1 || node->accept_id < *accept_id)
			*accept_id = node->accept_id;
	}

	if (node->visited == visited_id)
		return;
	node->visited = visited_id;
	for (transmap::iterator it=node->transition.first(); it; it=it->next_walk)
	{
		unsigned short value = it->key;
		Node* n = it->value;

		if (value != E) {
			tm.add(value, n);
		} else {
			calc_moves(n, tm, visited_id, accept_id);
		}
	}
}

struct statelist {
	State* first;
	State* last;
	statelist() {first=last=0;}
	void add(State* s)
	{
		s->next=0;
		if (!first) {
			first=s;
			last=first;
		} else {
			last->next=s;last=s;
		}
	}
	int find(nodeset& s)
	{
		for (nodeset::iterator it=s.first(); it; it=it->next_walk)
		{
			Node* n = it->key;
			for (_vector<State*>::iterator sit = n->states.begin(); sit != n->states.end(); sit++)
			{
				State* st = *sit;
				if (st->states.compare(s))
					return st->id;
			}
		}
		return -1;
	}
};

//
// clear node visited flags
//
void clear_visited(Node* node)
{
	if (node->visited == -1) return;
	node->visited = -1;
	for (transmap::iterator it=node->transition.first(); it; it=it->next_walk)
		clear_visited(it->value);
	node->visited = 0;
}

 double time1 = 0;
 double time2 = 0;

 void get_all_states(Node* node, _vector<Node*>& nodes)
{
	if (node->visited == 2)
		return;
	node->visited = 2;
	if (nodes.size() <= node->id)
	{
		if (nodes.capacity() <= node->id)
			nodes.reserve(node->id+100);
		nodes.resize(node->id+1);
	}
	nodes[node->id] = node;
	for (transmap::iterator it=node->transition.first(); it; it=it->next_walk)
	{
		unsigned short c = it->key;
		Node* n = it->value;
		if (!n)	continue;
		if (c != E)	n->important = true;
		get_all_states(n, nodes);
	}
}

void _build_dfa(Node* nfa, _vdfa& dfa, taccept& accept)
{
	int next_dfa_state = 0;
	_statelist marked;
	_statelist unmarked;
	statelist state_list;
	int state_id;
	int dfa_size;

	_vector<unsigned short> newstate;
	newstate.resize(256);

	// reserve 1000 states
	dfa.reserve(1000);

	// add start state
	State* st = new State;
	st->marked = 0;
	st->id = next_dfa_state++;
	st->states.add(nfa);
	unmarked.push_back(st);
	state_list.add(st);

	// get vector of all states
	_vector<Node*> all_states;
	all_states.resize(nfa->id+1);
	nfa->important = true;  // node 0 is always important
	get_all_states(nfa, all_states);

	// get transitions for all important states
	int visited_id = 4;
	for (_vector<Node*>::iterator it = all_states.begin(); it != all_states.end(); it++)
	{
		Node* n = *it;
		if (n && n->important)
			calc_moves(n, n->state_trans, visited_id++, &n->accept_id);
	}

	for (;unmarked.size();)
	{
		State* s = unmarked.front();
		unmarked.pop_front();
		s->marked = 1;
		marked.push_back(s);

		// current state id
		state_id = s->id;

		// get transitions
		vtran tm;
		tm.resize(256);
		int accept_id = -1;
		for (nodeset::iterator sit = s->states.first(); sit; sit=sit->next_walk)
			get_move(sit->key, tm, &accept_id);

		// check if this state is a final(accepting) state
		if (accept_id != -1)
		{
			// see if this state is already marked as an accepting state
			taccept::iterator it = accept.find(state_id);
			if (it)
			{
				// use lowest accepting id
				if (accept_id < it->value)
					accept[state_id] = accept_id;
			} else
				accept[state_id] = accept_id;

			// increase dfa table to include this state, in case this
			// state has no exit transitions
			if(dfa.size() <= state_id)
			{
				if (dfa.capacity() <= state_id)
					dfa.reserve(state_id+1000);
				int cap = dfa.capacity();
				while(dfa.size() <= state_id)
					dfa.push_back(newstate);
				dfa_size = (int)dfa.size();
			}
		}

		// iterate over each transition symbol
		for (int tran=0;tran<256;tran++)
		{
			if (!tm[tran].size())
				continue;

			nodeset strans;
			Node** l = &tm[tran][0];
			int sz = tm[tran].size();
			for (int i=0;i<sz;i++,l++)
				strans.add(*l);

			int dfastate = state_list.find(strans);
			if (dfastate == -1)
			{
				// add new state
				State* nst = new State;
				nst->marked = 0;
				nst->id = dfastate = next_dfa_state++;
				nst->states = strans;
				for(nodeset::iterator asdf=nst->states.first(); asdf; asdf=asdf->next_walk)
					asdf->key->states.push_back(nst);
				unmarked.push_back(nst);
				state_list.add(nst);
			}

			// add states to dfa vector if necessary
			if(dfa.size() <= state_id)
			{
				if (dfa.capacity() <= state_id)
					dfa.reserve(state_id+1000);
				int cap = dfa.capacity();
				while(dfa.size() <= state_id)
					dfa.push_back(newstate);
				dfa_size = (int)dfa.size();
			}

			// add transition to state
			dfa[state_id][tran] = dfastate;
		}
	}

	// free nfa graph
	for (_vector<Node*>::iterator vit = all_states.begin(); vit != all_states.end(); vit++)
		delete *vit;
}

void build_dfa(Node* nfa, _vdfa& dfa, taccept& accept)
{
	// build full dfa
	_vdfa vdfa;
	taccept fullaccept;
	_build_dfa(nfa, vdfa, fullaccept);

	// get number of states
	int nstates = vdfa.size();
	if (!nstates)
		return;

	// setup initial partitions
	nnext_partition = 0;
	_partition* vaccept = new _partition;
	_partition* vnonaccept = new _partition;
	vnonaccept->id = nnext_partition++;
	vnonaccept->modified = false;
	vaccept->id = nnext_partition++;
	for (int state=0;state<vdfa.size();state++)
	{
		if ((state == 0) || (!fullaccept.find(state)))
			vnonaccept->states.push_back(state);
		else
			vaccept->states.push_back(state);
	}

	// divide partition
	int nnonaccept = (int)vnonaccept->states.size();
	int naccept = (int)vaccept->states.size();
	divide_partitions(vdfa, dfa, vnonaccept, vaccept, fullaccept, accept);
}
