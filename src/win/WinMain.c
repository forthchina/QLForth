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
* Description:	The main entry QLForth
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


#define WINVER 0x502

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

#include "hidsdi.h"
#include "Scintilla.h"
#include "SciLexer.h"
#include "ide.h"

#include "QLFORTH.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "setupapi.lib") 
#pragma comment(lib, "lib/hid.lib") 

static char * Version = "QLForth V1.0" ;

HINSTANCE hInst;
HWND hWndMain, hWndEdit, hWndTalk, hWndConsole, hDlgModeless;

static int		view_mode, compile_source_flag, cap_flag;
static TCHAR	szAppName[] = TEXT ("QLForth"), curr_file_name[MAX_PATH];
static UINT		messageFindReplace ;

// ************************************************************************************

static void new_file_buffer_for_edit(void) {
	char title[MAX_PATH * 2];
	
	SendMessage(hWndEdit, SCI_CANCEL, 0, 0);
	SendMessage(hWndEdit, SCI_SETUNDOCOLLECTION, 0, 0);
	SendMessage(hWndEdit, SCI_CLEARALL, 0, 0);
	SendMessage(hWndEdit, EM_EMPTYUNDOBUFFER, 0, 0);
	SendMessage(hWndEdit, SCI_CANCEL, 0, 0);
	SendMessage(hWndEdit, SCI_SETUNDOCOLLECTION, 1, 0);
	SendMessage(hWndEdit, SCI_SETSAVEPOINT, 0, 0);
	SendMessage(hWndEdit, SCI_GOTOPOS, 0, 0);
	
	sprintf(title, "QLForth V1.0 - %s\n", curr_file_name);
	SetWindowText(hWndMain, title);
}

static void ql_file_save(void) {
	FILE	* fd;
	char	* text, msg[1024];
	int		size;

	if (SendMessage(hWndEdit, SCI_GETMODIFY, 0, 0)) {
		if ((fd = fopen(curr_file_name, "wb")) == NULL) {
			sprintf(msg, "File %s open error.", curr_file_name);
			MessageBox(hWndMain, msg, NULL, MB_YESNOCANCEL);
			return;
		}

		size = SendMessage(hWndEdit, SCI_GETLENGTH, 0, 0);
		text = (char *) malloc(size + 8);
		if (text == NULL) {
			printf(msg, "Out of memory.", curr_file_name);
			MessageBox(hWndMain, msg, NULL, MB_YESNOCANCEL);
		}
		SendMessage(hWndEdit, SCI_GETTEXT, size + 1, (LPARAM)text);
		SendMessage(hWndEdit, SCI_SETSAVEPOINT, 0, 0);
		fwrite(text, 1, size, fd);
		fclose(fd);
	}
}

static void ql_file_open(int use_curr_file) {
	const			char * filter = "QLForth Source File (.qlf)\0*.qlf\0All files(*.*)\0*.*\0\0";
	FILE 			* fd;
	char 			* buf, openName[MAX_PATH] = "\0";
	int				size;
	OPENFILENAME ofn = { sizeof(OPENFILENAME) };

	if (SendMessage(hWndEdit, SCI_GETMODIFY, 0, 0)) {
		ql_file_save();
	}

	if (! use_curr_file) {
		ofn.hwndOwner = hWndMain;
		ofn.hInstance = hInst;
		ofn.lpstrFile = openName;
		ofn.nMaxFile = sizeof(openName);

		ofn.lpstrFilter = filter;
		ofn.lpstrCustomFilter = 0;
		ofn.nMaxCustFilter = 0;
		ofn.nFilterIndex = 0;
		ofn.lpstrTitle = "Open File";
		ofn.Flags = OFN_HIDEREADONLY;
		if (!GetOpenFileName(&ofn)) {
			return;
		}
		strcpy(curr_file_name, openName);
	}

	new_file_buffer_for_edit();
	if ((fd = fopen(curr_file_name, "rb")) == NULL) {
		return;
	}

	SendMessage(hWndEdit, SCI_CLEARALL, 0, 0);
	fseek(fd, 0L, SEEK_END);
	size = ftell(fd);
	buf = malloc(size + 16);
	if (buf != NULL) {
		fseek(fd, 0L, SEEK_SET);
		size = fread(buf, 1, size, fd);
		SendMessage(hWndEdit, SCI_ADDTEXT, size, (LPARAM)buf);
		SendMessage(hWndEdit, SCI_COLOURISE, 0, (LPARAM)-1);
		free(buf);
	}

	fclose(fd);
	SendMessage(hWndEdit, SCI_SETUNDOCOLLECTION, 1, 0);
	SendMessage(hWndEdit, EM_EMPTYUNDOBUFFER, 0, 0);
	SendMessage(hWndEdit, SCI_SETSAVEPOINT, 0, 0);
	SendMessage(hWndEdit, SCI_GOTOPOS, 0, 0);
}

