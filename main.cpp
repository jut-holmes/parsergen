
//#include <conio.h>
#include <time.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "pgen.h"
#include "version.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "bpecomp.h"
#include "regex.h"

// This is for timing the dfa/lalr builds
#ifdef WIN32
#include <windows.h>
#else
#include <sys/time.h>
#define _vsnprintf vsnprintf
#define _stat stat
#define _fstat fstat
#endif

#define MAX_STREAM_SIZE (1024*1024*3)
options opt;

struct STRM
{
	unsigned char* data;
	unsigned int ofs;
};
void swrite(const void* data, unsigned int size, unsigned int cnt, STRM* strm)
{
	memcpy(strm->data+strm->ofs,data,size*cnt);
	strm->ofs += size*cnt;
}
bool config_to_bytes(parse_config* config, STRM* strm, bool bsave_dfa, bool bsave_lalr, bool bsave_symbols, bool bsave_states);

#ifndef WIN32

//
// duplicate a string and convert to lowercase
//
char* strlwr(char* s)
{
	char* res = strdup(s);
	for (char* p=res;*p;p++)
		if (isupper(*p)) *p = tolower(*p);
	return res;
}

//
// string compare, not case sensitive
//
int stricmp(char* s1, char* s2)
{
	int result;
	char* s1l = strlwr(s1);
	char* s2l = strlwr(s2);
	result = strcmp(s1l,s2l);
	free(s1l);
	free(s2l);
	return result;
}
#endif

char escval(char c)
{
	switch(c)
	{
	case 'n': return '\n';
	case 'r': return '\r';
	case 't': return '\t';
	case 'b': return '\b';
	default: return c;
	}
}
int hexval(const char* c)
{
	int v;
	sscanf(c,"%x",&v);
	return v;
}
int chval(char** c)
{
	int val=0;
	char* p = *c;
	if (*p == '\\') {
		if (p[1] == 'x')
		{
			p+=2;
			val=hexval(p);
			while(isxdigit(*p)) p++;
		}
		else
		if (p[1] == 'u')
		{
			p+=2;
			val= atoi(p);
			while(isdigit(*p)) p++;
		}
		else
		{
			p++;
			val=escval(*p);
			p++;
		}
	} else
	{
		val = *p;
		p++;
	}
	*c = p;
	return val;
}

const char* chstr(unsigned char c)
{
	static unsigned char p[] = " ";
	static char	chbuf[8];

	switch(c)
	{
	case 0:	return "<NUL>";
	case 1: return "<SOH>";
	case 2: return "<STX>";
	case 3: return "<ETX>";
	case 4: return "<EOT>";
	case 5: return "<ENQ>";
	case 6: return "<ACK>";
	case 7:	return "<BEL>";
	case 8: return "<BS>";
	case 9: return "<TAB>";
	case 10:	return "<LF>";
	case 11:	return "<VT>";
	case 12:	return "<FF>";
	case 13:	return "<CR>";
	case 14:	return "<SO>";
	case 15:	return "<SI>";
	case 16:	return "<DLE>";
	case 17:	return "<DC1>";
	case 18:	return "<DC2>";
	case 19:	return "<DC3>";
	case 20:	return "<DC4>";
	case 21:	return "<NAK>";
	case 22:	return "<SYN>";
	case 23:	return "<ETB>";
	case 24:	return "<CAN>";
	case 25:	return "<EM>";
	case 26:	return "<SUB>";
	case 27:	return "<ESC>";
	case 28:	return "<FS>";
	case 29:	return "<GS>";
	case 30:	return "<RS>";
	case 31:	return "<US>";
	case 32:	return "<SPACE>";
	case 0xa0:	return "<NBSP>";
	default:
		if (c >= 127) {
			sprintf(chbuf, "%02d", c);
			return chbuf;
		}
		p[0] = c;
		return (char*)p;
	}
}

//
// Write the NFA to a .dot file
//
void _output_nfa(FILE* fout, Node* nfa)
{
	if (nfa->visited == 3) return;
	nfa->visited = 3;
	for (transmap::iterator it=nfa->transition.first(); it; it=it->next_walk) {
		if (it->key != E)
		{
			unsigned char ch[2] = { (char)it->key, 0 };
			unsigned char* p = ch;
			p = (unsigned char*)chstr((unsigned char)p[0]);
			fprintf(fout, "\tN%d -> N%d [ label = \"%s\" ];\n", nfa->id, it->value->id, p);
		}
		else
			fprintf(fout, "\tN%d -> N%d [ label = \"E\" ];\n", nfa->id, it->value->id);
		_output_nfa(fout, it->value);
	}
}

void find_accept_states(Node* node, taccept& accept)
{
	if (node->visited == 2) return;
	node->visited = 2;
	if (node->type == ACCEPT)
		accept[node->id] = node->accept_id;

	for (transmap::iterator it = node->transition.first(); it; it=it->next_walk) {
		find_accept_states(it->value, accept);
	}
}

void output_nfa(char* filename, Node* nfa)
{
	FILE* fout = fopen(filename, "wt");
	if (!fout) return;
	fprintf(fout, "digraph finite_state_machine {\n\trankdir=LR;\n\torientation=land;\n");

	taccept accept;
	find_accept_states(nfa, accept);
//	clear_visited(nfa);

	// output double-circle accept states
	fprintf(fout, "\tnode [shape = doublecircle];\n");
	for (taccept::iterator it = accept.first(); it; it=it->next_walk)
		fprintf(fout, "\tN%d;\n", it->key);

	// output regular states
	fprintf(fout, "\tnode [shape = circle];\n");
	_output_nfa(fout, nfa);

	for (taccept::iterator it = accept.first(); it; it=it->next_walk)
		fprintf(fout, "\tN%d -> A_%d;\n", it->key, it->value);

	fprintf(fout, "}\n");
	fclose(fout);
	clear_visited(nfa);
}

char spaces[256];
void pprintf(int cols, const char* fmt, ...)
{
	char buf[512];
	va_list marker;
	va_start(marker, fmt);
	int cnt = _vsnprintf(buf, 512, fmt, marker);
	va_end(marker);
	printf(buf);
	if (cnt>cols) cnt = cols;
	printf(spaces + (255 - (cols-cnt)));
}

void pfprintf(FILE* fout, int cols, const char* fmt, ...)
{
	char buf[512];
	va_list marker;
	va_start(marker, fmt);
	int cnt = _vsnprintf(buf, 512, fmt, marker);
	va_end(marker);
	fprintf(fout, buf);
	if (cnt>cols) cnt = cols;
	fprintf(fout, spaces + (255 - (cols-cnt)));
}

void pprintfr(int cols, const char* fmt, ...)
{
	char buf[512];
	va_list marker;
	va_start(marker, fmt);
	int cnt = _vsnprintf(buf, 512, fmt, marker);
	va_end(marker);
	if (cnt>cols) cnt = cols;
	printf(spaces + (255 - (cols-cnt)));
	printf(buf);
}

