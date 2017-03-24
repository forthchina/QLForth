/* ************************************************************************************
* Copyright (C) 2015-2017 Shaanxi QinWei Electroics Technologies. All rights reserved.
*
* $Date:        24. March 2017
* $Revision: 	V 0.1
*
* Project: 	    QLForth - A Forth for System-On-Chip Creative Makers
*
* Title:	   QLForth.c
*
* Description:	The main entry for QLForth
*
* Target:		Windows 7+
*
* Author:		Zhao Yu, forthchina@163.com, forthchina@gmail.com
*			    24/03/2017 (In dd/mm/yyyy)
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*   - Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   - Redistributions in binary form must reproduce the above copyright
*     notice, this list of conditions and the following disclaimer in
*     the documentation and/or other materials provided with the
*     distribution.
*   - Neither the name of author nor the names of its contributors
*     may be used to endorse or promote products derived from this
*     software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
* FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
* COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
* ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
* ********************************************************************************** */


#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <setjmp.h>

 
#include "QLForth.h"

#define LOAD_FILE_STACK_DEEP	16
#define SEARCH_ORDER_SIZE		8
#define HASH_SIZE				127 
#define CONSTANT_POOL_SIZE		32

QLF_LITERAL		token_value;
QLF_CELL		* ql4thvm_here,
				ql4thvm_tos, * ql4thvm_dp, * ql4thvm_rp, * ql4thvm_stack_top, * ql4thvm_stack;
Symbol			** program_counter, * ThisCreateWord, *ThisExecuteWord ;
char			token_word[TEXT_LINE_SIZE];
int				ql4thvm_state, ql4thvm_running, ql4thvm_force_break ;

static QLF_CELL dict_buffer[DICT_BUFFER_SIZE + 4], data_return_stack[STACK_DEEP_SIZE + 2],
				* ql4thvm_dict_top, * ql4thvm_dict, * ql4thvm_guard;
static char		* scan_ptr, * load_text, * text_ptr, * load_save_scan_ptr,
				* qlforth_text_buffer, system_directory[MAX_PATH], working_directory[MAX_PATH];
static Symbol	* root_bucket[HASH_SIZE + 1], *forth_bucket[HASH_SIZE + 1],
				** current, ** search_order[SEARCH_ORDER_SIZE + 1];
static int		err_flag, qlforth_text_length, display_number_base = 10;
static jmp_buf  e_buf;

// ************************************************************************************

static void display_symbole_table(Symbol * bucket[]) {
	int idx, cnt;
	Symbol * spc;

	for (idx = 0; idx < HASH_SIZE; idx ++) {
		if ((spc = bucket[idx]) != NULL) {
			QLForth_printf("HASH : %d\n", idx) ;
			cnt = 1;
			do {
				QLForth_printf("\t(%d) %s (DFA : 0x%08X)\n", cnt ++, spc->name, spc->dfa);
				spc = spc->link;
			} while (spc != NULL);
		}
	}
}

static unsigned int hash_bucket(unsigned char * name) {
	unsigned int  h ;

	for (h = 0 ; * name ; h += * name ++) ;
	return h  % HASH_SIZE ;
}

static int is_space(int ch) {
	return ch == ' ' || ch  == '\t' || ch == '\r' || ch == '\n';
}

void qlforth_fp_docreate(void) {
	* ql4thvm_dp ++ = ql4thvm_tos;
	ql4thvm_tos = ThisExecuteWord->dfa;
}

static void fp_dovocabulary(void) {
	*ql4thvm_dp++ = ql4thvm_tos;
	ql4thvm_tos = (QLF_CELL)ThisExecuteWord;
}

static void fp_doliteral(void) {
	*ql4thvm_dp++ = ql4thvm_tos;
	ql4thvm_tos = *(QLF_CELL *)program_counter++;
}

void * qlforth_alloc(int size_in_byte) {
	char * pc;

	size_in_byte = (size_in_byte - 1) / (sizeof(QLF_CELL)) + 1;
	if ((ql4thvm_here + size_in_byte + 1) > ql4thvm_dict_top) {
		QLForth_error("Memory Full", NULL);
	}
	else {
		pc	= (char *) ql4thvm_here;
		memset(pc, 0, (size_in_byte + 1)* (sizeof(QLF_CELL)));
		ql4thvm_here += size_in_byte;
	}
	return (void *) pc;
}

