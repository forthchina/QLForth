#ifndef _QLFORTH_H_
#define _QLFORTH_H_

#if defined(_WIN32)					// Winodws platform

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <setjmp.h>

#define strupr	_strupr

#else 

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>
#include <stdint.h>
#include <math.h>
#include <setjmp.h>

#endif

typedef int     QLF_CELL;
typedef float   QLF_REAL;

typedef enum {
	QLF_TRUE = -1L,
	QLF_FALSE = 0L,

	TEXT_LINE_SIZE			= 128,
	NAME_LENGTH				= 64,	
	STACK_DEEP_SIZE			= 256,
	DICT_BUFFER_SIZE		= (1024 * 1024),			// this will take 4MB for QLF_CELL 
	TEXT_BUFFER_STEP		= 16384,					// QLFORTH inner text buffer step = 16KB
	SST_NODE_MAX			= 65535,	
	MAX_CONTROL_STACK_DEEP	= 128,
	HASH_SIZE				= 127,

	QLF_STATE_INTERACTIVE	= 0,
	QLF_STATE_INTERPRET		= 1,
	QLF_STATE_COMPILE		= 2,
	QLF_STATE_MACRO			= 3,

	QLF_TYPE_COMMAND		= 0,
	QLF_TYPE_DEFINE,	
	QLF_TYPE_PRIMITIVE,
	QLF_TYPE_IMMEDIATE,
	QLF_TYPE_COMPILE,
	QLF_TYPE_WORD,
	QLF_TYPE_VARIABLE,		
	QLF_TYPE_CONSTANT,
	QLF_TYPE_MACRO,
	
	TOKEN_END_OF_FILE		= 0,		
	TOKEN_UNKOWN,		
	TOKEN_WORD,		
	TOKEN_STRING,

	CSID_COLON				= 1,		
	CSID_MACRO,
	CSID_IF,			CSID_ELSE,		CSID_ENDIF,			CSID_BEGIN, 
	CSID_WHILE,			CSID_DO,		CSID_LEAVE,			CSID_FOR,
	
	SST_NOOP				= 0,		// COMMON OPCODE
	SST_COLON,			SST_RET,		SST_SEMICOLON,		SST_VARIABLE,		
	SST_LITERAL,		SST_VAR_REF,	SST_WORD_REF,		SST_LABEL,		
	SST_0_BRANCH,		SST_BRANCH,		SST_DO,				SST_LOOP,		
	SST_LEAVE,			SST_I,			SST_J,				SST_FOR,		
	SST_NEXT,			SST_FETCH,		SST_STORE,			SST_ALLOT,

	SST_ADD,							// CHIP ASSEMBLY
	SST_XOR,			SST_AND,		SST_OR,				SST_INVERT, 
	SST_EQUAL,			SST_LT, 		SST_U_LT,			SST_SWAP,		
	SST_DUP,			SST_DROP,		SST_OVER,			SST_NIP,		
	SST_TO_R,			SST_R_FROM,		SST_R_FETCH, 		SST_DSP, 		
	SST_LSHIFT,			SST_RSHIFT,		SST_1_SUB,			SST_2_R_FROM, 	
	SST_2_TO_R, 		SST_2_R_FETCH, 	SST_UNLOOP,			SST_DUP_FETCH,	
	SST_DUP_TO_R,		SST_2_DUP_XOR,	SST_2_DUP_EQUAL,	SST_STORE_NIP,	
	SST_2_DUP_STORE,	SST_UP_1,		SST_DOWN_1, 		SST_COPY,

	SST_END,
	
	END_OF_QLFORTH_CONSTANT 
} QLFORTH_CONSTANT ;

typedef void(*PRIMITIVE_FUNCTION) (void);

typedef struct _tag_primitive {
    char * pname;
	PRIMITIVE_FUNCTION fun;
} Primitive;

typedef union _tag_cell {
    QLF_CELL    ival;
    QLF_REAL    fval;
} QLF_LITERAL;

typedef struct _tag_symbol {
	struct	_tag_symbol * link;

	union {
		PRIMITIVE_FUNCTION fun;
		struct  _tag_symbol * sub_link;
		char	* scan_ptr;
		struct  _sst_node * sst;
	};

	union {
        char *  pname;
		char *	text;
	    QLF_CELL dfa;
        QLF_CELL ival;
        QLF_CELL fval;
    }; 

	char type, hidden, attr, name[1];
} Symbol;

typedef struct _tag_constant_pool {
	int			reg;
	QLF_CELL	val;
	char		* pos;
} CONSTANT_POOL;

typedef struct _sst_node {
	int location;												// for target memory position;
	union {
		int					val;
		Symbol				* spc;
		struct _sst_node	* sst;
	};
	char	type, not_used;
} SSTNode;

// ************************************************************************************

QLF_LITERAL		token_value;
QLF_CELL		* ql4thvm_here,
				ql4thvm_tos, *ql4thvm_dp, *ql4thvm_rp, *ql4thvm_stack_top, *ql4thvm_stack;
char			token_word[];
Symbol			** program_counter, *ThisCreateWord, *ThisExecuteWord;
int				ql4thvm_state, ql4thvm_running, ql4thvm_force_break;

// ************************************************************************************

#define STACK_LEFT(m)	do { if ((ql4thvm_dp - ql4thvm_stack) < m) { \
							stack_error_underflow(); return; } \
						} while (0);

#define STACK_SPACE(n)	do { if ((ql4thvm_rp - ql4thvm_dp) < n) { \
							stack_error_overflow(); return; } \
						} while (0);


#define STACK(m, n)		do { if (((ql4thvm_dp - ql4thvm_stack) < m) || \
						    ((ql4thvm_rp - ql4thvm_dp) < n) ) { \
							stack_error(); return; } \
						} while (0);

// ************************************************************************************

void	qlforth_fp_docreate		(void);
void	* qlforth_alloc			(int size_in_byte);

// ************************************************************************************

int  QLForth_printf				(const char * fmt, ...);
int  QLForth_display_stack		(int base, int depth, int data1, int data2);
void QLForth_report				(int msg, int data1, int data2, char * str);

void QLForth_init				(char * file_name);
void QLForth_interpreter		(char * text);
void QLForth_stop				(void);
int  QLForth_token				(void);
void QLForth_error				(char * msg, char * tk); 
Symbol * QLForth_symbol_search  (char * name);
Symbol * QLForth_symbol_add		(char * name);
SSTNode * QLForth_sst_append	(int type, SSTNode * pc);
void Compile_init				(void);
void Forth_init					(void);
void QLForth_sst_list			(SSTNode * start, SSTNode * sst);
void Code_init					(void);
void Code_generation			(SSTNode * start);

int	 QLForth_chip_read			(int vpc);
void QLForth_chip_write			(int vpc, int data);
void QLForth_chip_program		(unsigned char * code_buffer);
int  QLForth_chip_execute		(int vpc, int depth, int * dstack);

#endif // _QLFORTH_H_