void print_actions(parser* p)
{
	printf("Action Table\n");
	printf("------------\n");
	pprintf(10, "State");
	pprintf(20, "Symbol");
	pprintf(10, "Action");
	printf("\n");
	printf("----------------------------------------\n");
	for (taction::iterator it = p->lractions.first(); it; it=it->next_walk)
	{
		int state = it->key;
		_hashmap<int, action*>& m = it->value;
		pprintfr(5, "%d", state);
		pprintf(5,"");
		for (_hashmap<int, action*>::iterator mit = m.first(); mit; mit=mit->next_walk)
		{
			int symbol = mit->key;
			action* a = mit->value;
			switch(a->type)
			{
			case LR_ACCEPT:
				pprintf(20,"%s(%d)", p->symbols[symbol]->name, symbol);
				printf("Accept \"%s\"(%d)\n", p->rules[a->reduce_prod]->name, a->reduce_prod);
				pprintf(10,"");
				break;
			case LR_SHIFT:
				pprintf(20,"%s(%d)", p->symbols[symbol]->name, symbol);
				printf("Shift \"%s\"(%d), Goto %d\n", p->symbols[symbol]->name, symbol, a->val);
				pprintf(10,"");
				break;
			case LR_REDUCE:
				pprintf(20,"%s(%d)", p->symbols[symbol]->name, symbol);
				printf("Reduce \"%s\"(%d)\n", p->rules[a->reduce_prod]->name, a->reduce_prod);
				pprintf(10,"");
				break;
			case LR_GOTO:
				pprintf(20,"%s(%d)", p->symbols[symbol]->name, symbol);
				printf("Goto %d\n", a->val);
				pprintf(10,"");
				break;
			default:
				break;
			}
		}
		printf("\n");
	}
}

void print_actions(parse_config* cfg)
{
	printf("Action Table\n");
	printf("------------\n");
	pprintf(10, "State");
	pprintf(20, "Symbol");
	pprintf(10, "Action");
	printf("\n");
	printf("----------------------------------------\n");
	for (int state=0;state<cfg->nlraction;state++)
	{
		pprintfr(5, "%d", state);
		pprintf(5,"");
		for (int i=0;i<cfg->nsymbols;i++)
		{
			_lraction* a = &cfg->lraction[state][i];
			switch(a->type)
			{
			case LR_ACCEPT:
				pprintf(20,"%s(%d)", cfg->symbols[i].name, i);
				printf("Accept \"%s\"(%d)\n", cfg->productions[a->val].name, a->val);
				pprintf(10,"");
				break;
			case LR_SHIFT:
				pprintf(20,"%s(%d)", cfg->symbols[i].name, i);
				printf("Shift \"%s\"(%d), Goto %d\n", cfg->symbols[i].name, i, a->val);
				pprintf(10,"");
				break;
			case LR_REDUCE:
				pprintf(20,"%s(%d)", cfg->symbols[i].name, i);
				printf("Reduce \"%s\"(%d)\n", cfg->productions[a->reduce_production].name, a->reduce_production);
				pprintf(10,"");
				break;
			case LR_GOTO:
				pprintf(20,"%s(%d)", cfg->symbols[i].name, i);
				printf("Goto %d\n", a->val);
				pprintf(10,"");
				break;
			default:
				break;
			}
		}
		printf("\n");
	}
}

//
// fixup dfa action gotos after dfa's have been combined
//
void fixup_gotos(parser* p, _vector<int>& dfa_offsets)
{
	int sz=dfa_offsets.size();
	for (_vector<dfaaction*>::iterator it = p->actions.begin(); it != p->actions.end(); it++)
	{
		dfaaction* a = *it;
		if (a->flags & DFA_GOTO || a->flags & DFA_PUSH)
			a->goto_state = dfa_offsets[a->goto_state];
		int asdf=0;
	}
}

void report_error(parser* p, const char* format, ...)
{
	p->nerrors++;
	va_list marker;
	va_start(marker,format);
	printf("%s(%d) : error : ", p->input_file, p->pstate.line);
	vprintf(format, marker);
	va_end(marker);
}

void report_errorl(parser* p, int line, const char* format, ...)
{
	p->nerrors++;
	va_list marker;
	va_start(marker,format);
	printf("%s(%d) : error : ", p->input_file, line);
	vprintf(format, marker);
	va_end(marker);
}

void report_warning(parser* p, const char* format, ...)
{
	p->nwarnings++;
	va_list marker;
	va_start(marker,format);
	printf("%s(%d) : warning : ", p->input_file, p->pstate.line);
	vprintf(format, marker);
	va_end(marker);
}

//
// define currently undefined literal tokens
//
// these are tokens with names such as ';' and '::=', which are typically used
// as shortcuts in the grammar definitions, which may not be explicitly defined
// in a terminal definition
//
bool define_literal_tokens(parser* p)
{
	for (_vector<symbol*>::iterator it = p->symbols.begin(); it != p->symbols.end(); it++)
	{
		symbol* s = *it;
		if (s->type == NONTERM)
			continue;
		if (!s->bdefined && s->name[0] != '\'')
		{
			report_errorl(p, s->line, "token \"%s\" is undefined\n", s->name);
			return false;
		}
		for (symbolinst* inst=s->inst; inst; inst=inst->next)
		{
			if (!inst->bdefined)
			{
				inst->nfa = charliteral_to_nfa(s->name);
				inst->daction.accept_id = s->id;
				inst->daction.flags = DFA_ACCEPT;
				inst->nfa->accept->accept_id = inst->inst;
				s->bdefined = TRUE;
				inst->bdefined = TRUE;
			}
		}
	}
	return true;
}

bool check_undefined_rules(parser* p)
{
	for (_vector<symbol*>::iterator it = p->symbols.begin(); it != p->symbols.end(); it++)
	{
		symbol* s = *it;
		if (s->type == TERM)
			continue;
		if (!s->bdefined)
		{
			report_error(p, "rule \"%s\" is undefined\n", s->name);
			return false;
		}
	}
	return true;
}

bool check_undefined_states(parser* p)
{
	int sz = p->state_defined.size();
	for (int i=0;i<sz;i++)
	{
		if (!p->state_defined[i])
		{
			report_error(p, "state \"%s\" is undefined\n", p->states[i]);
			return false;
		}
	}
	return true;
}

//
// struct used for high resolution timing
//
typedef struct tag_timer
{
	long t1;
	long t2;
} _timer;

void startInstrument(_timer* t)
{
#ifdef WIN32
	LARGE_INTEGER start_time;
	QueryPerformanceCounter(&start_time);
	t->t1 = start_time.HighPart;
	t->t2 = start_time.LowPart;
#else
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME,&ts);
	t->t1 = ts.tv_sec;
	t->t2 = ts.tv_nsec;
#endif
}

double endInstrument(_timer* t)
{
#ifdef WIN32
	LARGE_INTEGER start_time, end_time, freq;
	start_time.HighPart = t->t1;
	start_time.LowPart = t->t2;
	QueryPerformanceCounter(&end_time);
	QueryPerformanceFrequency(&freq);
	return (double)(end_time.QuadPart-start_time.QuadPart)/(double)freq.QuadPart;
#else
	struct timespec te;
	clock_gettime(CLOCK_REALTIME,&te);
	return (double)((te.tv_sec*1000000+te.tv_nsec) - (t->t1*1000000+t->t2)) / 1000000.0;
#endif
}

