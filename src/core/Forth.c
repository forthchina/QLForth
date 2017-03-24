/* ************************************************************************************
* Copyright (C) 2015-2017 Shaanxi QinWei Electroics Technologies. All rights reserved.
*
* $Date:        24. March 2017
* $Revision: 	V 0.1
*
* Project: 	    QLForth - A Forth for System-On-Chip Creative Makers
*
* Title:	    main.c
*
* Description:	The Cross Compile functions for QLForth
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
	CSID_IF, CSID_ELSE, CSID_ENDIF,
	CSID_CASE, CSID_OF, CSID_ENDOF,
	CSID_BEGIN, CSID_WHILE, CSID_AGAIN,
	CSID_DO, CSID_QMARK_DO, CSID_EXIT,
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

// ************************************************************************************

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

static Symbol	symbol_0branch	= { NULL,{ fp_0branch } ,	{ "(0BRANCH)" },	0 },
				symbol_branch	= { NULL,{ fp_branch } ,	{ "(BRANCH)"  },	0 },
				symbol_caseof	= { NULL,{ fp_caseof } ,	{ "(CASEOF)"  },	0 },
				symbol_endcase	= { NULL,{ fp_endcase } ,	{ "(ENDCASE)" },	0 };

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
	* (Symbol **) ql4thvm_here++ = &symbol_do_qmark_do;
	* (QLF_CELL **) ql4thvm_here++ = cs_ptr->pos;
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

// ************************************************************************************

static Primitive immediate_table[] = {
	{ "IF",				cfp_if		},
	{ "ELSE",			cfp_else	},
	{ "ENDIF",			cfp_endif	},
	{ "THEN",			cfp_endif	},
	{ "CASE",			cfp_case	},
	{ "OF",				cfp_of		},
	{ "ENDOF",			cfp_endof	},
	{ "ENDCASE",		cfp_endcase },
	{ "BEGIN",			cfp_begin	},
	{ "WHILE",			cfp_while	},
	{ "AGAIN",			cfp_again	},
	{ "REPEAT",			cfp_repeat	},
	{ "UNTIL",			cfp_until	},
	{ "DO",				cfp_do		},
	{ "LOOP",			cfp_loop	},
	{ "+LOOP",			cfp_plus_loop},
	{ "EXIT",			cfp_exit	},
	{ "FOR",			cfp_for		},
	{ "NEXT",			cfp_next	},

	{ NULL,				NULL		}

};

// ************************************************************************************

void Forth_init(void) {
	Primitive * ppc;
	Symbol	* spc;

	for (ppc = immediate_table; ppc->pname != NULL; ppc++) {
		spc = QLForth_symbol_add(ppc->pname);
		spc->fun = ppc->fun;
		spc->type = QLF_TYPE_IMMEDIATE;
	}
}
