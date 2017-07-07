
#include "pgen.h"

int nnext_rule_inst = 0;

rule* rule_add(parser* p, char* lhs_name)
{
	rule* r = new rule;
	r->left = symbol_get(p, lhs_name);
	r->inst = nnext_rule_inst++;
	r->name = 0;
	r->prec = 0;
	r->bdefined = FALSE;
	r->next_inst = 0;

	rule* er = rule_find_id(p, r->left->id);
	if (er)
	{
		for (;er->next_inst;er=er->next_inst);
		er->next_inst = r;
		return r;
	}
	if (p->rules.size() <= r->left->id) {
		int nsz = p->rules.size();
		p->rules.resize(r->left->id+1);
		memset(&p->rules[nsz], 0, sizeof(rule*)*((r->left->id+1)-nsz));
	}
	p->rules[r->left->id] = r;
	return r;
}

rule* rule_find_inst(rule* r, int inst)
{
	for (;r;r=r->next_inst)
		if (r->inst == inst)
			return r;
	return 0;
}

rule* rule_next_inst(rule* r)
{
	return r->next_inst;
}

rule* rule_find_name(parser* p, char* lhs_name)
{
	int sz = (int)p->rules.size();
	for (int i=0;i<sz;i++)
	{
		if (p->rules[i] && !strcmp(p->rules[i]->left->name, lhs_name))
			return p->rules[i];
	}
	return 0;
}

symbol* rule_add_rhs(parser* p, rule* r, char* rhs_name)
{
	symbol* rhs = symbol_get(p, rhs_name);
	r->right.push_back(rhs);
	return rhs;
}

rule* rule_find_id(parser* p, int lhs_id)
{
	int sz = (int)p->rules.size();
	if (lhs_id < 0 || lhs_id >= sz || !p->rules[lhs_id])
		return 0;
	return p->rules[lhs_id];
}

char* rule_make_name(rule* r)
{
	if (!r->name)
	{
		char buf[2048];
		strcpy(buf, r->left->name);
		strcat(buf, " --> ");
		for (_vector<symbol*>::iterator it = r->right.begin(); it != r->right.end(); it++)
		{
			symbol* s = *it;
			strcat(buf, s->name);
			strcat(buf, " ");
		}
		r->name = _strdup(buf);
	}
	return r->name;
}
