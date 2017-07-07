
/*
 *
 *  pgen generated header file
 *  12/17/15 13:57:02
 *
 */

/*********************
 *   States
 *********************/

#define STATE_start                     0
#define STATE_READ_STATE_NAME           95
#define STATE_REGEX                     98
#define STATE_NONTERM                   140
#define STATE_CHARSET                   166
#define STATE_READ_START_SYMBOL         190
#define STATE_READ_PRECEDENCE           195
#define STATE_READ_PARAM                204
#define STATE_INNER_COMMENT             207
#define STATE_BRACKET_EXPRESSION        211
#define STATE_CLASSNAME                 243

/*********************
 *   Symbols
 *********************/

/*  EOF  */
#define __EOF__	0

/*  <S'>   */
#define SYMBOL_S_SINGLE_QUOTE_          1

/*  GOTO   */
#define SYMBOL_GOTO_                    2

/*  PUSH   */
#define SYMBOL_PUSH_                    3

/*  POP   */
#define SYMBOL_POP_                     4

/*  IGNORE   */
#define SYMBOL_IGNORE_                  5

/*  DISCARD   */
#define SYMBOL_DISCARD_                 6

/*  CONCAT   */
#define SYMBOL_CONCAT_                  7

/*  ACCEPT   */
#define SYMBOL_ACCEPT_                  8

/*  PRESERVE   */
#define SYMBOL_PRESERVE_                9

/*  SHORT   */
#define SYMBOL_SHORT_                   10

/*  TERM   */
#define SYMBOL_TERM_                    11

/*  NONTERM   */
#define SYMBOL_NONTERM_                 12

/*  CHARSET   */
#define SYMBOL_CHARSET_                 13

/*  WS   */
#define SYMBOL_WS_                      14

/*  START   */
#define SYMBOL_START_                   15

/*  STATE   */
#define SYMBOL_STATE_                   16

/*  RIGHT   */
#define SYMBOL_RIGHT_                   17

/*  LEFT   */
#define SYMBOL_LEFT_                    18

/*  NONASSOC   */
#define SYMBOL_NONASSOC_                19

/*  PARAM   */
#define SYMBOL_PARAM_                   20

/*  LINECOMMENT   */
#define SYMBOL_LINECOMMENT_             21

/*  BLOCKCOMMENT   */
#define SYMBOL_BLOCKCOMMENT_            22

/*  ORDCHAR   */
#define SYMBOL_ORDCHAR_                 23

/*  QUOTEDCHAR   */
#define SYMBOL_QUOTEDCHAR_              24

/*  OCTCHAR   */
#define SYMBOL_OCTCHAR_                 25

/*  HEXCHAR   */
#define SYMBOL_HEXCHAR_                 26

/*  CTLCHAR   */
#define SYMBOL_CTLCHAR_                 27

/*  USCHAR   */
#define SYMBOL_USCHAR_                  28

/*  BSCHAR   */
#define SYMBOL_BSCHAR_                  29

/*  CHARSETREF   */
#define SYMBOL_CHARSETREF_              30

/*  RANGEREF   */
#define SYMBOL_RANGEREF_                31

/*  STRLITERAL   */
#define SYMBOL_STRLITERAL_              32

/*  '['   */
#define SYMBOL_OPEN_BRACKET_            33

/*  NL   */
#define SYMBOL_NL_                      34

/*  ';'   */
#define SYMBOL_SEMI_                    35

/*  SPACE   */
#define SYMBOL_SPACE_                   36

/*  ']'   */
#define SYMBOL_CLOSE_BRACKET_           37

/*  '[:'   */
#define SYMBOL_OPEN_BRACKET_COLON_      38

/*  ENDCMNT   */
#define SYMBOL_ENDCMNT_                 39

/*  INNERCMNT   */
#define SYMBOL_INNERCMNT_               40

/*  START_SYMBOL   */
#define SYMBOL_START__SYMBOL_           41

/*  PARAMS   */
#define SYMBOL_PARAMS_                  42

