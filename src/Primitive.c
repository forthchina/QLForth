/* ************************************************************************************
* Copyright (C) 2015-2017 Shaanxi QinWei Electroics Technologies. All rights reserved.
*
* $Date:        25. April 2017
* $Revision: 	V 0.2
*
* Project: 	    QLForth - A Forth for System-On-Chip Creative Makers
*
* Title:	    Primitive.c
*
* Description:	The Forth Priitive Word for QLForth
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
	{ "DROP",			cfp_drop	},
	{ "DUP",			cfp_dup		},
	{ "?DUP",			cfp_qdup	},
	{ "OVER",			cfp_over	},
	{ "SWAP",			cfp_swap	},
	{ "ROT",			cfp_rot		},
	{ "-ROT",			cfp_mrot	},
	{ "PICK",			cfp_pick	},
	{ "ROLL",			cfp_roll	},
//	{ ">R",				cfp_to_r	},
//	{ "R@",				cfp_r_fetch },
//	{ "R>",				cfp_from_r	},
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
	{ "0<",				cfp_cmp_0_lt },
	{ "0=",				cfp_cmp_0_eq },
	{ "0>",				cfp_cmp_0_gt },

	{ "AND",			cfp_and		},
	{ "OR",				cfp_or		},
	{ "XOR",			cfp_xor		},
	{ "NOT",			cfp_not		},

//	{ "!",				cfp_store	},
//	{ "@",				cfp_fetch	},
//	{ ",",				cfp_comma	},

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
	{ "FNEGATE",		cfp_f_negate },
	{ "POW",			cfp_f_pow	},
	{ "SIN",			cfp_f_sin	},
	{ "SQRT",			cfp_f_sqrt	},
	{ "TAN",			cfp_f_tan	},

	{ NULL,				NULL }
};

// ************************************************************************************

void Primitive_init(void) {
	Primitive * ppc;
	Symbol	* spc;

	for (ppc = define_table; ppc->pname != NULL; ppc++) {
		spc = QLForth_symbol_add(ppc->pname);
		spc->fun = ppc->fun;
		spc->type = QLF_TYPE_PRIMITIVE;
	}
}
