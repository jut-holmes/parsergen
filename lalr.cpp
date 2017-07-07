
#include "pgen.h"
#include "containers.h"
//#include <conio.h>

extern bool bverbose;
int nrules = 0;

#define SHOW_PROGRESS if (!(ccur++ % 10)) {printf("%c",8);printf("%c", ctable[ccur++ % 4]);}

//
// print a configuration
//
void print_config(parser* p, config* cfg)
{
	printf("  %s ::= ", cfg->r->left->name);
	int idx = 0;
	for (_vector<symbol*>::iterator rit = cfg->r->right.begin(); rit != cfg->r->right.end(); rit++)
	{
		symbol* s = *rit;
		if (idx == cfg->dot)
			printf(" . ");
		printf(" %s ", s->name);
		idx++;
	}
	if (idx == cfg->dot)
		printf(" . ");
	for (int i=0;i<cfg->lookahead.size();i++)
	{
		if (cfg->lookahead[i])
			printf(", %s", i==_EOF_?"$":p->symbols[i]->name);
	}
	printf("\n");
}

void print_item_set_0(parser* p, tconfig* tcfg)
{
	for (tconfig::iterator cfgit = tcfg->first(); cfgit; cfgit=cfgit->next_walk)
	{
		config* cfg = cfgit->key;
		print_config(p, cfg);
	}
}

// merge set2 into set1
bool merge_sets(tset& set1, tset& set2)
{
	bool bchanged = false;
	for (tset::iterator it = set2.first(); it; it=it->next_walk)
	{
		if (!set1.find(it->key))
		{
			bchanged = true;
			set1.add(it->key);
		}
	}
	return bchanged;
}

// Mark nonterminal rules that can derive the empty set
void mark_empty(parser* pp)
{
	while(true)
	{
		int done = true;

		for (_vector<rule*>::iterator it = pp->rules.begin(); it != pp->rules.end();it++)
		{
			// get pointer to production and rule
			for (rule* r = *it; r; r=r->next_inst)
			{
				_vector<symbol*>::iterator lit;
				
				// if production's left hand side rule is already marked, skip this production
				if (r->left->hasE)
					break;

				// iterate over productions's right hand side rules, looking for rules that can derive E
				for (lit = r->right.begin(); lit != r->right.end(); lit++)
				{
					symbol* _s = *lit;
					if (!_s->hasE)
						break;
				}
				if (lit == r->right.end())
				{
					r->left->hasE = true;
					done = false;
					break;
				}
			}
		}

		// if nothing changed during the last iteration, we are done
		if (done)
			return;
	}
}

void set_terminal_first_sets(parser* p)
{
	for (_vector<symbol*>::iterator it = p->symbols.begin(); it != p->symbols.end(); it++)
	{
		symbol* s = *it;
		if (s && s->type == TERM)
			s->first.add(s->id);
	}
}

// build first sets for all rules
void find_first_sets(parser* p)
{
	while(true)
	{
		int done = true;
		for (_vector<rule*>::iterator it = p->rules.begin(); it != p->rules.end();it++)
		{
			// get pointer to rule and symbol
			for (rule* r = *it; r; r=r->next_inst)
			{
				// walk over the list of rules that appear on the right side of the production
				for (_vector<symbol*>::iterator lit = r->right.begin(); lit != r->right.end(); lit++)
				{
					symbol* _s = *lit;
					if (_s->id != r->left->id)
					{
						// add rhs's first set to the lhs rule's first set
						if (merge_sets(r->left->first, _s->first))
							done = false;
					}

					// if rhs symbol doesn't derive E, don't continue past it
					if (_s->type == TERM || !_s->hasE)
						break;
				}
			}
		}

		// if nothing changed during the last iteration, we are done
		if (done)
			return;
	}
}

//
// find an existing configuration in an item set
//
config* find_config(tconfig& configs, rule* r, int dot)
{
	for (tconfig::iterator it = configs.first(); it; it=it->next_walk)
	{
		config* cfg = it->key;
		if (cfg->r == r && cfg->dot == dot)
			return cfg;
	}
	return 0;
}

config* add_config(tconfig& configs, rule* r, int dot, bool* badded)
{
	config* cfg = find_config(configs, r, dot);
	if (!cfg)
	{
		cfg = new config;
		cfg->r = r;
		cfg->dot = dot;
		configs.add(cfg);
		cfg->lookahead.resize(nrules);
		*badded = true;
	} else
		*badded = false;
	return cfg;
}