static void ql_file_init(void) {
	strcpy(curr_file_name, "QLForth.qlf");
	ql_file_open(1);
}

static void app_start(void) {
	char path[MAX_PATH], * pc;

	GetModuleFileName(hInst, path, MAX_PATH);
	for (pc = path + strlen(path); pc > path && * pc != '\\' ; pc --);
	if (* pc == '\\') pc --;
	for (; pc > path && * pc != '\\' ; pc --);
	if (* pc == '\\') * pc = 0;
	QLForth_init(path);
}

// ************************************************************************************

static void display_editor_infomation(void) {
	char	szBuf[32] ;
	int		pos ;

	pos = SendMessage(hWndEdit, SCI_GETCURRENTPOS, 0, 0) ;
	sprintf(szBuf, " %d:%d",	SendMessage(hWndEdit, SCI_LINEFROMPOSITION, pos, 0) + 1, 
								SendMessage(hWndEdit, SCI_GETCOLUMN, pos, 0) + 1) ;
	SendMessage(hWndTalk, QL_SET_COLOR,		QL_PART_MODE,	RGB(0x00, 0x00, 0xFF));
	SendMessage(hWndTalk, QL_SET_TEXT,		QL_PART_MODE, (LPARAM) szBuf);
	SendMessage(hWndTalk, QL_UPDATE,		0, 0);
}

static TCHAR szFindText[TEXT_LINE_SIZE], szReplText[TEXT_LINE_SIZE];
static int FindPos;

BOOL edit_find_text(LPFINDREPLACE pfr) {
	int			uflag;
	FINDTEXTEX	findtext;

	uflag = 0;
	findtext.lpstrText = szFindText;
	if (pfr->Flags & FR_DOWN) {
		findtext.chrg.cpMin = FindPos;
		findtext.chrg.cpMax = SendMessage(hWndEdit, SCI_GETTEXTLENGTH, 0, 0);
	}
	else {
		findtext.chrg.cpMin = (FindPos == 0) ?
			SendMessage(hWndEdit, SCI_GETTEXTLENGTH, 0, 0) : FindPos;
		findtext.chrg.cpMax = 0;
	}
	if (pfr->Flags & FR_MATCHCASE) {
		uflag |= SCFIND_MATCHCASE;
	}
	if (pfr->Flags & FR_WHOLEWORD) {
		uflag |= SCFIND_WHOLEWORD;
	}
	FindPos = SendMessage(hWndEdit, SCI_FINDTEXT, uflag, (LPARAM)&findtext);
	if (FindPos == -1) {
		FindPos = 0;
		return FALSE;
	}
	uflag = strlen(szFindText);
	SendMessage(hWndEdit, SCI_SETSEL, FindPos, FindPos + uflag);
	if (pfr->Flags & FR_DOWN) FindPos += uflag;
	return TRUE;
}

static int edit_replace_text(LPFINDREPLACE pfr) {
	int			uflag;
	FINDTEXTEX	findtext;

	findtext.chrg.cpMin = 0;
	findtext.chrg.cpMax = SendMessage(hWndEdit, SCI_GETTEXTLENGTH, 0, 0);
	findtext.lpstrText = szFindText;
	uflag = 0;
	if (pfr->Flags & FR_MATCHCASE) uflag |= FR_MATCHCASE;
	if (pfr->Flags & FR_WHOLEWORD) uflag |= FR_WHOLEWORD;
	FindPos = SendMessage(hWndEdit, SCI_FINDTEXT, uflag, (LPARAM)&findtext);
	if (FindPos == -1) return FALSE;
	uflag = strlen(szFindText);
	SendMessage(hWndEdit, SCI_SETSEL, FindPos, FindPos + uflag);
	SendMessage(hWndEdit, SCI_REPLACESEL, 0, (LPARAM)szReplText);
	return TRUE;
}

// ****************************************************************

void Edit_find(void) {
	static FINDREPLACE fr;		// must be static for modeless dialog !!!

	fr.lStructSize = sizeof(FINDREPLACE);
	fr.hwndOwner = hWndMain;
	fr.hInstance = NULL;
	fr.Flags = FR_DOWN;
	fr.lpstrFindWhat = szFindText;
	fr.lpstrReplaceWith = NULL;
	fr.wFindWhatLen = TEXT_LINE_SIZE;
	fr.wReplaceWithLen = 0;
	fr.lCustData = 0;
	fr.lpfnHook = NULL;
	fr.lpTemplateName = NULL;
	FindPos = 0;
	hDlgModeless = FindText(&fr);
}

