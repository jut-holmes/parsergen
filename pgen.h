
#ifndef __PGEN_H_
#define __PGEN_H_

#undef min
#undef max
#undef TRUE
#undef FALSE

#define min(a,b) ((a<b)?(a):(b))
#define max(a,b) ((a>b)?(a):(b))
#define TRUE 1
#define FALSE 0


//#include <list>
#include <stdio.h>
#include "parser.h"
#include "containers.h"

struct config;
struct Node;

typedef _hashset<int> tset;
typedef _vector<int> tlookahead;
typedef _list<config*> tconfiglist;
typedef _hashmap<int, int> taccept;
typedef _vector<_vector<unsigned short> > _vdfa;
typedef _multimap<int,Node*> transmap;
typedef _hashset<Node*> nodeset;

extern int nNextNodeId;

#define ASSOC_UNK	0
#define ASSOC_NON	1
#define ASSOC_LEFT	2
#define ASSOC_RIGHT	3

#ifdef ALTERNATE
#undef ALTERNATE
#endif

// Special char types
#define E 0xffff
#define AC 0xfffe

// Node types
#define SIMPLE	0
#define ALTERNATE 1
#define CONCAT	2
#define KLEENE	3
#define ACCEPT	4

struct State;
struct Node
{
	Node() {type=SIMPLE;accept=0;important=false;accept_id=-1;id=nNextNodeId++;visited=0;}
	void addtrans(unsigned short c, Node* node) {transition.add(c, node);}
	char type;
	int visited;
	Node* accept;
	bool important;
	int id;
	int	accept_id;
	transmap transition;  // for nfa construction, includes E transitions
	transmap state_trans; // for dfa construction, no E transitions
	_vector<State*>	states;
	dfaaction	action;
};

#define SYMBOL_TYPE(name) ((name[0]=='<')?NONTERM:TERM)

struct symbolinst
{
	char	bdefined;	// is state defined?
	int		inst;		// instance
	Node*	nfa;		// nfa graph
	int		state;		// state
	dfaaction daction;	// action
	symbolinst* next;	// next instance
};

struct symbol
{
	int		id;
	char	type;
	char	hasE;
	char	marked;
	tset	first;
	char*	name;
	int		prec;
	int		assoc;
	char	bdefined;
	int		line;
	int		column;

	// for TERM only
	symbolinst* inst;	// the next instance of this symbol
};

struct rule
{
	char bdefined;	// is rule defined?
	char* name;		// name
	int inst;		// production rule instance

	symbol*			prec;	// ptr to precedence rule
	symbol*			left;	// ptr to lhs rule
	_vector<symbol*>	right;	// ptrs to rhs rules
	rule*			next_inst; // next instance of this rule
};

struct config
{
	rule* r;
	int dot;
	tlookahead lookahead;
	tconfiglist propagation_links;
	tconfiglist propagation_links_rev;
};

// used for dfa optimization
typedef _list<int> _listint;
struct _partition
{
	int id;
	bool modified;
	_listint states;
};

struct action
{
	enum {
		_SHIFT,
		_REDUCE,
		_ACCEPT,
		_GOTO
	} type;
	int val;
	int	reduce_prod;
	int reduce_cnt;
	int shift_symbol;
};

struct charset
{
	char* name;
	char set[32];
	enum {
		CS_ADD,
		CS_SUB
	} mode;
	charset* next;
};

// item set configuration tables
typedef _hashset<config*> tconfig;
typedef _hashmap<int, tconfig*> mtconfig;

// lalr output table
typedef _hashmap<int, _hashmap<int, action*> > taction;

struct parser
{
	// the start symbol
	symbol*	start_symbol;

	// input filename
	char*	input_file;

	// lalr grammar was defined in input file
	bool	bhave_grammar;

	// the output dfa
	_vdfa dfa;

	// the dfa accept states
	taccept dfaccept;

	// the dfa accept actions
	_vector<dfaaction*> actions;
	
	// the symbol list
	_vector<symbol*> symbols;
	
	// the rule list
	_vector<rule*> rules;
	
	// the state list
	_vector<char*> states;
	_vector<int>	  state_offsets;
	int	ncurstate;
	_vector<unsigned char>	state_defined;

	// the lalr dfa table
	taction lractions;

	// configurations
	mtconfig	configs;

	// character sets
	charset*	charsets;

	// parser state
	parse_state	pstate;

	// current precedence/associativity to assign to tokens
	int	ncur_precedence;
	int ncur_assoc;

	// error counting
	int nerrors;
	int nwarnings;

	// command line
	char* cmdline;
};

