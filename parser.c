
#include "parser.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef WIN32
#define _stat stat
#define _fstat fstat
#endif

// Decompress data from input to output
unsigned char* expand(unsigned char* in, long insize, unsigned int* output_size)
{
	unsigned char left[256], right[256], stack[30];
	short int c, count, i, size;
	unsigned char* output;
	long input_offset = 4; // skip output size header
	long output_offset = 0;
	int* decomp_size = (int*)in;
	if (insize < 1) return 0;
	if (*decomp_size < 0 || *decomp_size < insize || *decomp_size > (1024*1024*2))
		return 0;
	if (output_size) *output_size = *decomp_size;
	output = malloc(sizeof(unsigned char) * *decomp_size);

	// Unpack each block until end of file
	while (input_offset < insize) {
		count = in[input_offset++];

		// Set left to itself as literal flag
		for (i = 0; i < 256; i++) left[i] = (unsigned char)i;

		// Read pair table
		for (c = 0;;) {

			// Skip range of literal bytes
			if (count > 127) {
				c += count - 127;
				count = 0;
			}
			if (c == 256) break;

			// Read pairs, skip right if literal
			for (i = 0; i <= count; i++, c++) {
				left[c] = in[input_offset++];
				if (c != left[c])
					right[c] = in[input_offset++];
			}
			if (c == 256) break;
			count = in[input_offset++];
		}

		// Calculate packed data block size
		size = 256 * in[input_offset] + in[input_offset+1];
		input_offset += 2;

		// Unpack data block
		for (i = 0;;) {

			// Pop byte from stack or read byte
			if (i)
				c = stack[--i];     
			else {
				if (!size--) break;
				c = in[input_offset++];
			}

			// Output byte or push pair on stack
			if (c == left[c])      
				output[output_offset++] = (unsigned char)c;
			else {
				stack[i++] = right[c];
				stack[i++] = left[c];
			}
		}
	}
	return output;
}

//
// stack utility functions
//
void _goto_push(_goto_stack* s, int value)
{
	if (s->ncount==s->nsize)
	{
		int nsize = (s->nsize+1)*2;
		if (nsize < 16) nsize = 16;
		s->el = (int*)(s->el?realloc(s->el,sizeof(int)*nsize):malloc(sizeof(int)*nsize));
		s->nsize = nsize;
	}
	s->el[s->ncount++] = value;
}

int _goto_pop(_goto_stack* s)
{
	if (!s->ncount)
		return 0;
	return s->el[--s->ncount];
}

void _stack_push(_parser_stack* s, parser_stack_el* value)
{
	if (s->ncount==s->nsize)
	{
		int nsize = (s->nsize+1)*2;
		if (nsize < 16) nsize = 16;
		s->el = (parser_stack_el*)(s->el?realloc(s->el,sizeof(parser_stack_el)*nsize):malloc(sizeof(parser_stack_el)*nsize));
		s->nsize = nsize;
	}
	memcpy(&(s->el[s->ncount++]), value, sizeof(parser_stack_el));
}

parser_stack_el* _stack_pop(_parser_stack* s)
{
	if (!s->ncount)
		return 0;
	return &(s->el[--s->ncount]);
}

parser_stack_el* _stack_back(_parser_stack* s)
{
	if (!s->ncount)
		return 0;
	return &(s->el[s->ncount-1]);
}

// initialize a parser state
void parser_init_state(parse_state* state, char* input, int inputlen, char* token, int toklen, _cbgetbytes getbytes)
{
	state->goto_state = 0;
	state->accept_id = -1;
	state->dfastate = 0;
	state->s = state->input = input;
	state->inputlen = (int)((inputlen<0)?strlen(input):inputlen);
	state->ileft = state->inputlen;
	state->p = token;
	state->token = token;
	state->toklen = toklen;
	state->tleft = toklen;
	state->getbytes = getbytes;
	state->lrstate = 0;
	state->gettoken = (_cbgettoken)parser_get_token;
	state->curtok = -1;
	state->accepteof = 1;
	state->lra = 0;
	state->breduction = 0;
	state->el.lexeme = 0;
	state->line=1;
	state->column=1;
	state->stack.ncount = state->stack.nsize=0;
	state->stack.el=0;
	state->goto_stack.ncount = state->goto_stack.nsize=0;
	state->goto_stack.el=0;
}