void Edit_find_next(void) {
	FINDREPLACE fr;

	if (szFindText[0] == 0) {
		Edit_find();
	}
	else {
		fr.lpstrFindWhat = szFindText;
		fr.Flags |= FR_DOWN;
		edit_find_text(&fr);
	}
}

void Edit_replace(void) {
	static FINDREPLACE fr;		// must be static for modeless dialog !!!

	fr.lStructSize = sizeof(FINDREPLACE);
	fr.hwndOwner = hWndMain;
	fr.hInstance = NULL;
	fr.Flags = FR_DOWN;
	fr.lpstrFindWhat = szFindText;
	fr.lpstrReplaceWith = szReplText;
	fr.wFindWhatLen = TEXT_LINE_SIZE;
	fr.wReplaceWithLen = TEXT_LINE_SIZE;
	fr.lCustData = 0;
	fr.lpfnHook = NULL;
	fr.lpTemplateName = NULL;
	FindPos = 0;
	hDlgModeless = ReplaceText(&fr);
}

void Edit_search(LPFINDREPLACE	pfr) {
	if (pfr->Flags & FR_DIALOGTERM) hDlgModeless = NULL;
	if (pfr->Flags & FR_FINDNEXT) {
		if (!edit_find_text(pfr)) {
			// Prompt_Message("No Matched for \'%s\'", pfr -> lpstrFindWhat) ;
		}
	}
	if (pfr->Flags & FR_REPLACE || pfr->Flags & FR_REPLACEALL) {
		if (!edit_replace_text(pfr)) {
			// Prompt_Message("No Matched for \'%s\'", pfr -> lpstrFindWhat) ;
		}
	}
	if (pfr->Flags & FR_REPLACEALL) {
		while (edit_replace_text(pfr));
	}
}

// ************************************************************************************

void Console_init(void) {
	SendMessage(hWndConsole, SCI_STYLESETFORE, STYLE_DEFAULT, 0);
	SendMessage(hWndConsole, SCI_STYLESETBACK, STYLE_DEFAULT, RGB(0xff, 0xff, 0xff));
	SendMessage(hWndConsole, SCI_STYLESETSIZE, STYLE_DEFAULT, 12);
	SendMessage(hWndConsole, SCI_STYLESETFONT, STYLE_DEFAULT, (LPARAM) "Courier New");
	SendMessage(hWndConsole, SCI_STYLESETBOLD, STYLE_DEFAULT, FALSE);
	SendMessage(hWndConsole, SCI_STYLECLEARALL, 0, 0);
	SendMessage(hWndConsole, SCI_SETCODEPAGE, 936, 0);
	SendMessage(hWndConsole, SCI_SETREADONLY, TRUE, 0);
}

// ************************************************************************************

static void ql_build(void) {
	int		size;
	char	* text;

	if (SendMessage(hWndEdit, SCI_GETMODIFY, 0, 0)) {
		if (view_mode == 0) {				// this is project working mode
			ql_file_save();
		}
		SendMessage(hWndEdit, SCI_ANNOTATIONCLEARALL, 0, 0);
		size = SendMessage(hWndEdit, SCI_GETLENGTH, 0, 0);
		if (size != 0) {
			text = (char *)malloc(size + 8);
			if (text == NULL) {
				return;
			}
			SendMessage(hWndEdit, SCI_GETTEXT, size + 1, (LPARAM)text);
			text[size] = 0;
			text[size + 1] = 0;
			compile_source_flag = 1;		// set this flag for display possible error messsage
			SendMessage(hWndTalk, QL_CLEAR_ALL, 0, 0);
			if (view_mode == 0) {				// this is project working mode
				// size = QLForth_build(text, curr_file_name);
			}
			else {
				// size = QLForth_make(text);
			}
			if (size != 0) return;
		}
	}
	SendMessage(hWndTalk, QL_UPDATE, 0, 0);
	SetFocus(hWndTalk);
}

static void ql_interpret(char * str) {
	char * ptr;

	ptr = QLForth_preparation(0, NULL);
	memmove(ptr, str, TEXT_LINE_SIZE - 2);
	_beginthread(QLForth_interpreter, 0, NULL);
}

// ************************************************************************************

