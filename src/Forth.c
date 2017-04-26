/* ************************************************************************************
* Copyright (C) 2015-2017 Shaanxi QinWei Electroics Technologies. All rights reserved.
*
* $Date:        25. April 2017
* $Revision: 	V 0.2
*
* Project: 	    QLForth - A Forth for System-On-Chip Creative Makers
*
* Title:	    Forth.c
*
* Description:	The Cross Compiler for QLForth
*
* Target:		Windows 7+
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

#include <windows.h>
#include <math.h>

#include "QLForth.h"
#include "QLSOC.h"

typedef enum {
	CSID_COLON = 1,
	CSID_IF, CSID_ELSE, CSID_ENDIF,
	CSID_BEGIN, CSID_WHILE,
	CSID_DO, CSID_LEAVE,
	CSID_FOR,

	END_OF_CSID_CODE
} CSID_CODE;

typedef struct _cstack {
	int			id;
	union {
		QLF_CELL	* pos;
		SSTNode		* sst;
	};
} ControlStack;

static ControlStack	cs_stack[MAX_CONTROL_STACK_DEEP + 1], *cs_ptr;

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
	ThisCreateWord->sst		= QLForth_sst_append(SST_COLON, (int)ThisCreateWord);
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
	QLForth_sst_append(SST_RET, 0);
	QLForth_sst_append(SST_SEMICOLON, 0);
	ql4thvm_state			= QLF_STATE_INTERPRET;
	
	ThisCreateWord->hidden	= 0;
	ThisCreateWord			= NULL;
}

static void cfp_variable(void) {
	Symbol	* spc;
	SSTNode * sst;

	if (!QLForth_token()) {
		QLForth_error("Expect a word", NULL);
	}

	if ((spc = QLForth_symbol_add(token_word)) == NULL) {
		return;
	}
	spc->type = QLF_TYPE_VARIABLE;
	sst = QLForth_sst_append(SST_VARIABLE, (int)spc);
	spc->dfa = 0;
	spc->type = QLF_TYPE_PRIMITIVE;
	spc->fun = qlforth_fp_docreate;
	spc->dfa = (QLF_CELL)ql4thvm_here;
	ql4thvm_here++;
}

// ************************************************************************************

static void cfp_if(void) {
	cs_ptr++;
	cs_ptr->id = CSID_IF;
	cs_ptr->sst = QLForth_sst_append(SST_0_BRANCH, 0);
}

static void cfp_else(void) {
	SSTNode * sst;

	if (cs_ptr->id != CSID_IF) {
		QLForth_error("No IF for this ELSE", NULL);
	}
	sst = QLForth_sst_append(SST_BRANCH, 0);
	cs_ptr->sst->sst = QLForth_sst_append(SST_LABEL, 0);
	cs_ptr->sst = sst;
}

static void cfp_endif(void) {
	if (cs_ptr->id != CSID_IF) {
		QLForth_error("No IF for this THEN/ENDIF", NULL);
	}
	cs_ptr->sst->sst = QLForth_sst_append(SST_LABEL, 0);
	cs_ptr--;
}

static void cfp_begin(void) {
	cs_ptr++;
	cs_ptr->id = CSID_BEGIN;
	cs_ptr->sst = QLForth_sst_append(SST_LABEL, 0);
	cs_ptr->pos = ql4thvm_here;
}

static void cfp_while(void) {
	if (cs_ptr->id != CSID_BEGIN) {
		QLForth_error("No BEGIN for this WHILE", NULL);
	}
	cs_ptr++;
	cs_ptr->id = CSID_WHILE;
	cs_ptr->sst = QLForth_sst_append(SST_0_BRANCH, 0);
}

static void cfp_repeat(void) {
	SSTNode * sst;

	if (cs_ptr->id != CSID_WHILE) {
		QLForth_error("No WHILE for this REPEAT", NULL);
	}
	sst = QLForth_sst_append(SST_BRANCH, 0);
	cs_ptr->sst->sst = QLForth_sst_append(SST_LABEL, 0);
	cs_ptr--;
	sst->sst = cs_ptr->sst;
	cs_ptr--;
}

static void cfp_until(void) {
	if (cs_ptr->id != CSID_BEGIN) {
		QLForth_error("No WHILE for this UNTIL", NULL);
	}
	QLForth_sst_append(SST_0_BRANCH, (int)cs_ptr->sst);
	cs_ptr--;
}

static void cfp_do(void) {
	cs_ptr++;
	cs_ptr->id = CSID_DO;
	cs_ptr->sst = QLForth_sst_append(SST_DO, 0);
}

static void cfp_loop(void) {
	if (cs_ptr->id != CSID_DO) {
		QLForth_error("No DO matched this LOOP", NULL);
	}
	QLForth_sst_append(SST_LOOP, (int)cs_ptr->sst);
	cs_ptr--;
}

static void cfp_leave(void) {
	ControlStack * csp;

	for (csp = cs_ptr; csp->id != SST_COLON && csp > cs_stack; csp--) {
		if (csp->id == CSID_DO) {
			break;
		}
	}
	if (csp->id == SST_COLON || csp == cs_stack) {
		QLForth_error("No DO or FOR matched this EXIT", NULL);
	}
	QLForth_sst_append(SST_LEAVE, (int)csp->sst);
}

static void cfp_for(void) {
	cs_ptr++;
	cs_ptr->id = CSID_FOR;
	cs_ptr->sst = QLForth_sst_append(SST_FOR, 0);
	cs_ptr->pos = ql4thvm_here;
}

static void cfp_next(void) {

	if (cs_ptr->id != CSID_FOR) {
		QLForth_error("No FOR  with this NEXT", NULL);
	}
	QLForth_sst_append(SST_NEXT, (int)cs_ptr->sst);
	cs_ptr--;
}

static void cfp_tick(void) {
	Symbol * spc;

	if (!QLForth_token()) {
		QLForth_error("Expect a word", NULL);
	}
	_strupr(token_word);
	if ((spc = QLForth_symbol_search(token_word)) == NULL) {
		QLForth_error("Word not found", token_word);
	}
}

static void cfp_sharp(void) {
	QLForth_sst_append(SST_LITERAL, ql4thvm_tos);
	ql4thvm_tos = *(--ql4thvm_dp);
}

static void cfp_allot(void) {
	QLForth_sst_append(SST_ALLOT, ql4thvm_tos);
	ql4thvm_tos = *(--ql4thvm_dp);
}

// ....................................................................................

static Primitive define_table[] = {
	{ ":",				cfp_colon		},
	{ "VARIABLE",		cfp_variable	},

	{ NULL,				NULL			}
};

static Primitive immediate_table[] = {
	{ ";",				cfp_semicolon	},
	{ "\'",				cfp_tick		},
	{ "#",				cfp_sharp		},
	{ "ALLOT",			cfp_allot		},

	{ "IF",				cfp_if			},
	{ "ELSE",			cfp_else		},
	{ "ENDIF",			cfp_endif		},
	{ "THEN",			cfp_endif		},
	{ "BEGIN",			cfp_begin		},
	{ "WHILE",			cfp_while		},
	{ "REPEAT",			cfp_repeat		},
	{ "UNTIL",			cfp_until		},
	{ "DO",				cfp_do			},
	{ "LOOP",			cfp_loop		},
	{ "LEAVE",			cfp_leave		},
	{ "FOR",			cfp_for			},
	{ "NEXT",			cfp_next		},

	{ NULL,			NULL }
};

// ************************************************************************************

void Forth_init(void) {
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
		spc->type = QLF_TYPE_COMPILE;
	}
}
