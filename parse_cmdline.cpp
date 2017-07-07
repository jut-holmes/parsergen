
#include "pgen.h"
#include "cmdline_grammar.h"

void write_grammar_file(const char* filename);
extern unsigned char cmdline_grammar[];

char* strip_quotes(char* fn)
{
	if (*fn == '"')
	{
		int len = strlen(fn)-2;
		memmove(fn,fn+1,len);
		fn[len]=0;
	}
	return fn;
}

bool read_command_line(const char* cmdline)
{
	extern unsigned int cmdline_grammar_size;
	Parser p(cmdline_grammar,cmdline_grammar_size);
	p.init(cmdline, strlen(cmdline), 0);

	while(true)
	{
		switch(p.parse())
		{
		/*  <cmdline> -->    */
		case RULE_cmdline_:
			opt.bshow_usage = true;
			break;

		/*  <cmdline> --> FILENAME    */
		case RULE_cmdline_FILENAME_:
			{
			strcpy(opt.input_file, p.get_lexeme(0));
			strip_quotes(opt.input_file);
			strcpy(opt.grammar_file, opt.input_file);
			opt.btake_file_options = true;
			opt.bcompile_grammar_file = true;
			opt.boutput_grammar_file = true;
			opt.bverbose = true;
			char* ext = strrchr(opt.grammar_file, '.');
			if (ext) strcpy(ext, ".dat"); else strcat(opt.grammar_file,".dat");
			}
			break;

		/*  <testswitch> --> '-t' FILENAME    */
		case RULE_testswitch_MINUSt_FILENAME_:
			strcpy(opt.test_file, p.get_lexeme(1));
			strip_quotes(opt.test_file);
			opt.btest_grammar = true;
			break;

		/*  <testswitch> --> '-td' FILENAME    */
		case RULE_testswitch_MINUStd_FILENAME_:
			strcpy(opt.test_file, p.get_lexeme(1));
			strip_quotes(opt.test_file);
			opt.btest_dfa = true;
			break;

		/*  <testswitch> --> '-tr' FILENAME    */
		case RULE_testswitch_MINUStr_FILENAME_:
			strcpy(opt.test_file, p.get_lexeme(1));
			strip_quotes(opt.test_file);
			opt.btest_dfa_multi = true;
			break;

		/*  <switch> --> '-v'    */
		case RULE_switch_MINUSv_:
			opt.bverbose = true;
			break;

		/*  <inswitch> --> '-it' FILENAME    */
		case RULE_inswitch_MINUSit_FILENAME_:
			opt.bcompile_grammar_file = true;
			strcpy(opt.input_file, p.get_lexeme(1));
			strip_quotes(opt.input_file);
			break;

		/*  <inswitch> --> '-ig' FILENAME    */
		case RULE_inswitch_MINUSig_FILENAME_:
			opt.bread_grammar_file = true;
			strcpy(opt.input_file, p.get_lexeme(1));
			strip_quotes(opt.input_file);
			break;

		/*  <outswitch> --> '-og' FILENAME    */
		case RULE_outswitch_MINUSog_FILENAME_:
			opt.boutput_grammar_file = true;
			strcpy(opt.grammar_file, p.get_lexeme(1));
			strip_quotes(opt.grammar_file);
            break;

		/*  <outswitch> --> '-oh' FILENAME    */
		case RULE_outswitch_MINUSoh_FILENAME_:
			opt.bsave_header = true;
			strcpy(opt.header_file, p.get_lexeme(1));
			strip_quotes(opt.header_file);
			break;

		/*  <outswitch> --> '-oc' FILENAME    */
		case RULE_outswitch_MINUSoc_FILENAME_:
			opt.bsave_cfile = true;
			strcpy(opt.c_file, p.get_lexeme(1));
			strip_quotes(opt.c_file);
			break;

		/*  <outswitch> --> '-op' FILENAME    */
		case RULE_outswitch_MINUSop_FILENAME_:
			opt.boutput_parse_tables=true;
			strcpy(opt.parse_table_file, p.get_lexeme(1));
			strip_quotes(opt.parse_table_file);
			break;

		/*  <outswitch> --> '-oe' FILENAME    */
		case RULE_outswitch_MINUSoe_FILENAME_:
			opt.boutput_parse_engine = true;
			strcpy(opt.parse_engine_file, p.get_lexeme(1));
			strip_quotes(opt.parse_engine_file);
			break;

		/*  <outswitch> --> '-ore' FILENAME    */
		case RULE_outswitch_MINUSore_FILENAME_:
			opt.boutput_regex_engine = true;
			strcpy(opt.parse_regex_file, p.get_lexeme(1));
			strip_quotes(opt.parse_regex_file);
			break;

		/*  <outflags> --> '-nd'    */
		case RULE_outflags_MINUSnd_:
			opt.bsave_dfa = false;
			break;

		/*  <outflags> --> '-nl'    */
		case RULE_outflags_MINUSnl_:
			opt.bsave_lalr = false;
			break;

		/*  <outflags> --> '-ns'    */
		case RULE_outflags_MINUSns_:
			opt.bsave_symbols = false;
			break;

		/*  <outflags> --> '-dc'    */
		case RULE_outflags_MINUSdc_:
			opt.bcompress_output = false;
			break;

		/*  <switch> --> '-nc'    */
		case RULE_switch_MINUSnc_:
			if (opt.bnon_case_sensitive == false) {
				opt.bnon_case_sensitive = true;
				printf("Entering non case-sensitive mode\n");
			}
			break;

		/*  <switch> --> '-cs'    */
		case RULE_switch_MINUScs_:
			if (opt.bnon_case_sensitive == true) {
				opt.bnon_case_sensitive = false;
				printf("Leaving non case-sensitive mode\n");
			}
			break;

		/*  <switch> --> '-d'    */
		case RULE_switch_MINUSd_: opt.bshow_grammar_files=true;break;

		/*  <outswitch> --> '-ot' FILENAME    */
		case RULE_outswitch_MINUSot_FILENAME_: if (opt.bin_parser) break; write_grammar_file(strip_quotes(p.get_lexeme(1))); break;
		case RULE_showswitch_MINUSss_: opt.bshow_grammar_symbols=true; break;/*  <showswitch> --> '-ss'    */
		case RULE_showswitch_MINUSsc_: opt.bshow_grammar_configs=true; break;/*  <showswitch> --> '-sc'    */
		case RULE_showswitch_MINUSsa_: opt.bshow_grammar_actions=true; break;/*  <showswitch> --> '-sa'    */
		case RULE_helpswitch_MINUSh_: opt.bshow_usage=true; break;/*  <helpswitch> --> '-h'    */
		case RULE_helpswitch_MINUShf_: opt.bshow_grammar_help=true;break;/*  <helpswitch> --> '-hf'    */
		case -1: return false;
		case -2: return false;
		case 0: return true;
		}
	}
}