// ************************************************************************************

static int forth_number(char * str) {
	char	negate, base, * ptr, * end_dptr;

	ptr		= str;
	negate	= 0;
	base	= 10;
	if (* ptr == '-')	{	
		ptr ++;		
		negate = -1;	
	}
	if (* ptr == '%')	{	
		ptr ++;		
		base = 2;		
	} 
	else if (* ptr == '0' && (ptr[1] == 'x' || ptr[1] == 'X')) { 
		ptr += 2;	
		base = 16;	
	}
	token_value.ival = strtoul(ptr, &end_dptr, base);

	if (* end_dptr == '\0') {
        if (negate) token_value.ival = - token_value.ival;
		return -1;
	}

	token_value.fval = (float) strtod(ptr, & end_dptr);
    if (* end_dptr != '\0') return 0;
    if (negate)  token_value.fval = - token_value.fval;
	return 1;
}

static int forth_get_string(void) {
	int idx, cc;

	while (* scan_ptr != 0 && is_space(* scan_ptr)) {
		scan_ptr ++;
    }
	if (* scan_ptr == 0 || * scan_ptr != '\"') {
		return 0;
	}

	idx = 0;
	scan_ptr ++;
	while (* scan_ptr != 0 && (idx < (TEXT_LINE_SIZE - 2))) {
		cc = * scan_ptr ++;
		if (cc == '\"') {
			token_word[idx ++] = 0;
			return 1;
		}
		if (cc == '\\') {
			switch(* scan_ptr ++) {
				case 'b': cc = '\b';	break ;
                case 'n': cc = '\n';	break ;
                case 'r': cc = '\r';	break ;
                case 't': cc = '\t';	break ;
				default:			    break;
			}
		}
		token_word[idx ++] = cc;
	}
	token_word[idx ++] = 0;
	return 0;
}

// ************************************************************************************

static void forth_only(void) {
	int idx;

	for (idx = 0 ; idx < SEARCH_ORDER_SIZE; idx ++) {
		search_order[idx] = NULL;
	}
	search_order[0] = search_order[1] = root_bucket;
}

static void forth_forth(void) {
	search_order[0] = forth_bucket;
}

static void cfp_vocabulary(void) {
	QLF_CELL	i;
	Symbol * spc;
	
	if (! QLForth_token()) {
		QLForth_error("Expect a word", NULL);
		return ;
	}

	_strupr(token_word);
	i	= sizeof(Symbol) + strlen(token_word) + sizeof(root_bucket);
	spc = (Symbol *) qlforth_alloc(i);
	memset((char *) spc, 0, i);
	strcpy(spc->name, token_word);
	i			= hash_bucket((unsigned char *) token_word);
	spc->link	= root_bucket[i] ;
	current[i]	= spc;
	spc->type	= QLF_TYPE_VOCABULARY;
	spc->fun	= fp_dovocabulary;
	spc->dfa	= (QLF_CELL) ql4thvm_here;
}

static void cfp_definitions(void) {
	current = search_order[0] ;
}

static void cfp_also(void) {
	int idx;

	for (idx = SEARCH_ORDER_SIZE; idx > 0 ; idx --) {
		search_order[idx] = search_order[idx - 1];
	}
}

static void cfp_previous(void) {
	int idx;

	for (idx = 0; idx < SEARCH_ORDER_SIZE; idx ++) {
		search_order[idx] = search_order[idx + 1];
	}
}
 
// ************************************************************************************

static void forth_clear(void) {
	ql4thvm_stack		= ql4thvm_dp	= & data_return_stack[0]; 
	ql4thvm_stack_top	= ql4thvm_rp	= & data_return_stack[STACK_DEEP_SIZE - 1];
	ql4thvm_state		= QLF_STATE_INTERPRET;
}

static void qlforth_execute(Symbol * spc) {
	ThisExecuteWord	= spc;
	program_counter = NULL;
	ThisExecuteWord->fun();
	while (program_counter && ! ql4thvm_force_break) {
		ThisExecuteWord = * program_counter ++;
		ThisExecuteWord->fun();
	}
	ThisExecuteWord = NULL ;
}

