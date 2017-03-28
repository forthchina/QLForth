#ifndef _QLFORTH_IDE_H_
#define _QLFORTH_IDE_H_

#define IDC_STATIC			-1
#define ID_EDIT				1000
#define ID_TALK				2000
#define ID_CONSOLE			3000

#define IDI_ICON			101

#define IDM_FILE_OPEN		110
#define IDM_FILE_SAVE		120

#define IDM_EDIT_BEGIN		200
#define IDM_EDIT_UNDO		206
#define IDM_EDIT_REDO		210
#define IDM_EDIT_CUT		220
#define IDM_EDIT_COPY		230
#define IDM_EDIT_PASTE		240
#define IDM_EDIT_CLEAR		250
#define IDM_EDIT_COPY_RTF	260
#define IDM_EDIT_SEL_ALL	270
#define IDM_EDIT_FIND		280
#define IDM_EDIT_FIND_NEXT	285
#define IDM_EDIT_REPLACE	290
#define IDM_EDIT_SWAP		299

#define IDM_TEXT_MAKE		310
#define IDM_TEXT_BUILD		320

// ************************************************************************************

#define QL_PART_MODE		0x0002
#define QL_PART_PROMPT		0x0003

#define QL_SET_TEXT			2015
#define QL_SET_COLOR		2016
#define QL_SET_PROGRESS		2017
#define QL_CLEAR_ALL		2018

#define QL_SET_MAX			2019
#define QL_SET_TOS_NOS		2020

#define QL_UPDATE			2021
#define QL_GET_CHAR_HEIGHT	2022

#define QLN_TALK_GET_FOCUS	0
#define QLN_TALK_EXECUTE	1

// ************************************************************************************

typedef enum {
	SSC_LINE_BLOCK = 0,		SSC_DEFINE_WORD,	SSC_REMARK_EOL,
	SSC_REMARK_BLOCK,		SSC_IDENTIFIER,		SSC_PRIMITIVE,
	SSC_IMMEDIATE,			SSC_STRING_QUOTE,	SSC_STRING_TICK,	
	SSC_NUMBER,
	
	SCE_DEFAULT = 0,		SCE_COMMENT,		SCE_IDENTIFIER,
	SCE_PRIMITIVE,			SCE_COMPILE,		SCE_STRING,
	SCE_DEFINED_WORD,		SCE_NUMBER,		

	SCE_ANNOTATION_NOTE,	SCE_ANNOTATION_ERROR,

	END_OF_SHLC
	
} SYNTAX_HIGHLIGHT_CODE;

// ************************************************************************************

#define STRING_MAX			255

typedef struct _tag_font_selected {
	int		cxChar, cyChar;
	HFONT	hFontTitle ;
	HFONT	hFontTerm;
	HFONT	hFontText;
	HFONT	hFontNote;
} FontSelected;

// ************************************************************************************

extern HINSTANCE hInst;	
extern HWND hWndMain, hWndEdit, hWndTalk, hWndConsole;
extern HWND hDlgModeless ;

// ************************************************************************************

void Edit_init			(void);
void Edit_find			(void);
void Edit_find_next		(void);
void Edit_replace		(void);
void Edit_search		(LPFINDREPLACE	pfr);
void Edit_syntax		(struct SCNotification * notify);

void Edit_window_size	(void);
void Edit_mouse_move	(int flag, int xPos, int yPos);
void Console_init		(void);

HWND Talk_window_init	(int id);

#endif // _QLFORTH_IDE_H_
