/* ************************************************************************************
* Copyright (C) 2015-2017 Shaanxi QinWei Electroics Technologies. All rights reserved.
*
* $Date:        11 May 2017
* $Revision: 	V 0.5
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
*			    25/04/2017 (In dd/mm/yyyy)
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

QLF_CELL		ql4thvm_tos, *ql4thvm_dp, *ql4thvm_rp, *ql4thvm_stack_top, *ql4thvm_stack;
char			token_word[TEXT_LINE_SIZE];

static Symbol	* ThisCreateWord, *ThisExecuteWord ;
static QLF_CELL dict_buffer[DICT_BUFFER_SIZE + 4], data_stack[STACK_DEEP_SIZE + 2],
				* ql4thvm_dict_top, * ql4thvm_dict, * ql4thvm_guard, * ql4thvm_here;
static char		* scan_ptr,* text_ptr, * qlforth_text_buffer, interpret_text[TEXT_LINE_SIZE];
static Symbol	* root_bucket[HASH_SIZE + 1], * forth_bucket[HASH_SIZE + 1], 
				* working_bucket[HASH_SIZE + 1], ** current;
static int		err_flag, ql4thvm_state, display_number_base = 10;
static SSTNode  sst_buffer[SST_NODE_MAX + 1], *sst_current, * sst_hot_spot;
static jmp_buf  e_buf;

static CONTROL_STACK cs_stack[MAX_CONTROL_STACK_DEEP + 1], *cs_ptr;
static QLF_LITERAL	 token_value;


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
	token_value.ival = (QLF_CELL) strtoul(ptr, &end_dptr, base);

	if (*end_dptr == '\0') {
		if (negate) token_value.ival = -token_value.ival;
		return -1;
	}

	token_value.fval = (float)strtod(ptr, &end_dptr);
	if (*end_dptr != '\0') return 0;
	if (negate)  token_value.fval = -token_value.fval;
	return 1;
}

// ************  Symbol table management, part of has processing  *********************

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

static void qlforth_push(int val) {
	*ql4thvm_dp++ = ql4thvm_tos;
	ql4thvm_tos = val;
}

void qlforth_fp_docreate(void) {
	*ql4thvm_dp++ = ql4thvm_tos;
	ql4thvm_tos = ThisExecuteWord->dfa;
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

static void debug_word(Symbol * spc) {
	int cnt;

	*ql4thvm_dp = ql4thvm_tos;
	cnt = (QLF_CELL) (ql4thvm_dp - ql4thvm_stack);

	cnt = QLForth_chip_execute(spc->dfa, cnt, ql4thvm_stack);

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
		size = (QLF_CELL) ftell(fd);
		fseek(fd, 0L, SEEK_SET);
		if ((qlforth_text_buffer = (char *)malloc(size + 4)) == NULL) {
			QLForth_printf("Out of Memory.");
			fclose(fd);
		}
		else {
			size = (QLF_CELL) fread(qlforth_text_buffer, 1, size, fd);
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
			case QLF_TYPE_COMMAND:		spc->fun();					break;
			case QLF_TYPE_PRIMITIVE:	spc->fun();					break;
			case QLF_TYPE_WORD:			debug_word(spc);			break;
			case QLF_TYPE_VARIABLE:		debug_variable(spc);		break;
			case QLF_TYPE_CONSTANT:		qlforth_push(spc->ival);	break;
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
		qlforth_push(token_value.ival);
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
			case QLF_TYPE_COMMAND:		spc->fun();					break;
			case QLF_TYPE_DEFINE:		spc->fun();					break;
			case QLF_TYPE_PRIMITIVE:	spc->fun();					break;
			case QLF_TYPE_CONSTANT:		qlforth_push(spc->ival);	break;
			default:
				QLForth_error("Word - %s - cannot be used in INTERPRET mode", token_word);
				break;
			}
	}
	else if (forth_number(token_word)) {
		qlforth_push(token_value.ival);
	}
	else {
		QLForth_error("\'%s\' : word not found.[2]", token_word);
	}
}

static void qlforth_compile(void) {
	Symbol * spc;

	if ((spc = QLForth_symbol_search(token_word)) != NULL) {
		switch (spc->type) {
			case QLF_TYPE_PRIMITIVE:	spc->fun();					break;
			case QLF_TYPE_COMPILE:		spc->fun();					break;
			case QLF_TYPE_CONSTANT:		qlforth_push(spc->ival);	break;
			case QLF_TYPE_WORD:			QLForth_sst_append(SST_WORD_REF, (SSTNode *) spc); break;
			case QLF_TYPE_VARIABLE:		QLForth_sst_append(SST_VAR_REF,	 (SSTNode *) spc); break;
			default:
				QLForth_error("Word - %s - cannot be used in COMPILE mode", token_word);
				break;

		}
	}
	else if (forth_number(token_word)) {
		qlforth_push(token_value.ival);
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
			}
		}

		if (ql4thvm_state != QLF_STATE_INTERPRET && ql4thvm_state != QLF_STATE_INTERACTIVE) {
			QLForth_error("Unexpect END-OF-File", NULL);
		}
	}
}

// ************  Primitive, execution only on host like macro *************************

static void cfp_dup(void) {
	*ql4thvm_dp++ = ql4thvm_tos;
}

static void cfp_qdup(void) {
	if (ql4thvm_tos) {
		*ql4thvm_dp++ = ql4thvm_tos;
	}
}

static void cfp_drop(void) {
	ql4thvm_tos = *(--ql4thvm_dp);
}

static void cfp_over(void) {
	*ql4thvm_dp++ = ql4thvm_tos;
	ql4thvm_tos = *(ql4thvm_dp - 2);
}

static void cfp_swap(void) {
	QLF_CELL tmp;

	tmp = *(ql4thvm_dp - 1);
	*(ql4thvm_dp - 1) = ql4thvm_tos;
	ql4thvm_tos = tmp;
}

static void cfp_rot(void) {
	QLF_CELL tmp;

	tmp = ql4thvm_tos;
	ql4thvm_tos = *(ql4thvm_dp - 2);
	*(ql4thvm_dp - 2) = *(ql4thvm_dp - 1);
	*(ql4thvm_dp - 1) = tmp;
}

static void cfp_mrot(void) {
	QLF_CELL tmp;

	tmp = ql4thvm_tos;
	ql4thvm_tos = *(ql4thvm_dp - 1);
	*(ql4thvm_dp - 1) = *(ql4thvm_dp - 2);
	*(ql4thvm_dp - 2) = tmp;
}

static void cfp_pick(void) {
	if (&ql4thvm_stack[ql4thvm_tos] < ql4thvm_dp) {
		ql4thvm_tos = *(ql4thvm_dp - ql4thvm_tos - 1);
	}
}

static void cfp_roll(void) {
	QLF_CELL * ptr;

	if (&ql4thvm_stack[ql4thvm_tos] < ql4thvm_dp) {
		ptr = ql4thvm_dp - ql4thvm_tos - 1;
		ql4thvm_tos = *ptr;
		for (; ptr < ql4thvm_dp; ptr++) {
			*ptr = *(ptr + 1);
		}
		ql4thvm_dp--;
	}
}

static void cfp_depth(void) {
	*ql4thvm_dp++ = ql4thvm_tos;
	ql4thvm_tos = (QLF_CELL) (ql4thvm_dp - &ql4thvm_stack[1]);
}

static void cfp_plus(void) {
	ql4thvm_tos += *(--ql4thvm_dp);
}

static void cfp_minus(void) {
	QLF_CELL nos;

	nos = *(--ql4thvm_dp);
	ql4thvm_tos = nos - ql4thvm_tos;
}

static void cfp_mul(void) {
	ql4thvm_tos *= *(--ql4thvm_dp);
}

static void cfp_div(void) {
	QLF_CELL nos;

	nos = *(--ql4thvm_dp);
	ql4thvm_tos = nos / ql4thvm_tos;
}

static void cfp_mod(void) {
	QLF_CELL nos;

	nos = *(--ql4thvm_dp);
	ql4thvm_tos = nos % ql4thvm_tos;
}

static void cfp_divmod(void) {
	QLF_CELL nos, quot;

	if (ql4thvm_tos != 0) {
		nos = *(ql4thvm_dp - 1);
		quot = nos / ql4thvm_tos;
		*(ql4thvm_dp - 1) = nos % ql4thvm_tos;
		ql4thvm_tos = quot;
	}
}

static void cfp_1_add(void) {
	ql4thvm_tos++;
}

static void cfp_1_sub(void) {
	ql4thvm_tos--;
}

static void cfp_2_add(void) {
	ql4thvm_tos += 2;
}

static void cfp_2_sub(void) {
	ql4thvm_tos -= 2;
}

static void cfp_2_mul(void) {
	ql4thvm_tos <<= 1;
}

static void cfp_2_div(void) {
	ql4thvm_tos >>= 1;
}

static void cfp_4_add(void) {
	ql4thvm_tos += 4;
}

static void cfp_4_sub(void) {
	ql4thvm_tos -= 4;
}

static void cfp_min(void) {
	QLF_CELL nos;

	nos = *(--ql4thvm_dp);
	ql4thvm_tos = (nos < ql4thvm_tos) ? nos : ql4thvm_tos;
}

static void cfp_max(void) {
	QLF_CELL nos;

	nos = *(--ql4thvm_dp);
	ql4thvm_tos = (nos > ql4thvm_tos) ? nos : ql4thvm_tos;
}

static void cfp_abs(void) {
	ql4thvm_tos = (ql4thvm_tos >= 0) ? ql4thvm_tos : (-ql4thvm_tos);
}

static void cfp_negate(void) {
	ql4thvm_tos = -ql4thvm_tos;
}

static void cfp_cmp_lt(void) {
	QLF_CELL nos;

	nos = *(--ql4thvm_dp);
	ql4thvm_tos = (nos < ql4thvm_tos) ? QLF_TRUE : QLF_FALSE;
}

static void cfp_cmp_le(void) {
	QLF_CELL nos;

	nos = *(--ql4thvm_dp);
	ql4thvm_tos = (nos <= ql4thvm_tos) ? QLF_TRUE : QLF_FALSE;
}

static void cfp_cmp_ne(void) {
	QLF_CELL nos;

	nos = *(--ql4thvm_dp);
	ql4thvm_tos = (nos != ql4thvm_tos) ? QLF_TRUE : QLF_FALSE;
}

static void cfp_cmp_eq(void) {
	QLF_CELL nos;

	nos = *(--ql4thvm_dp);
	ql4thvm_tos = (nos == ql4thvm_tos) ? QLF_TRUE : QLF_FALSE;
}

static void cfp_cmp_gt(void) {
	QLF_CELL nos;

	nos = *(--ql4thvm_dp);
	ql4thvm_tos = (nos > ql4thvm_tos) ? QLF_TRUE : QLF_FALSE;
}

static void cfp_cmp_ge(void) {
	QLF_CELL nos;

	nos = *(--ql4thvm_dp);
	ql4thvm_tos = (nos >= ql4thvm_tos) ? QLF_TRUE : QLF_FALSE;
}

static void cfp_cmp_0_lt(void) {
	ql4thvm_tos = (ql4thvm_tos < 0) ? QLF_TRUE : QLF_FALSE;
}

static void cfp_cmp_0_eq(void) {
	ql4thvm_tos = (ql4thvm_tos == 0) ? QLF_TRUE : QLF_FALSE;
}

static void cfp_cmp_0_gt(void) {
	ql4thvm_tos = (ql4thvm_tos > 0) ? QLF_TRUE : QLF_FALSE;
}

static void cfp_and(void) {
	ql4thvm_tos &= *(--ql4thvm_dp);
}

static void cfp_or(void) {
	ql4thvm_tos |= *(--ql4thvm_dp);
}

static void cfp_xor(void) {
	ql4thvm_tos ^= *(--ql4thvm_dp);
}

static void cfp_not(void) {
	ql4thvm_tos = ~ql4thvm_tos;
}

static void cfp_f_plus(void) {
	QLF_LITERAL data;

	data.ival = ql4thvm_tos;
	data.fval += *(QLF_REAL *)(--ql4thvm_dp);
	ql4thvm_tos = data.ival;
}

static void cfp_f_minus(void) {
	QLF_LITERAL data;

	data.ival = ql4thvm_tos;
	data.fval = (*(QLF_REAL *)(--ql4thvm_dp)) - data.fval;
	ql4thvm_tos = data.ival;
}

static void cfp_f_mul(void) {
	QLF_LITERAL data;

	data.ival = ql4thvm_tos;
	data.fval *= *(QLF_REAL *)(--ql4thvm_dp);
	ql4thvm_tos = data.ival;
}

static void cfp_f_div(void) {
	QLF_LITERAL data;

	data.ival = ql4thvm_tos;
	data.fval = (*(QLF_REAL *)(--ql4thvm_dp)) / data.fval;
	ql4thvm_tos = data.ival;
}

static void cfp_f_lt(void) {
	QLF_LITERAL data;

	data.ival = ql4thvm_tos;
	ql4thvm_tos = (data.fval < (*(QLF_REAL *)(--ql4thvm_dp))) ? QLF_TRUE : QLF_FALSE;
}

static void cfp_f_le(void) {
	QLF_LITERAL data;

	data.ival = ql4thvm_tos;
	ql4thvm_tos = (data.fval <= (*(QLF_REAL *)(--ql4thvm_dp))) ? QLF_TRUE : QLF_FALSE;
}

static void cfp_f_ne(void) {
	QLF_LITERAL data;

	data.ival = ql4thvm_tos;
	ql4thvm_tos = (data.fval != (*(QLF_REAL *)(--ql4thvm_dp))) ? QLF_TRUE : QLF_FALSE;
}

static void cfp_f_eq(void) {
	QLF_LITERAL data;

	data.ival = ql4thvm_tos;
	ql4thvm_tos = (data.fval == (*(QLF_REAL *)(--ql4thvm_dp))) ? QLF_TRUE : QLF_FALSE;
}

static void cfp_f_gt(void) {
	QLF_LITERAL data;

	data.ival = ql4thvm_tos;
	ql4thvm_tos = (data.fval > (*(QLF_REAL *)(--ql4thvm_dp))) ? QLF_TRUE : QLF_FALSE;
}

static void cfp_f_ge(void) {
	QLF_LITERAL data;

	data.ival = ql4thvm_tos;
	ql4thvm_tos = (data.fval >= (*(QLF_REAL *)(--ql4thvm_dp))) ? QLF_TRUE : QLF_FALSE;
}

static void cfp_f_abs(void) {
	QLF_LITERAL data;

	data.ival = ql4thvm_tos;
	data.fval = (data.fval >= 0.0) ? data.fval : (-data.fval);
	ql4thvm_tos = data.ival;
}

static void cfp_f_acos(void) {
	QLF_LITERAL data;

	data.ival = ql4thvm_tos;
	data.fval = (QLF_REAL)acos((double)data.fval);
	ql4thvm_tos = data.ival;
}

static void cfp_f_asin(void) {
	QLF_LITERAL data;

	data.ival = ql4thvm_tos;
	data.fval = (QLF_REAL)asin((double)data.fval);
	ql4thvm_tos = data.ival;
}

static void cfp_f_atan(void) {
	QLF_LITERAL data;

	data.ival = ql4thvm_tos;
	data.fval = (QLF_REAL)acos((double)data.fval); ;
	ql4thvm_tos = data.ival;
}

static void cfp_f_atan2(void) {
	QLF_LITERAL data;

	data.ival = ql4thvm_tos;
	data.fval = (QLF_REAL)atan2((double)(*(QLF_REAL *)(--ql4thvm_dp)), (double)data.fval);
	ql4thvm_tos = data.ival;
}

static void cfp_f_cos(void) {
	QLF_LITERAL data;

	data.ival = ql4thvm_tos;
	data.fval = (QLF_REAL)cos((double)data.fval);
	ql4thvm_tos = data.ival;
}

static void cfp_f_exp(void) {
	QLF_LITERAL data;

	data.ival = ql4thvm_tos;
	data.fval = (QLF_REAL)exp((double)data.fval);
	ql4thvm_tos = data.ival;
}

static void cfp_f_float(void) {
}

static void cfp_f_log(void) {
	QLF_LITERAL data;

	data.ival = ql4thvm_tos;
	data.fval = (QLF_REAL)log((double)data.fval);
	ql4thvm_tos = data.ival;
}

static void cfp_f_log10(void) {
	QLF_LITERAL data;

	data.ival = ql4thvm_tos;
	data.fval = (QLF_REAL)log10((double)data.fval);
	ql4thvm_tos = data.ival;
}

static void cfp_f_max(void) {
	QLF_LITERAL a, b;

	a.ival = ql4thvm_tos;
	b.ival = *(--ql4thvm_dp);
	ql4thvm_tos = (a.fval > b.fval) ? a.ival : b.ival;
}

static void cfp_f_min(void) {
	QLF_LITERAL a, b;

	a.ival = ql4thvm_tos;
	b.ival = *(--ql4thvm_dp);
	ql4thvm_tos = (a.fval < b.fval) ? a.ival : b.ival;
}

static void cfp_f_negate(void) {
	QLF_LITERAL data;

	data.ival = ql4thvm_tos;
	data.fval = -(data.fval);
	ql4thvm_tos = data.ival;
}

static void cfp_f_pow(void) {
	QLF_LITERAL data;

	data.ival = ql4thvm_tos;
	data.fval = (QLF_REAL)pow((double)(*(QLF_REAL *)(--ql4thvm_dp)), (double)data.fval);
	ql4thvm_tos = data.ival;
}

static void cfp_f_sin(void) {
	QLF_LITERAL data;

	data.ival = ql4thvm_tos;
	data.fval = (QLF_REAL)sin((double)data.fval);
	ql4thvm_tos = data.ival;
}

static void cfp_f_sqrt(void) {
	QLF_LITERAL data;

	data.ival = ql4thvm_tos;
	data.fval = (QLF_REAL)sqrt((double)data.fval);
	ql4thvm_tos = data.ival;
}

static void cfp_f_tan(void) {
	QLF_LITERAL data;

	data.ival = ql4thvm_tos;
	data.fval = (QLF_REAL)tan((double)data.fval);
	ql4thvm_tos = data.ival;
}

// ************  CONSTANT, WORD, COLON, Complier words ********************************

static void fp_doconst(void) {
	* ql4thvm_dp ++ = ql4thvm_tos;
	ql4thvm_tos = ThisExecuteWord->dfa;
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
	spc->dfa	= ql4thvm_tos;
	ql4thvm_tos = * (-- ql4thvm_dp);
}

static void cfp_colon(void) {
	if (!QLForth_token()) {
		QLForth_error("Expect a word", NULL);
	}

	if ((ThisCreateWord = QLForth_symbol_add(token_word)) == NULL) {
		return;
	}
	ThisCreateWord->type = QLF_TYPE_WORD;
	ThisCreateWord->dfa = 0;
	ThisCreateWord->hidden = 1;
	ThisCreateWord->sst = QLForth_sst_append(SST_COLON, (SSTNode *) ThisCreateWord);
	cs_ptr = &cs_stack[0];
	cs_ptr->id		= CSID_COLON;
	ql4thvm_state	= QLF_STATE_COMPILE;
	cs_ptr++;
}

static void cfp_macro(void) {
	if (!QLForth_token()) {
		QLForth_error("Expect a word", NULL);
	}

	if ((ThisCreateWord = QLForth_symbol_add(token_word)) == NULL) {
		return;
	}
	ThisCreateWord->type	= QLF_TYPE_MACRO;
	ThisCreateWord->dfa		= 0;
	ThisCreateWord->hidden	= 1;
	cs_ptr					= &cs_stack[0];
	cs_ptr->id				= CSID_MACRO;
	ql4thvm_state = QLF_STATE_COMPILE;
	cs_ptr++;
}

static void cfp_semicolon(void) {
	cs_ptr--;
	if ((cs_ptr != &cs_stack[0]) || (cs_ptr->id != CSID_COLON)) {
		QLForth_error("Syntax structure mismatched or in-completely", NULL);
	}
	cs_ptr--;

	if (ql4thvm_state != QLF_STATE_COMPILE) {
		QLForth_error("State changed during word compilation", NULL);
	}
	QLForth_sst_append(SST_RET, NULL);
	QLForth_sst_append(SST_SEMICOLON, NULL);
	ql4thvm_state = QLF_STATE_INTERPRET;

	ThisCreateWord->hidden = 0;
	ThisCreateWord = NULL;
}

static void cfp_variable(void) {
	Symbol	* spc;

	if (!QLForth_token()) {
		QLForth_error("Expect a word", NULL);
	}

	if ((spc = QLForth_symbol_add(token_word)) == NULL) {
		return;
	}
	spc->type = QLF_TYPE_VARIABLE;
	QLForth_sst_append(SST_VARIABLE, (SSTNode *) spc);
	spc->dfa 	= 0;
	ql4thvm_here++;
}

static void cfp_if(void) {
	cs_ptr++;
	cs_ptr->id = CSID_IF;
	cs_ptr->sst = QLForth_sst_append(SST_0_BRANCH, NULL);
}

static void cfp_else(void) {
	SSTNode * sst;

	if (cs_ptr->id != CSID_IF) {
		QLForth_error("No IF for this ELSE", NULL);
	}
	sst = QLForth_sst_append(SST_BRANCH, NULL);
	cs_ptr->sst->sst = QLForth_sst_append(SST_LABEL, NULL);
	cs_ptr->sst = sst;
}

static void cfp_endif(void) {
	if (cs_ptr->id != CSID_IF) {
		QLForth_error("No IF for this THEN/ENDIF", NULL);
	}
	cs_ptr->sst->sst = QLForth_sst_append(SST_LABEL, NULL);
	cs_ptr--;
}

static void cfp_begin(void) {
	cs_ptr++;
	cs_ptr->id = CSID_BEGIN;
	cs_ptr->sst = QLForth_sst_append(SST_LABEL, NULL);
	cs_ptr->pos = ql4thvm_here;
}

static void cfp_while(void) {
	if (cs_ptr->id != CSID_BEGIN) {
		QLForth_error("No BEGIN for this WHILE", NULL);
	}
	cs_ptr++;
	cs_ptr->id = CSID_WHILE;
	cs_ptr->sst = QLForth_sst_append(SST_0_BRANCH, NULL);
}

static void cfp_repeat(void) {
	SSTNode * sst;

	if (cs_ptr->id != CSID_WHILE) {
		QLForth_error("No WHILE for this REPEAT", NULL);
	}
	sst = QLForth_sst_append(SST_BRANCH, NULL);
	cs_ptr->sst->sst = QLForth_sst_append(SST_LABEL, NULL);
	cs_ptr--;
	sst->sst = cs_ptr->sst;
	cs_ptr--;
}

static void cfp_until(void) {
	if (cs_ptr->id != CSID_BEGIN) {
		QLForth_error("No WHILE for this UNTIL", NULL);
	}
	QLForth_sst_append(SST_0_BRANCH, cs_ptr->sst);
	cs_ptr--;
}

static void cfp_do(void) {
	cs_ptr++;
	cs_ptr->id = CSID_DO;
	cs_ptr->sst = QLForth_sst_append(SST_DO, NULL);
}

static void cfp_loop(void) {
	if (cs_ptr->id != CSID_DO) {
		QLForth_error("No DO matched this LOOP", NULL);
	}
	QLForth_sst_append(SST_LOOP, cs_ptr->sst);
	cs_ptr--;
}

static void cfp_leave(void) {

	CONTROL_STACK * csp;

	for (csp = cs_ptr; csp->id != SST_COLON && csp > cs_stack; csp--) {
		if (csp->id == CSID_DO) {
			break;
		}
	}
	if (csp->id == SST_COLON || csp == cs_stack) {
		QLForth_error("No DO or FOR matched this EXIT", NULL);
	}
	QLForth_sst_append(SST_LEAVE, csp->sst);
}

static void cfp_for(void) {
	cs_ptr++;
	cs_ptr->id = CSID_FOR;
	cs_ptr->sst = QLForth_sst_append(SST_FOR, NULL);
	cs_ptr->pos = ql4thvm_here;
}

static void cfp_next(void) {

	if (cs_ptr->id != CSID_FOR) {
		QLForth_error("No FOR  with this NEXT", NULL);
	}
	QLForth_sst_append(SST_NEXT, cs_ptr->sst);
	cs_ptr--;
}

static void cfp_tick(void) {
	Symbol * spc;
	char * p;

	if (!QLForth_token()) {
		QLForth_error("Expect a word", NULL);
	}
	for (p = token_word; *p; ++p) * p = toupper(*p);
	if ((spc = QLForth_symbol_search(token_word)) == NULL) {
		QLForth_error("Word not found", token_word);
	}
}

static void cfp_sharp(void) {
	SSTNode * sst;
	
	sst = QLForth_sst_append(SST_LITERAL, NULL);
	sst->val = ql4thvm_tos;
	ql4thvm_tos = *(--ql4thvm_dp);
}

static void cfp_allot(void) {
	SSTNode * sst;
	
	sst = QLForth_sst_append(SST_ALLOT, NULL);
	sst->val = ql4thvm_tos;
	ql4thvm_tos = *(--ql4thvm_dp);
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
	for (cnt = (QLF_CELL) (ql4thvm_dp - ql4thvm_stack); cnt > 0; cnt--) {
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
		case QLF_TYPE_CONSTANT:		QLForth_printf("Constant %d, 0x%08X.\n", spc->ival, spc->ival);	break;
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

	{ NULL,				NULL				}
};

static Primitive define_table[] = {
	{ "CONSTANT",		cfp_constant		},
	{ ":",				cfp_colon			},
	{ "VARIABLE",		cfp_variable		},

	{ "MACRO",			cfp_macro			},
	{ "::",				cfp_macro			},
	
	{ NULL,				NULL				}

} ;

static Primitive immediate_table[] = {
	{ ";",				cfp_semicolon },
	{ "\'",				cfp_tick },
	{ "#",				cfp_sharp },
	{ "ALLOT",			cfp_allot },

	{ "IF",				cfp_if		},
	{ "ELSE",			cfp_else 	},
	{ "ENDIF",			cfp_endif },
	{ "THEN",			cfp_endif },
	{ "BEGIN",			cfp_begin },
	{ "WHILE",			cfp_while },
	{ "REPEAT",			cfp_repeat },
	{ "UNTIL",			cfp_until },
	{ "DO",				cfp_do },
	{ "LOOP",			cfp_loop },
	{ "LEAVE",			cfp_leave },
	{ "FOR",			cfp_for },
	{ "NEXT",			cfp_next },

	{ NULL,			NULL }
};

static Primitive primitive_table[] = {
	{ "DROP",			cfp_drop	},
	{ "DUP",			cfp_dup		},
	{ "?DUP",			cfp_qdup	},
	{ "OVER",			cfp_over	},
	{ "SWAP",			cfp_swap	},
	{ "ROT",			cfp_rot		},
	{ "-ROT",			cfp_mrot	},
	{ "PICK",			cfp_pick	},
	{ "ROLL",			cfp_roll	},

	{ "DEPTH",			cfp_depth	},

	{ "+",				cfp_plus	},
	{ "-",				cfp_minus	},
	{ "*",				cfp_mul		},
	{ "/",				cfp_div		},
	{ "MOD",			cfp_mod		},
	{ "/MOD",			cfp_divmod	},
	{ "1+",				cfp_1_add	},
	{ "1-",				cfp_1_sub	},
	{ "2+",				cfp_2_add	},
	{ "2-",				cfp_2_sub	},
	{ "2*",				cfp_2_mul	},
	{ "2/",				cfp_2_div	},
	{ "4+",				cfp_4_add	},
	{ "4-",				cfp_4_sub	},

	{ "MIN",			cfp_min		},
	{ "MAX",			cfp_max		},
	{ "ABS",			cfp_abs		},
	{ "NEGATE",			cfp_negate	},

	{ "<",				cfp_cmp_lt	},
	{ "<=",				cfp_cmp_le	},
	{ "<>",				cfp_cmp_ne	},
	{ "=",				cfp_cmp_eq	},
	{ ">",				cfp_cmp_gt	},
	{ ">=",				cfp_cmp_ge	},
	{ "0<",				cfp_cmp_0_lt},
	{ "0=",				cfp_cmp_0_eq},
	{ "0>",				cfp_cmp_0_gt},

	{ "AND",			cfp_and		},
	{ "OR",				cfp_or		},
	{ "XOR",			cfp_xor		},
	{ "NOT",			cfp_not		},

	{ "F+",				cfp_f_plus	},
	{ "F-",				cfp_f_minus },
	{ "F*",				cfp_f_mul	},
	{ "F/",				cfp_f_div	},
	{ "F<",				cfp_f_lt	},
	{ "F<=",			cfp_f_le	},
	{ "F<>",			cfp_f_ne	},
	{ "F=",				cfp_f_eq	},
	{ "F>",				cfp_f_gt	},
	{ "F>=",			cfp_f_ge	},
	{ "FABS",			cfp_f_abs	},
	{ "ACOS",			cfp_f_acos	},
	{ "ASIN",			cfp_f_asin	},
	{ "ATAN",			cfp_f_atan	},
	{ "ATAN2",			cfp_f_atan2 },
	{ "COS",			cfp_f_cos	},
	{ "EXP",			cfp_f_exp	},
	{ "FLOAT",			cfp_f_float },
	{ "LOG",			cfp_f_log	},
	{ "LOG10",			cfp_f_log10 },
	{ "FMAX",			cfp_f_max	},
	{ "FMIN",			cfp_f_min	},
	{ "FNEGATE",		cfp_f_negate},
	{ "POW",			cfp_f_pow	},
	{ "SIN",			cfp_f_sin	},
	{ "SQRT",			cfp_f_sqrt	},
	{ "TAN",			cfp_f_tan	},

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
	QLForth_report(1, line - 1, (QLF_CELL) (scan_ptr - text_ptr), buffer);
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
	QLF_CELL	i;
	Symbol	    * spc;
	char		* p;

	for (p = name; *p; ++p) {
		if (islower(*p)) {
			*p = toupper(*p);
		}
	}
	i	= sizeof(Symbol) + (QLF_CELL) strlen(name);
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
		QLForth_report(0, (QLF_CELL) (ql4thvm_here - ql4thvm_dict) * sizeof(QLF_CELL), DICT_BUFFER_SIZE, NULL);
		// display_symbole_table(root_bucket);
	}
	current = working_bucket;
}

void QLForth_interpreter(char * text) {
	strcpy(interpret_text, text);
	ql4thvm_state = QLF_STATE_INTERACTIVE;
	ql4th_interpreter(interpret_text);
	QLForth_display_stack(display_number_base, (QLF_CELL) (ql4thvm_dp - ql4thvm_stack), ql4thvm_tos, *(ql4thvm_dp - 1));
	if (qlforth_text_buffer != NULL) {
		free(qlforth_text_buffer);
		qlforth_text_buffer = NULL;
	}
}

void QLForth_stop(void) {
}