void parser_reset_state(parse_state* state)
{
	// dfa
	state->goto_state = 0;
	state->accept_id = -1;
	state->dfastate = 0;
	state->ileft = state->inputlen;
	state->s = state->input;
	state->p = state->token;
	state->tleft = state->toklen;
	state->line = 1;
	state->column = 1;
	state->goto_stack.ncount=0;

	// stack
	state->stack.ncount=0;

	// lr
	state->lrstate = 0;
	state->curtok = -1;

	// reduction parameters
	state->breduction=0;
	if (state->el.lexeme) free(state->el.lexeme);
	state->el.lexeme=0;
	state->reduction_cnt=state->reduction_inst=0;
	state->reduction_symbol=state->stack_index=0;
}

void parser_delete(parse_config* config, parse_state* state)
{
	int i;
	if (config)
	{
		if (config->dfa->size>0 && config->dfa)
			free(config->dfa);
		if (config->accept)
			free(config->accept);
		if (config->ndaction && config->daction)
			free(config->daction);
		if (config->nlraction>0 && config->lraction) {
			for (i=0;i<config->nlraction;i++)
				free(config->lraction[i]);
			free(config->lraction);
		}
		if (config->nsymbols && config->symbols)
			free(config->symbols);
		if (config->nproductions && config->productions)
			free(config->productions);
	}
	if (state)
	{
		if (state->el.lexeme)
			free(state->el.lexeme);
		state->el.lexeme=0;
		state->goto_stack.ncount=state->goto_stack.nsize=0;
		state->stack.ncount=state->stack.nsize=0;
		if (state->goto_stack.el)
			free(state->goto_stack.el);
		if (state->stack.el)
			free(state->stack.el);
	}
}

#define TESTREAD(_val, _size) 	{if (size<_size) {if (ob && bytes) free(ob); return 0;} memcpy(_val, b, _size); b+= _size;}
#define TESTREAD2(_val, _size) {if (size<_size) {if (ob && bytes) free(ob); return 1;} memcpy(_val, b, _size); b+= _size;}
#define TESTALLOC(cond) if (!(cond)) {parser_delete(config,0); if (ob && bytes) free(ob); return 0;}