void Edit_window_size(void) {
	RECT rect;
	int  user_width, talk_height, user_height,
		edit_x, edit_y, edit_w, con_x, con_y, con_w;

	GetClientRect(hWndMain, &rect);
	talk_height = SendMessage(hWndTalk, QL_GET_CHAR_HEIGHT, 0, 0) * 2;
	user_width = rect.right - rect.left;
	user_height = rect.bottom - rect.top - talk_height;
	edit_x = edit_y = 0;
	edit_w = user_width / 2 - 3;
	con_x = edit_w + 3;
	con_y = 0;
	con_w = user_width / 2 - 20;
	MoveWindow(hWndEdit, edit_x, edit_y, edit_w, user_height, TRUE);
	MoveWindow(hWndConsole, con_x, con_y, con_w, user_height, TRUE);
	MoveWindow(hWndTalk, 0, user_height, user_width, talk_height, TRUE);
}

void Edit_mouse_move(int flag, int xPos, int yPos)
{

}

static void main_window_init(void) {
	hWndEdit = CreateWindowEx(WS_EX_CLIENTEDGE, "Scintilla", "Sketch", 
					WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
					ES_LEFT | ES_MULTILINE | ES_AUTOHSCROLL | ES_AUTOVSCROLL,
					0, 0, 0 , 0, hWndMain, (HMENU) ID_EDIT, hInst, NULL);
	if (hWndEdit == NULL) return ;
	Edit_init();

	hWndConsole = CreateWindowEx(WS_EX_CLIENTEDGE, "Scintilla", "Console",
					WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
					ES_LEFT | ES_MULTILINE | ES_AUTOHSCROLL | ES_AUTOVSCROLL,
					0, 0, 0, 0, hWndMain, (HMENU) ID_CONSOLE, hInst, NULL);
	if (hWndConsole == NULL) return;
	Console_init();
	
	hWndTalk	= Talk_window_init(ID_TALK);

	messageFindReplace	= RegisterWindowMessage(FINDMSGSTRING) ;

	ql_file_init();

	SetFocus(hWndTalk);
}

static void process_notify(NMHDR * nmhdr, char * str) {
	switch(nmhdr->idFrom) {
		case ID_EDIT:
			switch(nmhdr->code)  {
				case SCN_UPDATEUI:			display_editor_infomation();	break;
				case SCN_STYLENEEDED:		Edit_syntax((void *) nmhdr);	break;
			}
			break;

		case ID_TALK:
			switch(nmhdr->code) {
				case QLN_TALK_GET_FOCUS:	ql_build();						break;
				case QLN_TALK_EXECUTE:		ql_interpret(str);				break;
			}
			break;
	}
}

static void do_command(int cmd) {
	switch(cmd) {
		case IDM_FILE_OPEN:			ql_file_open(0);							break;	
		case IDM_FILE_SAVE:			ql_file_save();								break;
	
		case IDM_EDIT_UNDO:			SendMessage(hWndEdit, WM_UNDO, 0, 0);		break;
		case IDM_EDIT_REDO:			SendMessage(hWndEdit, SCI_REDO, 0, 0);		break ;	
		case IDM_EDIT_CUT:			SendMessage(hWndEdit, WM_CUT, 0, 0);		break;
		case IDM_EDIT_COPY:			SendMessage(hWndEdit, WM_COPY, 0, 0);		break ;	
		case IDM_EDIT_PASTE:		SendMessage(hWndEdit, WM_PASTE, 0, 0);		break;		
		case IDM_EDIT_CLEAR:		SendMessage(hWndEdit, WM_CLEAR, 0, 0);		break;
		case IDM_EDIT_SEL_ALL:		SendMessage(hWndEdit,SCI_SELECTALL, 0, 0);	break;
		case IDM_EDIT_FIND:			Edit_find();	            				break;
		case IDM_EDIT_FIND_NEXT:	Edit_find_next();							break;
		case IDM_EDIT_REPLACE:		Edit_replace();								break;

		case IDM_MAKE_BUILD:		ql_build();									break;
	}
}

// ************************************************************************************