bool merge_vectors(tlookahead& s1, tlookahead& s2)
{
	bool bchanged = false;
	int sz1 = s1.size();
	int sz2 = s2.size();
	for (int i=0; i<sz1 && i<sz2; i++)
	{
		if (!s2[i]) continue;
		if (!s1[i])
		{
			bchanged = true;
			s1[i] = 1;
		}
	}
	return bchanged;
}

bool merge_set_to_vector(tlookahead& s1, tset& s2)
{
	bool bchanged = false;
	for (tset::iterator it = s2.first(); it; it=it->next_walk)
	{
		int id = it->key;
		if (!s1[id])
			bchanged = true;
		s1[id] = 1;
	}
	return bchanged;
}

// compute the closure of a set of items
void compute_closure_0(parser* p, tconfig& items)
{
	while(true)
	{
		bool bdone = true;

		//
		// step over every configuration, adding states for production rules
		//
		for (tconfig::iterator it = items.first(); it; it=it->next_walk)
		{
			int i;
			config* cfg = it->key;
			rule* r = cfg->r;
			int dot = cfg->dot;
			tlookahead& lookahead = cfg->lookahead;

			// check if dot is at end of production
			if (dot >= r->right.size())
				continue;

			// get pointer to symbol
			symbol* s = r->right[dot];

			// there is no symbol to the right of the dot for this production
			if (!s || s->type == TERM)
				continue;

			// create lookahead vector
			tlookahead follow;
			follow.resize(p->symbols.size());

			// add follow set symbols from current config production starting at symbol to right of dot
			for (i=dot+1;i<r->right.size();i++)
			{
				symbol* _s = r->right[i];

				// merge follow sets
				merge_set_to_vector(follow, _s->first);

				// if symbol is a terminal or doesn't derive E then stop
				if (_s->type==TERM || !_s->hasE)
					break;
			}

			// add a propagation link if we walked to the end of the production
			bool baddlink = (i==r->right.size()) ? true : false;

			// get productions which correspond to symbol
			for (rule* nr=p->rules[s->id]; nr; nr=nr->next_inst)
			{
//				printf("  Production: ");
//				display_production(np, rules);

				bool badded;
				config* ncfg = add_config(items, nr, 0, &badded);

				// add lookaheads to existing configuration
				merge_vectors(ncfg->lookahead, follow);

				if (badded)
					bdone = false;

				if (baddlink)
					cfg->propagation_links.push_back(ncfg);
			}
		}
		if (bdone)
			return;
	}
}

//
// compare two sets of items
//
bool compare_sets(tconfig& set1, tconfig& set2)
{
	if (set1.size() != set2.size())
		return false;
	for (tconfig::iterator sit1 = set1.first(); sit1; sit1=sit1->next_walk)
	{
		bool bmatch = false;
		for (tconfig::iterator sit2 = set2.first(); !bmatch && sit2; sit2=sit2->next_walk)
		{
			config* c1 = sit1->key;
			config* c2 = sit2->key;
			if (c1->r == c2->r && c1->dot == c2->dot)
				bmatch = true;
		}
		if (!bmatch)
			return false;
	}
	return true;
}

// see if a set of items already exists in C
int set_exists(mtconfig& C, tconfig& Cnew)
{
	// iterate over each state
	for (mtconfig::iterator it = C.first(); it; it=it->next_walk)
	{
		tconfig* cfg = it->value;
		// see if item sets match
		if (compare_sets(*cfg, Cnew))
			return it->key;
	}
	return -1;
}

tconfig* compute_goto_0(parser* p, tconfig& Si, symbol* sym)
{
	tconfig* configs = new tconfig;
	
	// iterate over each item in set Si
	for (tconfig::iterator it = Si.first(); it; it=it->next_walk)
	{
		config* cfg = it->key;
		rule* r = cfg->r;
		int dot = cfg->dot;

		// can't shift this one
		if (dot >= r->right.size())
			continue;

		// get symbol to right of dot in production referenced by item
		symbol* s = r->right[dot];

		// symbol not the same as s
		if (sym != s)
			continue;

		// add a new configuration
		bool badded;
		config* newcfg = add_config(*configs, cfg->r, cfg->dot+1, &badded);
		newcfg->propagation_links_rev.push_back(cfg);
	}
	if (!configs->size())
		return configs;
	compute_closure_0(p, *configs);
	return configs;
}

