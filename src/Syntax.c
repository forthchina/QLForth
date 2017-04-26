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
* Description:	The editor's syntax highlight functions with Scintilla for QLForth
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
#include <windowsx.h>
#include <commctrl.h>
#include <richedit.h>
#include <string.h>
#include <stdio.h>

#include "Scintilla.h"
#include "SciLexer.h"

#include "ide.h"
#include "QLFORTH.h"

#define WORD_NAME_LENGTH	32

#define HASH_SIZE			127

typedef struct _key_word {
	unsigned char		* name;
	unsigned int		attr;
	struct   _key_word	* next;
} KeyWord;

static KeyWord	* bucket[HASH_SIZE + 1];
static KeyWord  key_table[] = {
	{ ":",				SSC_DEFINE_WORD },
	{ "VARIABLE",		SSC_DEFINE_WORD },
//	{ "CREATE",			SSC_DEFINE_WORD },
//	{ "DOES>",			SSC_DEFINE_WORD },
	
	{ ";",				SSC_IMMEDIATE	},
	{ "\'",				SSC_IMMEDIATE	},
	{ "#",				SSC_IMMEDIATE	},

	{ "IF",				SSC_IMMEDIATE	},
	{ "ELSE",			SSC_IMMEDIATE	},
	{ "ENDIF",			SSC_IMMEDIATE	},
	{ "THEN",			SSC_IMMEDIATE	},
	{ "BEGIN",			SSC_IMMEDIATE	},
	{ "WHILE",			SSC_IMMEDIATE	},
	{ "AGAIN",			SSC_IMMEDIATE	},
	{ "REPEAT",			SSC_IMMEDIATE	},
	{ "UNTIL",			SSC_IMMEDIATE	},
	{ "DO",				SSC_IMMEDIATE	},
	{ "LOOP",			SSC_IMMEDIATE	},
	{ "+LOOP",			SSC_IMMEDIATE	},
	{ "EXIT",			SSC_IMMEDIATE	},
	{ "FOR",			SSC_IMMEDIATE	},
	{ "NEXT",			SSC_IMMEDIATE	},

	{ "DROP",			SSC_PRIMITIVE	},
	{ "DUP",			SSC_PRIMITIVE	},
	{ "?DUP",			SSC_PRIMITIVE	},
	{ "OVER",			SSC_PRIMITIVE	},
	{ "SWAP",			SSC_PRIMITIVE	},
	{ "ROT",			SSC_PRIMITIVE	},
	{ "-ROT",			SSC_PRIMITIVE	},
	{ "PICK",			SSC_PRIMITIVE	},
	{ "ROLL",			SSC_PRIMITIVE	},
	{ ">R",				SSC_PRIMITIVE	},
	{ "R@",				SSC_PRIMITIVE	},
	{ "R>",				SSC_PRIMITIVE	},
	{ "DEPTH",			SSC_PRIMITIVE	},

	{ "+",				SSC_PRIMITIVE	},
	{ "-",				SSC_PRIMITIVE	},
	{ "*",				SSC_PRIMITIVE	},
	{ "/",				SSC_PRIMITIVE	},
	{ "MOD",			SSC_PRIMITIVE	},
	{ "/MOD",			SSC_PRIMITIVE	},
	{ "1+",				SSC_PRIMITIVE	},
	{ "1-",				SSC_PRIMITIVE	},
	{ "2+",				SSC_PRIMITIVE	},
	{ "2-",				SSC_PRIMITIVE	},
	{ "2*",				SSC_PRIMITIVE	},
	{ "2/",				SSC_PRIMITIVE	},
	{ "4+",				SSC_PRIMITIVE	},
	{ "4-",				SSC_PRIMITIVE	},

	{ "MIN",			SSC_PRIMITIVE	},
	{ "MAX",			SSC_PRIMITIVE	},
	{ "ABS",			SSC_PRIMITIVE	},
	{ "NEGATE",			SSC_PRIMITIVE	},

	{ "<",				SSC_PRIMITIVE	},
	{ "<=",				SSC_PRIMITIVE	},
	{ "<>",				SSC_PRIMITIVE	},
	{ "=",				SSC_PRIMITIVE	},
	{ ">",				SSC_PRIMITIVE	},
	{ ">=",				SSC_PRIMITIVE	},
	{ "0<",				SSC_PRIMITIVE	},
	{ "0=",				SSC_PRIMITIVE	},
	{ "0>",				SSC_PRIMITIVE	},

	{ "AND",			SSC_PRIMITIVE	},
	{ "OR",				SSC_PRIMITIVE	},
	{ "XOR",			SSC_PRIMITIVE	},
	{ "NOT",			SSC_PRIMITIVE	},

	{ "!",				SSC_PRIMITIVE	},
	{ "@",				SSC_PRIMITIVE	},
	{ ",",				SSC_PRIMITIVE	},

	{ "F+",				SSC_PRIMITIVE	},
	{ "F-",				SSC_PRIMITIVE	},
	{ "F*",				SSC_PRIMITIVE	},
	{ "F/",				SSC_PRIMITIVE	},
	{ "F<",				SSC_PRIMITIVE	},
	{ "F<=",			SSC_PRIMITIVE	},
	{ "F<>",			SSC_PRIMITIVE	},
	{ "F=",				SSC_PRIMITIVE	},
	{ "F>",				SSC_PRIMITIVE	},
	{ "F>=",			SSC_PRIMITIVE	},
	{ "FABS",			SSC_PRIMITIVE	},
	{ "ACOS",			SSC_PRIMITIVE	},
	{ "ASIN",			SSC_PRIMITIVE	},
	{ "ATAN",			SSC_PRIMITIVE	},
	{ "ATAN2",			SSC_PRIMITIVE	},
	{ "COS",			SSC_PRIMITIVE	},
	{ "EXP",			SSC_PRIMITIVE	},
	{ "FLOAT",			SSC_PRIMITIVE	},
	{ "LOG",			SSC_PRIMITIVE	},
	{ "LOG10",			SSC_PRIMITIVE	},
	{ "FMAX",			SSC_PRIMITIVE	},
	{ "FMIN",			SSC_PRIMITIVE	},
	{ "FNEGATE",		SSC_PRIMITIVE	},
	{ "POW",			SSC_PRIMITIVE	},
	{ "SIN",			SSC_PRIMITIVE	},
	{ "SQRT",			SSC_PRIMITIVE	},
	{ "TAN",			SSC_PRIMITIVE	},
};