typedef struct
{
	bool bverbose;
	bool bshow_grammar_symbols;
	bool bshow_grammar_configs;
	bool bshow_grammar_actions;
	bool bshow_grammar_files;
	bool bshow_grammar_help;
	bool btest_grammar;
	bool btest_dfa;
	bool btest_dfa_multi;
	bool btake_file_options;
	bool bsave_header;
	bool bsave_cfile;
	bool boutput_parse_tables;
	bool boutput_grammar_file;
	bool boutput_parse_engine;
	bool boutput_regex_engine;
	bool bcompile_grammar_file;
	bool bread_grammar_file;
	bool bnon_case_sensitive;
	bool bsave_dfa;
	bool bsave_lalr;
	bool bsave_symbols;
	bool bsave_states;
	bool bcompress_output;
	bool bin_parser;
	bool bexit;
	bool bshow_usage;
	char input_file[512];
	char grammar_file[512];
	char test_file[512];
	char header_file[512];
	char parse_table_file[512];
	char parse_engine_file[512];
	char parse_regex_file[512];
	char c_file[512];
} options;
extern options opt;

//
// nfa functions
//
Node* create_node(unsigned short c);
Node* combine_branches(Node* n1, Node* n2);
Node* concat_nodes(Node* n1, Node* n2);
Node* alternate_nodes(Node* n1, Node* n2);
Node* copy_node(Node* n1);
Node* pos_close_node(Node* n1);
Node* opt_close_node(Node* n1);
Node* kleene_close_node(Node* n1);
Node* charset_to_nfa(charset* cs);
Node* strliteral_to_nfa(const char* strliteral);
Node* charliteral_to_nfa(const char* charliteral);
void output_nfa(char* filename, Node* nfa);

//
// dfa functions
//
void build_dfa(Node* nfa, _vdfa& dfa, taccept& accept);
void clear_visited(Node* node);
void add_dfa(_vdfa& dfa, Node* nfa, taccept& accept, int& offset);

//
// symbol functions
//

int	symbol_get_max_inst();

// add a symbol, error if already there
symbol* symbol_add(parser* p, char* name);

// get a symbol, add if not already created
symbol* symbol_get(parser* p, char* name);

// add a new symbol instance
symbolinst* symbol_add_inst(symbol* sym);

// get a symbol instance, add if not already there
symbolinst* symbol_get_inst(symbol* sym, int state);

// find a symbol by name
symbol* symbol_find_name(parser* p, char* name);

// find a symbol by id
symbol* symbol_find_id(parser* p, int id);

// find a symbol instance
symbolinst* symbol_find_inst(symbol* sym, int inst);

// find a symbol state
symbolinst* symbol_find_state(symbol* sym, int state);

//
// state functions
//

// get state id, add if not already created
int state_get(parser* p, const char* name);
int state_get_max(parser* p);
void state_define(parser* p, const char* name);
const char* state_name(parser* p, int id);
int state_set_offset(parser* p, const char* name, int offset);

//
// rule functions
//

// add a rule
rule* rule_get(parser* p, char* lhs_name);
rule* rule_find_name(parser* p, char* lhs_name);
rule* rule_find_inst(rule* r, int inst);
rule* rule_add(parser* p, char* lhs_name);
rule* rule_find_id(parser* p, int lhs_id);
rule* rule_next_inst(rule* r);
symbol* rule_add_rhs(parser* p, rule* r, char* rhs_name);
char* rule_make_name(rule* r);

// lalr functions
void build_lalr(parser* p);
void print_configs(parser* p);

// load and parse a grammar input file
bool loadfile(const char* filename, parser* p);

//
// character set functions
//
#define IsSet(set, bit)		(set[(bit >> 3)] & (1 << (bit % 8)))
void charset_build_default(parser* p);
charset* charset_add(parser* p, const char* name);
charset* charset_find(parser* p, const char* name);
charset* charset_combine(charset* dst, charset* src, char badd);
charset* charset_set(charset* dst, int ch, char bset);
charset* charset_set_range(charset* dst, int start, int end, char bset);

//
// error reporting
//
void report_error(parser* p, const char* format, ...);
void report_errorl(parser* p, int line, const char* format, ...);
void report_warning(parser* p, const char* format, ...);

//
// compressed files
//
typedef char (*compressed_cb)(const char* name);
char load_compressed(const char* filename, char** bytes, int* size);
char dir_compressed(compressed_cb cbfunc);

// parse command line arguments
bool read_command_line(const char* cmdline);

// the global path that was used to invoke the current instance
extern char app_path[];

// the global command line was used to invoke the current instance
extern char command_line[512];

const char* chstr(unsigned char c);
int chval(char** c);
int hexval(const char* c);
char escval(char c);

#endif