symbol* get_rule_prec(parser* p, int rule_lhs_id, int rule_inst)
{
	for (rule* r=p->rules[rule_lhs_id]; r; r=r->next_inst)
	{
		if (r->inst == rule_inst)
			return r->prec;
	}
	return 0;
}

action* resolve_conflict(parser* p, action* shift_action, action* reduce_action, bool& bclear)
{
	symbol* shift = p->symbols[shift_action->shift_symbol];
	symbol* reduce = get_rule_prec(p, reduce_action->val, reduce_action->reduce_prod);
	if (!shift || !reduce || shift->prec == -1 || shift->assoc == ASSOC_UNK || reduce->prec == -1 || reduce->assoc == ASSOC_UNK)
		return 0;
	if (shift->prec > reduce->prec)
		return shift_action;
	if (reduce->prec > shift->prec)
		return reduce_action;

	// same precedence, compare rule and token
	if (shift->assoc == ASSOC_LEFT)
		return reduce_action;
	if (shift->assoc == ASSOC_RIGHT)
		return shift_action;

	// token is non-associative(ASSOC_NON)
	bclear = true;
	return 0;
}

void print_action(parser* p, const char* name, action* a)
{
	printf("%s\n-----\n",name);
	printf("type: %d\n", a->type);
	printf("val: %d\n", a->val);
	printf("reduce_prod: %d\n", a->reduce_prod);
	printf("reduce_cnt: %d\n", a->reduce_cnt);
	printf("shift_symbol: %d\n", a->type);
	printf("\np->rules[%s->reduce_prod] = 0x%08x\n", name, p->rules[a->reduce_prod]);
}

