/* ************************************************************************************
* Copyright (C) 2015-2017 Shaanxi QinWei Electroics Technologies. All rights reserved.
*
* $Date:        18 Oct 2017
* $Revision: 	V 0.6
*
* Project: 	    QLForth - A Forth for System-On-Chip Creative Makers
*
* Title:	    QLForth.c
*
* Description:	The main entry for QLForth
*
* Target:		Windows 7+ / Linux / Mac OS X
*
* Author:		Zhao Yu, forthchina@163.com, forthchina@gmail.com
*			    19/10/2017 (In dd/mm/yyyy)
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

#include "QLForth.h"

QLF_CELL		token_value, * ql4thvm_here,	
				ql4thvm_tos, *ql4thvm_dp, *ql4thvm_rp, *ql4thvm_stack_top, *ql4thvm_stack;
char			token_word[TEXT_LINE_SIZE];
Symbol			** program_counter, *ThisCreateWord, *ThisExecuteWord;
int				ql4thvm_state, ql4thvm_running, ql4thvm_force_break;

static QLF_CELL dict_buffer[DICT_BUFFER_SIZE + 4], data_stack[STACK_DEEP_SIZE + 2],
				* ql4thvm_dict_top, * ql4thvm_dict, * ql4thvm_guard;
static char		* scan_ptr,* text_ptr, * qlforth_text_buffer, interpret_text[TEXT_LINE_SIZE];
static Symbol	* root_bucket[HASH_SIZE + 1], * forth_bucket[HASH_SIZE + 1], 
				* working_bucket[HASH_SIZE + 1], ** current;
static int		err_flag, display_number_base = 10;
static SSTNode  sst_buffer[SST_NODE_MAX + 1], *sst_current, * sst_hot_spot;
static jmp_buf  e_buf;

// ************  Scanner functions , Text process *************************************

static int is_space(int ch) {
	return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

static int forth_number(char * str) {
	char	negate, base, *ptr, *end_dptr;

	ptr = str;
	negate = 0;
	base = 10;
	if (*ptr == '-') {
		ptr++;
		negate = -1;
	}
	if (*ptr == '%') {
		ptr++;
		base = 2;
	}
	else if (*ptr == '0' && (ptr[1] == 'x' || ptr[1] == 'X')) {
		ptr += 2;
		base = 16;
	}
	token_value.ival = (int) strtoul(ptr, &end_dptr, base);

	if (*end_dptr == '\0') {
		if (negate) token_value.ival = -token_value.ival;
		return -1;
	}

	token_value.fval = (float)strtod(ptr, &end_dptr);
	if (*end_dptr != '\0') return 0;
	if (negate)  token_value.fval = -token_value.fval;
	return 1;
}

// ************  Symbol table management, part of hash processing  ********************

static unsigned int hash_bucket(unsigned char * name) {
	unsigned int  h;

	for (h = 0; *name; h += *name++);
	return h  % HASH_SIZE;
}

static Symbol * qlforth_root_search(char * name) {
	Symbol * spc;
	int		h;

	h = hash_bucket((unsigned char *)name);
	spc = root_bucket[h];
	while (spc != NULL) {
		if ((!spc->hidden) && strcmp(spc->name, name) == 0) {
			return spc;
		}
		spc = spc->link;
	}
	return NULL;
}

// ************  Target forth support and interpreter help functions ******************

static void qlforth_push(QLF_CELL * vptr) {
	ql4thvm_dp->ptr  = ql4thvm_tos.ptr;
	ql4thvm_dp++;
	ql4thvm_tos.ptr = vptr->ptr;
}

void * qlforth_alloc(int size_in_byte) {
	char * pc;

    pc = NULL;
	size_in_byte = (size_in_byte - 1) / (sizeof(QLF_CELL)) + 1;
	if ((ql4thvm_here + size_in_byte + 1) > ql4thvm_dict_top) {
		QLForth_error("Memory Full", NULL);
	}
	else {
		pc = (char *)ql4thvm_here;
		memset(pc, 0, (size_in_byte + 1)* (sizeof(QLF_CELL)));
		ql4thvm_here += size_in_byte;
	}
	return (void *)pc;
}

static void fp_doliteral(void) {
	ql4thvm_dp->ptr = ql4thvm_tos.ptr;
	ql4thvm_dp++;
	ql4thvm_tos.ival = *(int *) program_counter++;
}

static void macro_execute(Symbol * spc) {
	ThisExecuteWord = spc;
	program_counter = NULL;
	ThisExecuteWord->fun();
	while (program_counter && !ql4thvm_force_break) {
		ThisExecuteWord = *program_counter++;
		ThisExecuteWord->fun();
	}
	ThisExecuteWord = NULL;
}

static void debug_word(Symbol * spc) {
	int cnt;

	*ql4thvm_dp = ql4thvm_tos;
	cnt = (int) (ql4thvm_dp - ql4thvm_stack);

	cnt = QLForth_chip_execute(spc->value.ival, cnt, ql4thvm_stack);

	ql4thvm_dp = &data_stack[cnt];
	ql4thvm_tos = *ql4thvm_dp;
}

static void debug_variable(Symbol * spc) {

}

static void load_file_to_text(char * fname) {
	FILE	* fd;
	int		size;

	qlforth_text_buffer = NULL;
	if ((fd = fopen(fname, "rb")) == NULL) {
		QLForth_printf("Can not open file : %s\n", fname);
	}
	else {
		fseek(fd, 0L, SEEK_END);
		size = (int) ftell(fd);
		fseek(fd, 0L, SEEK_SET);
		if ((qlforth_text_buffer = (char *)malloc(size + 4)) == NULL) {
			QLForth_printf("Out of Memory.");
			fclose(fd);
		}
		else {
			size = (int) fread(qlforth_text_buffer, 1, size, fd);
			fclose(fd);
			*(qlforth_text_buffer + size) = 0;
			*(qlforth_text_buffer + size + 1) = 0;
		}
	}
}

static void qlforth_interactive(void) {
	Symbol * spc;

	if ((spc = QLForth_symbol_search(token_word)) != NULL) {
		switch (spc->type) {
			case QLF_TYPE_COMMAND:		spc->fun();						break;
			case QLF_TYPE_PRIMITIVE:	spc->fun();						break;
			case QLF_TYPE_MACRO:		macro_execute(spc);				break;
			case QLF_TYPE_WORD:			debug_word(spc);				break;
			case QLF_TYPE_VARIABLE:		debug_variable(spc);			break;
			case QLF_TYPE_CONSTANT:		qlforth_push(& (spc->value));	break;
			case QLF_TYPE_COMPILE:
				if ((spc = qlforth_root_search(token_word)) != NULL) {
					if (spc->type == QLF_TYPE_PRIMITIVE) {
						spc->fun();									break;
					}
				}
				// fall into
			default:
				QLForth_error("Word - %s - cannot be used in INTERACTIVE mode", token_word);
				break;
		}
	}
	else if (forth_number(token_word)) {
		qlforth_push(&token_value);
	}
	else {
		QLForth_error("\'%s\' : word not found.[1]", token_word);
	}
}

static void qlforth_interpret(void) {
	Symbol * spc;

	if ((spc = qlforth_root_search(token_word)) == NULL) {
		spc = QLForth_symbol_search(token_word);
	}
	if (spc != NULL) {
		switch (spc->type) {
			case QLF_TYPE_COMMAND:		spc->fun();						break;
			case QLF_TYPE_DEFINE:		spc->fun();						break;
			case QLF_TYPE_PRIMITIVE:	spc->fun();						break;
			case QLF_TYPE_MACRO:		macro_execute(spc);				break;
			case QLF_TYPE_CONSTANT:		qlforth_push(&(spc->value));	break;
			default:
				QLForth_error("Word - %s - cannot be used in INTERPRET mode", token_word);
				break;
		}
	}
	else if (forth_number(token_word)) {
		qlforth_push(& token_value);
	}
	else {
		QLForth_error("\'%s\' : word not found.[2]", token_word);
	}
}

static void qlforth_compile(void) {
	Symbol * spc;

	if ((spc = QLForth_symbol_search(token_word)) != NULL) {
		switch (spc->type) {
			case QLF_TYPE_PRIMITIVE:	spc->fun();											break;
			case QLF_TYPE_COMPILE:		spc->fun();											break;
			case QLF_TYPE_IMMEDIATE:	spc->fun();											break;
			case QLF_TYPE_MACRO:		macro_execute(spc);									break;
			case QLF_TYPE_CONSTANT:		qlforth_push(&(spc->value));						break;
			case QLF_TYPE_WORD:			QLForth_sst_append(SST_WORD_REF, (SSTNode *) spc);	break;
			case QLF_TYPE_VARIABLE:		QLForth_sst_append(SST_VAR_REF,	 (SSTNode *) spc);	break;
			default:
				QLForth_error("Word - %s - cannot be used in COMPILE mode", token_word);
				break;

		}
	}
	else if (forth_number(token_word)) {
		qlforth_push(& token_value);
	}
	else {
		QLForth_error("\'%s\' : word not found. [3]", token_word);
	}
}

static void qlforth_macro(void) {
	static Symbol symbol_do_literal = { NULL,{ fp_doliteral },{ "(DOLIT)" },	0 };

	Symbol * spc;

	if ((spc = qlforth_root_search(token_word)) == NULL) {
		spc = QLForth_symbol_search(token_word);
	}
	if (spc != NULL) {
		switch (spc->type) {
			case QLF_TYPE_IMMEDIATE:	spc->fun();							break;
			case QLF_TYPE_CONSTANT:		
			case QLF_TYPE_PRIMITIVE:	
			case QLF_TYPE_MACRO:		*(Symbol **)ql4thvm_here = spc;	
										ql4thvm_here++;						break;
			default:
				QLForth_error("Word - %s - cannot be used in Macro mode", token_word);
				break;
		}
	}
	else if (forth_number(token_word)) {
		*(Symbol **)ql4thvm_here++ = &symbol_do_literal;
		*(int *)ql4thvm_here = token_value.ival;
		ql4thvm_here++;
	}
	else {
		QLForth_error("\'%s\' : word not found. [3]", token_word);
	}
}

static void ql4th_interpreter(char * text) {
	char * p;
	scan_ptr = text_ptr = text;

	if (setjmp(e_buf) == 0) {
		while (QLForth_token() != 0) {
			for (p = token_word; *p; ++ p) * p = toupper(*p);
			switch (ql4thvm_state) {
				case QLF_STATE_INTERACTIVE: qlforth_interactive();					break;
				case QLF_STATE_INTERPRET:	qlforth_interpret();					break;
				case QLF_STATE_COMPILE:		qlforth_compile();						break;
				case QLF_STATE_MACRO:		qlforth_macro();						break;
			}
		}

		if (ql4thvm_state != QLF_STATE_INTERPRET && ql4thvm_state != QLF_STATE_INTERACTIVE) {
			QLForth_error("Unexpect END-OF-File", NULL);
		}
	}
}

// ************  CONSTANT, WORD, COLON, Complier words ********************************

static void fp_doconst(void) {
	ql4thvm_dp->ptr = ql4thvm_tos.ptr;
	ql4thvm_dp++;
	ql4thvm_tos.ival = ThisExecuteWord->value.ival;
}

static void cfp_constant(void) {
	Symbol * spc;

	if (!QLForth_token()) {
		QLForth_error("Expect a word", NULL);
	}

	if ((spc = QLForth_symbol_add(token_word)) == NULL) {
		return;
	}

	spc->type	= QLF_TYPE_CONSTANT;
	spc->fun	= fp_doconst;
	spc->dfa	= (QLF_CELL *) ql4thvm_tos.ptr;
	ql4thvm_dp--;
	ql4thvm_tos.ival = ql4thvm_dp->ival;
}

// ************  Forth Command for developers *****************************************

static void forth_clear(void) {
	ql4thvm_stack = ql4thvm_dp = &data_stack[0];
	ql4thvm_stack_top = ql4thvm_rp = &data_stack[STACK_DEEP_SIZE - 1];
}

static void forth_make(void) {

}

static void forth_quit(void) {
    exit(0);
}

static void cfp_dot_s(void) {
	int cnt;

	* ql4thvm_dp = ql4thvm_tos;
	for (cnt = (int) (ql4thvm_dp - ql4thvm_stack); cnt > 0; cnt--) {
		QLForth_printf("[%d] %d,  0x%08X\n", cnt, ql4thvm_stack[cnt], ql4thvm_stack[cnt]);
	}
}

static void cfp_see(void) {
	Symbol * spc;
	char * p;

	if (!QLForth_token()) {
		QLForth_error("Expect a word", NULL);
	}

	for (p = token_word; *p; ++p) * p = toupper(*p);
	if ((spc = QLForth_symbol_search(token_word)) == NULL) {
		QLForth_error("word - %s - not found.\n", token_word); 
		return;
	}
	switch (spc->type) {
		case QLF_TYPE_COMMAND:		QLForth_printf("Command.\n");									break;
		case QLF_TYPE_PRIMITIVE:	QLForth_printf("Primitive.\n");									break;
		case QLF_TYPE_CONSTANT:		QLForth_printf("Constant %d, 0x%08X.\n", spc->value.ival, spc->value.ival);	break;
		case QLF_TYPE_COMPILE:		QLForth_printf("Compile word.\n");								break;
		case QLF_TYPE_WORD:			QLForth_sst_list(spc-> sst, sst_buffer);						break;
		case QLF_TYPE_VARIABLE:		QLForth_printf("Variable @0x%08X.\n", spc->dfa);				break;
	}
}

// ************  Primitive entry table  ***********************************************

static Primitive command_table[] = {
	{ "MAKE",			forth_make			},
    { "QUIT",           forth_quit          },

	{ ".S",				cfp_dot_s			},
	{ "SEE",			cfp_see				},
	{ "CLEAR",			forth_clear			},

	{ NULL,				NULL 	}
};

static Primitive define_table[] = {
	{ "CONSTANT",		cfp_constant		},
	
	{ NULL,				NULL	}

} ;

static Primitive immediate_table[] = {

	{ NULL,			NULL }
};

static Primitive primitive_table[] = {

	{ NULL,				NULL }
};

// ************  Internal Shared Functions ********************************************

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
	QLForth_report(1, line - 1, (int) (scan_ptr - text_ptr), buffer);
	longjmp(e_buf, -1) ;	
}

int QLForth_token(void) {
	int idx;

	while (1) {
		while (*scan_ptr != 0 && is_space(*scan_ptr)) scan_ptr++;
		if (*scan_ptr == 0) return 0;

		// get a word seperated by BLANK 
		idx = 0;
		while ((*scan_ptr != 0) && (idx < (TEXT_LINE_SIZE - 2)) && !is_space(*scan_ptr)) {
			token_word[idx++] = *scan_ptr++;
		}
		token_word[idx] = 0;

		if (idx == 1 && token_word[0] == '(') {
			while (*scan_ptr != 0 && * scan_ptr != ')') {
				scan_ptr++;
			}
			if (*scan_ptr == ')') {
				scan_ptr++;
			}
			continue;
		}
		if (idx == 1 && token_word[0] == '\\') {
			while (*scan_ptr != 0) {
				if (*scan_ptr == 0x0d || *scan_ptr == 0x0a) {
					break;
				}
				scan_ptr++;
			}
			continue;
		}
		break;
	}
	return 1;
}

Symbol * QLForth_symbol_search(char * name) {
	static Symbol ** search_order[] = { working_bucket, forth_bucket, root_bucket, NULL };
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
	int	i;
	Symbol	    * spc;
	char		* p;

	for (p = name; *p; ++p) {
		if (islower(*p)) {
			*p = toupper(*p);
		}
	}
	i	= sizeof(Symbol) + (int) strlen(name);
	spc = (Symbol *) qlforth_alloc(i);
	strcpy(spc->name, name);
	i	= hash_bucket((unsigned char *)name);
	spc->link = current[i];
	current[i] = spc;
	return spc;
}

SSTNode * QLForth_sst_append(int type, SSTNode * pc) {
	SSTNode * ptr;

	if (sst_current > & sst_buffer[SST_NODE_MAX]) {
		QLForth_error("Out of SST Node", NULL);
	}
	ptr			= sst_current++;
	ptr->type	= type;
	ptr->sst	= pc;
	return ptr;
}

// ************  QLForth Interface ****************************************************

void QLForth_init(char * fname) {
	Primitive	* ppc;
	Symbol		* spc;

	ql4thvm_dict		= ql4thvm_here = &dict_buffer[0];
	ql4thvm_dict_top	= &dict_buffer[DICT_BUFFER_SIZE];
	memset(dict_buffer,		0, sizeof(dict_buffer));
	memset(root_bucket,		0, sizeof(root_bucket));
	memset(forth_bucket,	0, sizeof(forth_bucket));
	memset(working_bucket,	0, sizeof(working_bucket));

	current = root_bucket;
	for (ppc = command_table; ppc->pname != NULL; ppc++) {
		spc = QLForth_symbol_add(ppc->pname);
		spc->fun = ppc->fun;
		spc->type = QLF_TYPE_COMMAND;
	}

	for (ppc = define_table; ppc->pname != NULL; ppc++) {
		spc = QLForth_symbol_add(ppc->pname);
		spc->fun = ppc->fun;
		spc->type = QLF_TYPE_DEFINE;
	}

	for (ppc = immediate_table; ppc->pname != NULL; ppc++) {
		spc = QLForth_symbol_add(ppc->pname);
		spc->fun = ppc->fun;
		spc->type = QLF_TYPE_COMPILE;
	}

	for (ppc = primitive_table; ppc->pname != NULL; ppc++) {
		spc = QLForth_symbol_add(ppc->pname);
		spc->fun = ppc->fun;
		spc->type = QLF_TYPE_PRIMITIVE;
	}
	
	Forth_init();
	current = forth_bucket;
	Compile_init();
	Code_init();

	current			= root_bucket;
	ql4thvm_stack	= ql4thvm_dp	= &data_stack[0];
	ql4thvm_stack_top = ql4thvm_rp	= &data_stack[STACK_DEEP_SIZE - 1];
	sst_hot_spot	= sst_current	= &sst_buffer[0];
	ql4thvm_state	= QLF_STATE_INTERACTIVE;

	load_file_to_text(fname);
	if (qlforth_text_buffer != NULL) {
		ql4thvm_state = QLF_STATE_INTERPRET;
		ql4th_interpreter(qlforth_text_buffer);
		free(qlforth_text_buffer);
		qlforth_text_buffer = NULL;
		sst_current->type = SST_END;
		Code_generation(sst_hot_spot);
		ql4thvm_guard	= ql4thvm_here;
		sst_hot_spot	= sst_current;
		QLForth_report(0, (int) (ql4thvm_here - ql4thvm_dict) * sizeof(QLF_CELL), DICT_BUFFER_SIZE, NULL);
		// display_symbole_table(root_bucket);
	}
	current = working_bucket;
}

void QLForth_interpreter(char * text) {
	strcpy(interpret_text, text);
	ql4thvm_state = QLF_STATE_INTERACTIVE;
	ql4th_interpreter(interpret_text);
	QLForth_display_stack(display_number_base, (int) (ql4thvm_dp - ql4thvm_stack), & ql4thvm_tos, ql4thvm_dp - 1);
	if (qlforth_text_buffer != NULL) {
		free(qlforth_text_buffer);
		qlforth_text_buffer = NULL;
	}
}

void QLForth_stop(void) {
}