/*  STATE_NAME   */
#define SYMBOL_STATE__NAME_             43

/*  TOKENNAME   */
#define SYMBOL_TOKENNAME_               44

/*  '|'   */
#define SYMBOL_VERT_BAR_                45

/*  ASSIGN   */
#define SYMBOL_ASSIGN_                  46

/*  PREC   */
#define SYMBOL_PREC_                    47

/*  CLASSNAME   */
#define SYMBOL_CLASSNAME_               48

/*  CHARLITERAL   */
#define SYMBOL_CHARLITERAL_             49

/*  CHARRANGE   */
#define SYMBOL_CHARRANGE_               50

/*  CHARUVAL   */
#define SYMBOL_CHARUVAL_                51

/*  ESCCHAR   */
#define SYMBOL_ESCCHAR_                 52

/*  <grammar>   */
#define SYMBOL_grammar_                 53

/*  <statements>   */
#define SYMBOL_statements_              54

/*  <statement>   */
#define SYMBOL_statement_               55

/*  <statementexpr>   */
#define SYMBOL_statementexpr_           56

/*  <nontermexpr>   */
#define SYMBOL_nontermexpr_             57

/*  <termexpr>   */
#define SYMBOL_termexpr_                58

/*  <keyword>   */
#define SYMBOL_keyword_                 59

/*  <charset>   */
#define SYMBOL_charset_                 60

/*  <action>   */
#define SYMBOL_action_                  61

/*  <regex>   */
#define SYMBOL_regex_                   62

/*  <EREbranch>   */
#define SYMBOL_EREbranch_               63

/*  <EREAlternate>   */
#define SYMBOL_EREAlternate_            64

/*  <EREexpression>   */
#define SYMBOL_EREexpression_           65

/*  <onecharacterERE>   */
#define SYMBOL_onecharacterERE_         66

/*  '^'   */
#define SYMBOL_CARET_                   67

/*  '$'   */
#define SYMBOL_STRING_                  68

/*  '('   */
#define SYMBOL_LEFT_PAREN_              69

/*  ')'   */
#define SYMBOL_RIGHT_PAREN_             70

/*  <EREduplsymbol>   */
#define SYMBOL_EREduplsymbol_           71

/*  <collelem>   */
#define SYMBOL_collelem_                72

/*  '.'   */
#define SYMBOL_PERIOD_                  73

/*  <bracketExpression>   */
#define SYMBOL_bracketExpression_       74

/*  '*'   */
#define SYMBOL_STAR_                    75

/*  '+'   */
#define SYMBOL_PLUS_                    76

/*  '?'   */
#define SYMBOL_QUESTION_                77

/*  <brackExprStart>   */
#define SYMBOL_brackExprStart_          78

/*  <matchinglist>   */
#define SYMBOL_matchinglist_            79

/*  <nonmatchinglist>   */
#define SYMBOL_nonmatchinglist_         80

/*  <bracketList>   */
#define SYMBOL_bracketList_             81

/*  <expressionTerm>   */
#define SYMBOL_expressionTerm_          82

/*  <singleExpression>   */
#define SYMBOL_singleExpression_        83

/*  <rangeExpression>   */
#define SYMBOL_rangeExpression_         84

/*  <endRange>   */
#define SYMBOL_endRange_                85

/*  <characterClass>   */
#define SYMBOL_characterClass_          86

/*  <equivalenceClass>   */
#define SYMBOL_equivalenceClass_        87

/*  <startRange>   */
#define SYMBOL_startRange_              88

/*  '-'   */
#define SYMBOL_MINUS_                   89

/*  <collatingSymbol>   */
#define SYMBOL_collatingSymbol_         90

/*  '[.'   */
#define SYMBOL_OPEN_BRACKET_PERIOD_     91

/*  '.]'   */
#define SYMBOL_PERIOD_CLOSE_BRACKET_    92

/*  <metachar>   */
#define SYMBOL_metachar_                93