void build_actions(parser* p)
{
	//
	// build action table
	//
	if (opt.bverbose)
		printf("Build Actions...\n");

	// iterate over all states
	for (mtconfig::iterator sit = p->configs.first(); sit; sit=sit->next_walk)
	{
		int stateId = sit->key;
		tconfig* set = sit->value;

		// iterate over each item in state
		for (tconfig::iterator it = set->first(); it; it=it->next_walk)
		{
			config* cfg = it->key;
			rule* r = cfg->r;

			// if dot is at right end
			if (cfg->dot == (int)r->right.size())
			{
				int look_size = cfg->lookahead.size();
				for (int lookahead=0;lookahead<look_size;lookahead++)
				{
					if (!cfg->lookahead[lookahead])
						continue;

					// if we are at a [S' = S, $] item, then accept
					if (r->left->id == 1 && lookahead == _EOF_)
					{
						// accept
						action* a = new action;
						a->type = action::_ACCEPT;
						a->val = r->inst;
						a->shift_symbol = 0;
						a->reduce_cnt = 1;
						a->reduce_prod = 0;
						action* oa = p->lractions[stateId][_EOF_];
						if (oa)
							printf("warning: accept/reduce conflict, accepting\n");
						p->lractions[stateId][_EOF_] = a;
					} else
					{
						// reduce
						action* a = new action;
						a->type = action::_REDUCE;
						a->reduce_prod = r->inst;
						a->val = r->left->id;
						a->reduce_cnt = (int)r->right.size();
						a->shift_symbol = 0;
						action* oa = p->lractions[stateId][lookahead];
						if (oa)
						{
							switch(oa->type)
							{
							case action::_REDUCE:
//								if (a->reduce_prod == oa->reduce_prod)
//									break;

								printf("warning: reduce/reduce conflict on state %d\n  lookahead \"%s\"(%d)\n  productions \"%s\"(%d) and \"%s\"(%d)\n",
									stateId, p->symbols[lookahead]->name, lookahead, p->symbols[a->val]->name, a->val, p->symbols[oa->val]->name, oa->val);
								if (a->reduce_prod < oa->reduce_prod)
								{
									if (p->rules[a->reduce_prod])
										printf("  taking \"%s\" as reduction\n", p->rules[a->reduce_prod]->name);
									else
										printf("  taking %d as reduction\n", a->reduce_prod);
									p->lractions[stateId][lookahead] = a;
								} else
								{
									if (p->rules[oa->reduce_prod])
										printf("  taking \"%s\" as reduction\n", p->rules[oa->reduce_prod]->name);
									else
										printf("  taking production %d as reduction\n", oa->reduce_prod);
									p->lractions[stateId][lookahead] = oa;
								}
								break;
							case action::_SHIFT:
								{
								bool bclear = false;
								action* resolved_action = resolve_conflict(p, oa, a, bclear);
								if (resolved_action == NULL)
								{
									printf("warning: shift/reduce conflict on state %d\n  lookahead \"%s\"(%d)\n  production \"%s\"(%d), symbol \"%s\"(%d)\n",
										stateId, p->symbols[lookahead]->name, lookahead, r->name, r->inst, p->symbols[oa->shift_symbol]->name, oa->shift_symbol);
									printf(" shifting \"%s\"\n", p->symbols[oa->shift_symbol]->name);
									delete a;
								} else
								{
									if (bclear)
									{
										delete a;
										delete oa;
										p->lractions[stateId].erase(lookahead);
									}
									else
									{
										p->lractions[stateId][lookahead] = resolved_action;
										if (resolved_action == oa)
											delete a;
										else
											delete oa;
									}
								}
								}
								break;
							}
						} else
							p->lractions[stateId][lookahead] = a;
					}
				}
				continue;
			}

			//
			// shift
			//

			symbol* s = r->right[cfg->dot];
			if (s->type == NONTERM)
				continue;

			// compute goto of [Ii, r]
			tconfig* target = compute_goto_0(p, *set, s); // Si, set, symbol
			if (target->size() != 0)
			{
				// see which state the item set/symbol resolves to
				int st = set_exists(p->configs, *target);
				if (st != -1)
				{
					action* a = new action;
					a->type = action::_SHIFT;
					a->val = st;
					a->shift_symbol = s->id;
					action* oa = p->lractions[stateId][s->id];
					if (oa)
					{
						switch(oa->type)
						{
						case action::_REDUCE:
							{
							bool bclear = false;
							action* resolved_action = resolve_conflict(p, a, oa, bclear);
							if (resolved_action == NULL)
							{
								printf("warning: shift/reduce conflict on state %d, symbol \"%s\"(%d)\n  production \"%s\"(%d)\n",
									stateId, p->symbols[s->id]->name, s->id, rule_find_inst(p->rules[oa->val], oa->reduce_prod)->name, oa->reduce_prod);
								printf(" shifting \"%s\"\n", p->symbols[s->id]->name);
								delete oa;
								p->lractions[stateId][s->id] = a;
							} else
							{
								if (bclear)
								{
									p->lractions[stateId].erase(s->id);
									delete a;
									delete oa;
								}
								else
								{
									p->lractions[stateId][s->id] = resolved_action;
									if (resolved_action == oa)
										delete a;
									else
										delete oa;
								}
							}
							}
							break;
						}
					} else
						p->lractions[stateId][s->id] = a;
				}
			}
			for (tconfig::iterator it = target->first(); it; it=it->next_walk)
				delete it->key;
			delete target;
		}
	}

	//
	// compute goto actions
	//

	// iterate over all states
	for (mtconfig::iterator sit = p->configs.first(); sit; sit=sit->next_walk)
	{
		int stateId = sit->key;
		tconfig* set = sit->value;

		for (_vector<symbol*>::iterator sit = p->symbols.begin(); sit != p->symbols.end(); sit++)
		{
			// get symbol pointer
			symbol* s = *sit;

			// goto's only happen on non-terminals
			if (s->type == TERM)
				continue;

			// get target item set
			tconfig* goto_target = compute_goto_0(p, *set, s); // Si, set, symbol

			if (goto_target->size() != 0)
			{
				// see which state the item set/symbol resolves to
				int st = set_exists(p->configs, *goto_target);
				if (st != -1)
				{
					action* g = new action;
					g->type = action::_GOTO;
					g->val = st;
					p->lractions[stateId][s->id] = g;
				}
			}
			
			for (tconfig::iterator it = goto_target->first(); it; it=it->next_walk)
				delete it->key;
			delete goto_target;
		}
	}
}

//
// return a set of all symbols that appear to the right of the dot
// for all the configurations in this set
//
void get_shift_symbols(tconfig* items, _hashset<symbol*>& shift_symbols)
{
	for (tconfig::iterator it = items->first(); it; it=it->next_walk)
	{
		config* cfg = it->key;
		rule* r = cfg->r;
		
		// can't shift this config
		if (cfg->dot >= r->right.size())
			continue;

		// insert rule to right of dot
		shift_symbols.add(r->right[cfg->dot]);
	}
}