static unsigned int hash_bucket(unsigned char * name)
{
	unsigned int  h;

	for (h = 0; *name; h += *name++);
	return h  % HASH_SIZE;
}

// ****************************************************************

static int  fromStyled, endStyled, currStyled, FontSize ;

static void skip_blank(void) {
	int cc ;

	for (currStyled = 0 ; fromStyled <= endStyled ; currStyled ++) {
		cc = SendMessage(hWndEdit, SCI_GETCHARAT, fromStyled, 0) ;
        if (cc == '\n' || cc == '\r' || cc == '\t' || cc == ' ')  {
			fromStyled ++ ;
        }
        else {
            break ;
        }
	}
}

static int scan_word(void) {
	char	buf[WORD_NAME_LENGTH] ;
	int		cc, h ;
	KeyWord	* kpc;
	
	cc = SendMessage(hWndEdit, SCI_GETCHARAT, fromStyled, 0) ;
	if (cc == '\"') return SSC_STRING_QUOTE;
	if (cc == '\'') return SSC_STRING_TICK;
	for (currStyled = 0 ; fromStyled <= endStyled ; currStyled ++) {
		cc = SendMessage(hWndEdit, SCI_GETCHARAT, fromStyled, 0) ;
		if (cc == '\n' || cc == '\r' || cc == '\t' || cc == ' ')  {
			break ;
        }
		fromStyled ++ ;
        if (currStyled < WORD_NAME_LENGTH) {
			buf[currStyled] = (cc >= 'a' && cc <= 'z') ? ('A' - 'a' + cc ) : cc ;
        }
	}	

    if (currStyled >= WORD_NAME_LENGTH) {
		return SSC_IDENTIFIER ;
    }

	buf[currStyled] = 0;
	h = hash_bucket(buf);
	kpc = bucket[h];
	while (kpc != NULL) {
		if (strcmp(kpc->name, buf) == 0)
			return kpc->attr;
		kpc = kpc->next;
	}
	if ((buf[0] >= '0' && buf[0] <= '9') ||
        (buf[0] == '0' && (buf[1] == 'x' || buf[1] == 'X'))) {
		return SSC_NUMBER ;
    }

	return SSC_IDENTIFIER ;
}