/*  '[='   */
#define SYMBOL_OPEN_BRACKET_EQUALS_     94

/*  '=]'   */
#define SYMBOL_EQUALS_CLOSE_BRACKET_    95

/*  ':]'   */
#define SYMBOL_COLON_CLOSE_BRACKET_     96

/*  <termstart>   */
#define SYMBOL_termstart_               97

/*  <nontermstart>   */
#define SYMBOL_nontermstart_            98

/*  <productions>   */
#define SYMBOL_productions_             99

/*  <prodalternate>   */
#define SYMBOL_prodalternate_           100

/*  <productionexpr>   */
#define SYMBOL_productionexpr_          101

/*  <production>   */
#define SYMBOL_production_              102

/*  <optprec>   */
#define SYMBOL_optprec_                 103

/*  <nament>   */
#define SYMBOL_nament_                  104

/*  <kwleft>   */
#define SYMBOL_kwleft_                  105

/*  <kwright>   */
#define SYMBOL_kwright_                 106

/*  <kwnonassoc>   */
#define SYMBOL_kwnonassoc_              107

/*  <kwleftstart>   */
#define SYMBOL_kwleftstart_             108

/*  <kwleftexpr>   */
#define SYMBOL_kwleftexpr_              109

/*  <kwrightstart>   */
#define SYMBOL_kwrightstart_            110

/*  <kwrightexpr>   */
#define SYMBOL_kwrightexpr_             111

/*  <kwnonassocstart>   */
#define SYMBOL_kwnonassocstart_         112

/*  <kwnonassocexpr>   */
#define SYMBOL_kwnonassocexpr_          113

/*  <charsetstart>   */
#define SYMBOL_charsetstart_            114

/*  <charsetexpr>   */
#define SYMBOL_charsetexpr_             115

/*  <charsetitem>   */
#define SYMBOL_charsetitem_             116

/*********************
 *   Rules
 *********************/

/*  <S'> --> <grammar>    */
#define RULE_S_SINGLE_QUOTE_grammar_                        	0

/*  <grammar> --> <statements>    */
#define RULE_grammar_statements_                            	1

/*  <statements> --> <statements> <statement>    */
#define RULE_statements_statements_statement_               	2

/*  <statements> --> <statement>    */
#define RULE_statements_statement_                          	3

/*  <statement> --> <statementexpr> ';'    */
#define RULE_statement_statementexpr_SEMI_                  	4

/*  <statementexpr> --> <nontermexpr>    */
#define RULE_statementexpr_nontermexpr_                     	5

/*  <statementexpr> --> <termexpr>    */
#define RULE_statementexpr_termexpr_                        	6

/*  <statementexpr> --> <keyword>    */
#define RULE_statementexpr_keyword_                         	7

/*  <statementexpr> --> <charset>    */
#define RULE_statementexpr_charset_                         	8

/*  <statementexpr> --> <action>    */
#define RULE_statementexpr_action_                          	9

/*  <regex> --> <EREbranch>    */
#define RULE_regex_EREbranch_                               	10

/*  <regex> --> <regex> <EREAlternate> <EREbranch>    */
#define RULE_regex_regex_EREAlternate_EREbranch_            	11

/*  <EREAlternate> --> '|'    */
#define RULE_EREAlternate_VERT_BAR_                         	12

/*  <EREAlternate> --> NL    */
#define RULE_EREAlternate_NL_                               	13

/*  <EREbranch> --> <EREbranch> <EREexpression>    */
#define RULE_EREbranch_EREbranch_EREexpression_             	14

/*  <EREbranch> --> <EREexpression>    */
#define RULE_EREbranch_EREexpression_                       	15

/*  <EREexpression> --> <onecharacterERE>    */
#define RULE_EREexpression_onecharacterERE_                 	16

/*  <EREexpression> --> '^'    */
#define RULE_EREexpression_CARET_                           	17

/*  <EREexpression> --> '$'    */
#define RULE_EREexpression_STRING_                          	18

