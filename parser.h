
#ifndef __PARSER_H_
#define __PARSER_H_

#ifndef PARSER_VERSION
#define PARSER_VERSION 2
#endif

#include <sys/stat.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

//
// symbol types
//
#define TERM 0
#define NONTERM 1

// eof marker
#define _EOF_	0

//
// dfa action flags
//
#define DFA_IGNORE 0     // ignore token
#define DFA_ACCEPT 1     // accept
#define DFA_GOTO   2     // goto state
#define DFA_CONCAT 4     // don't reset token
#define DFA_PUSH   8     // push state
#define DFA_POP    16    // pop state
#define DFA_PRESERVE 32  // don't consume
#define DFA_SHORTEST 64  // shortest match

//
// dfa action struct
//

typedef struct dfaaction
{
	int	 accept_id;
	short flags;
	short goto_state;
} dfaaction;

//
// lr action types
//
#define LR_SHIFT	0
#define LR_REDUCE	1
#define LR_ACCEPT	2
#define LR_GOTO		3

//
// lr action struct
//
typedef struct _lraction
{
	char type;
	short val;
	short reduce_production;
	short reduce_count;
} _lraction;


//
// symbol
//
typedef struct symbol_
{
	char	type;
	char*	name;
} symbol_;

//
// production
//
typedef struct production
{
	char*	name;
	int		lhs;
	int		nrhs;
	int*	rhs;
} production;

//
// dfa tables
//
typedef struct _dfa
{
	unsigned short size;
	unsigned short table[1][256];
} _dfa;

// a parser stack element
typedef struct parser_stack_el
{
	int		id;
	int		state;
	int		line;
	int		column;
	char*	lexeme;
	void*	user_data;
} parser_stack_el;

//
// parser stack
//
typedef struct _parser_stack
{
	parser_stack_el* el;
	int		nsize;
	int		ncount;
} _parser_stack;

//
// goto stack
//
typedef struct _goto_stack
{
	int*	el;
	int		nsize;
	int		ncount;
} _goto_stack;

//
// state info
//
typedef struct _state
{
	char*	name;
	int		dfa_offset;
} _state;

//
// parser configuration
//
typedef struct parse_config
{
	// dfa state table
	_dfa* dfa;

	// size of dfa action table
	unsigned short ndaction;

	// dfa action table
	dfaaction* daction;
	int nrealactions;

	// dfa accept table
	short* accept;
	unsigned short nrealaccept; // number of non-void accepting states

	// lr state table
	int nlraction;
	_lraction** lraction;

	// symbols
	int nsymbols;
	symbol_* symbols;

	// states
	int nstates;
	_state*	states;

	// productions
	int nproductions;
	production* productions;
} parse_config;

typedef void (*_cbgetbytes)(struct parse_state* state);
typedef int (*_cbgettoken)(parse_config* cfg, struct parse_state* state);

//
// parser state
//
typedef struct parse_state
{
	// user data
	void* user;

	// input buffer
	char*	input;
	int		inputlen;
	int		ileft;
	int     accepteof;

	// token output buffer
	char*	token;
	int		toklen;
	int		tok_line;
	int		tok_col;

	// data fetch callback
	_cbgetbytes getbytes;

	// lexer callback
	_cbgettoken gettoken;

	// current state
	char* p;
	const char* s;
	int tleft;
	_goto_stack goto_stack;
	int goto_state;
	int accept_inst;
	int accept_id;
	int dfastate;
	int curtok;
	int	line;
	int column;

	_parser_stack stack;		// parse stack
	parser_stack_el el;			// current stack element
	int lrstate;				// current lr state
	_lraction* lra;				// current parser action

	// the following items are valid directly after a reduction
	char breduction;			// in reduction
	int	reduction_symbol;		// lhs symbol of reduction
	int	reduction_inst;			// instance of reduction rule
	int reduction_cnt;			// number of rhs symbols in production being reduced
	int stack_index;			// offset from beginning of stack
} parse_state;


/* multi-match flags */
#define MF_SHORTEST 1	/* take shortest match(greedy is default) */

/* parser_get_token_multi() return type */
typedef struct token
{
	char*	text;
	int		length;

	int		id;
	int		inst;

	int		col;
	int		line;

	parse_config*	config;
	parse_state*	parser;

	struct token* next;
} token;

/* delete a token pointer returned from parser_get_token_multi() */
void token_delete(token* t);

// initialize a parser state
void parser_init_state(parse_state* state, char* input, int inputlen, char* token, int toklen, _cbgetbytes getbytes);

// reset the parser state
void parser_reset_state(parse_state* state);

// delete a parser config and/or a parser state
void parser_delete(parse_config* config, parse_state* state);

// create a parser config from an image in memory
int parser_create_config(parse_config* config, const void* bytes, int size);

// load a parser config from a file
int parser_load_config(const char* filename, parse_config* config);

// set the lexeme for the last reduced production
void parser_set_lexeme(parse_state* state, const char* lexeme);

// set the user data for the last reduced production
void parser_set_userdata(parse_state* state, void* user_data);

// get the stack element for the last reduced production
parser_stack_el* parser_get_item(parse_state* state, int index);

// get the next token from the lexer
int parser_get_token(parse_config* cfg, parse_state* state);

/* get an array of regex matches from the input */
token* parser_get_token_multi(parse_config* cfg, parse_state* state);

// get the next rule
int parser_get_rule(parse_config* cfg, parse_state* state);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
// a simple c++ parser class
class Parser {
public:
	Parser(const char* filename) {parser_load_config(filename,&cfg);state.el.lexeme=0;}
	Parser(const void* bytes, int len) {parser_create_config(&cfg,bytes,len);state.el.lexeme=0;}
	~Parser() {parser_delete(&cfg,&state);}
	void init(const void* bytes, int len,_cbgetbytes cb) {parser_init_state(&state,(char*)bytes,len,token,4096,cb);}
	char* get_lexeme(int idx) {return parser_get_item(&state,idx)->lexeme;}
	void* get_userdata(int idx) {return parser_get_item(&state,idx)->user_data;}
	void set_lexeme(const char* lexeme) {parser_set_lexeme(&state,lexeme);}
	void set_userdata(void* user_data) {parser_set_userdata(&state,user_data);}
	int parse() {return parser_get_rule(&cfg,&state);}
	parse_config cfg;
	parse_state state;
	char token[4096];
};
#endif

#endif
