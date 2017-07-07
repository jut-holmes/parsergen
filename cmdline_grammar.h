
/*
 *
 *  pgen generated header file
 *  06/19/05 09:04:07
 *
 */

/*********************
 *   States
 *********************/

#define STATE_start                     0

/*********************
 *   Symbols
 *********************/

/*  EOF  */
#define __EOF__	0

/*  <S'>   */
#define SYMBOL_S_SINGLE_QUOTE_          1

/*  <grammar>   */
#define SYMBOL_grammar_                 2

/*  FILENAME   */
#define SYMBOL_FILENAME_                3

/*  WS   */
#define SYMBOL_WS_                      4

/*  <cmdline>   */
#define SYMBOL_cmdline_                 5

/*  <switchexpr>   */
#define SYMBOL_switchexpr_              6

/*  <switch>   */
#define SYMBOL_switch_                  7

/*  <inswitch>   */
#define SYMBOL_inswitch_                8

/*  <outswitch>   */
#define SYMBOL_outswitch_               9

/*  <showswitch>   */
#define SYMBOL_showswitch_              10

/*  <helpswitch>   */
#define SYMBOL_helpswitch_              11

/*  <outflags>   */
#define SYMBOL_outflags_                12

/*  <testswitch>   */
#define SYMBOL_testswitch_              13

/*  '-v'   */
#define SYMBOL_MINUSv_                  14

/*  '-d'   */
#define SYMBOL_MINUSd_                  15

/*  '-nc'   */
#define SYMBOL_MINUSnc_                 16

/*  '-cs'   */
#define SYMBOL_MINUScs_                 17

/*  '-it'   */
#define SYMBOL_MINUSit_                 18

/*  '-ig'   */
#define SYMBOL_MINUSig_                 19

/*  '-og'   */
#define SYMBOL_MINUSog_                 20

/*  '-oh'   */
#define SYMBOL_MINUSoh_                 21

/*  '-oc'   */
#define SYMBOL_MINUSoc_                 22

/*  '-op'   */
#define SYMBOL_MINUSop_                 23

/*  '-ot'   */
#define SYMBOL_MINUSot_                 24

/*  '-oe'   */
#define SYMBOL_MINUSoe_                 25

/*  '-ore'   */
#define SYMBOL_MINUSore_                26

/*  '-nd'   */
#define SYMBOL_MINUSnd_                 27

/*  '-nl'   */
#define SYMBOL_MINUSnl_                 28

/*  '-ns'   */
#define SYMBOL_MINUSns_                 29

/*  '-dc'   */
#define SYMBOL_MINUSdc_                 30

/*  '-t'   */
#define SYMBOL_MINUSt_                  31

/*  '-td'   */
#define SYMBOL_MINUStd_                 32

/*  '-tr'   */
#define SYMBOL_MINUStr_                 33

/*  '-ss'   */
#define SYMBOL_MINUSss_                 34

/*  '-sc'   */
#define SYMBOL_MINUSsc_                 35

/*  '-sa'   */
#define SYMBOL_MINUSsa_                 36

/*  '-h'   */
#define SYMBOL_MINUSh_                  37

/*  '-hf'   */
#define SYMBOL_MINUShf_                 38

/*********************
 *   Rules
 *********************/

/*  <S'> --> <grammar>    */
#define RULE_S_SINGLE_QUOTE_grammar_                        	0

/*  <grammar> --> FILENAME <cmdline>    */
#define RULE_grammar_FILENAME_cmdline_                      	1

/*  <cmdline> --> FILENAME    */
#define RULE_cmdline_FILENAME_                              	2

/*  <cmdline> --> <switchexpr>    */
#define RULE_cmdline_switchexpr_                            	3

/*  <cmdline> -->    */
#define RULE_cmdline_                                       	4

/*  <switchexpr> --> <switchexpr> <switch>    */
#define RULE_switchexpr_switchexpr_switch_                  	5

/*  <switchexpr> --> <switch>    */
#define RULE_switchexpr_switch_                             	6

/*  <switch> --> <inswitch>    */
#define RULE_switch_inswitch_                               	7

/*  <switch> --> <outswitch>    */
#define RULE_switch_outswitch_                              	8

/*  <switch> --> <showswitch>    */
#define RULE_switch_showswitch_                             	9

/*  <switch> --> <helpswitch>    */
#define RULE_switch_helpswitch_                             	10

/*  <switch> --> <outflags>    */
#define RULE_switch_outflags_                               	11

/*  <switch> --> <testswitch>    */
#define RULE_switch_testswitch_                             	12

/*  <switch> --> '-v'    */
#define RULE_switch_MINUSv_                                 	13

/*  <switch> --> '-d'    */
#define RULE_switch_MINUSd_                                 	14

/*  <switch> --> '-nc'    */
#define RULE_switch_MINUSnc_                                	15

/*  <switch> --> '-cs'    */
#define RULE_switch_MINUScs_                                	16

/*  <inswitch> --> '-it' FILENAME    */
#define RULE_inswitch_MINUSit_FILENAME_                     	17

/*  <inswitch> --> '-ig' FILENAME    */
#define RULE_inswitch_MINUSig_FILENAME_                     	18

/*  <outswitch> --> '-og' FILENAME    */
#define RULE_outswitch_MINUSog_FILENAME_                    	19

/*  <outswitch> --> '-oh' FILENAME    */
#define RULE_outswitch_MINUSoh_FILENAME_                    	20

/*  <outswitch> --> '-oc' FILENAME    */
#define RULE_outswitch_MINUSoc_FILENAME_                    	21

/*  <outswitch> --> '-op' FILENAME    */
#define RULE_outswitch_MINUSop_FILENAME_                    	22

/*  <outswitch> --> '-ot' FILENAME    */
#define RULE_outswitch_MINUSot_FILENAME_                    	23

/*  <outswitch> --> '-oe' FILENAME    */
#define RULE_outswitch_MINUSoe_FILENAME_                    	24

/*  <outswitch> --> '-ore' FILENAME    */
#define RULE_outswitch_MINUSore_FILENAME_                   	25

/*  <outflags> --> '-nd'    */
#define RULE_outflags_MINUSnd_                              	26

/*  <outflags> --> '-nl'    */
#define RULE_outflags_MINUSnl_                              	27

/*  <outflags> --> '-ns'    */
#define RULE_outflags_MINUSns_                              	28

/*  <outflags> --> '-dc'    */
#define RULE_outflags_MINUSdc_                              	29

/*  <testswitch> --> '-t' FILENAME    */
#define RULE_testswitch_MINUSt_FILENAME_                    	30

/*  <testswitch> --> '-td' FILENAME    */
#define RULE_testswitch_MINUStd_FILENAME_                   	31

/*  <testswitch> --> '-tr' FILENAME    */
#define RULE_testswitch_MINUStr_FILENAME_                   	32

/*  <showswitch> --> '-ss'    */
#define RULE_showswitch_MINUSss_                            	33

/*  <showswitch> --> '-sc'    */
#define RULE_showswitch_MINUSsc_                            	34

/*  <showswitch> --> '-sa'    */
#define RULE_showswitch_MINUSsa_                            	35

/*  <helpswitch> --> '-h'    */
#define RULE_helpswitch_MINUSh_                             	36

/*  <helpswitch> --> '-hf'    */
#define RULE_helpswitch_MINUShf_                            	37

