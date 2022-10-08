/* File containing types and definitions used along the project, and also
   the used libraries.
   Created: 23-apr-2007
   Last modified: 24-aug-2008
   Author: Willian dos Santos Lima*/

/* ========================================================================= */

/* PATTERNS ADOPTED WITHIN THE PROJECT:
   - global variables always finish with an underscore;
   - macro-defined constants always have their name in CAPITAL LETTERS;
   - simple-type abstractions have their name in lower-case letters;
   - structure-defined types have their name starting with a capital letter;
   - functions and variables always have their name with lower-case letters;
   - internal file names are started with an underscore.
*/

/* definitions are valid when not reached yet*/
#ifndef DEF
#define DEF

#pragma-pack(1)

/* ------------------------------------------------------------------------- */
/* default libraries inclusion*/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <conio.h>
#include <limits.h>
#include <math.h>
#include <assert.h>
#include <time.h>

/* ------------------------------------------------------------------------- */
/* contants definitions*/

/* project's data*/
#define __NAME "JaNi"
#define __VERSION "1.0"
#define __AUTHOR "Willian dos Santos Lima"
#define __GROUP "Distributed and Parallel Systems Group"
#define __LOCAL "Sao Paulo State University - Sao Jose do Rio Preto - Brazil"

/* boolean values*/
#define FALSE 0
#define TRUE 1

/* ------------------------------------------------------------------------- */
/* macro-functions definition*/
#define f_eq(A, B) fabs(A - B) > 0.000001 ? FALSE : TRUE /*comp. float/double*/
#define s_eq(A, B) strcmp(A, B) == 0 ? TRUE : FALSE      /*comp. string*/

/* assembler macros*/
#define lo(imm32) ( imm32 & 0xFFFF )
#define hi(imm32) ( (imm32 >> 16) & 0xFFFF )
#define hiadj(imm32) ( (imm32 >> 16) + 0xFFFF + ((imm32 >> 15) & 0x1) )

/* ------------------------------------------------------------------------- */
/* types and structures definition*/
#define u1 unsigned char
#define u2 unsigned short
#define u4 unsigned long
#define u8 unsigned long long

#define s1 char
#define s2 short
#define s4 long
#define s8 long long

#define boolean char

/* compiler phases*/
typedef enum { RECOGNIZER, BYPASSER, GRAPH_GEN,
               REGISTER_ALLOC, CODE_GEN } TPhase;

/* ------------------------------------------------------------------------- */
/* limits definition for types and structures*/
#define U1_MAX (u1) -1
#define U2_MAX (u2) -1
#define U4_MAX (u4) -1
#define U8_MAX (u8) -1

/* ------------------------------------------------------------------------- */
/* global variables' declaration - declaration is in 'globals.c'*/
extern u2 err_, warn_;           /* errors and warnings counters*/
extern char output_[U1_MAX+1];   /* output file name (executable)*/
extern u1 gravar_info_;          /* whether informations of ClassFile must be available*/

/* ------------------------------------------------------------------------- */
/* inclusion of other definitions files*/
#include "def_cf.h"
#include "def_inst.h"

#endif
