/* ************************************************************************************
* Copyright (C) 2015-2017 Shaanxi QinWei Electroics Technologies. All rights reserved.
*
* $Date:        24. March 2017
* $Revision: 	V 0.1
*
* Project: 	    QLForth - A Forth for System-On-Chip Creative Makers
*
* Title:	    Compile.c
*
* Description:	The Forth Compiler for QLForth
*
* Target:		Windows 7+
*
* Author:		Zhao Yu, forthchina@163.com, forthchina@gmail.com
*			    12/04/2017 (In dd/mm/yyyy)
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

typedef struct _cstack {
	int		id;
	union {
		QLF_CELL	* pos;
		SSTNode		* sst;
	};

} CONTROL_STACK;

static CONTROL_STACK cs_stack[MAX_CONTROL_STACK_DEEP + 1], *cs_ptr;

// ************  Primitive, execution only on host like macro *************************

static void cfp_dup(void) {
	ql4thvm_dp->ptr = ql4thvm_tos.ptr;
	ql4thvm_dp++;
}

static void cfp_qdup(void) {
	if (ql4thvm_tos.ival) {
		ql4thvm_dp->ptr = ql4thvm_tos.ptr;
		ql4thvm_dp++;
	}
}

static void cfp_drop(void) {
	ql4thvm_dp--;
	ql4thvm_tos.ptr = ql4thvm_dp->ptr;
}

static void cfp_over(void) {
	ql4thvm_dp->ptr = ql4thvm_tos.ptr;
	ql4thvm_dp++;
	ql4thvm_tos.ptr = (ql4thvm_dp - 2)->ptr;
}

static void cfp_swap(void) {
	void * ptr_save;
	
	ptr_save = (ql4thvm_dp - 1)->ptr;
	(ql4thvm_dp - 1)->ptr = ql4thvm_tos.ptr;
	ql4thvm_tos.ptr = ptr_save;
}

static void cfp_rot(void) {
	void * ptr_save;
	
	ptr_save = ql4thvm_tos.ptr;
	ql4thvm_tos.ptr = (ql4thvm_dp - 2)->ptr;
	(ql4thvm_dp - 2)->ptr = (ql4thvm_dp - 1)->ptr;
	(ql4thvm_dp - 1)->ptr = ptr_save;
}

static void cfp_mrot(void) {
	void * ptr_save;

	ptr_save = ql4thvm_tos.ptr;
	ql4thvm_tos.ptr = (ql4thvm_dp - 1)->ptr;
	(ql4thvm_dp - 1)->ptr = (ql4thvm_dp - 2)->ptr;
	(ql4thvm_dp - 2)->ptr = ptr_save;
}

static void cfp_pick(void) {
	if (&ql4thvm_stack[ql4thvm_tos.ival] < ql4thvm_dp) {
		ql4thvm_tos.ptr = (ql4thvm_dp - ql4thvm_tos.ival - 1)->ptr;
	}
}

static void cfp_roll(void) {
	QLF_CELL * ptr;

	if (&ql4thvm_stack[ql4thvm_tos.ival] < ql4thvm_dp) {
		ptr = ql4thvm_dp - ql4thvm_tos.ival - 1;
		ql4thvm_tos.ptr = ptr->ptr;
		for (; ptr < ql4thvm_dp; ptr++) {
			ptr->ptr = (ptr + 1)->ptr;
		}
		ql4thvm_dp--;
	}
}

static void cfp_depth(void) {
	*ql4thvm_dp++ = ql4thvm_tos;
	ql4thvm_tos.ival = (int)(ql4thvm_dp - &ql4thvm_stack[1]);
}

static void cfp_plus(void) {
	ql4thvm_dp --;
	ql4thvm_tos.ival += ql4thvm_dp->ival;
}

static void cfp_minus(void) {
	ql4thvm_dp --;
	ql4thvm_tos.ival = ql4thvm_dp->ival - ql4thvm_tos.ival;
}

static void cfp_mul(void) {
	ql4thvm_dp--;
	ql4thvm_tos.ival += ql4thvm_dp->ival;
}

static void cfp_div(void) {
	ql4thvm_dp--;
	ql4thvm_tos.ival = ql4thvm_dp->ival / ql4thvm_tos.ival;
}

static void cfp_mod(void) {
	ql4thvm_dp--;
	ql4thvm_tos.ival = ql4thvm_dp->ival % ql4thvm_tos.ival;
}

static void cfp_divmod(void) {
	int nos, quot;

	if (ql4thvm_tos.ival != 0) {
		nos = (ql4thvm_dp - 1)->ival;
		quot = nos / ql4thvm_tos.ival;
		(ql4thvm_dp - 1)->ival = nos % ql4thvm_tos.ival;
		ql4thvm_tos.ival = quot;
	}
}

static void cfp_1_add(void) {
	ql4thvm_tos.ival++;
}

static void cfp_1_sub(void) {
	ql4thvm_tos.ival--;
}

static void cfp_2_add(void) {
	ql4thvm_tos.ival += 2;
}

static void cfp_2_sub(void) {
	ql4thvm_tos.ival -= 2;
}

static void cfp_2_mul(void) {
	ql4thvm_tos.ival <<= 1;
}

static void cfp_2_div(void) {
	ql4thvm_tos.ival >>= 1;
}

static void cfp_4_add(void) {
	ql4thvm_tos.ival += 4;
}

static void cfp_4_sub(void) {
	ql4thvm_tos.ival -= 4;
}

static void cfp_min(void) {
	ql4thvm_dp--;
	if (ql4thvm_dp->ival < ql4thvm_tos.ival) {
		ql4thvm_tos.ival = ql4thvm_dp->ival;
	}
}

static void cfp_max(void) {
	ql4thvm_dp--;
	if (ql4thvm_dp->ival > ql4thvm_tos.ival) {
		ql4thvm_tos.ival = ql4thvm_dp->ival;
	}
}

static void cfp_abs(void) {
	if (ql4thvm_tos.ival < 0) {
		ql4thvm_tos.ival = -ql4thvm_tos.ival;
	}
}

static void cfp_negate(void) {
	ql4thvm_tos.ival = - ql4thvm_tos.ival;
}

static void cfp_cmp_lt(void) {
	ql4thvm_dp--;
	ql4thvm_tos.ival = (ql4thvm_dp->ival < ql4thvm_tos.ival) ? QLF_TRUE : QLF_FALSE;
}

static void cfp_cmp_le(void) {
	ql4thvm_dp--;
	ql4thvm_tos.ival = (ql4thvm_dp->ival <= ql4thvm_tos.ival) ? QLF_TRUE : QLF_FALSE;
}

static void cfp_cmp_ne(void) {
	ql4thvm_dp--;
	ql4thvm_tos.ival = (ql4thvm_dp->ival != ql4thvm_tos.ival) ? QLF_TRUE : QLF_FALSE;
}

static void cfp_cmp_eq(void) {
	ql4thvm_dp--;
	ql4thvm_tos.ival = (ql4thvm_dp->ival == ql4thvm_tos.ival) ? QLF_TRUE : QLF_FALSE;
}

static void cfp_cmp_gt(void) {
	ql4thvm_dp--;
	ql4thvm_tos.ival = (ql4thvm_dp->ival > ql4thvm_tos.ival) ? QLF_TRUE : QLF_FALSE;
}

static void cfp_cmp_ge(void) {
	ql4thvm_dp--;
	ql4thvm_tos.ival = (ql4thvm_dp->ival >= ql4thvm_tos.ival) ? QLF_TRUE : QLF_FALSE;
}

static void cfp_cmp_0_lt(void) {
	ql4thvm_tos.ival = (ql4thvm_tos.ival < 0) ? QLF_TRUE : QLF_FALSE;
}

static void cfp_cmp_0_eq(void) {
	ql4thvm_tos.ival = (ql4thvm_tos.ival == 0) ? QLF_TRUE : QLF_FALSE;
}

static void cfp_cmp_0_gt(void) {
	ql4thvm_tos.ival = (ql4thvm_tos.ival > 0) ? QLF_TRUE : QLF_FALSE;
}

static void cfp_and(void) {
	ql4thvm_dp--;
	ql4thvm_tos.ival &= ql4thvm_dp->ival;
}

static void cfp_or(void) {
	ql4thvm_dp--;
	ql4thvm_tos.ival |= ql4thvm_dp->ival;
}

static void cfp_xor(void) {
	ql4thvm_dp--;
	ql4thvm_tos.ival ^= ql4thvm_dp->ival;

}

static void cfp_not(void) {
	ql4thvm_tos.ival = ~ql4thvm_tos.ival;
}

static void cfp_f_plus(void) {
	ql4thvm_dp--;
	ql4thvm_tos.fval += (ql4thvm_dp->fval);
}

static void cfp_f_minus(void) {
	ql4thvm_tos.fval = (ql4thvm_dp - 1)->fval - ql4thvm_tos.fval;
	ql4thvm_dp--;
}

static void cfp_f_mul(void) {
	ql4thvm_tos.fval *= ((ql4thvm_dp - 1)->fval);
	ql4thvm_dp--;
}

static void cfp_f_div(void) {
	ql4thvm_dp--;
	ql4thvm_tos.fval = ql4thvm_dp->fval / ql4thvm_tos.fval;
	
}

static void cfp_f_lt(void) {
	ql4thvm_dp--;
	ql4thvm_tos.ival = (ql4thvm_dp->fval < ql4thvm_tos.fval) ? QLF_TRUE : QLF_FALSE;
}

static void cfp_f_le(void) {
	ql4thvm_dp--;
	ql4thvm_tos.ival = (ql4thvm_dp->fval <= ql4thvm_tos.fval) ? QLF_TRUE : QLF_FALSE;
}

static void cfp_f_ne(void) {
	ql4thvm_dp--;
	ql4thvm_tos.ival = (ql4thvm_dp->fval != ql4thvm_tos.fval) ? QLF_TRUE : QLF_FALSE;
}

static void cfp_f_eq(void) {
	ql4thvm_dp--;
	ql4thvm_tos.ival = (ql4thvm_dp->fval == ql4thvm_tos.fval) ? QLF_TRUE : QLF_FALSE;
}

static void cfp_f_gt(void) {
	ql4thvm_dp--;
	ql4thvm_tos.ival = (ql4thvm_dp->fval > ql4thvm_tos.fval) ? QLF_TRUE : QLF_FALSE;
}

static void cfp_f_ge(void) {
	ql4thvm_dp--;
	ql4thvm_tos.ival = (ql4thvm_dp->fval >= ql4thvm_tos.fval) ? QLF_TRUE : QLF_FALSE;
}

static void cfp_f_abs(void) {
	if (ql4thvm_tos.fval < 0.0) {
		ql4thvm_tos.fval = -ql4thvm_tos.fval;
	}
}

static void cfp_f_acos(void) {
	ql4thvm_tos.fval = (float) acos((double) ql4thvm_tos.fval);
}

static void cfp_f_asin(void) {
	ql4thvm_tos.fval = (float)asin((double)ql4thvm_tos.fval);
}

static void cfp_f_atan(void) {
	ql4thvm_tos.fval = (float)atan((double)ql4thvm_tos.fval);
}

static void cfp_f_atan2(void) {
	ql4thvm_dp--;
	ql4thvm_tos.fval = (float)atan2((double)ql4thvm_dp->fval, (double)ql4thvm_tos.fval);
}

static void cfp_f_cos(void) {
	ql4thvm_tos.fval = (float)cos((double)ql4thvm_tos.fval);
}

static void cfp_f_exp(void) {
	ql4thvm_tos.fval = (float)exp((double)ql4thvm_tos.fval);
}

static void cfp_f_float(void) {
}

static void cfp_f_log(void) {
	ql4thvm_tos.fval = (float)log((double)ql4thvm_tos.fval);
}

static void cfp_f_log10(void) {
	ql4thvm_tos.fval = (float)log10((double)ql4thvm_tos.fval);
}

static void cfp_f_max(void) {
	ql4thvm_dp--;
	if (ql4thvm_dp->fval > ql4thvm_tos.fval) {
		ql4thvm_tos.fval = ql4thvm_dp->fval;
	}
}

static void cfp_f_min(void) {
	ql4thvm_dp--;
	if (ql4thvm_dp->fval < ql4thvm_tos.fval) {
		ql4thvm_tos.fval = ql4thvm_dp->fval;
	}
}

static void cfp_f_negate(void) {
	ql4thvm_tos.fval = - ql4thvm_tos.fval;
}

static void cfp_f_pow(void) {
	ql4thvm_dp--;
	ql4thvm_tos.fval = (float)pow((double)ql4thvm_dp->fval, (double)ql4thvm_tos.fval);
}

static void cfp_f_sin(void) {
	ql4thvm_tos.fval = (float)sin((double)ql4thvm_tos.fval);
}

static void cfp_f_sqrt(void) {
	ql4thvm_tos.fval = (float)sqrt((double)ql4thvm_tos.fval);
}

static void cfp_f_tan(void) {
	ql4thvm_tos.fval = (float)tan((double)ql4thvm_tos.fval);
}

// ************  Helper functions for macro flow control ******************************

static void fp_docol(void) {
	ql4thvm_rp--;
	ql4thvm_rp->ptr = (void *) program_counter;
	program_counter = (Symbol **)ThisExecuteWord->dfa;
}

static void fp_donext(void) {
	ql4thvm_rp++;
	program_counter = (Symbol **)(ql4thvm_rp->ptr);
}

static void fp_0branch(void) {
	if (ql4thvm_tos.ival == 0) {
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

static void fp_do_for_next(void) {
	ql4thvm_tos.ival--;
	if (ql4thvm_tos.ival > 0) {
		program_counter = *(Symbol ***) program_counter;
	}
	else {
		ql4thvm_dp--;
		ql4thvm_tos.ival = ql4thvm_dp->ival;
		program_counter++;
	}
}

static void fp_do_leave(void) {
	program_counter = *(Symbol ***)program_counter;
}

static void fp_do_loop(void) {
	ql4thvm_tos.ival++;
	if (ql4thvm_tos.ival < (ql4thvm_dp - 1)->ival) {
		program_counter = *(Symbol ***)program_counter;
	}
	else {
		ql4thvm_tos = *(ql4thvm_dp - 2);
		ql4thvm_dp -= 2;
		program_counter++;
	}
}

static Symbol	symbol_0branch	= { NULL,{ fp_0branch } ,{ "(0BRANCH)" } ,	0 },
				symbol_branch	= { NULL,{ fp_branch } ,{ "(BRANCH)" },	0 };

// ************************************************************************************

static void cfp_if(void) {
	cs_ptr++;
	cs_ptr->id = CSID_IF;
	if (ql4thvm_state == QLF_STATE_COMPILE) {
		cs_ptr->sst = QLForth_sst_append(SST_0_BRANCH, NULL);
	}
	else {
		*(Symbol **) ql4thvm_here++ = &symbol_0branch;
		cs_ptr->pos = ql4thvm_here;
		ql4thvm_here++;
	}
}

static void cfp_else(void) {
	SSTNode * sst;

	if (cs_ptr->id != CSID_IF) {
		QLForth_error("No IF for this ELSE", NULL);
	}
	if (ql4thvm_state == QLF_STATE_COMPILE) {
		sst = QLForth_sst_append(SST_BRANCH, NULL);
		cs_ptr->sst->sst = QLForth_sst_append(SST_LABEL, NULL);
		cs_ptr->sst = sst;
	}
	else {
		*(Symbol **)ql4thvm_here++ = &symbol_branch;
		ql4thvm_here++;
		cs_ptr->pos->ptr = (void *)ql4thvm_here;
		cs_ptr->pos = (QLF_CELL *)(ql4thvm_here - 1);
	}
}

static void cfp_endif(void) {
	if (cs_ptr->id != CSID_IF) {
		QLForth_error("No IF for this THEN/ENDIF", NULL);
	}
	if (ql4thvm_state == QLF_STATE_COMPILE) {
		cs_ptr->sst->sst = QLForth_sst_append(SST_LABEL, NULL);
	}
	else {
		cs_ptr->pos->ptr = (void *)ql4thvm_here;
	}
	cs_ptr--;
}

static void cfp_begin(void) {
	cs_ptr++;
	cs_ptr->id = CSID_BEGIN;
	if (ql4thvm_state == QLF_STATE_COMPILE) {
		cs_ptr->sst = QLForth_sst_append(SST_LABEL, NULL);
	}
	else {
		cs_ptr->pos = ql4thvm_here;
	}
}

static void cfp_while(void) {
	if (cs_ptr->id != CSID_BEGIN) {
		QLForth_error("No BEGIN for this WHILE", NULL);
	}
	cs_ptr++;
	cs_ptr->id = CSID_WHILE;
	if (ql4thvm_state == QLF_STATE_COMPILE) {
		cs_ptr->sst = QLForth_sst_append(SST_0_BRANCH, NULL);
	}
	else {
		*(Symbol **)ql4thvm_here++ = &symbol_0branch;
		cs_ptr->pos = (QLF_CELL *)ql4thvm_here;			// pointer for fill back
		ql4thvm_here++;									// reserved a hole for fill
	}
}

static void cfp_repeat(void) {
	SSTNode * sst;

	if (cs_ptr->id != CSID_WHILE) {
		QLForth_error("No WHILE for this REPEAT", NULL);
	}
	if (ql4thvm_state == QLF_STATE_COMPILE) {
		sst = QLForth_sst_append(SST_BRANCH, 0);
		cs_ptr->sst->sst = QLForth_sst_append(SST_LABEL, NULL);
		cs_ptr--;
		sst->sst = cs_ptr->sst;
		cs_ptr--;
	}
	else {
		cs_ptr->pos->ptr = (void *)(ql4thvm_here + 2);
		cs_ptr--;
		*(Symbol **)ql4thvm_here++ = &symbol_branch;
		*(QLF_CELL **)ql4thvm_here++ = cs_ptr->pos;
		cs_ptr--;
	}
}

static void cfp_until(void) {
	if (cs_ptr->id != CSID_BEGIN) {
		QLForth_error("No WHILE for this UNTIL", NULL);
	}
	if (ql4thvm_state == QLF_STATE_COMPILE) {
		QLForth_sst_append(SST_0_BRANCH, cs_ptr->sst);
	}
	else {
		cs_ptr->pos->ptr = (void *) (ql4thvm_here + 2);
		*(Symbol **)  ql4thvm_here++ = &symbol_0branch;
		*(QLF_CELL **)ql4thvm_here++ = cs_ptr->pos;
	}
	cs_ptr--;
}

static void cfp_do(void) {
	cs_ptr++;
	cs_ptr->id = CSID_DO;
	if (ql4thvm_state == QLF_STATE_COMPILE) {
		cs_ptr->sst = QLForth_sst_append(SST_DO, NULL);
	}
	else {
		cs_ptr->pos = ql4thvm_here;
	}
}

static void cfp_loop(void) {
	static Symbol symbol_do_loop = { NULL,{ fp_do_loop } ,{ "(LOOP)" }, 0 };

	if (cs_ptr->id != CSID_DO) {
		QLForth_error("No DO matched this LOOP", NULL);
	}
	if (ql4thvm_state == QLF_STATE_COMPILE) {
		QLForth_sst_append(SST_LOOP, cs_ptr->sst);
	}
	else {
		*(Symbol **)ql4thvm_here++ = &symbol_do_loop;
		*(QLF_CELL **)ql4thvm_here++ = cs_ptr->pos;
	}
	cs_ptr--;
}

static void cfp_leave(void) {
	static Symbol symbol_do_exit = { NULL,{ fp_do_leave } ,{ "(LEAVE)" }, 0 };
	CONTROL_STACK * csp;

	for (csp = cs_ptr; csp->id != SST_COLON && csp > cs_stack; csp--) {
		if (csp->id == CSID_DO) {
			break;
		}
	}
	if (csp->id == SST_COLON || csp == cs_stack) {
		QLForth_error("No DO or FOR matched this EXIT", NULL);
	}
	if (ql4thvm_state == QLF_STATE_COMPILE) {
		QLForth_sst_append(SST_LEAVE, csp->sst);
	}
	else {
		*(Symbol **)ql4thvm_here++ = &symbol_do_exit;
		cs_ptr->pos = ql4thvm_here;
		*(QLF_CELL **)ql4thvm_here++ = 0;
	}
}

static void cfp_for(void) {
	cs_ptr++;
	cs_ptr->id	= CSID_FOR;
	if (ql4thvm_state == QLF_STATE_COMPILE) {
		cs_ptr->sst = QLForth_sst_append(SST_FOR, NULL);
	}
	else {
		cs_ptr->pos = ql4thvm_here;
	}
}

static void cfp_next(void) {
	static Symbol symbol_do_for_next = { NULL,{ fp_do_for_next },{ "(FOR-NEXT)" } ,0 };

	if (cs_ptr->id != CSID_FOR) {
		QLForth_error("No FOR  with this NEXT", NULL);
	}
	if (ql4thvm_state == QLF_STATE_COMPILE) {
		QLForth_sst_append(SST_NEXT, cs_ptr->sst);
	}
	else {
		QLForth_sst_append(SST_NEXT, cs_ptr->sst);
		*(Symbol **)ql4thvm_here++ = &symbol_do_for_next;
		*(QLF_CELL **)ql4thvm_here++ = cs_ptr->pos;
	}
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
	sst->val = ql4thvm_tos.ival;
	ql4thvm_dp--;
	ql4thvm_tos.ptr = ql4thvm_dp->ptr;
}

static void cfp_allot(void) {
	SSTNode * sst;

	sst = QLForth_sst_append(SST_ALLOT, NULL);
	sst->val = ql4thvm_tos.ival;
	ql4thvm_dp--;
	ql4thvm_tos.ptr = ql4thvm_dp->ptr;
}

// ************************************************************************************

static void cfp_macro(void) {
	if (!QLForth_token()) {
		QLForth_error("Expect a word", NULL);
	}

	if ((ThisCreateWord = QLForth_symbol_add(token_word)) == NULL) {
		return;
	}
	ThisCreateWord->type	= QLF_TYPE_MACRO;
	ThisCreateWord->fun		= fp_docol;
	ThisCreateWord->dfa		= ql4thvm_here;
	ThisCreateWord->hidden	= 1;
	cs_ptr					= &cs_stack[0];
	cs_ptr->id				= CSID_MACRO;
	ql4thvm_state			= QLF_STATE_MACRO;
	cs_ptr++;
}

static void cfp_macro_semicolon(void) {
	static Symbol symbol_do_next = { NULL,{ fp_donext },{ "(NEXT)" },	0 };

	cs_ptr--;
	if ((cs_ptr != &cs_stack[0]) || (cs_ptr->id != CSID_MACRO)) {
		QLForth_error("Syntax structure mismatched or in-completely", NULL);
	}
	cs_ptr--;

	if (ql4thvm_state == QLF_STATE_MACRO) {
		*(Symbol **)ql4thvm_here++ = &symbol_do_next;
		ql4thvm_state = QLF_STATE_INTERPRET;
	}
	else {
		QLForth_error("State changed during word compilation", NULL);
	}
	ThisCreateWord->hidden = 0;
	ThisCreateWord = NULL;
}

static void cfp_colon(void) {
	if (!QLForth_token()) {
		QLForth_error("Expect a word", NULL);
	}

	if ((ThisCreateWord = QLForth_symbol_add(token_word)) == NULL) {
		return;
	}
	ThisCreateWord->type	= QLF_TYPE_WORD;
	ThisCreateWord->dfa		= 0;
	ThisCreateWord->hidden	= 1;
	ThisCreateWord->sst		= QLForth_sst_append(SST_COLON, (SSTNode *)ThisCreateWord);
	cs_ptr					= &cs_stack[0];
	cs_ptr->id				= CSID_COLON;
	ql4thvm_state			= QLF_STATE_COMPILE;
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
	QLForth_sst_append(SST_VARIABLE, (SSTNode *)spc);
	spc->dfa = 0;
	ql4thvm_here++;
}

// ************************************************************************************

static Primitive define_table[] = {
	{ ":",				cfp_colon			},
	{ "VARIABLE",		cfp_variable		},

	{ "::",				cfp_macro			},

	{ NULL,				NULL	}
};

static Primitive immediate_table[] = {
	{ ";",				cfp_semicolon		},
	{ ";;",				cfp_macro_semicolon },
	{ "\'",				cfp_tick			},
	{ "#",				cfp_sharp			},
	{ "ALLOT",			cfp_allot			},

	{ "IF",				cfp_if				},
	{ "ELSE",			cfp_else			},
	{ "ENDIF",			cfp_endif			},
	{ "THEN",			cfp_endif			},
	{ "BEGIN",			cfp_begin			},
	{ "WHILE",			cfp_while			},
	{ "REPEAT",			cfp_repeat			},
	{ "UNTIL",			cfp_until			},
	{ "DO",				cfp_do				},
	{ "LOOP",			cfp_loop			},
	{ "LEAVE",			cfp_leave			},
	{ "FOR",			cfp_for				},
	{ "NEXT",			cfp_next			},

	{ NULL,			NULL }
};

static Primitive primitive_table[] = {
	{ "DROP",			cfp_drop			},
	{ "DUP",			cfp_dup				},
	{ "?DUP",			cfp_qdup			},
	{ "OVER",			cfp_over			},
	{ "SWAP",			cfp_swap			},
	{ "ROT",			cfp_rot				},
	{ "-ROT",			cfp_mrot			},
	{ "PICK",			cfp_pick			},
	{ "ROLL",			cfp_roll			},

	{ "DEPTH",			cfp_depth			},

	{ "+",				cfp_plus			},
	{ "-",				cfp_minus			},
	{ "*",				cfp_mul				},
	{ "/",				cfp_div				},
	{ "MOD",			cfp_mod				},
	{ "/MOD",			cfp_divmod			},
	{ "1+",				cfp_1_add			},
	{ "1-",				cfp_1_sub			},
	{ "2+",				cfp_2_add			},
	{ "2-",				cfp_2_sub			},
	{ "2*",				cfp_2_mul			},
	{ "2/",				cfp_2_div			},
	{ "4+",				cfp_4_add			},
	{ "4-",				cfp_4_sub			},

	{ "MIN",			cfp_min				},
	{ "MAX",			cfp_max				},
	{ "ABS",			cfp_abs				},
	{ "NEGATE",			cfp_negate			},

	{ "<",				cfp_cmp_lt			},
	{ "<=",				cfp_cmp_le			},
	{ "<>",				cfp_cmp_ne			},
	{ "=",				cfp_cmp_eq			},
	{ ">",				cfp_cmp_gt			},
	{ ">=",				cfp_cmp_ge			},
	{ "0<",				cfp_cmp_0_lt		},
	{ "0=",				cfp_cmp_0_eq		},
	{ "0>",				cfp_cmp_0_gt		},

	{ "AND",			cfp_and				},
	{ "OR",				cfp_or				},
	{ "XOR",			cfp_xor				},
	{ "NOT",			cfp_not				},

	{ "F+",				cfp_f_plus			},
	{ "F-",				cfp_f_minus			},
	{ "F*",				cfp_f_mul			},
	{ "F/",				cfp_f_div			},
	{ "F<",				cfp_f_lt			},
	{ "F<=",			cfp_f_le			},
	{ "F<>",			cfp_f_ne			},
	{ "F=",				cfp_f_eq			},
	{ "F>",				cfp_f_gt			},
	{ "F>=",			cfp_f_ge			},
	{ "FABS",			cfp_f_abs			},
	{ "ACOS",			cfp_f_acos			},
	{ "ASIN",			cfp_f_asin			},
	{ "ATAN",			cfp_f_atan			},
	{ "ATAN2",			cfp_f_atan2			},
	{ "COS",			cfp_f_cos			},
	{ "EXP",			cfp_f_exp			},
	{ "FLOAT",			cfp_f_float			},
	{ "LOG",			cfp_f_log			},
	{ "LOG10",			cfp_f_log10			},
	{ "FMAX",			cfp_f_max			},
	{ "FMIN",			cfp_f_min			},
	{ "FNEGATE",		cfp_f_negate		},
	{ "POW",			cfp_f_pow			},
	{ "SIN",			cfp_f_sin			},
	{ "SQRT",			cfp_f_sqrt			},
	{ "TAN",			cfp_f_tan			},

	{ NULL,				NULL	}
};

// ************************************************************************************

void Forth_init(void) {
	Primitive * ppc;
	Symbol	* spc;

	for (ppc = define_table; ppc->pname != NULL; ppc++) {
		spc			= QLForth_symbol_add(ppc->pname);
		spc->fun	= ppc->fun;
		spc->type	= QLF_TYPE_DEFINE;
	}

	for (ppc = immediate_table; ppc->pname != NULL; ppc++) {
		spc			= QLForth_symbol_add(ppc->pname);
		spc->fun	= ppc->fun;
		spc->type	= QLF_TYPE_IMMEDIATE;
	}

	for (ppc = primitive_table; ppc->pname != NULL; ppc++) {
		spc			= QLForth_symbol_add(ppc->pname);
		spc->fun	= ppc->fun;
		spc->type	= QLF_TYPE_PRIMITIVE;
	}
}