bool compile_grammar(char* filename, parser* p)
{
	// build timers
	_timer dfa_timer;
	_timer lalr_timer;

	// dfa
	int nfa_output_index = 0;

	// initialize parser struct
	p->charsets = 0;
	p->nerrors = p->nwarnings = 0;
	p->start_symbol = 0;
	p->ncur_precedence = 0;
	p->ncur_assoc = 0;
	p->input_file = _strdup(filename);
	charset_build_default(p);
	p->ncurstate = state_get(p, "start");
	state_define(p,"start");

	// Add EOF terminal
	symbol* _EOF = symbol_add(p, "EOF");
	_EOF->bdefined = TRUE;
	symbolinst* eofinst = symbol_add_inst(_EOF);
	eofinst->nfa = 0;
	eofinst->daction.accept_id = _EOF->id;
	eofinst->daction.flags = 0;
	eofinst->bdefined = TRUE;

	// Add S' Rule
	rule* prime_rule = rule_add(p, "<S'>");
	symbol* s_prime = symbol_find_name(p, "<S'>");
	s_prime->bdefined = TRUE;
	prime_rule->bdefined = TRUE;

	// load and parse the input file
	if (!loadfile(filename, p))
		return false;

	// look for undefined states
	if (!check_undefined_states(p))
		return false;

	// define literal tokens that are undefined
	if (!define_literal_tokens(p))
		return false;

	if (!p->bhave_grammar)
	{
		// delete S' rule if no rules defined in grammar
		for (int i=s_prime->id;i<p->symbols.size()-1;i++)
		{
			p->symbols[i] = p->symbols[i+1];
			p->symbols[i]->id--;
			for (symbolinst* si=p->symbols[i]->inst;si;si=si->next)
				si->daction.accept_id--;
		}
		delete prime_rule;
		delete s_prime;
		p->rules.clear();
		p->symbols.pop_back();
	} else
	{
		// Add start symbol as rhs of prime rule
		if (!p->start_symbol)
			p->start_symbol = symbol_find_name(p, "<S'>");
		rule_add_rhs(p, prime_rule, p->start_symbol->name);

		// look for undefined rules
		if (!check_undefined_rules(p))
			return false;
	}

	if (p->nerrors>0)
		return false;

	// build names for production rules(if any exist)
	for (_vector<rule*>::iterator rit = p->rules.begin(); rit != p->rules.end(); rit++)
	{
		for (rule* r = *rit; r; r=r->next_inst)
			rule_make_name(r);
	}

	if (opt.bverbose)
	{
		// get terminal count
		int ncnt=0;;
		for(_vector<symbol*>::iterator it=p->symbols.begin();it != p->symbols.end(); it++)
			if ((*it)->type == TERM) ncnt++;
		printf("  %d symbols\n", ncnt);
		ncnt=0;
		for (_vector<rule*>::iterator rit=p->rules.begin(); rit != p->rules.end(); rit++)
			for (rule* r = *rit; r; r=r->next_inst)	ncnt++;
		printf("  %d productions\n", ncnt);
	}
	
	// reserve room for dfa action table
	int max_inst = symbol_get_max_inst();
	if (max_inst > 0)
		p->actions.resize(max_inst);

	// Time the DFA build
	if (opt.bverbose)
	{
		startInstrument(&dfa_timer);
		printf("Compiling DFA...\n");
	}

	// build nfa graph
	for (int cur_state=0; cur_state<state_get_max(p); cur_state++)
	{
		// combine all terminals symbols into one nfa for each state
		Node* nfa = 0;
		for (_vector<symbol*>::iterator it = p->symbols.begin(); it != p->symbols.end(); it++)
		{
			symbol* s = *it;
			if (s->type == NONTERM)
				continue;
			symbolinst* inst = symbol_find_state(s, cur_state);
			if (inst)
			{
				nfa = nfa?combine_branches(nfa, inst->nfa) : inst->nfa;
				while (p->actions.size() <= inst->inst)
					p->actions.push_back(0);
				p->actions[inst->inst] = new dfaaction;
				memcpy(p->actions[inst->inst], &inst->daction, sizeof(dfaaction));
			}
		}

		// convert the nfa into a dfa
		if (nfa)
		{
			int offset;
			add_dfa(p->dfa, nfa, p->dfaccept, offset);
			state_set_offset(p, state_name(p, cur_state), offset);

			// check if dfa accepts token at state 0 and issue a warning
			if (p->dfaccept.contains(0)) {
				int s_inst = p->dfaccept[offset];
				int s_id = p->actions[s_inst]->accept_id;
				int state_id = p->state_offsets.size()-1;
				printf("warning: dfa state \"%s\"(%d) accepts terminal \"%s\"(%d), on no input\n",
					state_name(p, state_id), state_id,
					symbol_find_id(p, s_id)->name, s_id);
			}
		}
	}
	if (opt.bverbose)
	{
		printf("  %d states\n  Finished in %0.02f seconds\n", p->dfa.size(), endInstrument(&dfa_timer));
	}

	//
	// finalize dfa actions
	//
	fixup_gotos(p, p->state_offsets);

	// build LR action table
	if (p->rules.size())
	{
		if (opt.bverbose)
		{
			printf("Compiling LALR action table...\n");
			printf("  Start Symbol \"%s\"\n", p->start_symbol->name);
			startInstrument(&lalr_timer);
		}

		// compile the grammar
		build_lalr(p);

		if (opt.bverbose)
		{
			printf("  %d states\n  Finished in %0.04f seconds\n", p->lractions.size(), endInstrument(&lalr_timer));
		}
	} else
		printf("no productions specified, skipping LR generation\n");

	return true;
}

bool save_file(const char* filename, BYTE* parser_data, long parser_size)
{
	FILE* fout = fopen(filename, "wb");
	if (!fout) return false;

	if (opt.bverbose)
		printf("Saving grammar file \"%s\"...\n", filename);

	if (parser_size)
		fwrite(parser_data, parser_size, 1, fout);

	fclose(fout);
	return true;
}

void print_dfa(parse_config* cfg)
{
	printf("DFA\n");
	printf("-----\n");
	for (int i=0;i<cfg->dfa->size;i++)
	{
		printf("State %d\n", i);
		
		_hashmap<int, _list<int> > m;
		for (int j=0;j<256;j++)
			if (cfg->dfa->table[i][j])
				m[cfg->dfa->table[i][j]].push_back(j);
		for (_hashmap<int, _list<int> >::iterator it = m.first(); it; it=it->next_walk)
		{
			printf("  goto %3d\t", it->key);
			for (_list<int>::iterator it2 = it->value.first(); it2; it2=it2->next)
			{
				printf("%s ", chstr(it2->val));
			}
			printf("\n");
		}
		if (cfg->accept[i] != -1) {
			int inst = cfg->accept[i];
			dfaaction* a = &cfg->daction[cfg->accept[i]];
			printf("  %s %d(%d)", (a->flags & DFA_ACCEPT)?"accept":"ignore", a->accept_id, cfg->accept[i]);
			if (a->flags & DFA_GOTO)
				printf("   goto %d", a->goto_state);
			else
			if (a->flags & DFA_PUSH)
				printf("   push %d", a->goto_state);
			else
			if (a->flags & DFA_POP)
				printf("   pop");
			if (a->flags & DFA_CONCAT)
				printf("   concat");
			if (a->flags & DFA_PRESERVE)
				printf("   preserve");
			printf("\n");
		}
	}
}

