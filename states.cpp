
#include "pgen.h"

int state_get(parser* p, const char* name)
{
	int sz = (int)p->states.size();
	for (int i=0; i<sz; i++)
	{
		if (!_stricmp(p->states[i], name))
			return i;
	}
	p->states.push_back(_strdup(name));
	int sd_size = p->state_defined.size();
	p->state_defined.resize(p->states.size());
	if (p->state_defined.size() != sd_size)
		p->state_defined.back() = 0;
	p->state_offsets.push_back(0);
	return sz;
}

int state_get_max(parser* p)
{
	return p->states.size();
}

void state_define(parser* p, const char* name)
{
	int idx = state_get(p, name);
	p->state_defined[idx] = 1;
}

const char* state_name(parser* p, int idx)
{
	return p->states[idx];
}

int state_set_offset(parser* p, const char* name, int offset)
{
	int sz = (int)p->states.size();
	for (int i=0; i<sz; i++)
	{
		if (!_stricmp(p->states[i], name)) {
			p->state_offsets[i] = offset;
			return 0;
		}
	}
	return -1;
}