static void scan_to_eol(void) {
	int cc ;

	while (fromStyled <= endStyled) {
		cc = SendMessage(hWndEdit, SCI_GETCHARAT, fromStyled, 0) ;
        if (cc == '\n' || cc == '\r') {
			break ;
        }
		fromStyled ++ ;
		currStyled ++ ;
	}	
}

static void scan_to_block(void) {
	int cc ;

	while (fromStyled <= endStyled) {
		cc = SendMessage(hWndEdit, SCI_GETCHARAT, fromStyled, 0) ;
		fromStyled ++ ;
		currStyled ++ ;
        if (cc == ')') {
            break ;
        }
	}	
}

static void scan_string(int end_char) {
	int cc ;

	currStyled = 1;
	fromStyled ++;
	while (fromStyled <= endStyled) {
		cc = SendMessage(hWndEdit, SCI_GETCHARAT, fromStyled, 0) ;
        if (cc == '\n' || cc == '\r') {
			break ;
        }
		fromStyled ++ ;
		currStyled ++ ;
        if (cc == end_char) {
            break ;
        }
	}	
}

// ****************************************************************

static void set_style(int style, int fore, int back, char * font, int size, int bold) {
	SendMessage(hWndEdit, SCI_STYLESETFORE, style, fore) ;
	SendMessage(hWndEdit, SCI_STYLESETBACK, style, back) ;
	SendMessage(hWndEdit, SCI_STYLESETSIZE, style, size) ;
	SendMessage(hWndEdit, SCI_STYLESETFONT, style, (LPARAM) font) ;
	SendMessage(hWndEdit, SCI_STYLESETBOLD, style, bold) ;
}

void Edit_syntax(struct SCNotification * notify) {

	fromStyled	= SendMessage(hWndEdit, SCI_GETENDSTYLED, 0, 0) ;
	endStyled	= SendMessage(hWndEdit, SCI_LINEFROMPOSITION, fromStyled, 0) ;
	fromStyled	= SendMessage(hWndEdit, SCI_POSITIONFROMLINE, endStyled, 0) ;
	endStyled   = notify->position ;

	SendMessage(hWndEdit, SCI_STARTSTYLING, fromStyled, 0x1f);
	while (fromStyled <= endStyled) {
		skip_blank() ;
		SendMessage(hWndEdit, SCI_SETSTYLING, currStyled, SCE_DEFAULT) ;
		switch(scan_word()) {
			case SSC_LINE_BLOCK:
				break ;

			case SSC_DEFINE_WORD:
				SendMessage(hWndEdit, SCI_SETSTYLING, currStyled, SCE_PRIMITIVE) ;
				skip_blank() ;
				SendMessage(hWndEdit, SCI_SETSTYLING, currStyled, SCE_DEFAULT) ;
				scan_word() ;
				SendMessage(hWndEdit, SCI_SETSTYLING, currStyled, SCE_DEFINED_WORD) ;
				break ;

			case SSC_REMARK_EOL:
				scan_to_eol() ;	
				SendMessage(hWndEdit, SCI_SETSTYLING, currStyled, SCE_COMMENT) ;
				break ;	

			case SSC_REMARK_BLOCK:
				scan_to_block() ;	
				SendMessage(hWndEdit, SCI_SETSTYLING, currStyled, SCE_COMMENT) ;
				break ;	

			case SSC_IDENTIFIER:
				SendMessage(hWndEdit, SCI_SETSTYLING, currStyled, SCE_IDENTIFIER) ;
				break ;

			case SSC_PRIMITIVE:
				SendMessage(hWndEdit, SCI_SETSTYLING, currStyled, SCE_PRIMITIVE) ;
				break ;

			case SSC_IMMEDIATE:
				SendMessage(hWndEdit, SCI_SETSTYLING, currStyled, SCE_COMPILE) ;
				break ;

			case SSC_STRING_QUOTE:
				scan_string('\"') ;	
				SendMessage(hWndEdit, SCI_SETSTYLING, currStyled, SCE_STRING) ;
				break ;			

			case SSC_STRING_TICK:
				scan_string('\'') ;	
				SendMessage(hWndEdit, SCI_SETSTYLING, currStyled, SCE_STRING) ;
				break ;			

			case SSC_NUMBER:
				SendMessage(hWndEdit, SCI_SETSTYLING, currStyled, SCE_NUMBER) ;
				break ;
		}
	}	
}

