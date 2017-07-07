
#include "pgen.h"
#include "pgen_grammar.h"
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef WIN32
#define _snprintf snprintf
#define _stat stat
#define _fstat fstat
#endif

void assign_precedence(parser* p, int rule_index)
{
	symbol* s = symbol_get(p, parser_get_item(&p->pstate,rule_index)->lexeme);
	s->line = parser_get_item(&p->pstate,rule_index)->line;
	s->column = parser_get_item(&p->pstate,rule_index)->column;
	if (s->prec != -1 && s->prec != p->ncur_precedence && s->assoc != p->ncur_assoc)
		report_warning(p, "token \"%s\" already has precedence defined, ignoring redefinition\n", s->name);
	else {
		s->prec = p->ncur_precedence;
		s->assoc = p->ncur_assoc;
	}
}

Node* charset_name_to_nfa(parser* p, const char* charset_name)
{
	charset* cs = charset_find(p, charset_name);
	if (!cs)
	{
		report_error(p, "charset \"%s\" doesn't exist\n", charset_name);
		return 0;
	}
	return charset_to_nfa(cs);
}

bool parse_input(parser* p, parse_config* cfg, parse_state* state)
{
	char		t;
	void		*tptr1, *tptr2;
	charset		br_charset;
	bool		bregex_error = false;
	charset*	cur_cs       = 0;
	rule*		cur_rule     = 0;
	symbol*		cur_symbol   = 0;
	symbolinst*	cur_sym_inst = 0;

	while(true)
	{
		switch(parser_get_rule(cfg,state))
		{

		/*******************************
		 *
		 *  Character Set Definition
		 *
		 *******************************/

		/*  charsetstart --> CHARSET    */
		case RULE_charsetstart_CHARSET_:
			// start new charset
			cur_cs = charset_add(p, parser_get_item(state,0)->lexeme);
			break;

		/*  charsetitem --> CHARSET    */
		case RULE_charsetitem_CHARSET_:
			{
			charset* ecs = charset_find(p,parser_get_item(state,0)->lexeme);
			if (!ecs) {
				report_error(p,"character set %s doesn't exist\n", parser_get_item(state,0)->lexeme);
				bregex_error = true;
			} else
				charset_combine(cur_cs, ecs, cur_cs->mode==charset::CS_ADD);
			}
			break;

		/*  charsetitem --> CHARRANGE    */
		case RULE_charsetitem_CHARRANGE_:
			{
			char* p = parser_get_item(state,0)->lexeme+1;
			int mode = cur_cs->mode;
			if (*p =='^') {p++;mode=cur_cs->CS_SUB;}
			int lastch = -1;
			while(*p != ']') {
				if (lastch != -1 && p[0] == '-' && p[1] != ']') {
					// range
					p++;
					int nextch = chval(&p);
					charset_set_range(cur_cs,lastch,nextch,cur_cs->mode==charset::CS_ADD);
				} else
					charset_set(cur_cs,lastch=chval(&p),cur_cs->mode==charset::CS_ADD);
			}
			cur_cs->mode = (mode==charset::CS_ADD)? charset::CS_ADD : charset::CS_SUB;
			}
			break;

		/*  charsetitem --> ESCCHAR    */
		case RULE_charsetitem_ESCCHAR_:
			charset_set(cur_cs, escval(parser_get_item(state,0)->lexeme[1]), cur_cs->mode==charset::CS_ADD);
			break;

		/*  charsetitem --> CHARUVAL    */
		case RULE_charsetitem_CHARUVAL_:
			charset_set(cur_cs, atoi(parser_get_item(state,0)->lexeme+2), cur_cs->mode==charset::CS_ADD);
			break;

		/*  <charsetitem> --> HEXCHAR    */
		case RULE_charsetitem_HEXCHAR_:
			{
			int val;
			sscanf(parser_get_item(state,0)->lexeme+2, "%x", &val);
			charset_set(cur_cs, val, cur_cs->mode==charset::CS_ADD);
			}
			break;

		/*  <charsetitem> --> CHARLITERAL    */
		case RULE_charsetitem_CHARLITERAL_:
			charset_set(cur_cs, parser_get_item(state,0)->lexeme[1], cur_cs->mode==charset::CS_ADD);
			break;

		/*  <charsetitem> --> '+'    */
		case RULE_charsetitem_PLUS_:
			cur_cs->mode = charset::CS_ADD;
			break;

		/*  <charsetitem> --> '-'    */
		case RULE_charsetitem_MINUS_:
			cur_cs->mode = charset::CS_SUB;
			break;

		/*******************************
		 *
		 *  Terminal Definition
		 *
		 *******************************/

		/*  termexpr --> termstart regex    */
		case RULE_termexpr_termstart_regex_:
			{
			if (!cur_symbol) break;
			Node* nfa = (Node*)parser_get_item(state,1)->user_data;
			if (!nfa || bregex_error) {
				report_error(p, "symbol \"%s\" not added to dfa\n", cur_symbol->name);
				break;
			}
			/* add and define new instance */
			cur_sym_inst = symbol_add_inst(cur_symbol);
			cur_sym_inst->state = p->ncurstate;
			cur_sym_inst->bdefined = TRUE;
			cur_sym_inst->nfa = nfa;
			cur_sym_inst->daction.accept_id = cur_symbol->id;
			cur_sym_inst->nfa->accept->accept_id = cur_sym_inst->inst;
			}
			break;

		/*  termexpr --> termstart    */
		case RULE_termexpr_termstart_:
			/* not needed, symbol is already marked as defined */
			break;

		/*  termstart --> TERM    */
		case RULE_termstart_TERM_:
			{
			bregex_error = false;
			cur_symbol = symbol_get(p,parser_get_item(state,0)->lexeme);
			cur_symbol->line = parser_get_item(state,0)->line;
			cur_symbol->column = parser_get_item(state,0)->column;
			symbolinst* inst = symbol_find_state(cur_symbol, p->ncurstate);
			if (!inst || !inst->bdefined)
			{
				cur_symbol->bdefined = TRUE;
				parser_set_lexeme(state, parser_get_item(state,0)->lexeme);
			} else
			{
				report_warning(p, "token \"%s\" already defined in state %d, ignoring redefinition\n", cur_symbol->name, p->ncurstate);
				cur_symbol = 0;
				cur_sym_inst = 0;
			}
			}
			break;

		/*******************************
		 *
		 *  Action Declaration
		 *
		 *******************************/

		/*  <action> --> ACCEPT    */
		case RULE_action_ACCEPT_:
			if (!cur_sym_inst) break;
			cur_sym_inst->daction.flags |= DFA_ACCEPT;
			break;

		/*  <action> --> PUSH STATE_NAME    */
		case RULE_action_PUSH_STATE__NAME_:
			if (!cur_sym_inst) break;
			if (cur_sym_inst->daction.flags & DFA_GOTO || cur_sym_inst->daction.flags & DFA_POP)
				report_error(p, "invalid mix of actions for terminal symbol \"%s\"\n", cur_symbol->name);
			else
			{
				cur_sym_inst->daction.flags |= DFA_PUSH;
				cur_sym_inst->daction.goto_state = state_get(p, parser_get_item(state,1)->lexeme);
			}
			break;

		/*  <action> --> POP    */
		case RULE_action_POP_:
			if (!cur_sym_inst) break;
			if (cur_sym_inst->daction.flags & DFA_PUSH || cur_sym_inst->daction.flags & DFA_GOTO)
				report_error(p, "invalid mix of actions for terminal symbol \"%s\"\n", cur_symbol->name);
			else
			{
				cur_sym_inst->daction.flags |= DFA_POP;
			}
			break;

		/*  <action> --> GOTO STATE_NAME    */
		case RULE_action_GOTO_STATE__NAME_:
			if (!cur_sym_inst) break;
			if (cur_sym_inst->daction.flags & DFA_PUSH || cur_sym_inst->daction.flags & DFA_POP)
				report_error(p, "invalid mix of actions for terminal symbol \"%s\"\n", cur_symbol->name);
			else
			{
				cur_sym_inst->daction.flags |= DFA_GOTO;
				cur_sym_inst->daction.goto_state = state_get(p, parser_get_item(state,1)->lexeme);
			}
			break;

		/*  action --> CONCAT    */
		case RULE_action_CONCAT_:
			if (!cur_sym_inst) break;
			cur_sym_inst->daction.flags |= DFA_CONCAT;
			break;

		/*  action --> SHORT    */
		case RULE_action_SHORT_:
			if (!cur_sym_inst) break;
			cur_sym_inst->daction.flags |= DFA_SHORTEST;
			break;

		case RULE_action_IGNORE_:	/*  action --> IGNORE    */
		case RULE_action_DISCARD_:	/*  action --> DISCARD    */
			if (!cur_sym_inst) break;
			cur_sym_inst->daction.flags &= ~DFA_ACCEPT;
			break;

		/*  <action> --> PRESERVE    */
		case RULE_action_PRESERVE_:
			if (!cur_sym_inst) break;
			cur_sym_inst->daction.flags |= DFA_PRESERVE;
			break;

		/*******************************
		 *
		 *  Non-Terminal Definition
		 *
		 *******************************/

		/*  <nontermstart> --> NONTERM ASSIGN    */
		case RULE_nontermstart_NONTERM_ASSIGN_:
			{
			char* name = parser_get_item(state,0)->lexeme;
			cur_rule = rule_add(p, name);
			cur_rule->bdefined = TRUE;
			cur_symbol = cur_rule->left;
			cur_symbol->bdefined = TRUE;
			if (!p->start_symbol)
				p->start_symbol = cur_symbol;
			}
			p->bhave_grammar = true;
			break;

		/*  <nament> --> TERM    */
		case RULE_nament_TERM_:
			{
			char* name = parser_get_item(state,0)->lexeme;
			symbol* rhs = rule_add_rhs(p, cur_rule, name);
			rhs->line = parser_get_item(state,0)->line;
			rhs->column = parser_get_item(state,0)->column;
			if (name[0] == '\'') // only add state instance for literal terminals( eg. ';' )
				symbol_get_inst(rhs, p->ncurstate);

			// assign precedence
			if (!cur_rule->prec) cur_rule->prec = rhs;
			}
			break;

		/*  <nament> --> NONTERM    */
		case RULE_nament_NONTERM_:
			{
			symbol* rhs = rule_add_rhs(p, cur_rule, parser_get_item(state,0)->lexeme);
			rhs->line = parser_get_item(state,0)->line;
			rhs->column = parser_get_item(state,0)->column;
			}
			break;

		/*  <prodalternate> --> '|'    */
		case RULE_prodalternate_VERT_BAR_:
			cur_rule = rule_add(p, cur_symbol->name);
			cur_rule->bdefined = TRUE;
			break;

		/*  <optprec> --> PREC TERM    */
		case RULE_optprec_PREC_TERM_:
			{
			cur_rule->prec = symbol_get(p, parser_get_item(state,1)->lexeme);
			cur_rule->prec->line = parser_get_item(state,1)->line;
			cur_rule->prec->column = parser_get_item(state,1)->column;
			}
			break;

		/*******************************
		 *
		 *  Start Symbol Declaration
		 *
		 *******************************/

		/*  <keyword> --> START START_SYMBOL    */
		case RULE_keyword_START_START__SYMBOL_:
			{
			char buf[512];
			_snprintf(buf, 512, "<%s>", parser_get_item(state,1)->lexeme);
			p->start_symbol = symbol_get(p, buf);
			p->start_symbol->line = parser_get_item(state,1)->line;
			p->start_symbol->column = parser_get_item(state,1)->column;
			}
			break;

		/*******************************
		 *
		 *  State Declaration
		 *
		 *******************************/

		/*  <keyword> --> STATE STATE_NAME    */
		case RULE_keyword_STATE_STATE__NAME_:
			p->ncurstate = state_get(p, parser_get_item(state,1)->lexeme);
			state_define(p, parser_get_item(state,1)->lexeme);
			break;

		/*****************************************
		 *
		 *  Command line Declaration
		 *
		 *****************************************/

		/*  <keyword> --> PARAM PARAMS    */
		case RULE_keyword_PARAM_PARAMS_:
			if (opt.btake_file_options) {
				// add a dummy first parameter
				parser_stack_el* pse = parser_get_item(state, 0);
				char* params = parser_get_item(state,1)->lexeme;
				char* buf = new char[strlen(params)+3];
				strcpy(buf, "a "); strcat(buf, params);
				bool bresult = read_command_line(buf);
				delete[] buf;
				if (!bresult) {
					report_error(p, "error in embedded parameter arguments\n");
					break;
				}
			}
			break;

		/*****************************************
		 *
		 *  Precedence/Associativity Declaration
		 *
		 *****************************************/

		/*  <kwleft> --> <kwleftstart> <kwleftexpr>    */
		case RULE_kwleft_kwleftstart_kwleftexpr_:
			p->ncur_precedence++;
			break;

		/*  <kwright> --> <kwrightstart> <kwrightexpr>    */
		case RULE_kwright_kwrightstart_kwrightexpr_:
			p->ncur_precedence++;
			break;

		/*  <kwnonassoc> --> <kwnonassocstart> <kwnonassocexpr>    */
		case RULE_kwnonassoc_kwnonassocstart_kwnonassocexpr_:
			break;

		case RULE_kwleftexpr_TOKENNAME_:			/*  kwleftexpr --> TOKENNAME    */
		case RULE_kwrightexpr_TOKENNAME_:			/*  kwrightexpr --> TOKENNAME    */
			assign_precedence(p, 0);
			break;

		case RULE_kwleftexpr_kwleftexpr_TOKENNAME_:		/*  kwleftexpr --> kwleftexpr TOKENNAME    */
		case RULE_kwrightexpr_kwrightexpr_TOKENNAME_:	/*  kwrightexpr --> kwrightexpr TOKENNAME    */
			assign_precedence(p, 1);
			break;

		/*  kwleftstart --> LEFT    */
		case RULE_kwleftstart_LEFT_:
			p->ncur_assoc = ASSOC_LEFT;
			break;

		/*  kwrightstart --> RIGHT    */
		case RULE_kwrightstart_RIGHT_:
			p->ncur_assoc = ASSOC_RIGHT;
			break;

		/*  kwnonassocstart --> NONASSOC    */
		case RULE_kwnonassocstart_NONASSOC_:
			p->ncur_assoc = ASSOC_NON;
			break;

		/*******************************
		 *
		 *  Regex Declaration
		 *
		 *******************************/

		/*  regex --> EREbranch    */
		case RULE_regex_EREbranch_:
			parser_set_userdata(state,parser_get_item(state,0)->user_data);
			break;

		/*  regex --> regex EREAlternate EREbranch    */
		case RULE_regex_regex_EREAlternate_EREbranch_:
			{
			Node* n1 = (Node*)parser_get_item(state,0)->user_data;
			Node* n2 = (Node*)parser_get_item(state,2)->user_data;
			parser_set_userdata(state, alternate_nodes(n1, n2));
			}
			break;

		//
		// branch
		//

		/*  EREbranch --> EREbranch EREexpression    */
		case RULE_EREbranch_EREbranch_EREexpression_:
			{
			Node* n1 = (Node*)parser_get_item(state,0)->user_data;
			Node* n2 = (Node*)parser_get_item(state,1)->user_data;
			parser_set_userdata(state, concat_nodes(n1, n2));
			}
			break;

		/*  EREbranch --> EREexpression    */
		case RULE_EREbranch_EREexpression_:
			parser_set_userdata(state, parser_get_item(state,0)->user_data);
			break;

		//
		// expression
		//

		/*  EREexpression --> onecharacterERE    */
		case RULE_EREexpression_onecharacterERE_:
			parser_set_userdata(state, parser_get_item(state,0)->user_data);
			break;

		/*  <EREexpression> --> '^'    */
		case RULE_EREexpression_CARET_:
			parser_set_userdata(state, create_node((unsigned short)parser_get_item(state,0)->lexeme[0]));
			break;

		/*  <EREexpression> --> '$'    */
		case RULE_EREexpression_STRING_:
			parser_set_userdata(state,create_node((unsigned short)parser_get_item(state,0)->lexeme[0]));
			break;

		/*  <EREexpression> --> '(' <regex> ')'    */
		case RULE_EREexpression_LEFT_PAREN_regex_RIGHT_PAREN_:
			parser_set_userdata(state, parser_get_item(state,1)->user_data);
			break;

		/*  EREexpression --> EREexpression EREduplsymbol    */
		case RULE_EREexpression_EREexpression_EREduplsymbol_:
			{
			char* dup = parser_get_item(state,1)->lexeme;
			Node* nfa = (Node*)parser_get_item(state, 0)->user_data;
			if (!nfa) break;
			switch(*dup)
			{
			case '+': nfa = pos_close_node(nfa);	parser_set_userdata(state, nfa); break;
			case '*': nfa = kleene_close_node(nfa);	parser_set_userdata(state, nfa); break;
			case '?': nfa = opt_close_node(nfa);	parser_set_userdata(state, nfa); break;
			default:
				{
				int i;
				char* p = dup;
				int min = atoi(p);
				if (min<0) min=0;
				Node* new_nfa=0;
				for (i=0;i<min;i++)
					new_nfa=new_nfa?concat_nodes(new_nfa, copy_node(nfa)):copy_node(nfa);
				while(*p && *p != ',') p++;
				if (*p == ',' && p[1])
				{
					// {1,3}
					int max = atoi(p+1);
					if (max<min) max=min;
					for (int j=i;j<max;j++)
						new_nfa=new_nfa?concat_nodes(new_nfa, opt_close_node(copy_node(nfa))):opt_close_node(copy_node(nfa));
				} else
				if (*p == ',')
				{
					// {1,}
					new_nfa=new_nfa?concat_nodes(new_nfa, kleene_close_node(copy_node(nfa))):kleene_close_node(copy_node(nfa));
				} else
				{
					// {5}
					// Do nothing, already taken care of
				}
				parser_set_userdata(state, new_nfa?new_nfa:create_node(E));
				}
				break;
			
			}
			}
			break;

		//
		// onecharacterERE
		//

		/*  onecharacterERE --> bracketExpression    */
		case RULE_onecharacterERE_bracketExpression_:
			parser_set_userdata(state, parser_get_item(state,0)->user_data);
			break;

		/*  onecharacterERE --> collelem    */
		case RULE_onecharacterERE_collelem_:
			tptr1 = parser_get_item(state,0)->user_data;
			switch((int)tptr1) {
			case -1: parser_set_userdata(state, charset_to_nfa(charset_find(p, "{Word}"))); break;
			case -11:parser_set_userdata(state, charset_to_nfa(charset_find(p, "{NotWord}"))); break;
			case -2: parser_set_userdata(state, charset_to_nfa(charset_find(p, "{Whitespacenl}"))); break;
			case -12:parser_set_userdata(state, charset_to_nfa(charset_find(p, "{NotWhitespacenl}"))); break;
			case -3: parser_set_userdata(state, charset_to_nfa(charset_find(p, "{Digit}"))); break;
			case -13:parser_set_userdata(state, charset_to_nfa(charset_find(p, "{NotDigit}"))); break;
			default: parser_set_userdata(state, create_node((unsigned short)tptr1)); break;
			}
			break;

		/*  <onecharacterERE> --> '.'    */
		case RULE_onecharacterERE_PERIOD_:
			parser_set_userdata(state,charset_name_to_nfa(p, "{Notnl}"));
			break;

		/*  onecharacterERE --> CHARSETREF    */
		case RULE_onecharacterERE_CHARSETREF_:
			parser_set_userdata(state,charset_name_to_nfa(p, parser_get_item(state,0)->lexeme));
			break;

		/*  onecharacterERE --> STRLITERAL    */
		case RULE_onecharacterERE_STRLITERAL_:
			parser_set_userdata(state,strliteral_to_nfa(parser_get_item(state,0)->lexeme));
			break;

		//
		// duplication symbol
		//

		/*  <EREduplsymbol> --> '*'    */
		case RULE_EREduplsymbol_STAR_:
			parser_set_lexeme(state,"*");
			break;

		/*  <EREduplsymbol> --> '+'    */
		case RULE_EREduplsymbol_PLUS_:
			parser_set_lexeme(state,"+");
			break;

		/*  <EREduplsymbol> --> '?'    */
		case RULE_EREduplsymbol_QUESTION_:
			parser_set_lexeme(state,"?");
			break;

		/*  EREduplsymbol --> RANGEREF    */
		case RULE_EREduplsymbol_RANGEREF_:
			{
			char buf[256];
			strcpy(buf, parser_get_item(state,0)->lexeme+1);
			buf[strlen(buf)-1]=0;
			parser_set_lexeme(state, buf);
			}
			break;

		//
		// bracket expression
		//

		/*  <bracketExpression> --> <brackExprStart> <matchinglist> ']'    */
		case RULE_bracketExpression_brackExprStart_matchinglist_CLOSE_BRACKET_:
			parser_set_userdata(state, charset_to_nfa(&br_charset));
			break;

		/*  <bracketExpression> --> <brackExprStart> <nonmatchinglist> ']'    */
		case RULE_bracketExpression_brackExprStart_nonmatchinglist_CLOSE_BRACKET_:
			parser_set_userdata(state, charset_to_nfa(&br_charset));
			break;

		/*  <brackExprStart> --> '['    */
		case RULE_brackExprStart_OPEN_BRACKET_:
			// New bracket expression
			memset(&br_charset,0,sizeof(br_charset));
			br_charset.mode = charset::CS_ADD;
			break;

		/*  <nonmatchinglist> --> '^' <bracketList>    */
		case RULE_nonmatchinglist_CARET_bracketList_:
			// Invert the bracket expression character set
			for (int i=0;i<32;i++)
				br_charset.set[i] = ~br_charset.set[i];
			break;

		/*  rangeExpression --> startRange endRange    */
		case RULE_rangeExpression_startRange_endRange_:
			tptr1 = parser_get_item(state,0)->user_data;
			tptr2 = parser_get_item(state,1)->user_data;
			if (tptr1 == (void*)-1 || tptr1 == (void*)-2 || tptr1 == (void*)-3 || tptr1 == (void*)-11 || tptr1 == (void*)-12 || tptr1 == (void*)-13) {
				report_error(p, "syntax error near line %d, column %d: start of range expression cannot use class specifier\n", state->line, state->column);
				return false;
			} else
			if (tptr2 == (void*)-1 || tptr2 == (void*)-2 || tptr2 == (void*)-3 || tptr2 == (void*)-11 || tptr2 == (void*)-12 || tptr2 == (void*)-13) {
				report_error(p, "syntax error near line %d, column %d: end of range expression cannot use class specifier\n", state->line, state->column);
				return false;
			}
			charset_set_range(&br_charset, (int)tptr1, (int)tptr2, TRUE);
			break;

		/*  <expressionTerm> --> SPACE    */
		case RULE_expressionTerm_SPACE_:
			charset_set(&br_charset, ' ', TRUE);
			break;

		/*  <startRange> --> <collelem> '-'    */
		case RULE_startRange_collelem_MINUS_:
			parser_set_userdata(state, parser_get_item(state,0)->user_data);
			break;

		/*  endRange --> collelem    */
		case RULE_endRange_collelem_:
			parser_set_userdata(state, parser_get_item(state,0)->user_data);
			break;

		/*  singleExpression --> endRange    */
		case RULE_singleExpression_endRange_:
			tptr1 = parser_get_item(state,0)->user_data;
			if (tptr1 == (void*)-1) {
				charset_combine(&br_charset, charset_find(p, "{Word}"), TRUE);
			} else
			if (tptr1 == (void*)-2) {
				charset_combine(&br_charset, charset_find(p, "{Whitespacenl}"), TRUE);
			} else
			if (tptr1 == (void*)-3) {
				charset_combine(&br_charset, charset_find(p, "{Digit}"), TRUE);
			} else
			if (tptr1 == (void*)-11) {
				charset_combine(&br_charset, charset_find(p, "{NotWord}"), TRUE);
			} else
			if (tptr1 == (void*)-12) {
				charset_combine(&br_charset, charset_find(p, "{NotWhitespacenl}"), TRUE);
			} else
			if (tptr1 == (void*)-13) {
				charset_combine(&br_charset, charset_find(p, "{NotDigit}"), TRUE);
			} else
				charset_set(&br_charset, (int)tptr1, TRUE);
			break;

		/*  singleExpression --> characterClass    */
		case RULE_singleExpression_characterClass_:
			{
			const char* csname = parser_get_item(state,1)->lexeme;
			charset* cs = charset_find(p, csname);
			if (cs)
				charset_combine(&br_charset, cs, TRUE);
			else
				report_error(p, "character set %s does not exist\n", csname);
			}
			break;

		/*  <characterClass> --> '[:' CLASSNAME ':]'    */
		case RULE_characterClass_OPEN_BRACKET_COLON_CLASSNAME_COLON_CLOSE_BRACKET_:
			{
			char buf[256];
			_snprintf(buf,256,"{%s}",parser_get_item(state,1)->lexeme);
			parser_set_lexeme(state,buf);
			}
			break;

		//
		// collelem
		//

		/*  collelem --> ORDCHAR    */
		case RULE_collelem_ORDCHAR_:
			parser_set_userdata(state, (void*)parser_get_item(state,0)->lexeme[0]);
			break;
		
		case RULE_collelem_QUOTEDCHAR_:		/*  collelem --> QUOTEDCHAR    */
		case RULE_collelem_BSCHAR_:			/*  collelem --> BSCHAR    */
			t = parser_get_item(state,0)->lexeme[1];
			switch(t) {
			case 'w': parser_set_userdata(state,(void*)-1); break; // {Word}
			case 'W': parser_set_userdata(state,(void*)-11);break; // {^Word}
			case 's': parser_set_userdata(state,(void*)-2);	break; // {Whitespacenl}
			case 'S': parser_set_userdata(state,(void*)-12);break; // {^Whitespacenl}
			case 'd': parser_set_userdata(state,(void*)-3);	break; // {Digit}
			case 'D': parser_set_userdata(state,(void*)-13);break; // {^Digit}
			default:  parser_set_userdata(state, (void*)escval(t));	break;
			}
			break;

		/*  collelem --> OCTCHAR    */
		case RULE_collelem_OCTCHAR_:
			{
			int val;
			sscanf("%o", parser_get_item(state,0)->lexeme+2, &val);
			parser_set_userdata(state, (void*)val);
			}
			break;

		/*  collelem --> HEXCHAR    */
		case RULE_collelem_HEXCHAR_:
			{
			int val;
			int nconv = sscanf(parser_get_item(state,0)->lexeme+2, "%x", &val);
			parser_set_userdata(state, (void*)val);
			}
			break;

		/*  collelem --> CTLCHAR    */
		case RULE_collelem_CTLCHAR_:
			parser_set_userdata(state, (void*)((parser_get_item(state,0)->lexeme[2] - 'A') + 1));
			break;

		/*  collelem --> USCHAR    */
		case RULE_collelem_USCHAR_:
			{
			int val;
			sscanf(parser_get_item(state,0)->lexeme+2, "%u", &val);
			parser_set_userdata(state, (void*)val);
			}
			break;

		case 0:  // accepted
			return true;
		case -1: // syntax error
			report_error(p, "syntax error near line %d, column %d\n", state->line, state->column);
			report_error(p, "found character '%c', expecting one of the following:\n", *state->s);
			report_error(p, "");
			for (int i=0;i<255;i++)	{
				if (cfg->dfa->table[state->dfastate][i])
					printf("%s ", chstr(i));
			}
			printf("\n");
			return false;
		case -2: // grammar error
			report_error(p, "grammatical error near line %d, column %d\n", state->tok_line, state->tok_col);
			report_error(p, " Found token \"%s\", expecting one of the following:\n", state->token);
			report_error(p, "");
			for (int i=0;i<cfg->nsymbols;i++)
				if (cfg->lraction[state->lrstate][i].type != -1)
					printf("%s ", cfg->symbols[i].name);
			printf("\n");
			return false;
		default:
			break;
		}
	}
}

bool loadfile(const char* filename, parser* p)
{
	char token[256];
	parse_config cfg;

	char* data; int size;
	if (!load_compressed("pgen_grammar.dat", &data, &size)) {
		printf("application error 0x5c\n");
		return false;
	}

	// load parse tables
	if (!parser_create_config(&cfg,data,size))
	{
		printf("Error loading parse tables\n");
		return false;
	}

	// load grammar file
	FILE* fin = fopen(filename, "rb");
	if (!fin) {
		printf("Error opening grammar file \"%s\"\n", filename);
		parser_delete(&cfg,0);
		return false;
	}
	struct _stat st;
	_fstat(fin->_file, &st);
	char* binput = new char[st.st_size];
	fread(binput, st.st_size, 1, fin);
	fclose(fin);

	// init parser state
	if (opt.bverbose)
		printf("Reading grammar file...\n");
	parser_init_state(&p->pstate, binput, st.st_size, token, 256, 0);

	// parse grammar file
	opt.bin_parser = true;
	bool result = parse_input(p, &cfg, &p->pstate);
	opt.bin_parser = false;

	// clean up
	parser_delete(&cfg, &p->pstate);
	if (!result)
		return false;

	return true;
}
