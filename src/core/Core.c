/* ************************************************************************************
* Copyright (C) 2015-2017 Shaanxi QinWei Electroics Technologies. All rights reserved.
*
* $Date:        24. March 2017
* $Revision: 	V 0.1
*
* Project: 	    QLForth - A Forth for System-On-Chip Creative Makers
*
* Title:	    Core.c
*
* Description:	The host core for QLForth
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
#include <math.h>

#include "QLForth.h"

typedef struct _cstack {
	int			id;
	QLF_CELL	* pos;
} ControlStack;

static ControlStack	cs_stack[MAX_CONTROL_STACK_DEEP + 1], *cs_ptr;

typedef enum {
	CSID_COLON = 1,
	CSID_IF,	CSID_ELSE,		CSID_ENDIF,
	CSID_CASE,	CSID_OF,		CSID_ENDOF,
	CSID_BEGIN, CSID_WHILE,		CSID_AGAIN,
	CSID_DO,	CSID_QMARK_DO,	CSID_EXIT,
	CSID_FOR,

	END_OF_CSID_CODE
} CSID_CODE;

static void stack_error_underflow(void) {
	QLForth_error("Stack empty", NULL);
}

static void stack_error_overflow(void) {
	QLForth_error("Stack full", NULL);
}

static void stack_error(void) {
	QLForth_error("Stack error", NULL);
}

// ....................................................................................

static void fp_docol(void) {
	*ql4thvm_rp-- = ((QLF_CELL)program_counter);
	program_counter = (Symbol **)ThisExecuteWord->dfa;
}

static void fp_donext(void) {
	ql4thvm_rp++;
	program_counter = (Symbol **)(*ql4thvm_rp);
}

static void fp_dodoes(void) {
	QLF_CELL * dfa;

	dfa = (QLF_CELL *)(ThisExecuteWord->dfa);
	program_counter = *(Symbol ***)dfa++;
	*ql4thvm_dp++ = ql4thvm_tos;
	ql4thvm_tos = (QLF_CELL)dfa;
}

static void cfp_create(void) {
	if ((ThisCreateWord = QLForth_create()) == NULL) {
		return;
	}
	ThisCreateWord->fun = qlforth_fp_docreate;
	ThisCreateWord->dfa = (QLF_CELL) ql4thvm_here;
}

static void cfp_colon(void) {
	if ((ThisCreateWord = QLForth_create()) == NULL) {
		return;
	}
	ql4thvm_state = QLF_STATE_CORE;
	ThisCreateWord->type = QLF_TYPE_PRIMITIVE;
	ThisCreateWord->hidden = 1;
	ThisCreateWord->fun = fp_docol;
	ThisCreateWord->dfa = (QLF_CELL)ql4thvm_here;

	cs_ptr = &cs_stack[0];
	cs_ptr->id = CSID_COLON;
	cs_ptr++;
}

static void cfp_semicolon(void) {
	static Symbol symbol_do_next = { NULL,{ fp_donext },{ "(NEXT)" },	0 };

	cs_ptr--;
	if ((cs_ptr != &cs_stack[0]) || (cs_ptr->id != CSID_COLON)) {
		QLForth_error("Syntax structure mismatched or in-completely", NULL);
	}
	cs_ptr--;

	if (ql4thvm_state != QLF_STATE_CORE) {
		QLForth_error("Syntax structure mismatched or in-completely", NULL);
	}

	*(Symbol **)ql4thvm_here++ = &symbol_do_next;
	ThisCreateWord->hidden = 0;
	ql4thvm_state = QLF_STATE_INTERPRET;
	ThisCreateWord = NULL;
}

static void cfp_variable(void) {
	Symbol * spc;

	if ((spc = QLForth_create()) == NULL) {
		return;
	}
	spc->type = QLF_TYPE_PRIMITIVE;
	spc->fun = qlforth_fp_docreate;
	spc->dfa = (QLF_CELL)ql4thvm_here;
	ql4thvm_here++;
}

static void cfp_does(void) {
	QLF_CELL * ptr, *cfa;

	if (ThisCreateWord == NULL) {
		QLForth_error("Bad of using DOES>", NULL);
	}
	cfa = (QLF_CELL *)ThisCreateWord->dfa;
	for (ptr = ql4thvm_here; ptr > cfa; ptr--) {
		*ptr = *(ptr - 1);
	}
	ql4thvm_here++;
	ThisCreateWord->fun = fp_dodoes;
	*cfa = (QLF_CELL)program_counter;
	ql4thvm_rp++;
	program_counter = (Symbol **)(*ql4thvm_rp);
}

static void fp_0branch(void) {
	if (ql4thvm_tos == 0) {
		program_counter = *(Symbol ***)program_counter;
	}
	else {
		program_counter++;
	}
	ql4thvm_tos = *(--ql4thvm_dp);
}

static void fp_branch(void) {
	program_counter = *(Symbol ***)program_counter;
}

static void fp_caseof(void) {
	int val;

	val = ql4thvm_tos;
	ql4thvm_tos = *(--ql4thvm_dp);
	if (val == ql4thvm_tos) {
		ql4thvm_tos = *(--ql4thvm_dp);
		program_counter++;
	}
	else {
		program_counter = *(Symbol ***)program_counter;
	}
}

static void fp_endcase(void) {
	ql4thvm_tos = *(--ql4thvm_dp);
}

static void fp_do_qmark_do(void) {
	if (ql4thvm_tos != *(ql4thvm_dp - 1)) {
		program_counter++;
	}
	else {
		ql4thvm_tos = *(ql4thvm_dp - 2);
		ql4thvm_dp -= 2;
		program_counter = *(Symbol ***)program_counter;
	}
}

static void fp_do_add_loop(void) {
	ql4thvm_tos += *(--ql4thvm_dp);
	if (ql4thvm_tos < *(ql4thvm_dp - 1)) {
		program_counter = *(Symbol ***)program_counter;
	}
	else {
		ql4thvm_tos = *(ql4thvm_dp - 2);
		ql4thvm_dp -= 2;
		program_counter++;
	}
}

static void fp_do_for_next(void) {
	ql4thvm_tos--;
	if (ql4thvm_tos > 0) {
		program_counter = *(Symbol ***)program_counter;
	}
	else {
		ql4thvm_tos = *(--ql4thvm_dp);
		program_counter++;
	}
}

static void fp_do_exit(void) {
	program_counter = *(Symbol ***)program_counter;
}

static void fp_do_loop(void) {
	ql4thvm_tos++;
	if (ql4thvm_tos < *(ql4thvm_dp - 1)) {
		program_counter = *(Symbol ***)program_counter;
	}
	else {
		ql4thvm_tos = *(ql4thvm_dp - 2);
		ql4thvm_dp -= 2;
		program_counter++;
	}
}

static Symbol	symbol_0branch	= { NULL, { fp_0branch } ,{ "(0BRANCH)" } ,	0 },
				symbol_branch	= { NULL, { fp_branch  } ,{ "(BRANCH)"  },	0 },
				symbol_caseof	= { NULL, { fp_caseof  } ,{ "(CASEOF)"  } ,	0 },
				symbol_endcase	= { NULL, { fp_endcase } ,{ "(ENDCASE)" },	0 };

// ************************************************************************************

static void cfp_if(void) {
	cs_ptr++;
	cs_ptr->id = CSID_IF;
	*(Symbol **)ql4thvm_here++ = &symbol_0branch;
	cs_ptr->pos = ql4thvm_here;
	ql4thvm_here++;
}

static void cfp_else(void) {
	if (cs_ptr->id != CSID_IF) {
		QLForth_error("No IF for this ELSE", NULL);
	}
	*(Symbol **)ql4thvm_here++ = &symbol_branch;
	ql4thvm_here++;
	*(QLF_CELL *)(cs_ptr->pos) = (QLF_CELL)ql4thvm_here;
	cs_ptr->pos = (QLF_CELL *)(ql4thvm_here - 1);
}

static void cfp_endif(void) {
	if (cs_ptr->id != CSID_IF) {
		QLForth_error("No IF for this THEN/ENDIF", NULL);
	}
	*(QLF_CELL *)(cs_ptr->pos) = (QLF_CELL)ql4thvm_here;
	cs_ptr--;
}

static void cfp_case(void) {
	cs_ptr++;
	cs_ptr->id = CSID_CASE;
	cs_ptr->pos = 0;
}

static void cfp_of(void) {
	if (cs_ptr->id != CSID_CASE) {
		QLForth_error("No CASE for this OF", NULL);
	}
	cs_ptr++;
	cs_ptr->id = CSID_OF;
	*(Symbol **)ql4thvm_here++ = &symbol_caseof;
	cs_ptr->pos = ql4thvm_here;
	ql4thvm_here++;
}

static void cfp_endof(void) {
	QLF_CELL * here_save;

	if (cs_ptr->id != CSID_OF) {
		QLForth_error("No OF for this ENDOF", NULL);
	}
	*(Symbol **)ql4thvm_here++ = &symbol_branch;
	here_save = ql4thvm_here++;
	*(QLF_CELL *)(cs_ptr->pos) = (QLF_CELL)ql4thvm_here;
	cs_ptr--;
	*here_save = (QLF_CELL)cs_ptr->pos;
	cs_ptr->pos = here_save;
}

static void cfp_endcase(void) {
	QLF_CELL * here_save, *here_next;

	if (cs_ptr->id != CSID_CASE) {
		QLForth_error("No CASE for this ENDCASE", NULL);
	}
	*(Symbol **)ql4thvm_here++ = &symbol_endcase;
	here_save = (QLF_CELL *)cs_ptr->pos;
	while (here_save) {
		here_next = *(QLF_CELL **)here_save;
		*(QLF_CELL *)here_save = (QLF_CELL)ql4thvm_here;
		here_save = here_next;
	}
	cs_ptr--;
}

static void cfp_begin(void) {
	cs_ptr++;
	cs_ptr->id = CSID_BEGIN;
	cs_ptr->pos = ql4thvm_here;
}

static void cfp_again(void) {
	if (cs_ptr->id != CSID_BEGIN) {
		QLForth_error("No BEGIN for this AGAIN", NULL);
	}
	*(Symbol **)ql4thvm_here++ = &symbol_branch;
	*(QLF_CELL **)ql4thvm_here++ = cs_ptr->pos;
	cs_ptr--;
}

static void cfp_while(void) {
	if (cs_ptr->id != CSID_BEGIN) {
		QLForth_error("No BEGIN for this WHILE", NULL);
	}
	*(Symbol **)ql4thvm_here++ = &symbol_0branch;
	cs_ptr++;
	cs_ptr->id = CSID_WHILE;
	cs_ptr->pos = (QLF_CELL *)ql4thvm_here;	// pointer for fill back
	ql4thvm_here++;							// reserved a hole for fill
}

static void cfp_repeat(void) {
	if (cs_ptr->id != CSID_WHILE) {
		QLForth_error("No WHILE for this REPEAT", NULL);
	}
	*(QLF_CELL *)(cs_ptr->pos) = (QLF_CELL)(ql4thvm_here + 2);
	cs_ptr--;
	*(Symbol **)ql4thvm_here++ = &symbol_branch;
	*(QLF_CELL **)ql4thvm_here++ = cs_ptr->pos;
	cs_ptr--;
}

static void cfp_until(void) {
	if (cs_ptr->id != CSID_BEGIN) {
		QLForth_error("No WHILE for this UNTIL", NULL);
	}
	*(QLF_CELL *)(cs_ptr->pos) = (QLF_CELL)(ql4thvm_here + 2);
	*(Symbol **)ql4thvm_here++ = &symbol_0branch;
	*(QLF_CELL **)ql4thvm_here++ = cs_ptr->pos;
	cs_ptr--;
}

static void cfp_do(void) {
	cs_ptr++;
	cs_ptr->id = CSID_DO;
	cs_ptr->pos = ql4thvm_here;
}

static void cfp_qmark_do(void) {
	static Symbol symbol_do_qmark_do = { NULL,{ fp_do_qmark_do } ,{ "(?DO)" }, 0 };
	cs_ptr++;
	cs_ptr->id = CSID_QMARK_DO;
	cs_ptr->pos = ql4thvm_here;
	*(Symbol **)ql4thvm_here++ = &symbol_do_qmark_do;
	*(QLF_CELL **)ql4thvm_here++ = cs_ptr->pos;
}

static void cfp_loop(void) {
	static Symbol symbol_do_loop = { NULL,{ fp_do_loop } ,{ "(LOOP)" }, 0 };

	if (cs_ptr->id == CSID_DO) {
		*(Symbol **)ql4thvm_here++ = &symbol_do_loop;
		*(QLF_CELL **)ql4thvm_here++ = cs_ptr->pos;
	}
	else if (cs_ptr->id != CSID_QMARK_DO) {
		*(Symbol **)ql4thvm_here++ = &symbol_do_loop;
		*(QLF_CELL **)ql4thvm_here++ = (cs_ptr->pos + 1);
		*(QLF_CELL **)(cs_ptr->pos) = ql4thvm_here;
	}
	else {
		QLForth_error("No DO or ?DO with this LOOP", NULL);
	}
	cs_ptr--;
}

static void cfp_plus_loop(void) {
	static Symbol symbol_do_add_loop = { NULL,{ fp_do_add_loop },{ "(+LOOP)" },	0 };

	if (cs_ptr->id == CSID_DO) {
		*(Symbol **)ql4thvm_here++ = &symbol_do_add_loop;
		*(QLF_CELL **)ql4thvm_here++ = cs_ptr->pos;
	}
	else if (cs_ptr->id != CSID_QMARK_DO) {
		*(Symbol **)ql4thvm_here++ = &symbol_do_add_loop;
		*(QLF_CELL **)ql4thvm_here++ = (cs_ptr->pos + 1);
		*(QLF_CELL **)(cs_ptr->pos) = ql4thvm_here;
	}
	else {
		QLForth_error("No DO or ?DO with this +LOOP", NULL);
	}
	cs_ptr--;
}

static void cfp_exit(void) {
	static Symbol symbol_do_exit = { NULL,{ fp_do_exit } ,{ "(EXIT)" }, 0 };

	if (cs_ptr->id != CSID_DO && cs_ptr->id != CSID_QMARK_DO && cs_ptr->id != CSID_EXIT) {
		QLForth_error("No DO or ?DO with this EXIT", NULL);
	}
	cs_ptr++;
	cs_ptr->id = CSID_EXIT;
	*(Symbol **)ql4thvm_here++ = &symbol_do_exit;
	cs_ptr->pos = ql4thvm_here;
	*(QLF_CELL **)ql4thvm_here++ = 0;
	cs_ptr--;
}

static void cfp_for(void) {
	cs_ptr++;
	cs_ptr->id = CSID_FOR;
	cs_ptr->pos = ql4thvm_here;
}

static void cfp_next(void) {
	static Symbol symbol_do_for_next = { NULL,{ fp_do_for_next },{ "(FOR-NEXT)" } ,0 };

	if (cs_ptr->id != CSID_FOR) {
		QLForth_error("No FOR  with this NEXT", NULL);
	}
	*(Symbol **)ql4thvm_here++ = &symbol_do_for_next;
	*(QLF_CELL **)ql4thvm_here++ = cs_ptr->pos;
	cs_ptr--;
}

static void cfp_tick(void) {
	Symbol * spc;

	if (! QLForth_token()) {
		QLForth_error("Expect a word", NULL);
	}
	_strupr(token_word);
	if ((spc = QLForth_symbol_search(token_word)) == NULL) {
		QLForth_error("Word not found", token_word);
	}


}

// ************************************************************************************

static void cfp_dup(void) {
	*ql4thvm_dp++ = ql4thvm_tos;
}

static void cfp_qdup(void) {
	if (ql4thvm_tos) {
		*ql4thvm_dp++ = ql4thvm_tos;
	}
}

// ....................................................................................

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

static void cfp_to_r(void) {
	*ql4thvm_rp-- = ql4thvm_tos;
	ql4thvm_tos = *(--ql4thvm_dp);
}

static void cfp_r_fetch(void) {
	*ql4thvm_dp++ = ql4thvm_tos;
	ql4thvm_tos = *(ql4thvm_rp + 1);
}

static void cfp_from_r(void) {
	*ql4thvm_dp++ = ql4thvm_tos;
	ql4thvm_tos = *(++ql4thvm_rp);
}

static void cfp_depth(void) {
	*ql4thvm_dp++ = ql4thvm_tos;
	ql4thvm_tos = (ql4thvm_dp - &ql4thvm_stack[1]);
}

// ....................................................................................

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

// ....................................................................................

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

// ************************************************************************************

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

// ************************************************************************************

static void cfp_store(void) {
	*(QLF_CELL *)ql4thvm_tos = *(--ql4thvm_dp);
	ql4thvm_tos = *(--ql4thvm_dp);
}

static void cfp_fetch(void) {

	ql4thvm_tos = *(QLF_CELL *)ql4thvm_tos;
}

static void cfp_comma(void) {
	*ql4thvm_here++ = ql4thvm_tos;
	ql4thvm_tos = *(--ql4thvm_dp);
}

// ....................................................................................

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

// ....................................................................................

static Primitive define_table[] = {
	{ ":",			cfp_colon },
	{ "VARIABLE",	cfp_variable },
	{ "CREATE",		cfp_create },
	{ "DOES>",		cfp_does },
	{ NULL,			NULL }
};

static Primitive immediate_table[] = {
	{ ";",			cfp_semicolon	},
	{ "\'",			cfp_tick		},

	{ "IF",			cfp_if },
	{ "ELSE",			cfp_else },
	{ "ENDIF",		cfp_endif },
	{ "THEN",			cfp_endif },
	{ "CASE",			cfp_case },
	{ "OF",			cfp_of },
	{ "ENDOF",		cfp_endof },
	{ "ENDCASE",		cfp_endcase },
	{ "BEGIN",		cfp_begin },
	{ "WHILE",		cfp_while },
	{ "AGAIN",		cfp_again },
	{ "REPEAT",		cfp_repeat },
	{ "UNTIL",		cfp_until },
	{ "DO",			cfp_do },
	{ "LOOP",			cfp_loop },
	{ "+LOOP",		cfp_plus_loop },
	{ "EXIT",			cfp_exit },
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
	{ ">R",				cfp_to_r	},
	{ "R@",				cfp_r_fetch },
	{ "R>",				cfp_from_r	},
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

	{ "!",				cfp_store	},
	{ "@",				cfp_fetch	},
	{ ",",				cfp_comma	},

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

	{ NULL,			NULL }
};

// ************************************************************************************

void Core_init(void) {
	Primitive * ppc;
	Symbol	* spc;

	for (ppc = define_table; ppc->pname != NULL; ppc++) {
		spc = QLForth_symbol_add(ppc->pname);
		spc->fun = ppc->fun;
		spc->type = QLF_TYPE_DEFINE;
	}

	for (ppc = immediate_table; ppc->pname != NULL; ppc++) {
		spc = QLForth_symbol_add(ppc->pname);
		spc->fun = ppc->fun;
		spc->type = QLF_TYPE_IMMEDIATE;
	}

	for (ppc = primitive_table; ppc->pname != NULL; ppc++) {
		spc = QLForth_symbol_add(ppc->pname);
		spc->fun = ppc->fun;
		spc->type = QLF_TYPE_PRIMITIVE;
	}
}