void show_grammar(parse_config* cfg, options* opt)
{
	//
	// display symbols
	//
	if (opt->bshow_grammar_symbols)
	{
		printf("Symbols\n");
		printf("-----\n");
		if (!cfg->nsymbols)
			printf("<N/A>\n");
		for (int i=0; i<cfg->nsymbols; i++)
		{
			symbol_* s = &cfg->symbols[i];
			pprintf(30, "%d] %s", i, s->name);
			printf("%s\n", (s->type==TERM)?"Terminal":"Nonterminal");
		}
		printf("\n");

		//
		// display productions
		//
		printf("Rules\n");
		printf("-----------\n");
		if (!cfg->nproductions)
			printf("<N/A>\n");
		for (int i=0;i<cfg->nproductions;i++)
		{
			production* p = &cfg->productions[i];
			printf("%d(%d)] %s\n", i, p->lhs, p->name);
		}
		printf("\n");
	}

	if (opt->bshow_grammar_actions)
	{
		//
		// display grammar dfa
		//
		print_dfa(cfg);
		printf("\n");

		//
		// display grammar actions
		//
		if (cfg->nlraction)
			print_actions(cfg);
	}
}

const char* make_char_name(char* c)
{
	switch(*c){
	case ' ': return "SPACE";
	case '/': return "FORWARD_SLASH";
	case '\\': return "BACKSLASH";
	case '\n': return "NEWLINE";
	case '\t': return "TAB";
	case '.': return "PERIOD";
	case ',': return "COMMA";
	case ';': return "SEMI";
	case ':': return "COLON";
	case '>': return "GREATER";
	case '<': return "LESS";
	case '?': return "QUESTION";
	case '\'': return "SINGLE_QUOTE";
	case '\"': return "QUOTE";
	case '[': return "OPEN_BRACKET";
	case ']': return "CLOSE_BRACKET";
	case '~': return "TILDE";
	case '`': return "GRAVE_ACCENT";
	case '!': return "EXCLAMATION";
	case '@': return "AT";
	case '#': return "NUMBER";
	case '$': return "STRING";
	case '%': return "PERCENT";
	case '^': return "CARET";
	case '&': return "AMPERSAND";
	case '*': return "STAR";
	case '(': return "LEFT_PAREN";
	case ')': return "RIGHT_PAREN";
	case '_': return "_";
	case '-': return "MINUS";
	case '+': return "PLUS";
	case '=': return "EQUALS";
	case '|': return "VERT_BAR";
	case '{': return "OPEN_BRACE";
	case '}': return "CLOSE_BRACE";
	case '\xa0': return "NBSP";
	case '\xa1': return "IEXCLAMATION";
	case '\xa2': return "CENT";
	case '\xa3': return "POUND";
	case '\xa4': return "CURRENCY";
	case '\xa5': return "YEN";
	case '\xa6': return "BROKEN_VERT";
	case '\xa7': return "SECTION";
	case '\xa8': return "UMLAUT";
	case '\xa9': return "COPYRIGHT";
	case '\xaa': return "FEMORD";
	case '\xab': return "LDBLANGLEQUOTE";
	case '\xac': return "NOTSIGN";
	case '\xad': return "SOFTHYPHEN";
	case '\xae': return "REG_TRADEMARK";
	case '\xaf': return "OVERLINE";
	case '\xb0': return "DEGREE";
	case '\xb1': return "PLUS_MINUS_SIGN";
	case '\xb2': return "SQUARED";
	case '\xb3': return "CUBED";
	case '\xb4': return "ACUTE_ACCENT";
	default: return c;
	}
}

const char* make_symbol_name(char* symbol_name)
{
	static char buf[512];
	char sym_name[512];
	strcpy(sym_name, (symbol_name[0]=='<' || symbol_name[0]=='\'')?symbol_name+1:symbol_name);
	char* sn = sym_name + strlen(sym_name)-1;
	if (*sn == '>' || (*sn == '\'' && sn[-1] != '\\'))
		*sn = 0;

	buf[0]=0;
	bool bskiplast=false;
	for (char* p=sym_name;*p;p++)
	{
		const char* c = make_char_name(p);
		if (c == p)
		{
			char tmp[2] = {*p,0};
			strcat(buf, tmp);
		} else
		{
			if (p!=sym_name)
				strcat(buf, "_");
			strcat(buf, c);
		}
	}
	return buf;
}

void save_header(const char* filename, parse_config* cfg)
{
	FILE* fout = fopen(filename, "wt");
	if (!fout) {
		printf("error opening header file \"%s\" for output\n", filename);
		return;
	}

	if (opt.bverbose)
		printf("Saving header file \"%s\"...\n", filename);

	time_t tim = time(0);
	char strtime[256];
	strftime(strtime, 256, "%m/%d/%y %H:%M:%S", localtime(&tim));
	fprintf(fout, "\n");
	fprintf(fout, "/*\n");
	fprintf(fout, " *\n");
	fprintf(fout, " *  pgen generated header file\n");
	fprintf(fout, " *  %s\n", strtime);
	fprintf(fout, " *\n");
	fprintf(fout, " */\n");
	if (cfg->nstates)
	{
		fprintf(fout, "\n/*********************\n *   States\n *********************/\n\n");
		for (int i=0;i<cfg->nstates;i++)
		{
			pfprintf(fout, 40, "#define STATE_%s", cfg->states[i].name);
			fprintf(fout, "%d\n", cfg->states[i].dfa_offset);
		}
	}

	fprintf(fout, "\n/*********************\n *   Symbols\n *********************/\n\n");
	fprintf(fout, "/*  EOF  */\n#define __EOF__\t0\n\n");
	for (int i=1;i<cfg->nsymbols;i++)
	{
		fprintf(fout, "/*  %s   */\n", cfg->symbols[i].name);
		pfprintf(fout, 40, "#define SYMBOL_%s_", make_symbol_name(cfg->symbols[i].name));
		fprintf(fout, "%d\n\n", i);
	}
	if (opt.bsave_lalr)
	{
		fprintf(fout, "/*********************\n *   Rules\n *********************/\n\n");
		for (int i=0;i<cfg->nproductions;i++)
		{
			char buf[2048] = "";
			fprintf(fout, "/*  %s   */\n", cfg->productions[i].name);
			sprintf(buf, "#define RULE_%s_", make_symbol_name(cfg->symbols[cfg->productions[i].lhs].name));
			for (int j=0;j<cfg->productions[i].nrhs;j++)
			{
				char buf2[1024];
				sprintf(buf2, "%s_", make_symbol_name(cfg->symbols[cfg->productions[i].rhs[j]].name));
				strcat(buf, buf2);
			}
			pfprintf(fout, 60, "%s", buf);
			fprintf(fout, "\t%d\n\n", i);
		}
	}
	fclose(fout);
}

