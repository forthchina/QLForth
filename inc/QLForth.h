#ifndef _QLFORTH_H_
#define _QLFORTH_H_

#if defined(_WIN32)					// Winodws platform
    #if defined(_WIN64)				// defined _WIN64, is a x64 platform
        #define QLF_SIZE_64         1
        typedef __int64 QLF_CELL;
        typedef double  QLF_REAL;
    #else							// not Win64, so it is a x86 32 bits platform
        #define QLF_SIZE_32         1
        typedef int     QLF_CELL;
        typedef float   QLF_REAL;
    #endif

#else								// is a Linux GCC compiler
    #if defined(__x86_64__)			// is a X64 Linux compiler
        #define QLF_SIZE_64         1
        typedef long	QLF_CELL;
        typedef float   QLF_REAL;
    #elif defined(__i386__)			// is a 32-bits Linux compiler
        #define QLF_SIZE_32         1
        typedef int     QLF_CELL;
        typedef float   QLF_REAL;
    #endif

#endif

#define TEXT_LINE_SIZE			128
#define NAME_LENGTH				64	
#define STACK_DEEP_SIZE			256
#define DICT_BUFFER_SIZE		(1024 * 1024)		// this will take 4MB for QLF_CELL 
#define TEXT_BUFFER_STEP		16384				// QLFORTH inner text buffer step = 16KB
		
#define USB_REPORT_SIZE			64

#define REPORT_PROCESS_OK		(1 << 0)
#define REPORT_PROCESS_WAIT		(1 << 1)
#define REPORT_PROCESS_FAULT	(1 << 2)
#define REPORT_PROCESS_ERROR	(1 << 3)

#define OPCODE					1
#define ADDRESS					(OPCODE	+ 1)
#define COUNT					(OPCODE	+ 2)
#define SEQUENCE				(OPCODE	+ 3)

typedef void (* PRIMITIVE_FUNCTION) (void) ;

#define QLF_STATE_INTERPRET		0
#define QLF_STATE_COMPILE		1
#define QLF_STATE_CORE			2
#define QLF_STATE_CODE			3

typedef enum {
	MAX_CONTROL_STACK_DEEP	= 32,

	QLF_TRUE	= -1L,
	QLF_FALSE	= 0L,

	QLF_TYPE_PRIMITIVE = 0,			QLF_TYPE_IMMEDIATE,
	QLF_TYPE_DEFINE,				QLF_TYPE_COMPILE,
	QLF_TYPE_VARIABLE,				QLF_TYPE_CONSTANT,
	QLF_TYPE_CORE,					QLF_TYPE_WORD,
	QLF_TYPE_VOCABULARY,
		

	TOKEN_END_OF_FILE	= 0,		TOKEN_UNKOWN		= 1,		
	TOKEN_WORD			= 2,		TOKEN_STRING		= 3,
	
	END_OF_QLFORTH_CONSTANT 
} QLFORTH_CONSTANT ;

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

typedef struct _core_mem_map {
	int	 base, size;
	char *  start, * here, * end;
} CORE_MEMORY_MAP;

typedef struct _tag_constant_pool {
	int			reg;
	QLF_CELL	val;
	char		* pos;
} CONSTANT_POOL;

// ************************************************************************************

extern QLF_LITERAL	token_value;
extern QLF_CELL *	ql4thvm_here,
					ql4thvm_tos, * ql4thvm_dp, * ql4thvm_rp, * ql4thvm_stack_top, * ql4thvm_stack;
extern Symbol		** program_counter, * ThisCreateWord, * ThisExecuteWord;
extern char			token_word[TEXT_LINE_SIZE];
extern int			ql4thvm_state, ql4thvm_running, ql4thvm_force_break;

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
void QLForth_report				(int msg, int data1, int data2, int data3);

void QLForth_init				(char * w_path);
char * QLForth_preparation		(int size, char * w_file);
void QLForth_interpreter		(void * ptr);

void Core_init					(void);
void Forth_init					(void);
int  QLForth_token				(void);
void QLForth_error				(char * msg, char * tk); 
Symbol * QLForth_symbol_search  (char * name);
Symbol * QLForth_symbol_add		(char * name);
Symbol * QLForth_create			(void);

void QLForth_compile_word		(QLF_CELL dfa);
void QLForth_compile_variable	(QLF_CELL dfa);
void QLForth_compile_literal	(QLF_CELL ival);
void QLForth_debug_word			(QLF_CELL dfa);
void QLForth_debug_variable		(QLF_CELL dfa);

int	 QLForth_chip_read			(int vpc);
void QLForth_chip_write			(int vpc, int data);
void QLForth_chip_program		(int vpc, int data);
int  QLForth_chip_execute		(int vpc, int depth, int * dstack);

#endif // _QLFORTH_H_