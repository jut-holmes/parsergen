
#include "pgen.h"

int nnext_symbol_id = 0;
int nnext_term_inst = 0;

int	symbol_get_max_inst()
{
	return nnext_term_inst;
}

symbol* symbol_find_name(parser* p, char* name)
{
	int sz = (int)p->symbols.size();
	for (int i=0;i<sz;i++)
	{
		if (!strcmp(p->symbols[i]->name, name))
			return p->symbols[i];
	}
	return 0;
}

symbol* symbol_find_id(parser* p, int id)
{
	if (id<0 || id>=p->symbols.size())
		return 0;
	return p->symbols[id];
}

symbolinst* symbol_find_inst(symbol* sym, int inst)
{
	if (!sym) return 0;
	for (symbolinst* si=sym->inst;si;si=si->next)
		if (si->inst==inst)
			return si;
	return 0;
}

symbolinst* symbol_find_state(symbol* sym, int state)
{
	if (!sym) return 0;
	for (symbolinst* si=sym->inst;si;si=si->next)
		if (si->state==state)
			return si;
	return 0;
}

symbolinst* symbol_add_inst(symbol* sym)
{
	symbolinst* newinst = new symbolinst;
	newinst->bdefined = FALSE;
	newinst->inst = nnext_term_inst++;
	newinst->next = 0;
	newinst->nfa = 0;
	newinst->state = 0;
	newinst->daction.flags = DFA_ACCEPT;
	newinst->daction.goto_state = 0;
	if (!sym->inst)
		sym->inst = newinst;
	else
	{
		symbolinst* si=sym->inst;
		for (;si->next;si=si->next);
		si->next=newinst;
	}
	return newinst;
}

symbolinst* symbol_get_inst(symbol* sym, int state)
{
	symbolinst* si = symbol_find_state(sym, state);
	if (si) return si;
	si = symbol_add_inst(sym);
	si->state = state;
	return si;
}

symbol* symbol_add(parser* p, char* name)
{
	symbol* s = symbol_find_name(p,name);
	if (s)	return NULL;

	symbol* ns = new symbol;
	ns->type = SYMBOL_TYPE(name);
	ns->name = _strdup(name);
	ns->bdefined = FALSE;
	ns->marked = 0;
	ns->hasE = 0;
	ns->inst = 0;
	ns->prec = -1;
	ns->assoc = 0;
	ns->line=ns->column=0;
	ns->id = nnext_symbol_id++;
	p->symbols.push_back(ns);

	return ns;
}

symbol* symbol_get(parser* p, char* name)
{
	symbol* s = symbol_find_name(p, name);
	if  (s) return s;
	return symbol_add(p,name);
}