bool save_parse_tables(const char* filename, BYTE* parser_data, long parser_size)
{
	FILE* fout = fopen(filename, "wb");
	if (!fout) return false;

	if (opt.bverbose)
		printf("Saving parse table file \"%s\"...\n", filename);

	// get filename w/o extension
	char nm[512];
	strncpy(nm,filename,512);
	nm[511]=0;
	char* ext = strrchr(nm,'.');
	if (ext) *ext = 0;

	fprintf(fout,"\nunsigned int %s_size = %d;\n", nm, parser_size);
	if (parser_size)
	{
		fprintf(fout, "unsigned char %s[%u] = {", nm, parser_size);
		int cnt=16;
		for (int i=0;i<parser_size;i++)
		{
			if (!(cnt % 16)) {
				fprintf(fout, "\n   ");
			}
			fprintf(fout, "0x%02x, ", parser_data[i]);
			cnt++;
		}
		if ((cnt-1) % 16)
			fprintf(fout, "\n");
		fprintf(fout, "};\n\n");
	}

	fclose(fout);
	return true;
}

bool parser_to_config(parser* p, parse_config* cfg)
{
	int i=0, j=0;
	// dfa states
	if (p->dfa.size() != 0) {
		cfg->dfa = (_dfa*)new byte[sizeof(unsigned short) + ((256*sizeof(unsigned short))*p->dfa.size())];
		cfg->dfa->size = p->dfa.size();
		for (_vector<_vector<unsigned short> >::iterator vit = p->dfa.begin(); vit != p->dfa.end(); vit++)
			memcpy(&cfg->dfa->table[i++], &(*vit)[0], sizeof(unsigned short)*256);
		// dfa accept
		i=0;
		cfg->accept = new short[p->dfa.size()];
		memset(cfg->accept, 0xff, sizeof(short)*p->dfa.size());
		for (_hashmap<int,int>::iterator it = p->dfaccept.first(); it; it=it->next_walk) {
			int k = it->key;
			int j = it->value;
			cfg->accept[it->key] = it->value;
		}
		// dfa actions
		i=0;
		cfg->ndaction = p->actions.size();
		cfg->daction = new dfaaction[p->actions.size()];
		memset(cfg->daction, 0, sizeof(dfaaction) * p->actions.size());
		for (_vector<dfaaction*>::iterator it = p->actions.begin(); it != p->actions.end(); it++) {
			dfaaction* s = *it;
			cfg->daction[i].accept_id = s->accept_id;
			cfg->daction[i].flags = s->flags;
			cfg->daction[i].goto_state = s->goto_state;
			if (!(cfg->daction[i].flags & (DFA_GOTO | DFA_PUSH)))
				cfg->daction[i].goto_state = 0;
			if (!(cfg->daction[i].flags & DFA_ACCEPT))
				cfg->daction[i].accept_id = 0;
			i++;
		}
	}
	// symbols
	cfg->nsymbols = 0;
	for (_vector<symbol*>::iterator it = p->symbols.begin(); it != p->symbols.end(); it++) if ((*it)->id != -1) cfg->nsymbols++;
	cfg->symbols = new symbol_[cfg->nsymbols];
	memset(cfg->symbols, 0, sizeof(symbol_) * cfg->nsymbols);
	for (i=0;i<cfg->nsymbols;i++) {
		cfg->symbols[i].type = p->symbols[i]->type;
		cfg->symbols[i].name = _strdup(p->symbols[i]->name);
	}
	// lr states
	if (p->lractions.size() != 0) {
		i=0;
		for (taction::iterator it =p->lractions.first(); it; it=it->next_walk) if (it->key > i) i = it->key;
		cfg->nlraction = i+1;
		cfg->lraction = new _lraction*[cfg->nlraction];
		for (i=0;i<cfg->nlraction;i++) {
			cfg->lraction[i] = new _lraction[cfg->nsymbols];
			memset(cfg->lraction[i], 0, sizeof(_lraction) * cfg->nsymbols);
			for (j=0;j<cfg->nsymbols;j++) cfg->lraction[i][j].type = -1;
		}
		for (taction::iterator it = p->lractions.first(); it; it=it->next_walk) {
			for (_hashmap<int, action*>::iterator jt= it->value.first(); jt; jt=jt->next_walk) {
				cfg->lraction[it->key][jt->key].val = jt->value->val;
				cfg->lraction[it->key][jt->key].type = jt->value->type;
				if (jt->value->type == LR_REDUCE) {
					cfg->lraction[it->key][jt->key].reduce_production = jt->value->reduce_prod;
					cfg->lraction[it->key][jt->key].reduce_count = jt->value->reduce_cnt;
				} else {
					cfg->lraction[it->key][jt->key].reduce_count = 0;
					cfg->lraction[it->key][jt->key].reduce_production = 0;
				}
			}
		}
	} else
		cfg->nlraction = 0;
	// rules
	cfg->nproductions=0;
	if (p->rules.size() != 0) {
		for (_vector<rule*>::iterator it=p->rules.begin(); it != p->rules.end(); it++)
			for (rule* r=*it; r; r=r->next_inst) cfg->nproductions++;
		cfg->productions = new production[cfg->nproductions];
		memset(cfg->productions, 0, sizeof(production)*cfg->nproductions);
		for (_vector<rule*>::iterator it = p->rules.begin(); it != p->rules.end(); it++) {
			for (rule* r=*it; r; r=r->next_inst) {
				cfg->productions[r->inst].lhs = r->left->id;
				cfg->productions[r->inst].name = _strdup(r->name);
				cfg->productions[r->inst].nrhs = r->right.size();
				cfg->productions[r->inst].rhs = new int[cfg->productions[r->inst].nrhs];
				for (j=0;j<cfg->productions[r->inst].nrhs;j++)
					cfg->productions[r->inst].rhs[j] = r->right[j]->id;
			}
		}
	}
	// states
	cfg->nstates = p->states.size();
	if (cfg->nstates != 0) {
		cfg->states = new _state[cfg->nstates];
		memset(cfg->states, 0, sizeof(_state) * cfg->nstates);
		for (i=0;i<cfg->nstates;i++) {
			char* c = p->states[i];
			cfg->states[i].name = _strdup(p->states[i]);
			cfg->states[i].dfa_offset = p->state_offsets[i];
		}
	}
	return true;
}