// ************************************************************************************

static void cfp_L_paren(void) {
	while (* scan_ptr != 0 && * scan_ptr != ')') {
		scan_ptr ++;
    }
	if (* scan_ptr == ')') {
		scan_ptr ++;
	}
}

static void cfp_r_slash(void) {
	while (* scan_ptr != 0) {
		if (* scan_ptr == 0x0d || * scan_ptr == 0x0a) {
			break;
		}
		scan_ptr ++;
    }
}

static void fp_doconst(void) {
	* ql4thvm_dp ++ = ql4thvm_tos;
	ql4thvm_tos = ThisExecuteWord->dfa;
}

static void cfp_constant(void) {
	Symbol * spc;

	if ((spc = QLForth_create()) == NULL) {
		return ;
	}
	spc->type	= QLF_TYPE_CONSTANT;
	spc->fun	= fp_doconst;
	spc->dfa	= ql4thvm_tos;
	ql4thvm_tos = * (-- ql4thvm_dp);
}

static void cfp_load(void) {
	FILE * fd;
	int  size;
	char file_path[MAX_PATH];

	if (!forth_get_string()) {
		QLForth_error("Expect a string as the file name for LOAD", NULL);
	}
	if (load_text == NULL || load_save_scan_ptr != NULL) {
		QLForth_error("Load nested.", NULL);
	}

	sprintf(file_path, "%s\\%s.qlf", working_directory, token_word);
	if ((fd = fopen(file_path, "r")) == NULL) {
		// try to open as a library file
		sprintf(file_path, "%s\\%s.qlf", system_directory, token_word);
		if ((fd = fopen(file_path, "r")) == NULL) {
	        QLForth_error("Load file not found", token_word) ;
		}
	}

	// read this source file
	fseek(fd, 0L, SEEK_END);
	size = ftell(fd);
	fseek(fd, 0L, SEEK_SET);
	if ((load_text = malloc(size + 16)) == NULL) {
		QLForth_error("Out of memory", NULL) ;
	}
	size = fread(load_text, 1, size, fd);
	fclose(fd);   

	// processing this source file 
	* (load_text + size) = 0;
	* (load_text + size + 1) = 0;
	load_save_scan_ptr	= scan_ptr;
	scan_ptr			= load_text;
}

static void cfp_alias(void) {
	Symbol	* spc, * new_spc;

	if ((new_spc = QLForth_create()) == NULL) {
		QLForth_error("Word not create", token_word);
	}

	if (! QLForth_token()) {
		QLForth_error("Expect a word", NULL);
	}
	_strupr(token_word);
	if ((spc = QLForth_symbol_search(token_word)) == NULL) {
		QLForth_error("Word not found", token_word);
	}
	new_spc->fun	= spc->fun;
	new_spc->type	= spc->type;
	new_spc->dfa	= spc->dfa;
	new_spc->attr	= spc->attr;
}

static void cfp_sharp(void) {
	static Symbol symbol_do_literal = { NULL,{ fp_doliteral },{ "(DOLIT)" },	0 };

	* (Symbol **)ql4thvm_here++ = & symbol_do_literal ;
	* ql4thvm_here++ = ql4thvm_tos;
	ql4thvm_tos = * (-- ql4thvm_dp);
}

// ************************************************************************************

static Primitive immediate_table[] = {
	{   "(",			cfp_L_paren			},
	{   "\\",			cfp_r_slash			},
	{   "#",			cfp_sharp			},
	{	NULL,			NULL }
};

static Primitive define_table[] = {
	{	"CONSTANT",			cfp_constant	},
	
	{	"ONLY",				forth_only		},
	{	"FORTH",			forth_forth		},
	{	"VOCABULARY",		cfp_vocabulary	},
	{	"DEFINITIONS",		cfp_definitions	},
	{	"ALSO",				cfp_also		},
	{	"PREVIOUS",			cfp_previous	},
	{	"LOAD",				cfp_load		},
	{	"CLEAR",			forth_clear		},
	{	"ALIAS",			cfp_alias		},

	{	NULL,				NULL			}

} ;

// ************************************************************************************

static void qlforth_mixed_word_used(void) {
	QLForth_error("Not used in this mode", NULL);
}

