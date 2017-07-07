
#include "pgen.h"

Node* charset_to_nfa(charset* cs)
{
	Node* nfa=0;
	for (int i=0;i<256;i++)
	{
		if (IsSet(cs->set,i))
			nfa = nfa?alternate_nodes(nfa,create_node(i)):create_node(i);
	}
	return nfa;
}

Node* strliteral_to_nfa(const char* strliteral)
{
	Node* nfa=0;
	strliteral++;
	while(*strliteral != '\"')
	{
		Node* nn;
		char c = chval((char**)&strliteral);
		if (opt.bnon_case_sensitive && c >= 'a' && c <= 'z') {
			Node* nnl = create_node(c);
			Node* nnu = create_node(c-('a'-'A'));
			nn = alternate_nodes(nnl,nnu);
		} else
		if (opt.bnon_case_sensitive && c >= 'A' && c <= 'Z') {
			Node* nnl = create_node(c+('a'-'A'));
			Node* nnu = create_node(c);
			nn = alternate_nodes(nnl,nnu);
		} else {
			nn = create_node(c);
		}
		nfa = nfa?concat_nodes(nfa,nn):nn;
	}
	return nfa;
}

Node* charliteral_to_nfa(const char* charliteral)
{
	Node* nfa=0;
	charliteral++;
	while(*charliteral != '\'')
	{
		char c = chval((char**)&charliteral);
		Node* nn = create_node(c);
		nfa = nfa?concat_nodes(nfa,nn):nn;
	}
	return nfa;
}

Node* create_node(unsigned short c)
{
	Node* n = new Node;
	Node* a = new Node;
	a->type = ACCEPT;
	n->accept = a;
	n->addtrans(c, a);
	return n;
}

Node* combine_branches(Node* n1, Node* n2)
{
	Node* n = new Node;
	n->addtrans(E, n1);
	n->addtrans(E, n2);
	n->type = ALTERNATE;
	return n;
}

Node* concat_nodes(Node* n1, Node* n2)
{
	Node* n = new Node;
	Node* a = new Node;
	n->addtrans(E, n1);
	a->type = ACCEPT;
	n1->accept->addtrans(E, n2);
	n2->accept->addtrans(E, a);
	n1->accept->type = SIMPLE;
	n2->accept->type = SIMPLE;
	n1->accept = 0;
	n2->accept = 0;
	n->accept = a;
	n->type = CONCAT;
	return n;
}

Node* alternate_nodes(Node* n1, Node* n2)
{
	Node* n = new Node;
	Node* a = new Node;
	a->type = ACCEPT;
	n->addtrans(E, n1);
	n->addtrans(E, n2);
	n1->accept->addtrans(E, a);
	n2->accept->addtrans(E, a);
	n1->accept->type = SIMPLE;
	n2->accept->type = SIMPLE;
	n1->accept = 0;
	n2->accept = 0;
	n->accept = a;
	n->type = ALTERNATE;
	return n;
}

Node* kleene_close_node(Node* n1)
{
	Node* n = new Node;
	Node* a = new Node;
	a->type = ACCEPT;
	n->accept = a;
	n->addtrans(E, a);
	a->addtrans(E, n1);
	n1->accept->addtrans(E, a);
	n1->accept->type = SIMPLE;
	n->type = KLEENE;
	return n;
}

Node* copy_node(Node* n1)
{
	Node* n;
	switch(n1->type) {
	case SIMPLE:
		n = create_node(n1->transition.first()->key);
		break;
	case CONCAT:
		{
		Node *f = n1->transition.first()->value;
		Node *m = f->transition.first()->value;
		n = concat_nodes(copy_node(f), copy_node(m->transition.first()->value));
		}
		break;
	case ALTERNATE:
		{
		transmap::iterator tmi = n1->transition.first();
		Node* left = tmi->value;
		tmi = tmi->next_walk;
		Node* right = tmi->value;
		n = alternate_nodes(copy_node(left), copy_node(right));
		}
		break;
	case KLEENE:
		n = kleene_close_node(copy_node(n1->transition.first()->value));
		break;
/*	case ACCEPT:
		n = new Node;
		*n = *n1;
		break;
*/	}
	return n;
}

Node* pos_close_node(Node* n1)
{
	Node* n = kleene_close_node(copy_node(n1));
	return concat_nodes(n1, n);
}

Node* opt_close_node(Node* n1)
{
	return alternate_nodes(n1, create_node(E));
}