bool config_to_bytes(parse_config* config, STRM* strm, bool bsave_dfa, bool bsave_lalr, bool bsave_symbols, bool bsave_states)
{
	unsigned int offset = strm->ofs;
	/** Write offset placeholders ****/
	swrite(&offset,sizeof(int),1,strm); /* dfa */
	swrite(&offset,sizeof(int),1,strm); /* lalr */
	swrite(&offset,sizeof(int),1,strm); /* symbols */
	swrite(&offset,sizeof(int),1,strm); /* states */
	/********** Write DFA ************/
	memcpy(strm->data+offset, &strm->ofs, 4); offset+=4; /* write dfa offset */
	if (bsave_dfa)
	{
		// write dfa table
		swrite(&config->dfa->size,2,1,strm);
		swrite(config->dfa->table, sizeof(unsigned short) * config->dfa->size, 256, strm);
		if (config->dfa->size != 0)
		{
			// write accept states
			swrite(config->accept, sizeof(short), config->dfa->size, strm);

			// write actions
			swrite(&config->ndaction, 2,1,strm);
			swrite(config->daction, sizeof(dfaaction), config->ndaction, strm);
		}
	} else
	{
		unsigned short zero=0;
		swrite(&zero,2,1,strm);
	}
	/********** Write LALR ************/
	memcpy(strm->data+offset, &strm->ofs, 4); offset+=4; /* write LALR offset */
	if (bsave_lalr && config->nlraction)
	{
		int ncolumns = config->nsymbols;
		int nrows = config->nlraction;
		swrite(&ncolumns,4,1,strm);
		if (ncolumns != 0)
		{
			swrite(&nrows, 4,1,strm);
			if (!ncolumns || !nrows) {return true;}
			for (int i=0;i<nrows;i++)
				swrite(config->lraction[i], sizeof(_lraction), ncolumns, strm);
		}
	} else
	{
		int zero=0;
		swrite(&zero,4,1,strm);
	}
	/********** Write Symbols ************/
	memcpy(strm->data+offset, &strm->ofs, 4); offset+=4; /* write Symbols offset */
	if (bsave_symbols)
	{
		swrite(&config->nsymbols,4,1,strm);
		for (int i=0;i<config->nsymbols;i++) {
			swrite(&config->symbols[i].type,1,1,strm);
			short sz = (short)strlen(config->symbols[i].name)+1;
			swrite(&sz,2,1,strm);
			swrite(config->symbols[i].name, sz,1,strm);
		}
		/********** Write Productions ************/
		swrite(&config->nproductions, 4,1,strm);
		for (int i=0;i<config->nproductions;i++) {
			production* p = &config->productions[i];
			swrite(&p->lhs, 4,1,strm);
			short sz = (short)strlen(p->name)+1;
			swrite(&sz,2,1,strm);
			swrite(p->name,sz,1,strm);
			swrite(&p->nrhs,4,1,strm);
			swrite(p->rhs,4,p->nrhs,strm);
		}
	} else
	{
		int zero=0;
		swrite(&zero,4,1,strm);
	}
	/********** Write States ************/
	memcpy(strm->data+offset, &strm->ofs, 4); offset+=4; /* write States offset */
	if (bsave_states)
	{
		swrite(&config->nstates,4,1,strm);
		for (int i=0;i<config->nstates;i++)
		{
			unsigned short sz= (unsigned short)strlen(config->states[i].name)+1;
			swrite(&sz,2,1,strm);
			swrite(config->states[i].name,sz,1,strm);
			swrite(&config->states[i].dfa_offset,4,1,strm);
		}
	} else
	{
		int zero=0;
		swrite(&zero,4,1,strm);
	}
	return true;
}

void save_cfile(const char* filename, parse_config* cfg)
{
	FILE* fout = fopen(filename, "wt");
	if (!fout) {
		printf("error opening file \"%s\" for output\n", filename);
		return;
	}

	if (opt.bverbose)
		printf("Saving c switch file \"%s\"...\n", filename);

	// get filename w/o extension
	char fn[512];
	strncpy(fn,filename,512);
	fn[511]=0;
	char* ext = strrchr(fn,'.');
	if (ext) *ext = 0;

	time_t tim = time(0);
	char strtime[256];
	strftime(strtime, 256, "%m/%d/%y %H:%M:%S", localtime(&tim));
	fprintf(fout, "\n");
	fprintf(fout, "/*\n");
	fprintf(fout, " *\n");
	fprintf(fout, " *  pgen generated switch file\n");
	fprintf(fout, " *  %s\n", strtime);
	fprintf(fout, " *\n");
	fprintf(fout, " */\n");
	fprintf(fout, "\n");
	fprintf(fout, "\n");
	fprintf(fout, "/*\n");
	fprintf(fout, " * You can cut and paste the following switch statement into your code\n");
	fprintf(fout, " */\n\n");
	fprintf(fout, "void cb_getbytes(parse_state* state)\n");
	fprintf(fout, "{\n/* add your code to read from a file/buffer here... */\n};\n\n");
	fprintf(fout, "void parse_%s(const void* input_bytes, int input_len)\n", fn);
	fprintf(fout, "{\n");
	fprintf(fout, "\tParser p(\"%s.dat\");\n", fn);
	fprintf(fout, "\tp.init(input_bytes, input_len, cb_getbytes);\n\n");
	fprintf(fout, "\twhile(true)\n");
	fprintf(fout, "\t{\n");
	fprintf(fout, "\t\tswitch(p.parse())\n");
	fprintf(fout, "\t\t{\n");

	for (int i=0;i<cfg->nproductions;i++)
	{
		fprintf(fout, "\t\t/*  %s   */\n", cfg->productions[i].name);
		fprintf(fout, "\t\tcase RULE_%s_", make_symbol_name(cfg->symbols[cfg->productions[i].lhs].name));
		for (int j=0;j<cfg->productions[i].nrhs;j++)
			fprintf(fout, "%s_", make_symbol_name(cfg->symbols[cfg->productions[i].rhs[j]].name));
		fprintf(fout, ":\n\t\t\tbreak;\n");
		if (i < cfg->nproductions-1)
			fprintf(fout, "\n");
	}
	fprintf(fout, "\n\t\t/* parse error */\n");
	fprintf(fout, "\t\tcase -1:\n");
	fprintf(fout, "\t\t\treturn;\n");
	fprintf(fout, "\n\t\t/* accepted */\n");
	fprintf(fout, "\t\tcase 0:\n");
	fprintf(fout, "\t\t\treturn;\n");
	fprintf(fout, "\t\t}\n");
	fprintf(fout, "}\n\n");

	fclose(fout);
}

bool write_compressed_file(const char* compressed_file, const char* filename)
{
	FILE* fout = fopen(filename, "wb");
	if (!fout) {
		printf("error opening output file \"%s\"\n", filename);
		return false;
	}
	char* bytes;
	int size;
	if (!load_compressed(compressed_file, &bytes, &size)) {
		printf("unknown compressed file \"%s\"\n", compressed_file);
		return false;
	}
	fwrite(bytes,size,1,fout);
	delete[] bytes;
	fclose(fout);
	return true;
}

void save_parse_engine(const char* base_filename)
{
	char fn[512];
	strcpy(fn, base_filename);
	strcat(fn,".h");
	if (!write_compressed_file("parser.h",fn))
		printf("Error: parse engine file not saved \"%s\"", fn);
	else
	if (opt.bverbose)
		printf("Parse engine header saved as \"%s\"\n", fn);

	strcpy(fn, base_filename);
	strcat(fn, ".c");
	if (!write_compressed_file("parser.c",fn))
		printf("Error: parse engine file not saved \"%s\"", fn);
	else
	if (opt.bverbose)
		printf("Parse engine saved as \"%s\"\n", fn);
}

void save_regex_engine(const char* base_filename)
{
	char fn[512];
	strcpy(fn, base_filename);
	strcat(fn,".h");
	if (!write_compressed_file("regex.h",fn))
		printf("Error: regex engine file not saved \"%s\"", fn);
	else
	if (opt.bverbose)
		printf("Regex engine header saved as \"%s\"\n", fn);

	strcpy(fn, base_filename);
	strcat(fn, ".cpp");
	if (!write_compressed_file("regex.cpp",fn))
		printf("Error: regex engine file not saved \"%s\"", fn);
	else
	if (opt.bverbose)
		printf("Regex engine saved as \"%s\"\n", fn);
}

