/* ************************************************************************************
* Copyright (C) 2015-2017 Shaanxi QinWei Electroics Technologies. All rights reserved.
*
* $Date:        11 May 2017
* $Revision: 	V 0.5
*
* Project: 	    QLForth - A Forth for System-On-Chip Creative Makers
*
* Title:	    Compiler.c
*
* Description:	The cross compiler for QLForth
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
#include "QLSOC.h"

static void cfp_fallthru(void) {
	QLForth_sst_append(SST_RET, 0);
}

static void cfp_noop(void) {
	QLForth_sst_append(SST_NOOP, 0);
}

static void cfp_add(void) {
	QLForth_sst_append(SST_ADD, 0);
}

static void cfp_xor(void) {
	QLForth_sst_append(SST_XOR, 0);
}

static void cfp_and(void) {
	QLForth_sst_append(SST_AND, 0);
}

static void cfp_or(void) {
	QLForth_sst_append(SST_OR, 0);
}

static void cfp_invert(void) {
	QLForth_sst_append(SST_INVERT, 0);
}

static void cfp_cmp_eq(void) {
	QLForth_sst_append(SST_EQUAL, 0);
}

static void cfp_cmp_lt(void) {
	QLForth_sst_append(SST_LT, 0);
}

static void cfp_cmp_u_lt(void) {
	QLForth_sst_append(SST_U_LT, 0);
}

static void cfp_swap(void) {
	QLForth_sst_append(SST_SWAP, 0);
}

static void cfp_dup(void) {
	QLForth_sst_append(SST_DUP, 0);
}

static void cfp_drop(void) {
	QLForth_sst_append(SST_DROP, 0);
}

static void cfp_over(void) {
	QLForth_sst_append(SST_OVER, 0);
}

static void cfp_nip(void) {
	QLForth_sst_append(SST_NIP, 0);
}

static void cfp_to_r(void) {
	QLForth_sst_append(SST_TO_R, 0);
}

static void cfp_from_r(void) {
	QLForth_sst_append(SST_R_FROM, 0);
}

static void cfp_r_fetch(void) {
	QLForth_sst_append(SST_R_FETCH, 0);
}

static void cfp_store(void) {
	QLForth_sst_append(SST_STORE, 0);
}

static void cfp_fetch(void) {
	QLForth_sst_append(SST_FETCH, 0);
}

static void cfp_dsp(void) {
	QLForth_sst_append(SST_DSP, 0);
}

static void cfp_lshift(void) {
	QLForth_sst_append(SST_LSHIFT, 0);
}

static void cfp_rshift(void) {
	QLForth_sst_append(SST_RSHIFT, 0);
}

static void cfp_1_sub(void) {
	QLForth_sst_append(SST_1_SUB, 0);
}

static void cfp_2_r_from(void) {
	QLForth_sst_append(SST_2_R_FROM, 0);
}

static void cfp_2_to_r(void) {
	QLForth_sst_append(SST_2_TO_R, 0);
}

static void cfp_2_r_fetch(void) {
	QLForth_sst_append(SST_2_R_FETCH, 0);
}

static void cfp_unloop(void) {
	QLForth_sst_append(SST_UNLOOP, 0);
}


static void cfp_dup_fetch(void) {
	QLForth_sst_append(SST_DUP_FETCH, 0);
}

static void cfp_dup_to_r(void) {
	QLForth_sst_append(SST_DUP_TO_R, 0);
}

static void cfp_2_dup_xor(void) {
	QLForth_sst_append(SST_2_DUP_XOR, 0);
}

static void cfp_2_dup_eq(void) {
	QLForth_sst_append(SST_2_DUP_EQUAL, 0);
}

static void cfp_store_nip(void) {
	QLForth_sst_append(SST_STORE_NIP, 0);
}

static void cfp_2_nip_store(void) {
	QLForth_sst_append(SST_2_DUP_STORE, 0);
}

static void cfp_up_1(void) {
	QLForth_sst_append(SST_UP_1, 0);
}

static void cfp_down_1(void) {
	QLForth_sst_append(SST_DOWN_1, 0);
}

static void cfp_copy(void) {
	QLForth_sst_append(SST_COPY, 0);
}

static void display_assembly_instruction(int word) {
	static char * instr[] = {
		"+",		"xor",		"and",		"or",		"invert",
		"=",		"<",		"u<",		"swap",		"dup",
		"drop",		"over",		"nip",		">r",		"r>",
		"r@",		"dsp",		"lshift",	"rshift",
		"1-",		"2r>",		"2>r",		"2r@",		"unloop",

		"dup@",		"dup>r",	"2dupxor",	"2dup=",	
		"!nip",		"2dup!",	
		"UP1",		"DOWN1",	"COPY"
	};

	if (word < SST_ADD || word > SST_COPY) {
		QLForth_printf("Unknown SST type cdoe : %d (0x%02x).\n", word, word);
	}
	QLForth_printf("%s\n", instr[word - SST_ADD]);
}

// ************************************************************************************

static Primitive extension_table[] = {
	{ "-;",			cfp_fallthru	},

	{ "NOOP",		cfp_noop		},
	{ "+",			cfp_add			},
	{ "XOR",		cfp_xor			},
	{ "AND",		cfp_and			},
	{ "OR",			cfp_or			},
	{ "INVERT",		cfp_invert		},
	{ "=",			cfp_cmp_eq		},
	{ "<",			cfp_cmp_lt		},
	{ "U<",			cfp_cmp_u_lt	},
	{ "SWAP",		cfp_swap		},
	{ "DUP",		cfp_dup			},
	{ "DROP",		cfp_drop		},
	{ "OVER",		cfp_over		},
	{ "NIP",		cfp_nip			},
	{ ">R",			cfp_to_r		},
	{ "R>",			cfp_from_r		},
	{ "R@",			cfp_r_fetch		},
	{ "!",			cfp_store		},
	{ "@",			cfp_fetch		},
	{ "DSP",		cfp_dsp			},
	{ "LSHIFT",		cfp_lshift		},
	{ "RSHIFT",		cfp_rshift		},

	{ "1-",			cfp_1_sub		},
	{ "2R>",		cfp_2_r_from	},
	{ "2>R",		cfp_2_to_r		},
	{ "2R@",		cfp_2_r_fetch	},
	{ "UNLOOP",		cfp_unloop		},

// Elided words
	{ "DUP@",		cfp_dup_fetch	},
	{ "DUP>R",		cfp_dup_to_r	},
	{ "2DUPXOR",	cfp_2_dup_xor	},
	{ "2DUP=",		cfp_2_dup_eq	},
	{ "!NIP",		cfp_store_nip	},
	{ "2NIP!",		cfp_2_nip_store },

// Words used to implement pick
	{ "UP1",		cfp_up_1		},
	{ "DOWN1",		cfp_down_1		},
	{ "COPY",		cfp_copy		},

	{ NULL,			NULL			}
};

void Compile_init(void) {
	Primitive * ppc;
	Symbol	* spc;

	for (ppc = extension_table; ppc->pname != NULL; ppc++) {
		spc = QLForth_symbol_add(ppc->pname);
		spc->fun	= ppc->fun;
		spc->type	= QLF_TYPE_COMPILE;
	}
}

void QLForth_sst_list(SSTNode * start, SSTNode * sst) {
	SSTNode * ptr;
	int count;

	count = (QLF_CELL) (start - sst);
	for (ptr = start; ptr->type != SST_SEMICOLON; ptr++, count++) {
		QLForth_printf("L_%08X  ", count);
		switch (ptr->type) {
			case SST_NOOP:			QLForth_printf("No Operation.\n");											break;

			case SST_COLON:			QLForth_printf(" ...... %s ( 0x%04x ) .......\n", 
																			ptr->spc->name, ptr->spc->dfa);		break;
			case SST_RET:			QLForth_printf("RETURN\n");													break;
			case SST_SEMICOLON:		QLForth_printf("END -; \n\n");												break;
			case SST_VARIABLE:		QLForth_printf("VARIABLE %s @ 0x%04x.\n", ptr->spc->name, ptr->spc->dfa);	break;

			case SST_LITERAL:		QLForth_printf("LITERAL %d, 0x%08X.\n", ptr->val, ptr->val);				break;
			case SST_VAR_REF:		QLForth_printf("VAR REF %s\n", ptr->spc->name);								break;
			case SST_WORD_REF:		QLForth_printf("CALL WORD %s\n", ptr->spc->name);							break;

			case SST_LABEL:			QLForth_printf("LABEL @ L_%08X\n",		ptr->val);							break;
			case SST_0_BRANCH:		QLForth_printf("ZERO BRANCH \tL_%08X\n",ptr->sst - sst);					break;
			case SST_BRANCH:		QLForth_printf("BRANCH \t\tL_%08X\n",	ptr->sst - sst);					break;

			case SST_DO:			QLForth_printf("DO\n");														break;
			case SST_LOOP:			QLForth_printf("LOOP \t\tL_%08X\n", ptr->sst - sst);						break;
			case SST_LEAVE:			QLForth_printf("LEAVE\t\tL_%08X\n", ptr->sst - sst);						break;
			case SST_I:				QLForth_printf("I\n");														break;
			case SST_J:				QLForth_printf("J\n");														break;

			case SST_FOR:			QLForth_printf("FOR\n");													break;
			case SST_NEXT:			QLForth_printf("NEXT \t\tL_%08X\n", ptr->sst - sst);						break;

			case SST_FETCH:			QLForth_printf("@\n");		break;
			case SST_STORE:			QLForth_printf("!\n");		break;
			case SST_ALLOT:			QLForth_printf("ALLOT  %d Bytes\n", ptr->val);								break;

			default:	  			display_assembly_instruction(ptr->type);												break;
		}
	}
}