/*  <EREexpression> --> '(' <regex> ')'    */
#define RULE_EREexpression_LEFT_PAREN_regex_RIGHT_PAREN_    	19

/*  <EREexpression> --> <EREexpression> <EREduplsymbol>    */
#define RULE_EREexpression_EREexpression_EREduplsymbol_     	20

/*  <onecharacterERE> --> <collelem>    */
#define RULE_onecharacterERE_collelem_                      	21

/*  <onecharacterERE> --> '.'    */
#define RULE_onecharacterERE_PERIOD_                        	22

/*  <onecharacterERE> --> <bracketExpression>    */
#define RULE_onecharacterERE_bracketExpression_             	23

/*  <onecharacterERE> --> CHARSETREF    */
#define RULE_onecharacterERE_CHARSETREF_                    	24

/*  <onecharacterERE> --> STRLITERAL    */
#define RULE_onecharacterERE_STRLITERAL_                    	25

/*  <EREduplsymbol> --> '*'    */
#define RULE_EREduplsymbol_STAR_                            	26

/*  <EREduplsymbol> --> '+'    */
#define RULE_EREduplsymbol_PLUS_                            	27

/*  <EREduplsymbol> --> '?'    */
#define RULE_EREduplsymbol_QUESTION_                        	28

/*  <EREduplsymbol> --> RANGEREF    */
#define RULE_EREduplsymbol_RANGEREF_                        	29

/*  <bracketExpression> --> <brackExprStart> <matchinglist> ']'    */
#define RULE_bracketExpression_brackExprStart_matchinglist_CLOSE_BRACKET_	30

/*  <bracketExpression> --> <brackExprStart> <nonmatchinglist> ']'    */
#define RULE_bracketExpression_brackExprStart_nonmatchinglist_CLOSE_BRACKET_	31

/*  <brackExprStart> --> '['    */
#define RULE_brackExprStart_OPEN_BRACKET_                   	32

/*  <matchinglist> --> <bracketList>    */
#define RULE_matchinglist_bracketList_                      	33

/*  <nonmatchinglist> --> '^' <bracketList>    */
#define RULE_nonmatchinglist_CARET_bracketList_             	34

/*  <bracketList> --> <bracketList> <expressionTerm>    */
#define RULE_bracketList_bracketList_expressionTerm_        	35

/*  <bracketList> --> <expressionTerm>    */
#define RULE_bracketList_expressionTerm_                    	36

/*  <expressionTerm> --> <singleExpression>    */
#define RULE_expressionTerm_singleExpression_               	37

/*  <expressionTerm> --> <rangeExpression>    */
#define RULE_expressionTerm_rangeExpression_                	38

/*  <expressionTerm> --> SPACE    */
#define RULE_expressionTerm_SPACE_                          	39

/*  <singleExpression> --> <endRange>    */
#define RULE_singleExpression_endRange_                     	40

/*  <singleExpression> --> <characterClass>    */
#define RULE_singleExpression_characterClass_               	41

/*  <singleExpression> --> <equivalenceClass>    */
#define RULE_singleExpression_equivalenceClass_             	42

/*  <rangeExpression> --> <startRange> <endRange>    */
#define RULE_rangeExpression_startRange_endRange_           	43

/*  <rangeExpression> --> <startRange> '-'    */
#define RULE_rangeExpression_startRange_MINUS_              	44

/*  <startRange> --> <collelem> '-'    */
#define RULE_startRange_collelem_MINUS_                     	45

/*  <endRange> --> <collelem>    */
#define RULE_endRange_collelem_                             	46

/*  <endRange> --> <collatingSymbol>    */
#define RULE_endRange_collatingSymbol_                      	47

/*  <collatingSymbol> --> '[.' <collelem> '.]'    */
#define RULE_collatingSymbol_OPEN_BRACKET_PERIOD_collelem_PERIOD_CLOSE_BRACKET_	48

/*  <collatingSymbol> --> '[.' <metachar> '.]'    */
#define RULE_collatingSymbol_OPEN_BRACKET_PERIOD_metachar_PERIOD_CLOSE_BRACKET_	49