void find_lr0_states(parser* p)
{
	// Mark rules that can derive the empty set
	mark_empty(p);

	// Find first sets
	set_terminal_first_sets(p);
	find_first_sets(p);

	// add start states to configuration sets
	tconfig* state0 = new tconfig;
	int next_state = 0;
	p->configs.add(next_state++, state0);

	// add start state as initial configuration
	bool badded;
	config* cfg = add_config(*state0, p->rules[1], 0, &badded);
	cfg->lookahead[_EOF_] = 1;

	// compute closure of S' = S, EOF
	compute_closure_0(p, *state0);

	if (opt.bverbose)
		printf("  Find LR0...");

	// iterate over all states
	for (mtconfig::iterator sit = p->configs.first(); sit; sit=sit->next_walk)
	{
		int stateId = sit->key;
		tconfig* Si = sit->value;

		// get all symbols to the right of dot for this configuration set
		_hashset<symbol*> shift_symbols;
		get_shift_symbols(Si, shift_symbols);

		// iterate over each shift symbol
		for (_hashset<symbol*>::iterator sit = shift_symbols.first(); sit; sit=sit->next_walk)
		{
			symbol* r = sit->key;
			tconfig contrib;

			// compute goto for this symbol
			tconfig* Cnew = compute_goto_0(p, *Si, r); // Si, Cnew, symbol
			int st = (Cnew->size()==0)?-2:set_exists(p->configs, *Cnew);
			if (st == -2)
			{
				// do nothing
			} else
			if (st == -1)
			{
				// state doesn't exist, add as new state
				p->configs.add(next_state++, Cnew);
			} else 
			{
				// get existing set
				tconfig* xSi = p->configs[st];

				// get config items in new set
				for (tconfig::iterator it = Cnew->first(); it; it=it->next_walk)
				{
					// new config item
					config* nc = it->key;

					// reverse propagation pointers are pointers from new config to existing config(s)
					for (tconfiglist::iterator clist = nc->propagation_links_rev.first(); clist; clist=clist->next)
					{
						// get pointer to current config
						// this configuration is "contributing" it's lookahead info to item nc
						config* cc = clist->val;

						// get config in existing set xSi that is equal to nc
						config* xc = find_config(*xSi, nc->r, nc->dot);

						// copy lookaheads for item cc to item xc
						merge_vectors(xc->lookahead, cc->lookahead);

						// add propagation links
                        cc->propagation_links.push_back(xc);
					}
					delete it->key;
				}
				delete Cnew;
			}
		}
	}
}

void find_follow_sets(parser* p)
{
	tconfig* cfgset;
	config* cfg;
	config* cfg2;
	mtconfig::iterator cit;
	tconfig::iterator sit;
	tconfiglist::iterator lit;

	if (opt.bverbose)
		printf("Find Follow Sets(Setup)...");

	// reverse backward propagation links
	for (cit = p->configs.first(); cit; cit=cit->next_walk)
	{
		cfgset = cit->value;
		for (sit = cfgset->first(); sit; sit=sit->next_walk)
		{
			cfg = sit->key;
			while(cfg->propagation_links_rev.size())
			{
				cfg2 = cfg->propagation_links_rev.front();
				cfg->propagation_links_rev.pop_front();
				cfg2->propagation_links.push_back(cfg);
			}
			cfg->propagation_links_rev.clear();
		}
	}

	if (opt.bverbose)
		printf("Find Follow Sets...");

	// propagate follow sets until no changes occur during a full iteration
	while(true)
	{
		bool bchanged = false;
		for (cit = p->configs.first(); cit; cit=cit->next_walk)
		{
			cfgset = cit->value;
			for (sit = cfgset->first(); sit; sit=sit->next_walk)
			{
				cfg = sit->key;
				for (lit = cfg->propagation_links.first(); lit; lit=lit->next)
				{
					cfg2 = lit->val;

					// merge the follow sets and detect if the target changed
					if (merge_vectors(cfg2->lookahead, cfg->lookahead))
						bchanged = true;
				}
			}
		}
		if (!bchanged)
			return;
	}
}

void print_configs(parser* p)
{
	printf("Configurations\n");
	printf("--------------\n");
	int state = 0;
	for (mtconfig::iterator it = p->configs.first(); it; it=it->next_walk)
	{
		printf("State %d\n", state++);
		tconfig* tcfg = it->value;
		print_item_set_0(p, tcfg);
	}
}

void build_lalr(parser* p)
{
	nrules = p->symbols.size();

	// find all LR(0) states and create follow set propagation links
	find_lr0_states(p);

	// compute follow sets
	find_follow_sets(p);

	// build action table for set of LR(1) items
	build_actions(p);
}