static void forth_forget(void) {
	int idx, subidx;
	Symbol * spc, * subspc, ** vpc;

	QLForth_printf("Enter FORGET\n");
	forth_clear();
	ql4thvm_here = ql4thvm_guard;	
	for (idx = 0; idx < HASH_SIZE ; idx ++) {
		spc = root_bucket[idx];
		while ((void *) spc >= (void *) ql4thvm_here) {
			QLForth_printf("Delete Symbol : %s\n", spc->name);
			spc = spc->link;
		}
		root_bucket[idx]= spc;
		while (spc != NULL) {
			if (spc->fun == fp_dovocabulary) {
				QLForth_printf("Enter Vocabulary : %s\n", spc->name);
				vpc = (Symbol **) spc->dfa;
				for (subidx = 0; subidx < HASH_SIZE; subidx ++) {
					subspc = * (vpc + subidx);
					while ((void *) subspc >= (void *) ql4thvm_here) {
						QLForth_printf(" -- Delete Symbol : %s\n", subspc->name);
						subspc = subspc->link;
					}
	    			* (vpc + subidx) = subspc;
				}
			}
			spc = spc->link;
		}
	}
}

static void qlforth_init(void) {
	Primitive * ppc;
	Symbol	* spc;

	ql4thvm_dict = ql4thvm_here = &dict_buffer[0];
	ql4thvm_dict_top = &dict_buffer[DICT_BUFFER_SIZE];
	memset(dict_buffer, 0, sizeof(dict_buffer));
	memset(root_bucket, 0, sizeof(root_bucket));
	memset(forth_bucket, 0, sizeof(forth_bucket));

	forth_only();
	current = root_bucket;

	for (ppc = immediate_table; ppc->pname != NULL; ppc++) {
		spc = QLForth_symbol_add(ppc->pname);
		spc->fun = ppc->fun;
		spc->type = QLF_TYPE_IMMEDIATE;
	}

	Core_init();
	current = forth_bucket;
	Forth_init();

	current = root_bucket;
	ql4thvm_guard = ql4thvm_here;
	forth_clear();
}

// ************************************************************************************

void QLForth_error(char * msg, char * tk) {
	char buffer[256], * pc;
	int  line;

	sprintf(buffer, msg, tk);
	for (line = 1, pc = text_ptr ; pc < scan_ptr ; pc ++) {
		if (* pc == 0x0a) {					// if Linux new line
			line ++ ;	
			pc ++; 
		}
		else if (* pc == 0x0d) {			// if MAC OS new line
			line ++; 
			if (* (pc + 1) == 0x0a) pc ++;	// skip MS-DOS CR LF
		}
	}
	err_flag = line;
	if (line == 1) {
		QLForth_printf("Error : %s\n", buffer);
	}
	else {
		QLForth_printf("Error at line %d : %s\n", line, buffer);
	}

	longjmp(e_buf, -1) ;	
}

int QLForth_token(void) {
	int idx;

	while (*scan_ptr != 0 && is_space(*scan_ptr)) scan_ptr++;
	if (*scan_ptr == 0) return 0;

	// get a word seperated by BLANK 
	idx = 0;
	while ((*scan_ptr != 0) && (idx < (TEXT_LINE_SIZE - 2)) && !is_space(*scan_ptr)) {
		token_word[idx++] = *scan_ptr++;
	}
	token_word[idx] = 0;
	return idx;
}

Symbol * QLForth_symbol_search(char * name) {
	Symbol * spc, *** vpc;
	int		h;

	h = hash_bucket((unsigned char *)name);
	for (vpc = search_order; *vpc != NULL; vpc++) {
		spc = *((*vpc) + h);
		while (spc != NULL) {
			if ((!spc->hidden) && strcmp(spc->name, name) == 0) {
				return spc;
			}
			spc = spc->link;
		}
	}
	return NULL;
}

Symbol * QLForth_symbol_add(char * name) {
	QLF_CELL	i;
	Symbol	    * spc;

	_strupr(name);
	i = sizeof(Symbol) + strlen(name);
	spc = (Symbol *)qlforth_alloc(i);
	strcpy(spc->name, name);
	i = hash_bucket((unsigned char *)name);
	spc->link = current[i];
	current[i] = spc;
	return spc;
}