/*  <equivalenceClass> --> '[=' <collelem> '=]'    */
#define RULE_equivalenceClass_OPEN_BRACKET_EQUALS_collelem_EQUALS_CLOSE_BRACKET_	50

/*  <characterClass> --> '[:' CLASSNAME ':]'    */
#define RULE_characterClass_OPEN_BRACKET_COLON_CLASSNAME_COLON_CLOSE_BRACKET_	51

/*  <collelem> --> ORDCHAR    */
#define RULE_collelem_ORDCHAR_                              	52

/*  <collelem> --> QUOTEDCHAR    */
#define RULE_collelem_QUOTEDCHAR_                           	53

/*  <collelem> --> OCTCHAR    */
#define RULE_collelem_OCTCHAR_                              	54

/*  <collelem> --> HEXCHAR    */
#define RULE_collelem_HEXCHAR_                              	55

/*  <collelem> --> CTLCHAR    */
#define RULE_collelem_CTLCHAR_                              	56

/*  <collelem> --> USCHAR    */
#define RULE_collelem_USCHAR_                               	57

/*  <collelem> --> BSCHAR    */
#define RULE_collelem_BSCHAR_                               	58

/*  <metachar> --> '^'    */
#define RULE_metachar_CARET_                                	59

/*  <metachar> --> '-'    */
#define RULE_metachar_MINUS_                                	60

/*  <metachar> --> ']'    */
#define RULE_metachar_CLOSE_BRACKET_                        	61

/*  <termexpr> --> <termstart> <regex>    */
#define RULE_termexpr_termstart_regex_                      	62

/*  <termexpr> --> <termstart>    */
#define RULE_termexpr_termstart_                            	63

/*  <termstart> --> TERM    */
#define RULE_termstart_TERM_                                	64

/*  <nontermexpr> --> <nontermstart> <productions>    */
#define RULE_nontermexpr_nontermstart_productions_          	65

/*  <nontermstart> --> NONTERM ASSIGN    */
#define RULE_nontermstart_NONTERM_ASSIGN_                   	66

/*  <productions> --> <productions> <prodalternate> <productionexpr>    */
#define RULE_productions_productions_prodalternate_productionexpr_	67

/*  <productions> --> <productionexpr>    */
#define RULE_productions_productionexpr_                    	68

/*  <prodalternate> --> '|'    */
#define RULE_prodalternate_VERT_BAR_                        	69

/*  <productionexpr> --> <production> <optprec>    */
#define RULE_productionexpr_production_optprec_             	70

/*  <productionexpr> -->    */
#define RULE_productionexpr_                                	71

/*  <production> --> <production> <nament>    */
#define RULE_production_production_nament_                  	72

/*  <production> --> <nament>    */
#define RULE_production_nament_                             	73

/*  <optprec> --> PREC TERM    */
#define RULE_optprec_PREC_TERM_                             	74

/*  <optprec> -->    */
#define RULE_optprec_                                       	75

/*  <nament> --> TERM    */
#define RULE_nament_TERM_                                   	76

/*  <nament> --> NONTERM    */
#define RULE_nament_NONTERM_                                	77

/*  <action> --> GOTO STATE_NAME    */
#define RULE_action_GOTO_STATE__NAME_                       	78

/*  <action> --> PUSH STATE_NAME    */
#define RULE_action_PUSH_STATE__NAME_                       	79

/*  <action> --> POP    */
#define RULE_action_POP_                                    	80

/*  <action> --> DISCARD    */
#define RULE_action_DISCARD_                                	81

/*  <action> --> CONCAT    */
#define RULE_action_CONCAT_                                 	82

/*  <action> --> ACCEPT    */
#define RULE_action_ACCEPT_                                 	83

/*  <action> --> PRESERVE    */
#define RULE_action_PRESERVE_                               	84

/*  <action> --> SHORT    */
#define RULE_action_SHORT_                                  	85

