/* ************************************************************************************
* Copyright (C) 2015-2017 Shaanxi QinWei Electroics Technologies. All rights reserved.
*
* $Date:        11 May 2017
* $Revision: 	V 0.5
*
* Project: 	    QLForth - A Forth for System-On-Chip Creative Makers
*
* Title:	    Code.c
*
* Description:	The architecture specifies target code generator for QLForth
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

static int code_ptr, code_ptr_save;
static unsigned char code_buffer[MEM_SIZE_IN_BYTE + 1];
static SSTNode * sst;

static void code_not_support(char * name) {
	QLForth_error("Sorry for not implemented word - %s.", name);
}

static void code_ref_word(char * name) {
	Symbol * spc;

	if ((spc = QLForth_symbol_search(name)) == NULL) {
		QLForth_error("The primitive word : %s : not found.", name);
	}
	
	* (short *)& code_buffer[code_ptr] = (short)((spc->dfa & 0x1FFF) | 0x4000);
	code_ptr += 2;
}

static void code_with_possible_ret(int opcode) {
	if ((sst + 1)->type == SST_RET) {
		opcode |= 0x100C;
		sst++;
	}

	*(unsigned short *)& code_buffer[code_ptr] = opcode;
	code_ptr += 2;
}

static void code_word_only(int opcode) {
	*(unsigned short *) & code_buffer[code_ptr] = opcode;	
	code_ptr += 2;			
}

static void code_literal(int val) {
	if (!(val & 0x8000)) {
		code_word_only(0x8000 + ((sst->val) & 0x7FFF));
	}
	else {
		val = ~val;
		code_word_only(0x8000 + (val & 0x7FFF));
		code_with_possible_ret(0x6600);
	}
}

static void alu_code_packet(void) {
	switch(sst->type) {
		case SST_NOOP:			code_with_possible_ret(0);			break;
	
		case SST_ADD:			code_with_possible_ret(0x6203);		break;
		case SST_XOR:			code_with_possible_ret(0x6503);		break;
		case SST_AND:			code_with_possible_ret(0x6303);		break;
		case SST_OR:			code_with_possible_ret(0x6403);		break;
		case SST_INVERT:		code_with_possible_ret(0x6600);		break;
		case SST_EQUAL:			code_with_possible_ret(0x6703);		break;
		case SST_LT:			code_with_possible_ret(0x6803);		break;
		case SST_U_LT:			code_with_possible_ret(0x6F03);		break;
		case SST_SWAP:			code_with_possible_ret(0x6180);		break;
		case SST_DUP:			code_with_possible_ret(0x6081);		break;
		case SST_DROP:			code_with_possible_ret(0x6103);		break;
		case SST_OVER:			code_with_possible_ret(0x6181);		break;
		case SST_NIP:			code_with_possible_ret(0x6003);		break;
		case SST_TO_R:			code_with_possible_ret(0x6147);		break;
		case SST_R_FROM:		code_word_only(0x6B8D);				break;
		case SST_R_FETCH:		code_with_possible_ret(0x6B81);		break;
		case SST_DSP:			code_with_possible_ret(0x6E81);		break;
		case SST_LSHIFT:		code_with_possible_ret(0x6D03);		break;
		case SST_RSHIFT:		code_with_possible_ret(0x6903);		break;
		case SST_1_SUB:			code_with_possible_ret(0x6A00);		break;

		case SST_2_R_FROM:	

		case SST_2_TO_R:

		case SST_2_R_FETCH:											break;

		case SST_UNLOOP:		code_word_only(0x600C);				
								code_word_only(0x600C);				break;

		case SST_DUP_FETCH:		code_with_possible_ret(0x6C81);		break;
		case SST_DUP_TO_R:		code_with_possible_ret(0x6044);		break;
		case SST_2_DUP_XOR:		code_with_possible_ret(0x6541);		break;
		case SST_2_DUP_EQUAL:	code_with_possible_ret(0x6781);		break;
		case SST_STORE_NIP:		code_with_possible_ret(0x6023);		break;
		case SST_2_DUP_STORE:	code_with_possible_ret(0x6020);		break;

		case SST_UP_1:			code_with_possible_ret(0x6001);		break;
		case SST_DOWN_1:		code_with_possible_ret(0x6003);		break;
		case SST_COPY:			code_with_possible_ret(0x6100);		break; 

	// Control Logic
		case SST_DO:			code_ref_word("(DO)");				break;
		case SST_LOOP:			code_ref_word("(LOOP)");			break;
		case SST_LEAVE:			code_ref_word("(LEAVE)");			break;
		case SST_I:				code_ref_word("(I)");				break;
		case SST_J:				code_not_support("J");				break;
		case SST_FOR:			code_with_possible_ret(0x6147);		break;			// >R, push the index to R STACK 
		case SST_NEXT:			code_ref_word("(NEXT)");			break;
		case SST_RET:			code_with_possible_ret(0x100C);		break;

	// Memory access
		case SST_FETCH:			code_with_possible_ret(0x6C00);		break;
		case SST_STORE:			code_word_only(0x6123);
								code_with_possible_ret(0x6103);		break;
		
		default:	  		QLForth_printf("Unknown SST type cdoe : %d (0x%02x).\n", sst->type, sst->type);	break;
	}

}

// ************************************************************************************

void Code_init(void) {
	code_ptr = code_ptr_save = CODE_START + 2;
}

void Code_generation(SSTNode * sst_start) {

	if (code_ptr != (CODE_START + 2)) {
		code_ptr = code_ptr_save;
	}

	for (sst = sst_start; sst->type != SST_END; sst++) {
		switch (sst->type) {
			case SST_VARIABLE:	sst->spc->dfa = code_ptr >> 1;							code_ptr += 2;			break;
			case SST_COLON:		sst->spc->dfa = code_ptr >> 1;													break;
			case SST_SEMICOLON:																					break;
			case SST_LITERAL:	code_literal(sst->val);																	break;
			case SST_ALLOT:		code_ptr += (sst->val);		
								if (code_ptr & 0x01) code_ptr++;												break;

			case SST_VAR_REF:	code_word_only((sst->spc->dfa & 0x1FFF) | 0x8000);								break;
			case SST_WORD_REF:  code_word_only((sst->spc->dfa & 0x1FFF) | 0x4000);								break;
			case SST_LABEL:		sst->val = code_ptr >> 1;														break;

			case SST_0_BRANCH:	
			case SST_BRANCH:	sst->location = code_ptr;								code_ptr += 2;			break;

			default:			alu_code_packet();																break;
		}
	}

	for (sst = sst_start; sst->type != SST_END; sst++) {
		unsigned short * pc;

		pc = (unsigned short *) &code_buffer[sst->location];
		switch (sst->type) {
			// case SST_VAR_REF:	* pc = (sst->spc->dfa & 0x1FFF) | 0x8000;	break;
			// case SST_WORD_REF:	* pc = (sst->spc->dfa & 0x1FFF) | 0x4000;	break;

			case SST_0_BRANCH:  * pc = (sst->sst->val & 0x1FFF) | 0x2000;	break;
			case SST_BRANCH:	* pc = (sst->sst->val & 0x1FFF) ;			break;
			default:			break;
		}
	}

	if (code_ptr_save != (CODE_START + 2)) {
		code_ptr = code_ptr_save;
	}

//
//	for (opcode = 0; opcode < code_ptr; opcode += 2) {
//		QLForth_printf("L_%04X: %04X\n", opcode, (* (unsigned short *) &code_buffer[opcode]) & 0xffff);
//	}

	QLForth_chip_program(code_buffer);
}

void Code_dump(SSTNode * sst_start) {
}