void _puts(char* p)
{
	while(*p)
	{
		printf("%c",*p);
		p++;
	}
}

//
// stack utility functions from parser.cpp
//
extern "C" {
	void _stack_push(_parser_stack* s, parser_stack_el* value);
	parser_stack_el* _stack_back(_parser_stack* s);
	parser_stack_el* _stack_pop(_parser_stack* s);
}

// get the next rule
int _dbg_parser_get_rule(parse_config* cfg, parse_state* state)
{
	while(true)
	{
		if (state->breduction)
		{
			// replace reduction symbols with reduction symbol id
			state->el.id = state->lra->val;
			for (int i=0;i<state->lra->reduce_count;i++)
			{
				if (i == state->lra->reduce_count - 1)
					state->el.state = _stack_back(&state->stack)->state;
				_stack_pop(&state->stack);
			}
			_stack_push(&state->stack, &state->el);
			state->el.lexeme=0;

			printf("%02d] Reduced Symbol %d, Production \"%s\"(%d)\n", state->lrstate, state->lra->val, cfg->productions[state->lra->reduce_production], state->lra->reduce_production);

			// set current state to state on top of stack
			if (state->lra->reduce_count>0)
				state->lrstate = state->el.state;

			// handle goto
			int oldstate = state->lrstate;
			_lraction* a1 = &cfg->lraction[state->lrstate][state->el.id];
			state->lrstate = a1->val;
			printf("%02d] Goto %d\n", oldstate, state->lrstate);
			state->breduction = false;
		}

		// get a new token if we need one
		if (state->curtok == -1)
			state->curtok = state->gettoken(cfg, state);
		if (state->curtok == -1)
			// lexer returned an error
			return -1;
		state->lra = &cfg->lraction[state->lrstate][state->curtok];
		if (state->lra->type == -1)	{
			// grammatical error or ran out of input prematurely
			if (state->curtok == 0 && cfg->lraction[state->lrstate][0].type == -1)
				return -3;
			return -2;
		}
		switch(state->lra->type)
		{
		case LR_SHIFT:
			state->el.id = state->curtok;
			state->el.state = state->lrstate;
			if (state->el.lexeme) free(state->el.lexeme); state->el.lexeme=0;
			printf("%02d] Shifting \"%s\"(%d), going to state %d\n", state->lrstate, state->token, state->curtok, state->lra->val);
			state->lrstate = state->lra->val;
			_stack_push(&state->stack, &state->el);
			// save current token onto stack
			if (state->token) _stack_back(&state->stack)->lexeme = _strdup(state->token);
			_stack_back(&state->stack)->line = state->line;
			_stack_back(&state->stack)->column = state->column;
			// get next token
			state->curtok = state->gettoken(cfg, state);
			break;
		case LR_REDUCE:
			// setup references for reduction
			state->reduction_symbol = state->lra->val;
			state->reduction_inst = state->lra->reduce_production;
			state->reduction_cnt = state->lra->reduce_count;
			state->stack_index = state->stack.nsize - state->reduction_cnt;
			state->breduction = 1;

			// return reduction rule
			return state->reduction_inst;
		case LR_ACCEPT:
			printf("%02d] Accepted\n", state->lrstate);
			return 0;
		case LR_GOTO:
			printf("%02d] Goto %d\n", state->lrstate, state->lra->val);
			state->curtok = state->gettoken(cfg, state);
			state->lrstate = state->lra->val;
			break;
		}
	}
}

void test_grammar(const char* filename, parse_config* cfg, bool bdfa_only)
{
	// load test file
	FILE* fin = fopen(filename, "rb");
	if (!fin) {printf("Error opening test file \"%s\"\n", filename); return;}
	struct _stat st;
	_fstat(fin->_file, &st);
	char* binput = new char[st.st_size];
	fread(binput, st.st_size, 1, fin);
	fclose(fin);

	char token[256];
	parse_state state;
	parser_init_state(&state, binput, st.st_size, token, 256, 0);

	//
	// test the LR table
	//
	printf("Grammar Test:\n");

	if (!cfg->nlraction)
		bdfa_only = true;

	int result = 0;
	if (!bdfa_only)
	{
		// test parser
		while((result = _dbg_parser_get_rule(cfg, &state))>0);
		if (result == 0)
			printf("Accepted\n");
	} else
	{
		// only test lexer, not parser
		while (true)
		{
			result = parser_get_token(cfg, &state);
			if (result < 0)
				break;
			if (result == 0)
			{
				printf("<End of File>\n");
				break;
			}
			if (cfg->nsymbols)
				printf("(%d,%d) %s:\t%s\n", state.tok_col, state.tok_line, cfg->symbols[result].name, state.token);
			else
				printf("(%d, %d) symbol %d:\t%s\n", state.tok_col, state.tok_line, result, state.token);
		}
	}

	//
	// display error messages if appropriate
	//
	if (result == -1)
	{
		printf("Error: Syntax error near line %d, column %d\n", state.line, state.column);
		printf(" Found character %c(%d), expecting:\n ", (*state.s > 32 && *state.s < 127)?*state.s:'.', *state.s);
		for (int i=0;i<255;i++)	{
			if (cfg->dfa->table[state.dfastate][i])
				printf("%s ", chstr(i));
		}
		printf("\n");
	} else
	if (result == -2)
	{
		printf("Error: Grammatical error near line %d, column %d\n", state.tok_line, state.tok_col);
		printf(" Found token \"%s\", expecting:\n ", state.token);
		for (int i=0;i<cfg->nsymbols;i++)
			if (cfg->lraction[state.lrstate][i].type != -1)
				printf("%s ", cfg->symbols[i].name);
	} else
	if (result == -3)
	{
		printf("Error: Ran out of input\n");
	}

	printf("\n");
}

void test_dfa_multi(const char* filename, parse_config* cfg)
{
	token *tok, *cur;

	// load test file
	FILE* fin = fopen(filename, "rb");
	if (!fin) {printf("Error opening test file \"%s\"\n", filename); return;}
	struct _stat st;
	_fstat(fin->_file, &st);
	char* binput = new char[st.st_size];
	fread(binput, st.st_size, 1, fin);
	fclose(fin);

	char token[256];
	parse_state state;
	parser_init_state(&state, binput, st.st_size, token, 256, 0);

	printf("DFA multi-match Test:\n");

	/* get multiple matches from lexer */
	tok = parser_get_token_multi(cfg, &state);
	if (!tok) {
		printf("No matches found\n");
		return;
	}

	/* iterate over matches and display tokens */
	for (cur=tok; cur; cur=cur->next) {
		if (cfg->nsymbols)
			printf("(%d,%d) %s:\t%s\n", cur->col, cur->line, cfg->symbols[cur->id].name, cur->text);
		else
			printf("(%d, %d) symbol %d:\t%s\n", cur->col, cur->line, cur->id, cur->text);
	}

	printf("<End of File>\n");
}

void print_banner()
{
	printf("pgen v%d.%02d(build %d) (c)2004 Justin Holmes(justin@holmesweb.net)\n\n", VERSION/100,VERSION%100,BUILD);
}