LRESULT CALLBACK MainWndProc(HWND hwnd,UINT msg, WPARAM wParam,LPARAM lParam) {
	switch (msg) {
		case WM_CREATE:			hWndMain = hwnd;		main_window_init();			        break;
		case WM_SIZE:			Edit_window_size();											break;

		case WM_CAPTURECHANGED:	cap_flag = 0;												break;
		case WM_LBUTTONDOWN:	cap_flag = 1;	SetCapture(hwnd);							break;
		case WM_MOUSEMOVE:		Edit_mouse_move(cap_flag, LOWORD(lParam), HIWORD(lParam));	break;
		case WM_LBUTTONUP:		Edit_mouse_move(cap_flag, LOWORD(lParam), HIWORD(lParam));	
								cap_flag = 0;	ReleaseCapture();							break;


   		case WM_COMMAND:		do_command(LOWORD(wParam));									break;
		case WM_NOTIFY:			process_notify((NMHDR *) lParam, (char *) wParam);          break;

		case WM_DEVICECHANGE:
			{ 	PDEV_BROADCAST_HDR lpdb = (PDEV_BROADCAST_HDR)lParam ;

			//	if (wParam == DBT_DEVICEARRIVAL)				USB_HID_plug_in();
			//	else if (wParam == DBT_DEVICEREMOVECOMPLETE)	USB_HID_plug_out();
			}
			break; 

		case WM_CLOSE:			ql_file_save();           DestroyWindow (hwnd) ;           break;
		case WM_DESTROY:        PostQuitMessage(0);										   break;

		default:	
			if (msg == messageFindReplace) { 
				Edit_search((LPFINDREPLACE) lParam) ;
                return 0 ;
			}
			return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nCmdShow) {
	MSG			msg;
	HACCEL 		hAccelTable;
	HMODULE		hLib ;
	WNDCLASS	wc;

	hInst = hInstance;
	memset(&wc, 0, sizeof(WNDCLASS));
	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS ;
	wc.lpfnWndProc		= (WNDPROC) MainWndProc;
	wc.hInstance		= hInst;
	wc.hbrBackground	= (HBRUSH) GetStockObject (WHITE_BRUSH) ;
	wc.lpszClassName	= szAppName;
	wc.lpszMenuName		= NULL;
	wc.hCursor			= LoadCursor(NULL, IDC_SIZEWE) ;
	wc.hIcon			= LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON));
	if (!RegisterClass (&wc)) {
		MessageBox(NULL, TEXT ("This program has been tested on Windows 7 only."), szAppName, MB_ICONERROR) ;
        return 0 ;
    }

	if ((hLib = LoadLibrary("SciLexer.DLL")) == NULL) {
		MessageBox(NULL, "Failed to load SciLexer.DLL", "Loading DLL", MB_OK | MB_ICONERROR);
		return 0 ;
	}

	hWndMain = CreateWindow(szAppName, Version,
				WS_MINIMIZEBOX | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | 
				WS_MAXIMIZEBOX | WS_CAPTION | WS_BORDER | WS_SYSMENU | WS_THICKFRAME,
				CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
				NULL,	NULL,	hInst,	NULL);
	if (hWndMain == NULL) return 0;

	ShowWindow(hWndMain, SW_SHOW); 
//	ShowWindow(hWndMain, SW_MAXIMIZE) ; 
	UpdateWindow(hWndMain) ;

	app_start();

	hAccelTable = LoadAccelerators(hInst, szAppName);
	while (GetMessage(&msg,NULL, 0, 0)) {
		if (hDlgModeless == NULL || ! IsDialogMessage (hDlgModeless, &msg)) {
			if (! TranslateAccelerator (hWndMain, hAccelTable, &msg)) {
				TranslateMessage (&msg) ;
				DispatchMessage (&msg) ;
            }
        }
	}

	FreeLibrary(hLib);
	return msg.wParam;
}

// ************************************************************************************

void USB_register_for_device_notifications(GUID	HidGuid, char * device_path) {
	DEV_BROADCAST_DEVICEINTERFACE DevBroadcastDeviceInterface;
	HDEVNOTIFY DeviceNotificationHandle;

	HidD_GetHidGuid(&HidGuid);
	DevBroadcastDeviceInterface.dbcc_size = sizeof(DevBroadcastDeviceInterface);
	DevBroadcastDeviceInterface.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	DevBroadcastDeviceInterface.dbcc_classguid = HidGuid;
	DeviceNotificationHandle = RegisterDeviceNotification(hWndMain,
		&DevBroadcastDeviceInterface, DEVICE_NOTIFY_WINDOW_HANDLE);
}

int QLForth_printf(const char * fmt, ...) {
	va_list	argptr;
	int		len, line, lineStart;
	char 	buf[2048];

	va_start(argptr, fmt);
	wvsprintf(buf, fmt, argptr);
	va_end(argptr);

	len = strlen(buf);
	SendMessage(hWndConsole, SCI_SETREADONLY, FALSE, 0);
	SendMessage(hWndConsole, SCI_APPENDTEXT, len, (LPARAM)buf);
	line = SendMessage(hWndConsole, SCI_GETLINECOUNT, 0, 0);
	lineStart = SendMessage(hWndConsole, SCI_POSITIONFROMLINE, line, 0);
	SendMessage(hWndConsole, SCI_GOTOPOS, lineStart, 0);
	SendMessage(hWndConsole, SCI_SETREADONLY, TRUE, 0);
	return len;
}