/*  <action> --> IGNORE    */
#define RULE_action_IGNORE_                                 	86

/*  <keyword> --> START START_SYMBOL    */
#define RULE_keyword_START_START__SYMBOL_                   	87

/*  <keyword> --> STATE STATE_NAME    */
#define RULE_keyword_STATE_STATE__NAME_                     	88

/*  <keyword> --> PARAM PARAMS    */
#define RULE_keyword_PARAM_PARAMS_                          	89

/*  <keyword> --> <kwleft>    */
#define RULE_keyword_kwleft_                                	90

/*  <keyword> --> <kwright>    */
#define RULE_keyword_kwright_                               	91

/*  <keyword> --> <kwnonassoc>    */
#define RULE_keyword_kwnonassoc_                            	92

/*  <kwleft> --> <kwleftstart> <kwleftexpr>    */
#define RULE_kwleft_kwleftstart_kwleftexpr_                 	93

/*  <kwleftstart> --> LEFT    */
#define RULE_kwleftstart_LEFT_                              	94

/*  <kwleftexpr> --> <kwleftexpr> TOKENNAME    */
#define RULE_kwleftexpr_kwleftexpr_TOKENNAME_               	95

/*  <kwleftexpr> --> TOKENNAME    */
#define RULE_kwleftexpr_TOKENNAME_                          	96

/*  <kwright> --> <kwrightstart> <kwrightexpr>    */
#define RULE_kwright_kwrightstart_kwrightexpr_              	97

/*  <kwrightstart> --> RIGHT    */
#define RULE_kwrightstart_RIGHT_                            	98

/*  <kwrightexpr> --> <kwrightexpr> TOKENNAME    */
#define RULE_kwrightexpr_kwrightexpr_TOKENNAME_             	99

/*  <kwrightexpr> --> TOKENNAME    */
#define RULE_kwrightexpr_TOKENNAME_                         	100

/*  <kwnonassoc> --> <kwnonassocstart> <kwnonassocexpr>    */
#define RULE_kwnonassoc_kwnonassocstart_kwnonassocexpr_     	101

/*  <kwnonassocstart> --> NONASSOC    */
#define RULE_kwnonassocstart_NONASSOC_                      	102

/*  <kwnonassocexpr> --> <kwnonassocexpr> TOKENNAME    */
#define RULE_kwnonassocexpr_kwnonassocexpr_TOKENNAME_       	103

/*  <kwnonassocexpr> --> TOKENNAME    */
#define RULE_kwnonassocexpr_TOKENNAME_                      	104

/*  <charset> --> <charsetstart> <charsetexpr>    */
#define RULE_charset_charsetstart_charsetexpr_              	105

/*  <charsetstart> --> CHARSET    */
#define RULE_charsetstart_CHARSET_                          	106

/*  <charsetexpr> --> <charsetexpr> <charsetitem>    */
#define RULE_charsetexpr_charsetexpr_charsetitem_           	107

/*  <charsetexpr> --> <charsetitem>    */
#define RULE_charsetexpr_charsetitem_                       	108

/*  <charsetitem> --> CHARSET    */
#define RULE_charsetitem_CHARSET_                           	109

/*  <charsetitem> --> ESCCHAR    */
#define RULE_charsetitem_ESCCHAR_                           	110

/*  <charsetitem> --> '+'    */
#define RULE_charsetitem_PLUS_                              	111

/*  <charsetitem> --> '-'    */
#define RULE_charsetitem_MINUS_                             	112

/*  <charsetitem> --> CHARRANGE    */
#define RULE_charsetitem_CHARRANGE_                         	113

/*  <charsetitem> --> HEXCHAR    */
#define RULE_charsetitem_HEXCHAR_                           	114

/*  <charsetitem> --> CHARUVAL    */
#define RULE_charsetitem_CHARUVAL_                          	115

/*  <charsetitem> --> CHARLITERAL    */
#define RULE_charsetitem_CHARLITERAL_                       	116