int parser_create_config(parse_config* config, const void* bytes, int insize)
{
	unsigned int size;
	unsigned short ndfa;
	unsigned char* b, *ob;
	int i, ncolumns, nrows;

	/* decompress if necessary */
	if (!memcmp(bytes, "pgc", 3)) {
		b = expand(((unsigned char*)bytes)+5, insize-5, &size);
		if (!b) return 0;
		ob = b;
	} else
	if (!memcmp(bytes, "pgn", 3)) {
		b = ((unsigned char*)bytes) + 5;
		ob = 0;
		bytes = 0;
	} else
		return 0;
	memset(config, 0, sizeof(parse_config));
	b += 16; /* skip offsets */
	/********** Read DFA ************/
	TESTREAD(&ndfa, 2);
	TESTALLOC(config->dfa = (_dfa*)malloc( sizeof(char) * (sizeof(_dfa)+((ndfa-1)*512))) );
	config->dfa->size = ndfa;
	TESTREAD(config->dfa->table, ndfa*512);
	TESTALLOC(config->accept = malloc(sizeof(short)*ndfa));
	TESTREAD(config->accept, (ndfa * 2));
	TESTREAD(&config->ndaction, 2);
	TESTALLOC(config->daction = malloc(sizeof(dfaaction) * config->ndaction));
	TESTREAD(config->daction, (sizeof(dfaaction) * config->ndaction));
	/********** Read LALR ************/
	TESTREAD(&ncolumns, 4);
	if (ncolumns)
	{
		TESTREAD(&nrows, 4);
		config->nlraction=nrows;
		config->lraction = malloc(sizeof(_lraction*) * config->nlraction);
		for (i=0;i<config->nlraction;i++) {
			config->lraction[i] = malloc(sizeof(_lraction) * ncolumns);
			TESTREAD(config->lraction[i], (sizeof(_lraction)*ncolumns));
		}
	}
	/********** Read Symbols ************/
	TESTREAD2(&config->nsymbols,4);
	if (config->nsymbols)
	{
		config->symbols = malloc(sizeof(symbol_)*config->nsymbols);
		for (i=0;i<config->nsymbols;i++) {
			short sz;
			TESTREAD(&config->symbols[i].type, 1);
			TESTREAD(&sz, 2);
			config->symbols[i].name = malloc(sizeof(char)*sz);
			TESTREAD(config->symbols[i].name, sz);
		}
		/********** Read Productions ************/
		TESTREAD(&config->nproductions, 4);
		config->productions = malloc(sizeof(production)*config->nproductions);
		for (i=0;i<config->nproductions;i++) {
			short sz;
			production* p = &config->productions[i];
			TESTREAD(&p->lhs, 4);
			TESTREAD(&sz, 2);
			p->name = malloc(sizeof(char)*sz);
			TESTREAD(p->name, sz);
			TESTREAD(&p->nrhs, 4);
			p->rhs = malloc(sizeof(int)*p->nrhs);
			TESTREAD(p->rhs, (4*p->nrhs));
		}
	}
	/********** Read States ************/
	TESTREAD2(&config->nstates,4);
	if (config->nstates)
	{
		config->states = malloc(sizeof(_state)*config->nstates);
		for (i=0;i<config->nstates;i++)
		{
			short sz;
			TESTREAD(&sz, 2);
			config->states[i].name = malloc(sizeof(char)*sz);
			TESTREAD(config->states[i].name, sz);
			TESTREAD(&config->states[i].dfa_offset, 4);
		}
	}
	if (ob && bytes)
		free(ob);
	return 1;
}

// load a parser config from a file
int parser_load_config(const char* filename, parse_config* config)
{
	FILE* fin;
	char* buf;
	struct _stat st; int ret = 0;
	memset(config,0,sizeof(parse_config));
	fin = fopen(filename, "rb");
	if (!fin) return 0;
	_fstat(fin->_file, &st);
	buf = malloc(sizeof(char)*st.st_size);
	if (buf) if (fread(buf, st.st_size, 1, fin))
		if (parser_create_config(config, buf, st.st_size)) ret = 1;
	if (buf) free(buf);
	return ret;
};