void Edit_init(void) {
	int i, h, font_size, back;

	for (i = 0; i < HASH_SIZE; i++)
		bucket[i] = NULL;
	for (i = 0; key_table[i].name != NULL; i++) {
		h = hash_bucket(key_table[i].name);
		key_table[i].next = bucket[h];
		bucket[h] = &key_table[i];
	}

	SendMessage(hWndEdit, SCI_SETCODEPAGE, 936, 0) ;
	SendMessage(hWndEdit, SCI_SETTABWIDTH, 4, 0)  ;
	SendMessage(hWndEdit, SCI_SETINDENT,   4, 0) ;
	SendMessage(hWndEdit, SCI_SETINDENTATIONGUIDES, TRUE, 0) ;
	SendMessage(hWndEdit, SCI_STYLESETFORE, STYLE_INDENTGUIDE, RGB(0xff, 0xca, 0xff));
	SendMessage(hWndEdit, SCI_STYLESETBACK, STYLE_INDENTGUIDE, RGB(0xff, 0xff, 0xff)) ;
	SendMessage(hWndEdit, SCI_SETCARETLINEBACK, RGB(0xFD, 0xF5, 0xE6), 0) ;
	SendMessage(hWndEdit, SCI_SETCARETLINEVISIBLE, TRUE, 0) ;

	font_size = 14;
	SendMessage(hWndEdit, SCI_SETLEXER,	SCLEX_CONTAINER, 0);
	set_style(STYLE_DEFAULT,	0x00, RGB(0xff, 0xff, 0xff), "Consolas", font_size, FALSE);
	SendMessage(hWndEdit, SCI_STYLECLEARALL, 0, 0);

	set_style(STYLE_DEFAULT,	0x00, RGB(0xff, 0xff, 0xff), "Consolas", font_size, FALSE) ;
	SendMessage(hWndEdit, SCI_STYLECLEARALL, 0, 0);

	back = RGB(0xff, 0xff, 0xff);
	set_style(SCE_DEFINED_WORD,	RGB(0xff, 0x00, 0x00),	back, "Consolas",		font_size, TRUE)  ;
	set_style(SCE_COMMENT,		RGB(0x00, 0x80, 0x00), 	back, "Consolas",		font_size, FALSE) ;	
	set_style(SCE_IDENTIFIER,	RGB(0x00, 0x00, 0x80),	back, "Consolas",		font_size, FALSE) ;
	set_style(SCE_PRIMITIVE,	RGB(0x80, 0x00, 0x00),	back, "Consolas",		font_size, TRUE)  ;
	set_style(SCE_STRING,		RGB(0xff, 0x00, 0xff),	back, "Consolas",		font_size, FALSE) ;
	set_style(SCE_NUMBER,		RGB(0x00, 0x80, 0x80),	back, "Consolas",		font_size, FALSE) ;
	set_style(SCE_COMPILE,		RGB(0x00, 0x00, 0xff),	back, "Consolas",		font_size, TRUE)  ;
	
	back = RGB(0xF0, 0xFF, 0xF0);
	set_style(SCE_ANNOTATION_NOTE,	RGB(0x00, 0x00, 0xFF),	back, "Arial",		font_size, FALSE);
	set_style(SCE_ANNOTATION_ERROR,	RGB(0xFF, 0x00, 0x00),	back, "Arial Black",font_size, FALSE);

	SendMessage(hWndEdit, SCI_ANNOTATIONSETVISIBLE, ANNOTATION_BOXED, 0);

	SendMessage(hWndEdit, SCI_SETMARGINTYPEN, 0, SC_MARGIN_NUMBER ) ;
	SendMessage(hWndEdit, SCI_SETMARGINWIDTHN, 0, 
			SendMessage(hWndEdit, SCI_TEXTWIDTH, STYLE_LINENUMBER, (LPARAM) "_9999")) ;

	SendMessage(hWndEdit, SCI_SETMODEVENTMASK, SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT, 0);
	
}
