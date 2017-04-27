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
* Description:	The Win32 console program main entry for QLForth
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
*
* ********************************************************************************** */

#include "QLForth.h"

static int ql_console_handler(int arg) {
	QLForth_stop();
	printf("QLForth VM Stoped!\n");
	return 1;
}



int QLForth_printf(const char * fmt, ...) {
	va_list data;
	int		ret;

	va_start(data, fmt);
	ret = vprintf(fmt, data);
	va_end(data);

	return ret;
}

int QLForth_display_stack(int base, int depth, int data1, int data2) {
	QLF_LITERAL ld1, ld2;
	if (depth < 1) {
		depth = 0;
	}

	if (base == 10) {
		switch (depth) {
			case 0:		break;
			case 1:		QLForth_printf("[TOS] %d ", data1);	break;
			case 2:		QLForth_printf("[TOS] %d, [NOS] = %d ", data1,  data2); break;
			default:	QLForth_printf("[TOS] %d, [NOS] = %d, [DEPTH] %d ", data1, data2, depth); break;
		}
	}
	else if (base == 16) {
		switch (depth) {
			case 0:		break;
			case 1:		QLForth_printf("[TOS] 0x%08X ", data1);	break;
			case 2:		QLForth_printf("[TOS] 0x%08X, [NOS] = 0x%08X ", data1, data2); break;
			default:	QLForth_printf("[TOS] 0x%08X, [NOS] = 0x%08X, [DEPTH] %d ", data1, data2, depth); break;
		}
	}
	else if (base == 0) {
		ld1.ival = data1;
		ld2.ival = data2;
		switch (depth) {
			case 0:		break;
			case 1:		QLForth_printf("[TOS] %6.2f ", ld1.fval);							break;
			case 2:		QLForth_printf("[TOS] %6.2f, [NOS] = %6.2f", ld1.fval, ld2.fval);	break;
			default:	QLForth_printf("[TOS] %6.2f, [NOS] = %6.2f, [DEPTH] %d ", ld1.fval, ld2.fval, depth); break;
		}
	}
	QLForth_printf("Ok. ");
	return 0;
}

void QLForth_report(int msg, int data1, int data2, int data3) {
	switch (msg) {
		case 0:		
			printf("Used Dictionary Space : %d of %d Bytes\n", data1, data2);		
			break;

		case 1:
			if (data1 == 0) printf("Error : %s\n", (char *) data3);
			else 			printf("Error at line %d : %s\n", data1 + 1, (char *)data3);
			break;

		case 2:
			break;
	}
}

int main(int argc, char * argv[]) {
	static char keypad_in_buffer[TEXT_LINE_SIZE + 2];

	QLForth_printf("QLForth Ver 0.3 \n") ;
	QLForth_printf("By ZhaoYu, 2014.01 - 2017.04, ALL Right Reserved.\n\n") ;

	SetConsoleCtrlHandler((PHANDLER_ROUTINE) ql_console_handler, TRUE);

	if (argc != 2) {
		QLForth_printf("Usage : QLForth project-file-name.QLF <CR>\n");
	    return 0;
    }

	QLForth_init(argv[1]);

	QLForth_printf("Ok. ");
	while (1) {
		if (fgets(keypad_in_buffer, TEXT_LINE_SIZE, stdin) != NULL) {
			QLForth_interpreter(keypad_in_buffer);
		}
    } 
}
