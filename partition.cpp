
#pragma warning(disable:4311)

//#include <conio.h>
#include "pgen.h"
#include <stdio.h>

typedef _list<_partition*> partlist;
int nnext_partition = 0;

// check partition for states with different accepting id's and separate them into their own partitions
void partition_accept_states(_partition* paccept, partlist& finished, taccept& oldaccept)
{
	finished.push_back(paccept);
	while(true)
	{
		bool bcontinue = false;
		for(partlist::iterator it=finished.first(); it; it=it->next)
		{
			_partition* cur_partition = it->val;
			cur_partition->modified = false;
			int test_acc_id = oldaccept[cur_partition->states.front()];
			_partition* new_partition = new _partition;
			new_partition->id = nnext_partition++;
			for (_listint::iterator it=cur_partition->states.first(); it; it=it->next)
			{
				int state_id = it->val;
				int acc_id = oldaccept[state_id];
				if (acc_id != test_acc_id)
				{
					cur_partition->modified = true;
					new_partition->states.push_back(state_id);
					it=cur_partition->states.erase(it);
				}
			}
			if (!cur_partition->modified)
			{
				delete new_partition;
				nnext_partition--;
			} else
			{
				bcontinue = true;
				finished.push_back(new_partition);
			}
		}
		if (!bcontinue)
			break;
	}
}

bool divide_partitions(_vdfa& vdfa, _vdfa& newdfa, _partition* start, _partition* accept, taccept& oldaccept, taccept& newaccept)
{
	partlist working;

	// break apart accept states based on their accepting id
	partition_accept_states(accept,working,oldaccept);

	// add initial partitions to working set
	working.push_back(start);

	// associate states with their partition set
	_vector<int> state_membership;
	state_membership.resize(vdfa.size()+1);
	for (int i=0;i<(int)vdfa.size();i++)
		state_membership[i] = -1;
	for (partlist::iterator it=working.first(); it; it=it->next)
	{
		_partition* p=it->val;
		for (_listint::iterator it=p->states.first(); it; it=it->next)
		{
			int state_id = it->val;
			state_membership[state_id] = p->id;
		}
	}

	while(true)
	{
		// divide partitions
		bool bcontinue = false;
		for(partlist::iterator wit=working.first(); wit; wit=wit->next)
		{
			int nout = 0;
			_partition* cur_partition = wit->val;
			cur_partition->modified = false;

			// skip partitions with no states
			if (cur_partition->states.size() == 0)
				continue;

			// check this partition
			for (int trans=0;trans<=255;trans++)
			{
				_partition* new_partition = new _partition;
				new_partition->id = nnext_partition++;

				// id to test against for this pass
				int test_state_id = cur_partition->states.front();
				int test_target = vdfa[test_state_id][trans];
				int test_set_id = state_membership[test_target];

				for (_listint::iterator it=cur_partition->states.first(); it; it=it->next)
				{
					int state_id = it->val;
					int target = vdfa[state_id][trans];
					int set_id = state_membership[target];

					// if one(and only one) of the transitions is not set, or transitions don't match,
					// move target state into new partition
					if ( ((!test_target || !target) && (test_target || target)) || (set_id != test_set_id) )
					{
						// state has trans to different set than first state in this set
						new_partition->states.push_back(state_id);
						it=cur_partition->states.erase(it);
						cur_partition->modified = true;
						bcontinue = true;
					}
				}

				if (new_partition->states.size())
				{
					for (_listint::iterator it=new_partition->states.first(); it; it=it->next)
					{
						int state_id = it->val;
						state_membership[state_id] = new_partition->id;
					}
					working.push_back(new_partition);
				} else
				{
					delete new_partition;
					nnext_partition--;
				}
			}
		}
		if (!bcontinue)
			break;
	}

	// build new dfa
	newdfa.resize(working.size());
	int state=0;

	for (partlist::iterator it=working.first(); it; it=it->next, state++)
	{
		_partition* p = it->val;
		if (p->states.size())
		{
			int representative = p->states.front();
			newdfa[p->id] = vdfa[representative];
			_vector<unsigned short>& v = newdfa[p->id];
			for (int i=0;i<256;i++)
			{
				unsigned short curstate = v[i];
				if (curstate)
				{
					int nnewstate = state_membership[curstate];
					unsigned short unewstate = nnewstate;
					v[i] = unewstate;
				}
			}
		}

		// free partition
		delete p;
	}

	// reassign accept states
	for (taccept::iterator it = oldaccept.first(); it; it=it->next_walk)
		newaccept[state_membership[it->key]] = it->value;

	return true;
}