// get the next token from the lexer
int parser_get_token(parse_config* cfg, parse_state* state)
{
	// save current line/column
	state->tok_col = state->column;
	state->tok_line = state->line;

	if (state->ileft<1) {
		if (!state->getbytes) return 0;
		state->getbytes(state);
		if (state->ileft<1) return 0;
	}
	while(1) {
		// loop until no more input or character doesn't match, or (v1.10) we find the shortest match
		while (state->ileft>0 && cfg->dfa->table[state->dfastate] && cfg->dfa->table[state->dfastate][*state->s]) {
			// adjust column/line count
			if (*state->s == '\r') state->column=1;
			else if (*state->s == '\n') state->line++;
			else state->column++;

			// save character to token buffer
			if (state->tleft>1) {
				state->tleft--;
				*state->p++ = *state->s;
			}

			// move to new state
			state->dfastate = cfg->dfa->table[state->dfastate][*state->s];
			state->s++;
			state->ileft--;

			// set accept state
			if (cfg->accept[state->dfastate] != -1) {
				state->accept_inst = cfg->accept[state->dfastate];
				state->accept_id = cfg->daction[state->accept_inst].accept_id;

				// take shortest match
				if (cfg->daction[state->accept_inst].flags & DFA_SHORTEST)
					break;
			}

			// check the input stream
			if (state->ileft<1) {
				*state->p = 0;
				if (state->getbytes)
					state->getbytes(state);
				if (state->ileft<1)
				{
					if (!state->accepteof)
						return 0;
					break;
				}
			}
		}

		// check for accept
		if (state->accept_id != -1)	{
			int accept_id = state->accept_id;
			int breturn = 0;

			// accept action
			if (cfg->daction[state->accept_inst].flags & DFA_ACCEPT) {
				// null-terminate token
				*state->p = 0;
				breturn = 1;
			}

			// goto action
			if (cfg->daction[state->accept_inst].flags & DFA_GOTO) {
				state->goto_state = cfg->daction[state->accept_inst].goto_state;
			} else
			if (cfg->daction[state->accept_inst].flags & DFA_PUSH) {
				_goto_push(&state->goto_stack, state->goto_state);
				state->goto_state = cfg->daction[state->accept_inst].goto_state;
			} else
			if (cfg->daction[state->accept_inst].flags & DFA_POP) {
				if (!state->goto_stack.ncount) return -2;
				state->goto_state = _goto_pop(&state->goto_stack);
			}

			// set new start index
			state->p = state->token;
			state->tleft = state->toklen;

			// reset dfa
			state->dfastate = state->goto_state;
			state->accept_id = -1;

			if (breturn)
				return accept_id;

			// update current line/column
			state->tok_col = state->column;
			state->tok_line = state->line;
		} else	{
			// we ran out of input
			if (state->ileft<1)
				return 0;
			// we didn't accept
			return -1;
		}
	}
}

void token_delete(token* t)
{
	token* next;
	while(t) {
		if (t->text)
			free(t->text);
		next = t->next;
		free(t);
		t = next;
	}
}

// get the next token from the lexer
token* parser_get_token_multi(parse_config* cfg, parse_state* state)
{
	struct token	*head=0, *tail;

	// save current line/column
	state->tok_col = state->column;
	state->tok_line = state->line;

	if (state->ileft<1) {
		if (!state->getbytes) return 0;
		state->getbytes(state);
		if (state->ileft<1) return 0;
	}
	while(1) {

		// update current line/column
		state->tok_col = state->column;
		state->tok_line = state->line;

		/* adjust line/column indices if we're not matching */
		if (state->ileft<=0 || !cfg->dfa->table[state->dfastate] || !cfg->dfa->table[state->dfastate][*state->s]) {
			if (*state->s == '\r') state->column=1;
			else if (*state->s == '\n') state->line++;
			else state->column++;
		}

		/* loop until no more input or character doesn't match, or (v1.10) we find the shortest match */
		while (state->ileft>0 && cfg->dfa->table[state->dfastate] && cfg->dfa->table[state->dfastate][*state->s]) {
			// adjust column/line count
			if (*state->s == '\r') state->column=1;
			else if (*state->s == '\n') state->line++;
			else state->column++;

			// save character to token buffer
			if (state->tleft>1) {
				state->tleft--;
				*state->p++ = *state->s;
			}

			// move to new state
			state->dfastate = cfg->dfa->table[state->dfastate][*state->s];
			state->s++;
			state->ileft--;

			// set accept state
			if (cfg->accept[state->dfastate] != -1) {
				state->accept_inst = cfg->accept[state->dfastate];
				state->accept_id = cfg->daction[state->accept_inst].accept_id;

				// take shortest match
				if (cfg->daction[state->accept_inst].flags & DFA_SHORTEST)
					break;
			}

			// check the input stream
			if (state->ileft<1) {
				*state->p = 0;
				if (state->getbytes)
					state->getbytes(state);
				if (state->ileft<1)
				{
					if (!state->accepteof)
						return 0;
					break;
				}
			}
		}

		// check for accept
		if (state->accept_id != -1)	{

			// goto action
			if (cfg->daction[state->accept_inst].flags & DFA_GOTO) {
				state->goto_state = cfg->daction[state->accept_inst].goto_state;
			} else
			if (cfg->daction[state->accept_inst].flags & DFA_PUSH) {
				_goto_push(&state->goto_stack, state->goto_state);
				state->goto_state = cfg->daction[state->accept_inst].goto_state;
			} else
			if (cfg->daction[state->accept_inst].flags & DFA_POP) {
				if (!state->goto_stack.ncount) return head;
				state->goto_state = _goto_pop(&state->goto_stack);
			}

			if (cfg->daction[state->accept_inst].flags & DFA_ACCEPT) {
				/* add to match list */
				token* t = (token*)malloc(sizeof(struct token));
				t->config = cfg;
				t->parser = state;
				t->length = state->toklen - state->tleft;
				t->text = (char*)memcpy(malloc(t->length+1), state->token, t->length);
				t->text[t->length] = 0;
				t->col = state->tok_col;
				t->line = state->tok_line;
				t->next = 0;
				t->id = state->accept_id;
				t->inst = state->accept_inst;
				if (!head) head=tail=t;
				else {tail->next=t;tail=t;}
			}

			// set new start index
			state->p = state->token;
			state->tleft = state->toklen;

			// reset dfa
			state->dfastate = state->goto_state;
			state->accept_id = -1;
		} else	{
			// we ran out of input
			if (state->ileft<1) return head;

			// we didn't accept, carry on
			state->s++;
			state->ileft--;
		}
	}

	return head;
}

