/* Header file for the bypasser.
   Created: 20-feb-2008
   Last modified: 08-fev-2009
   Author: Willian dos Santos Lima*/

/* ========================================================================= */
#ifndef BYPASS
#define BYPASS

/* ------------------------------------------------------------------------- */

#include "def.h"

/* ------------------------------------------------------------------------- */
/* to represent variables in the place of registers at instruction type*/
#define OFFSET_VARS 32
#define EXTRA_AUX 2

/* ------------------------------------------------------------------------- */
/* supported operand types*/
typedef enum { NULL_VALUE, BOOLEAN, BYTE, SHORT, INT, CHAR } TVarType;

/* a variable*/
typedef struct VARIABLE{
   int value;
   TVarType type;
}TVariable;

/* ------------------------------------------------------------------------- */

/* function to bypass the bytecodes into the intermediate code representation*/
char bypass ( TClassFile *cf, const TCodeAttribute *code );

/* function to translate a bytecode into respective intermediate
   instruction(s)*/
char process_bytecode ( u1 opcode );

/* function to process the delayed instructions*/
char process_delayed ( void );

/* function to optimize the generated code taking off unnecessary copies*/
void bypass_optimize ( void );

/* ------------------------------------------------------------------------- */

/* function to develop common operations over IF_ICMP<> bytecodes*/
void common_if_icmp ( s2 offset_addr );

/* function to develop common operations over IF<> bytecodes*/
void common_if ( s2 offset_addr );

/* function to return to other modules the intermediate code generated*/
TInstr* bypass_get_code ( u4 *code_length );
/* ------------------------------------------------------------------------- */

/* ========================================================================= */
/* error codes for this phase:
   - 1, when not able to process a bytecode;
   - 2, when not able to process a bytecode;
   - 3, when a fail ocurred when trying to allocate memory.
   - 4, when a fail ocurred on mapping instructions addresses
   - 5, when it generate a further null pointer error

/* ========================================================================= */

#endif
