/* ************************************************************************************
* Copyright (C) 2015-2017 Shaanxi QinWei Electroics Technologies. All rights reserved.
*
* $Date:        9. May 2017
* $Revision: 	V 0.4
*
* Project: 	    QLForth - A Forth for System-On-Chip Creative Makers
*
* Title:	    Simulator.c
*
* Description:	The Chip Simulator for QLForth
* 				Simulation FPGA J1 ( http://www.excamera.com/sphinx/fpga-j1.html )
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

#include "QLForth.h"
#include "QLSOC.h"

static unsigned short tos, nos, vdp, vrp, vpc;
static unsigned short dstack[DSTACK_DEEP];
static unsigned short rstack[RSTACK_DEEP];
static unsigned short * ram; 
static int signed_extension[4] = { 0, 1, -2, -1 };		// 2-bit sign extension 

static void push(int v)	{
	vdp =(vdp + 1) & 0x1F ;
	dstack[vdp] = tos;
	tos = v;
}

static int pop(void) {
	int v = tos;

	tos = dstack[vdp];
	vdp = 0x1f & (vdp - 1);
	return v;
}

static int mem_fetch(int addr) {
	if (addr >= 0 && addr < MEM_SIZE_IN_BYTE) {
		printf("FETCH [0x%04X] = %d\n", addr, ram[addr] & 0xFFFF);
		return ram[addr] & 0xffff;
	}
	return 0;
}

static void mem_store(int addr, int data) {
	if (addr >= 0 && addr < MEM_SIZE_IN_BYTE) {
		ram[addr] = data;
		printf("STORE %d->[0x%04X]\n", data, addr);
	}
}

static void execute(int entrypoint) {
	int tmp_pc, tmp_tos;
	int target, insn = 0x4000 | entrypoint;									// this is instruction : call entrypoint

    tmp_tos = 0;
	for (tmp_pc = -1; tmp_pc != 0; ) {
		tmp_pc = vpc + 1;
		if (insn & 0x8000) {												// FPGA J1 : literal
			push(insn & 0x7fff);
			vpc		= tmp_pc;
			insn	= ram[vpc];
			continue;
		}

		target = insn & 0x1fff;
		switch (insn >> 13) {
			case 0:	tmp_pc = target;						break;			// jump
			case 1:	if (pop() == 0) {										// conditional jump
						tmp_pc = target;
					}										break;			
			case 2:	vrp = (vrp + 1) & 0x1F;									// call
					rstack[vrp] = tmp_pc << 1;		
					tmp_pc = target;						break;			
			case 3:															// alu, r->pc 
				if (insn & 0x1000) {							
					tmp_pc = rstack[vrp] >> 1;
				}
				nos = dstack[vdp];
				switch ((insn >> 8) & 0xf) {
					case 0:   tmp_tos = tos;				break;		// nop 
					case 1:   tmp_tos = nos;				break;		// copy 
					case 2:   tmp_tos = tos + nos;			break;		// + 
					case 3:   tmp_tos = tos & nos;			break;		// and
					case 4:   tmp_tos = tos | nos;			break;		// or
					case 5:   tmp_tos = tos ^ nos;			break;		// xor
					case 6:   tmp_tos = ~ tos;				break;		// invert
					case 7:   tmp_tos = -(tos == nos);		break;		// =
					case 8:   tmp_tos = -((signed short)nos < (signed short)tos); break;		// < 
					case 9:   tmp_tos = nos >> tos;			break;		// rshift 
					case 0xa: tmp_tos = tos - 1;			break;		// 1- 
					case 0xb: tmp_tos = rstack[vrp];		break;		// r@
					case 0xc: tmp_tos = mem_fetch(tos);		break;		// @
					case 0xd: tmp_tos = nos << tos;			break;		// lshift 
					case 0xe: tmp_tos = (vrp << 8) + vdp;	break;		// dsp
					case 0xf: tmp_tos = -(nos < tos);		break;		// u< 
				}
				vdp = (vdp + signed_extension[insn		  & 0x03]) & 0x1F;			// dstack +/- 
				vrp = (vrp + signed_extension[(insn >> 2) & 0x03]) & 0x1F;			// rstack +/- 
				if (insn & 0x80) {		// T->N bit 
					dstack[vdp] = tos;
				}
				if (insn & 0x40) {		// T->R bit 
					rstack[vrp] = tos;
				}
				if (insn & 0x20) {		// N->[T] 
					mem_store(tos, nos);
				}
				tos = tmp_tos;
				break;
		}
		vpc = tmp_pc;
		insn = ram[vpc];
	} 
}

void QLForth_chip_program(unsigned char * code_buffer) {
	ram = (unsigned short *) code_buffer;
}

int QLForth_chip_execute(int entry, int depth, int * stk) {
	int idx, * dpc;

	for (vpc = vrp = -1, dpc = stk, idx = 0; idx < depth; ) {
		dstack[idx ++] = * dpc ++;
	}
	tos = * dpc;
	vdp = (depth - 1) & 0x1F;

	execute(entry);

	depth = ((vdp + 1) & 0x1F) - 1;
	for (dpc = stk, idx = 0; idx <= depth; ) {
		* dpc ++ = (signed short) (dstack[idx++]) ;
	}
	* dpc = (signed short) tos;
	return (vdp + 1) & 0x1F;
}