char dirfunc(const char* filename)
{
	const char* dot = strrchr(filename, '.');
	if (dot && !_stricmp(dot, ".txt")) {
		printf("  %s\n", filename);
	}
	return TRUE;
}

void show_grammar_files()
{
	printf("Available grammar templates:\n");
	dir_compressed(dirfunc);
}

void write_grammar_file(const char* filename)
{
	FILE* fout = fopen(filename, "wb");
	if (!fout) {
		printf("error opening output file \"%s\"\n", filename);
		return;
	}
	char* bytes;
	int size;
	if (!load_compressed(filename, &bytes, &size)) {
		printf("invalid grammar template \"%s\"\n", filename);
		return;
	}
	fwrite(bytes,size,1,fout);
	delete[] bytes;
	fclose(fout);
	printf("Grammar template saved as \"%s\"\n", filename);
}

char display_file(const char* filename)
{
	char* bytes, *p;
	int size;
	if (!load_compressed(filename, &bytes,&size)) {
		return FALSE;
	}
	p=bytes;
	while(size--) putchar(*p++);
	delete[] bytes;
	return TRUE;
}

void show_usage()
{
	if (!display_file("usage.hlp"))
		printf("error loading help\n");
}

void show_grammar_help()
{
	if (!display_file("grammar.hlp"))
		printf("error loading grammar help\n");
}

char app_path[512] = "";
char command_line[512] = "";

int main(int argc, char** argv)
{
	memset(&opt,0,sizeof(opt));
	memset(spaces,' ',255);
	spaces[255]=0;

	// setup default options
	opt.bsave_dfa = true;
	opt.bsave_lalr = true;
	opt.bsave_symbols = true;
	opt.bsave_states = true;
	opt.bcompress_output = true;
	
#ifdef WIN32
	// save app path and whole command line
	GetModuleFileName(0, app_path, 512);
	strcpy(command_line, GetCommandLine());
#else
	// save app path and whole command line
	strcpy(app_path, argv[0]);
	for (int i=0;i<argc;i++)
	{
		strcat(command_line, argv[i]);
		if (i < argc-1)
			strcat(command_line, " ");
	}
#endif

/*	printf("app_path: \"%s\"\n", app_path);
	printf("command_line: \"%s\"\n", command_line);
*/
	print_banner();

	opt.bin_parser = false;
	if (!read_command_line(command_line)) {
		printf("syntax error on command line\n");
		return 0;
	}

	if (opt.bshow_usage)
	{
		show_usage();
		return 0;
	}

	if (opt.bshow_grammar_files) {
		show_grammar_files();
		return 0;
	}

	if (opt.bshow_grammar_help) {
		show_grammar_help();
		return 0;
	}

	if (!opt.bcompile_grammar_file && opt.bshow_grammar_configs) {
		printf("can't show grammar configs unless -it is specified\n");
		return 1;
	}

	if (!opt.bcompile_grammar_file && !opt.bread_grammar_file && !opt.boutput_parse_engine) {
		if (opt.bsave_header || opt.boutput_grammar_file || opt.bsave_cfile || opt.boutput_parse_tables) {
			printf("can't output files unless -it or -ig is specified\n");
			return 1;
		}
		if (opt.btest_grammar || opt.btest_dfa || opt.btest_dfa_multi) {
			printf("can't test grammar unless -it or -ig is specified\n");
			return 1;
		}
		return 0;
	}

	parse_config cfg;
	if (opt.bcompile_grammar_file)
	{
		// read and parse grammar
		parser p;
		p.cmdline=0;
		p.bhave_grammar=false;
		if (!compile_grammar(opt.input_file, &p))
			return 1;

		if (opt.bshow_grammar_configs)
		{
			printf("\n");
			print_configs(&p);
		}

		// convert parser to parse_config
		parser_to_config(&p, &cfg);
	} else
	if (opt.bread_grammar_file)
	{
		// load parser from compiled grammar file
		if (!parser_load_config(opt.input_file, &cfg))
		{
			printf("error loading grammar file \"%s\"\n", opt.input_file);
			return 1;
		}
	}

	// save parse engine .h and .c files
	if (opt.boutput_parse_engine)
		save_parse_engine(opt.parse_engine_file);

	// save regex parse engine .h file
	if (opt.boutput_regex_engine)
		save_regex_engine(opt.parse_regex_file);

	if (!opt.bread_grammar_file && !opt.bcompile_grammar_file)
		return 0;

	if (opt.bverbose)
		printf("\n");

	// show grammar
	show_grammar(&cfg, &opt);

	if (opt.boutput_grammar_file || opt.boutput_parse_tables) {
		// show messages for entities that won't be output
		if (!opt.bsave_dfa)
			printf("Notice: DFA tables will not be saved to file\n");
		if (!opt.bsave_lalr)
			printf("Notice: LALR tables will not be saved to file\n");
		if (!opt.bsave_symbols)
			printf("Notice: Symbol information will not be saved to file\n");
		if (!opt.bsave_states)
			printf("Notice: State information will not be saved to file\n");

		// get byte stream from parse tables
		STRM* strm = new STRM;
		strm->data = new unsigned char[MAX_STREAM_SIZE];
		strm->ofs=0;
		config_to_bytes(&cfg, strm, opt.bsave_dfa, opt.bsave_lalr, opt.bsave_symbols, opt.bsave_states);

		BYTE* compressed_table = strm->data;
		long compressed_size = strm->ofs;
		double compressed_pct = 100;

		if (opt.bcompress_output) {
			if (opt.bverbose)
				printf("Compressing tables: <%d bytes> --> ", strm->ofs);

			// compress tables
			BpeCompress bpe;
			compressed_table = new BYTE[strm->ofs+5];
			compressed_size = bpe.compress(strm->data, strm->ofs, compressed_table+5)+5;
			compressed_pct = ((double)compressed_size / (double)strm->ofs) * 100.0;
			memcpy(compressed_table, "pgc10", 5);

			// delete the output stream
			delete[] strm->data;
			delete strm;

			if (opt.bverbose)
				printf("<%d bytes> (%f%%)\n", compressed_size, compressed_pct);
		} else {
			// add header
			memmove(compressed_table+5, compressed_table, compressed_size);
			memcpy(compressed_table, "pgn10", 5);
			compressed_size += 5;
		}

		// save grammar to a file
		if (opt.boutput_grammar_file)
			save_file(opt.grammar_file, compressed_table, compressed_size);

		// save parse tables
		if (opt.boutput_parse_tables)
			save_parse_tables(opt.parse_table_file, compressed_table, compressed_size);

		// delete the compressed tables
		delete[] compressed_table;
	}

	// save .c switch statement file
	if (opt.bsave_cfile)
		save_cfile(opt.c_file, &cfg);

	// save header file
	if (opt.bsave_header)
		save_header(opt.header_file, &cfg);

	// reload grammar from file and test it
	if (opt.btest_grammar || opt.btest_dfa)
		test_grammar(opt.test_file, &cfg, opt.btest_dfa);
	if (opt.btest_dfa_multi)
		test_dfa_multi(opt.test_file, &cfg);

	// delete the parser config
	parser_delete(&cfg,0);

	return 0;
}