// set the lexeme for the last reduced production
void parser_set_lexeme(parse_state* state, const char* lexeme)
{
	if (state->el.lexeme)
		free(state->el.lexeme);
	state->el.lexeme=0;
	if (lexeme)
		state->el.lexeme = _strdup(lexeme);
}

// set the user data for the last reduced production
void parser_set_userdata(parse_state* state, void* user_data)
{
	state->el.user_data = user_data;
}

// get the stack element for the last reduced production
parser_stack_el* parser_get_item(parse_state* state, int index)
{
	if (index<0||index>=(state->stack.ncount-state->stack_index))
		return 0;
	return &state->stack.el[state->stack_index+index];
}

// get the next rule
int parser_get_rule(parse_config* cfg, parse_state* state)
{
	int i;
	_lraction* a1;

	if (!cfg->nlraction)
		return parser_get_token(cfg,state);
	while(1)
	{
		if (state->breduction)
		{
			// replace reduction symbols with reduction symbol id
			state->el.id = state->lra->val;
			for (i=0;i<state->lra->reduce_count;i++)
			{
				if (i == state->lra->reduce_count - 1)
					state->el.state = _stack_back(&state->stack)->state;
				_stack_pop(&state->stack);
			}
			_stack_push(&state->stack, &state->el);
			state->el.lexeme=0;

			// set current state to state on top of stack
			if (state->lra->reduce_count>0)
				state->lrstate = state->el.state;

			// handle goto
			a1 = &cfg->lraction[state->lrstate][state->el.id];
			state->lrstate = a1->val;
			state->breduction = 0;
		}

		// get a new token if we need one
		if (state->curtok == -1 || state->curtok == 0)
			state->curtok = state->gettoken(cfg, state);
		if (state->curtok == -1)
			// lexer returned an error
			return -1;
		state->lra = &cfg->lraction[state->lrstate][state->curtok];
		switch(state->lra->type)
		{
		case -1: // grammatical error or ran out of input prematurely
			if (state->curtok == 0 && cfg->lraction[state->lrstate][0].type == -1)
				return -3;
			return -2;
		case LR_SHIFT:
			state->el.id = state->curtok;
			state->el.state = state->lrstate;
			if (state->el.lexeme) free(state->el.lexeme); state->el.lexeme=0;
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
			state->stack_index = state->stack.ncount - state->reduction_cnt;
			state->breduction = 1;

			// return reduction rule
			return state->reduction_inst;
		case LR_ACCEPT:
			return 0;
		case LR_GOTO:
			state->curtok = state->gettoken(cfg, state);
			state->lrstate = state->lra->val;
			break;
		}
	}
}
