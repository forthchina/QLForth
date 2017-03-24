/* ************************************************************************************
* Copyright (C) 2015-2017 Shaanxi QinWei Electroics Technologies. All rights reserved.
*
* $Date:        24. March 2017
* $Revision: 	V 0.1
*
* Project: 	    QLForth - A Forth for System-On-Chip Creative Makers
*
* Title:	    Talk.c
*
* Description:	The user talk windows for QLForth
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
#include <winuser.h>
#include <string.h>
#include <process.h> 
#include <stdio.h>
#include <setupapi.h>
#include <dbt.h>

#include "ide.h"

#include "QLFORTH.h"

#define STRING_SIZE			128
#define KEYPAD_START_X		60

static LOGFONT LogFontPack = {
	-18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
	OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
	DEFAULT_PITCH | FF_DONTCARE, TEXT("Consolas")
} ;

static LOGFONT LogFontText = {
	-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
	OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
	DEFAULT_PITCH | FF_DONTCARE, TEXT("Arial")
} ;

static LOGFONT LogFontSmall = {
	-10, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
	OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
	DEFAULT_PITCH | FF_DONTCARE, TEXT("Arial")
} ;

struct _part_structure { 
	RECT	rect;
	COLORREF fg;
	int		progress, slen;
	UINT	fmt;
	TCHAR	str[STRING_SIZE];
} ;

static struct _part_structure sb_mode, sb_prompt;
static HFONT hFontKey, hFontText, hFontSmall;
static HBRUSH hbrush;

static HWND hWndMe;
static TCHAR szAppName[] = TEXT ("_TalkClass");
static int  got_focus, cxChar, cyChar, Caret_X, Caret_Y, keypad_Y, key_count, 
			stack_max, stack_tos, stack_nos, stack_X1, stack_X2, stack_X3, keypad_mode;
static char keypad_buffer[STRING_SIZE + 1], keypad_history[STRING_SIZE + 1], stack_FMT[4];

static void draw_parts(HDC hdc, struct _part_structure * part) {
	RECT rc;

	if (part->progress != 0) {
		rc.top		= part->rect.top;
		rc.bottom	= part->rect.bottom;
		rc.left		= part->rect.left;
		rc.right	= part->rect.left + part->progress;
		FillRect(hdc, &rc, CreateSolidBrush(RGB(0, 255, 0)));
	}

	SetTextColor(hdc, part->fg);
	SelectObject(hdc, hFontText);
	if (part->slen != 0) {
		DrawText(hdc, part->str, part->slen, & (part->rect), part->fmt);
	}
}

static void draw_stack(HDC hdc, char * item, int x_pos, int ival) {
	int size;
	char buf[64];
	QLF_LITERAL data;

	SetTextColor(hdc, RGB(255, 0, 0));
	SelectObject(hdc, hFontSmall);
	TextOut (hdc, x_pos, 4, item, 3);
	data.ival = ival;
	size = 10;
	switch(stack_FMT[0]) {
		case 'D':	size = sprintf(buf, "%d",		data.ival);		break;
		case 'H':	size = sprintf(buf, "0x%08X",	data.ival);		break;
		case 'F':	size = sprintf(buf, "%6.2f",	data.fval);		break;
	} 	

	SetTextColor (hdc, RGB(0, 0, 0));
	SelectObject(hdc, hFontKey);
	TextOut (hdc, x_pos + 26, 4, buf, (size > 10) ? 10 : size);
}

static void display_stack(HDC hdc) {
	int size;
	char buf[32];

	if (stack_max == 0) {
		return;
	}
	if (stack_max == 1) {
		draw_stack(hdc, "TOS", stack_X2, stack_tos);
	}
	else {
		draw_stack(hdc, "TOS", stack_X1, stack_tos);
		draw_stack(hdc, "NOS", stack_X2, stack_nos);
	}

	SetTextColor(hdc, RGB(0x00, 0, 0xFF));
	SelectObject(hdc, hFontSmall);

	TextOut (hdc, stack_X3, 4, stack_FMT, 3);
	if (stack_max > 2) {
		size = sprintf(buf, "%02d", stack_max);	
		TextOut (hdc, stack_X3, 20, buf, 2);		// only output 2 digitals
	}
}
            
static void draw_window(HDC hdc, RECT * rc) {
	SetBkMode (hdc, TRANSPARENT);
	draw_parts(hdc, & sb_prompt);

	if (! got_focus) {
		draw_parts(hdc, & sb_mode);
	}
	
	SelectObject(hdc, hFontKey) ;
	SetTextColor(hdc, RGB(0x00, 0x00, 0xFF));
	TextOut (hdc, KEYPAD_START_X, keypad_Y, keypad_buffer, key_count);

	display_stack(hdc);
	SetTextColor (hdc, RGB(0xFF, 0, 0));

	SelectObject(hdc, hFontSmall);
	SetTextColor (hdc, RGB(0xFF, 0, 0x00));
	TextOut (hdc, 4, 4,	 keypad_mode ?	"QLForth>>" : " QLForth ", 9);
	SetTextColor (hdc, RGB(0x00, 0, 0xFF));
	TextOut (hdc, 4, 16, "Interpreter", 11);
}

static void draw_screen(HWND hwnd) {
	PAINTSTRUCT ps;
    HDC         hdc;
    HDC         hdc_virtual		= NULL;
	HBITMAP     hbmp_virtual	= NULL;
    HGDIOBJ     hobj_old		= NULL;
    RECT        rc;
	
    hdc = BeginPaint(hwnd, &ps);

    GetClientRect(hwnd, &rc);
    hdc_virtual  = CreateCompatibleDC(hdc);
	if ( hdc_virtual == NULL )
		draw_window(hdc, & rc);
	else {
		hbmp_virtual = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
		if (hbmp_virtual == NULL)
			draw_window(hdc, & rc);
		else {
			hobj_old = SelectObject(hdc_virtual, hbmp_virtual);
			// hbrush = CreateSolidBrush( RGB(0xF0, 0xF0, 0xF0) );
			FillRect(hdc_virtual, &rc, hbrush);
			draw_window(hdc_virtual, &rc);
			BitBlt(hdc, 0, 0, rc.right, rc.bottom, hdc_virtual, 0, 0, SRCCOPY);
		//	DeleteObject(hbrush);
			SelectObject(hdc_virtual, hobj_old);
			DeleteObject(hbmp_virtual) ;
		}
		DeleteObject(hdc_virtual);
	}
    EndPaint(hwnd, &ps);
}

static void keypad_new(void) {
	Caret_X		= KEYPAD_START_X;
	Caret_Y		= 4;
	key_count	= 0;
	SetCaretPos(Caret_X, Caret_Y);
}

static void create_me(void) {
	HDC hdc ;
	TEXTMETRIC tm ;

	hFontKey	= CreateFontIndirect(& LogFontPack) ;
	hFontText	= CreateFontIndirect(& LogFontText) ;
	hFontSmall  = CreateFontIndirect(& LogFontSmall) ;

	hdc = GetDC (hWndMe);
		SelectObject(hdc, hFontKey);
        GetTextMetrics (hdc, &tm);
        cxChar = tm.tmAveCharWidth;
        cyChar = tm.tmHeight;
    ReleaseDC(hWndMe, hdc);

	sb_prompt.fmt = DT_LEFT | DT_VCENTER;

	keypad_Y = 4;

	keypad_new();
	stack_max = 0;
	strcpy(stack_FMT, "DEC");
}

static void size_me(void) {
	RECT rect ;
	int  UserWidth, UserHeight, x, x1, x2, y;
		 
	GetClientRect(hWndMe, &rect);
	UserWidth	= rect.right - rect.left;
	UserHeight  = rect.bottom - rect.top;
	x1 = UserWidth * 7 / 10;
	x  = cxChar * 12;
	x2 = x1 + (UserWidth - x1 - x);

	y = UserHeight - cyChar - 4;

	sb_prompt.rect.top		= y;
	sb_prompt.rect.bottom	= rect.bottom;
	sb_prompt.rect.left		= 4 ;
	sb_prompt.rect.right	= x1 - 2;
	
	sb_mode.rect.top		= y;
	sb_mode.rect.bottom		= rect.bottom;
	sb_mode.rect.left		= x2 + 2;
	sb_mode.rect.right		= rect.right;

	stack_X1 = rect.right - cxChar * 32; 
	stack_X2 = rect.right - cxChar * 18;
	stack_X3 = rect.right - cxChar * 4;
}

static void set_text(int part, char * str) {
	struct _part_structure * ptr;

	if (part == QL_PART_MODE)			ptr = & sb_mode;
	else if (part == QL_PART_PROMPT)	ptr = & sb_prompt;
	ptr->slen = _snprintf(ptr->str, STRING_SIZE, "%s", str);
}

static void set_color(int part, COLORREF clr) {
	struct _part_structure * ptr;

	if (part == QL_PART_MODE)			ptr = & sb_mode;
	else if (part == QL_PART_PROMPT)	ptr = & sb_prompt;
	ptr->fg = clr;
}

static void set_progress(int part, int percent) {
	struct _part_structure * ptr;

	if (part == QL_PART_MODE)			ptr = & sb_mode;
	else if (part == QL_PART_PROMPT)	ptr = & sb_prompt;
	ptr->progress = (ptr->rect.right - ptr->rect.left) * percent / 100;
}

static void keypad_prev(void) {
	strcpy(keypad_buffer, keypad_history);
	key_count = strlen(keypad_buffer);
	Caret_X		= KEYPAD_START_X + cxChar * key_count;
	SetCaretPos(Caret_X, Caret_Y);
}

static void keypad_execute(void) {
	NMHDR nmhdr;

	keypad_buffer[key_count] = 0;

	nmhdr.hwndFrom = hWndMe;
	nmhdr.idFrom   = ID_TALK;
	nmhdr.code	   = QLN_TALK_EXECUTE;
	SendMessage(hWndMain, WM_NOTIFY, (WPARAM) keypad_buffer, (LPARAM) & nmhdr);

	key_count = 0;
	strcpy(keypad_history, keypad_buffer);
	keypad_new();
}

static void keypad_char(int cc, int num) {
	int idx;

	for (idx = 0 ; idx < num ; idx++) {
		switch (cc) {
			case '\b':	 // backspace
				if (key_count == 0) keypad_prev();
				else {
					key_count --;
					keypad_buffer[key_count] = 0;
					Caret_X -=  cxChar;
				} 
				break ;

			case '\t':   // tab
				keypad_mode = (keypad_mode == 0) ? 1 : 0;
				break ;
				
		    case '\n':   // line feed
		    case '\r':   // carriage return
				if (key_count == 0) keypad_prev();
				else {
					keypad_execute();
				}
				break ;

		    case '\x1B': // escape
				keypad_new();
				break ;

			default:
				if (cc == ' ' && keypad_mode == 0 && key_count > 0) {
					keypad_execute();
					return ;
				}
				if (key_count < STRING_SIZE) {
					keypad_buffer[key_count ++] = cc;
					Caret_X +=  cxChar;
				}
				break ;	
       }
	}
}

static void keypad_down(int key) {
	if (key == VK_UP || key == VK_DOWN) {
		if	(stack_FMT[0] == 'D')		strcpy(stack_FMT, "HEX");	
		else if (stack_FMT[0] == 'H')	strcpy(stack_FMT, "FP ");	 
		else							strcpy(stack_FMT, "DEC");	 
	}
}

static void get_focus(void) {
	NMHDR nmhdr;

	nmhdr.hwndFrom = hWndMe;
	nmhdr.idFrom   = ID_TALK;
	nmhdr.code	   = QLN_TALK_GET_FOCUS;
	SendMessage(hWndMain, WM_NOTIFY, 0, (LPARAM) & nmhdr);
}

static LRESULT CALLBACK ThisWndProc(HWND hwnd,UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
		case WM_CREATE:		hWndMe = hwnd;		create_me();		break ;
		case WM_SIZE:		size_me();								break ;
    	case WM_ERASEBKGND:	break ;									// For Compatible DC,  is needed
 		case WM_PAINT:		draw_screen(hwnd) ;						break ;
		case WM_DESTROY:	PostQuitMessage(0);			            break ;

// ......................................................

		case WM_SETFOCUS:			// create and show the caret
			CreateCaret (hwnd, NULL, cxChar, cyChar);
            SetCaretPos(Caret_X, Caret_Y);
			ShowCaret (hwnd);
			got_focus = 1;
            break ;
        
		case WM_KILLFOCUS:			// hide and destroy the caret
            HideCaret(hwnd);
            DestroyCaret ();
			got_focus = 0;
			break;

		case WM_LBUTTONDOWN:
			get_focus();
			break;

		case WM_KEYDOWN:
			keypad_down(wParam);
			InvalidateRect(hwnd, NULL, FALSE);
			break ;

 		case WM_CHAR:
			keypad_char(wParam, LOWORD (lParam));
			SetCaretPos(Caret_X, Caret_Y);
			InvalidateRect (hwnd, NULL, FALSE);
			break;

// ......................................................

		case QL_SET_TEXT:		set_text(wParam, (char *) lParam);					break;
		case QL_CLEAR_ALL:		sb_mode.slen = sb_prompt.slen = 0;					break;
		case QL_SET_COLOR:		set_color(wParam, (COLORREF) lParam);				break;
		case QL_SET_PROGRESS:	set_progress(wParam, lParam);						break;
		case QL_SET_MAX:		stack_max = wParam;									break;	
		case QL_SET_TOS_NOS:	stack_tos = wParam ; stack_nos = lParam;			break;
		case QL_UPDATE:			InvalidateRect(hWndMe, NULL, FALSE) ;				break;
		case QL_GET_CHAR_HEIGHT:return cyChar;
		
		default:				return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

// ************************************************************************************

int QLForth_display_stack(int base, int depth, int data1, int data2) {
	SendMessage(hWndTalk, QL_SET_MAX, depth, 0);
	SendMessage(hWndTalk, QL_SET_TOS_NOS, data1, data2);
	SendMessage(hWndTalk, QL_UPDATE, 0, 0);
	return 0;
}

HWND Talk_window_init(int id) {
	WNDCLASS	wc;

	memset(&wc, 0, sizeof(WNDCLASS));
	hbrush = CreateSolidBrush(RGB(0xF0, 0xF0, 0xF0));
	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wc.lpfnWndProc		= (WNDPROC) ThisWndProc;
	wc.hInstance		= hInst;
	wc.hbrBackground	= hbrush;
	wc.lpszClassName	= szAppName;
	wc.lpszMenuName		= NULL;
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
	wc.hIcon			= NULL;
	if ( ! RegisterClass (&wc)) {
		MessageBox(NULL, TEXT ("Creating Profile Window"), szAppName, MB_ICONERROR);
        return 0 ;
    }
	hWndMe = CreateWindowEx(WS_EX_CLIENTEDGE, szAppName, NULL,
					WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, 
					hWndMain, (HMENU) id, hInst, NULL);

	keypad_mode = 0;
	return hWndMe;
}