Symbol * QLForth_create(void) {
	if (!QLForth_token()) {
		QLForth_error("Expect a word", NULL);
	}
	return QLForth_symbol_add(token_word);
}

// ************************************************************************************

void QLForth_init(char * w_path) {
	if (w_path != NULL) {
		strcpy(system_directory, w_path);
	}

	qlforth_init();
	if (qlforth_text_buffer = malloc(TEXT_BUFFER_STEP)) {
		qlforth_text_length = TEXT_BUFFER_STEP;
	}
}

char * QLForth_preparation(int size, char * w_file) {
	int cnt;

	for (cnt = 0; ql4thvm_running && cnt < 10; cnt++) {
		ql4thvm_force_break = 0xff;
		Sleep(100);
	}

	if (size > 0) {
		if (w_file != NULL) {
			strcpy(working_directory, w_file);
			qlforth_init();
		}
		else {
			forth_forget();
		}

		if (size > qlforth_text_length) {
			free(qlforth_text_buffer);
			qlforth_text_length = size + TEXT_BUFFER_STEP;
			qlforth_text_buffer = malloc(size + TEXT_BUFFER_STEP);
		}
	}
	return qlforth_text_buffer;
}

void QLForth_interpreter(void) {
	Symbol * spc;

	scan_ptr = text_ptr = qlforth_text_buffer;
	load_text = load_save_scan_ptr = NULL;

	ql4thvm_running = 0xff;
	ql4thvm_force_break = 0;
	if (setjmp(e_buf) == 0) {
		while (!ql4thvm_force_break) {
			while (QLForth_token() != 0 && !ql4thvm_force_break) {
				_strupr(token_word);
				if ((spc = QLForth_symbol_search(token_word)) != NULL) {
					switch (ql4thvm_state) {
					case QLF_STATE_INTERPRET:
						switch (spc->type) {
						case QLF_TYPE_PRIMITIVE:	qlforth_execute(spc);				break;
						case QLF_TYPE_CONSTANT:		qlforth_execute(spc);				break;
						case QLF_TYPE_DEFINE:		qlforth_execute(spc);				break;
						case QLF_TYPE_VARIABLE:		QLForth_debug_variable(spc->dfa);	break;
						case QLF_TYPE_WORD:			QLForth_debug_word(spc->dfa);		break;
						default:					qlforth_mixed_word_used();			break;
						}
						break;

					case QLF_STATE_CORE:
						switch (spc->type) {
						case QLF_TYPE_PRIMITIVE:	*(Symbol **)ql4thvm_here++ = spc; 	break;
						case QLF_TYPE_CONSTANT:		*(Symbol **)ql4thvm_here++ = spc; 	break;
						case QLF_TYPE_IMMEDIATE:	qlforth_execute(spc);				break;
						default:					qlforth_mixed_word_used();			break;
						}
						break;

					case QLF_STATE_COMPILE:
						switch (spc->type) {
						case QLF_TYPE_WORD:			QLForth_compile_word(spc->dfa);		break;
						case QLF_TYPE_CONSTANT:		QLForth_compile_literal(spc->ival); break;
						case QLF_TYPE_VARIABLE:		QLForth_compile_variable(spc->ival); break;
						case QLF_TYPE_CORE:			qlforth_execute(spc);				break;
						case QLF_TYPE_COMPILE:		qlforth_execute(spc);
						default:					qlforth_mixed_word_used();			break;
						}
						break;
					}
				}
				else if (forth_number(token_word)) {						// Literal
					*ql4thvm_dp++ = ql4thvm_tos;
					ql4thvm_tos = token_value.ival;
				}
				else {
					QLForth_error("\'%s\' : word not found.", token_word);
				}
			}

			if (ql4thvm_state != QLF_STATE_INTERPRET) {
				QLForth_error("Unexpect END-OF-File", NULL);
			}

			if (load_text == NULL && load_save_scan_ptr == NULL) {
				break;
			}

			free(load_text);
			scan_ptr = load_save_scan_ptr;
			load_text = load_save_scan_ptr = NULL;
		}
		QLForth_display_stack(display_number_base, ql4thvm_dp - ql4thvm_stack, ql4thvm_tos, *(ql4thvm_dp - 1));
	}
	ql4thvm_running = 0;
}
