/* File with implementations for the bypasser functions.
   Created: 20-feb-2007
   Last modified: 10-feb-2009
   Author: Willian dos Santos Lima*/

/* ========================================================================= */

#include "bypass.h"
#include "error.h"

/* ========================================================================= */
/* bypasser global variables declaration*/

/* where the bytecodes are and their related information*/
const TCodeAttribute *code;
u4 pos_code;

/* where the the code's variable array goes*/
TVariable *var;

/* where the intermediate code goes ---------------------------------------- */
TInstr *int_code;
u4 code_position;

/* the operand stack is seen as an array of auxiliar variables*/
TVariable *operand_stack;
u4 stack_top;
u4 OFFSET_AUX_VARS;

/* table to map bytecode address into intermediate code address*/
u4 *map_addr;

/* list of instructions to be edited after first pass*/
u4 *delayed;
u4 delay_last;
/* ========================================================================= */

/** Function to create the intermediate code throght bypassing the bytecodes
  * Returns: - 0, for successful execution;
             - 1, for failure;
  * Arguments:
  * * cf: pointer to the class file internal representation
  * * code: the code attribute ccontaining the bytodes to be processed
  */
char bypass ( TClassFile *cf, const TCodeAttribute *c ){
   register u4 i, cnt_arrays;
   char status;
   FILE *_txt_bytecodes;
   TInstr mem_disp;
   
   /* making the code global to the bypasser*/
   code = c;
   
   /* pre-allocating space for intermediate code*/
   int_code = malloc( code->code_length * sizeof( TInstr ) * 2 );
   delayed = malloc( code->code_length * sizeof( u4 ) );
   delay_last = 0;
   
   /* allocating space for map table*/
   map_addr = malloc( code->code_length * sizeof( u4 ) );
   for ( i = 0; i < code->code_length; i++ )
      map_addr[i] = -1;
   
   /* allocating space for variables array*/
   var = malloc( code->max_locals * sizeof( TVariable ) );
   OFFSET_AUX_VARS = code->max_locals + OFFSET_VARS;
   
   /* allocating space for the (and extended) operand stack*/
   operand_stack = malloc( ( code->max_stack + EXTRA_AUX ) * sizeof( TVariable ) );
   stack_top = 0;
   
   /* writing the bytecodes literally to text file*/
   if ( (_txt_bytecodes = fopen( "output/bytecodes.dat", "w" )) == NULL ){
      printf("WARNING: Cannot write bytecodes description to file.\n");
      warn_++;
   }else{
      fprintf( _txt_bytecodes, "max_stack = %d, max_locals = %d, code_length = %d\n",
                                code->max_stack,code->max_locals,code->code_length);
      i = 0;
      while ( i < code->code_length ){
         fprintf( _txt_bytecodes, "%s (%d)\n", bytecode_name[code->code[i]], bytecode_length[code->code[i]] );
         i += bytecode_length[code->code[i]];
      }
      fclose( _txt_bytecodes );
   }
   
   /* mapping NEWARRAY`s bytecodes to pre-allocate structure*/
   i = cnt_arrays = 0;
   while ( !cnt_arrays && i < code->code_length ){
      if ( bytecode_name[code->code[i]] != NULL && !s_eq( bytecode_name[code->code[i]], "" ) )
         if ( code->code[i] == NEWARRAY )
            cnt_arrays++;
      i++;
   }
   
   /* catching unknown bytecodes*/
   status = 0;
   i = 0;
   while ( status != -1 && i < code->code_length ){
      if ( bytecode_name[code->code[i]] != NULL && !s_eq( bytecode_name[code->code[i]], "" ) ){
         if ( !is_implemented[code->code[i]] ){
            err_++;
            printf("ERROR: Non-implemented bytecode %s at line %d.\n",bytecode_name[code->code[i]],i);
            status = 1;
         }
      }else{
         err_++;
         printf("ERROR: Unknown bytecode at line %d: 0x%X.\n",i,code->code[i]);
         status = 1;
      }
      i += bytecode_length[code->code[i]];
      if ( bytecode_length[code->code[i]] == 0 ){
         err_++;
         printf("ERROR: Bytecodes with variable length are not supported yet.\n");
         /* compilation must not proceed when this erros ocurrs*/
         status = -1;
      }
   }
   if ( status )
      return 1;
   
   /* before starting processing, include instruction to initialize displacement register for memory addresses*/
   /*movi disp_reg_mem, 4 -> addi disp_reg_mem, 0, 4*/
   mem_disp.type = ITYPE;
   mem_disp.ra = mem_disp.rc = 0;
   mem_disp.rb = DISP_REG_MEM;
   mem_disp.op = ADDI_OP;
   mem_disp.opx = ITYPE_OPX;
   mem_disp.imm16 = 0x04;
   mem_disp.imm5 = mem_disp.imm26 = 0x00;
   int_code[0] = mem_disp;
   
   if ( cnt_arrays ){
      /* before starting processing, include instruction to initialize displacement register for arrays, if any*/
      /*movi disp_reg_struct, 8 -> addi disp_reg_struct, 0, 8*/
      mem_disp.type = ITYPE;
      mem_disp.ra = mem_disp.rc = 0;
      mem_disp.rb = DISP_REG_STRUCT;
      mem_disp.op = ADDI_OP;
      mem_disp.opx = ITYPE_OPX;
      mem_disp.imm16 = 0x08;
      mem_disp.imm5 = mem_disp.imm26 = 0x00;
      int_code[1] = mem_disp;
      /* also, the register that tracks the length of the last array created*/
      /*movi array_length_reg, 0 -> addi array_length_reg, 0, 0*/
      mem_disp.type = ITYPE;
      mem_disp.ra = mem_disp.rc = 0;
      mem_disp.rb = ARRAY_LENGTH_REG;
      mem_disp.op = ADDI_OP;
      mem_disp.opx = ITYPE_OPX;
      mem_disp.imm16 = 0x00;
      mem_disp.imm5 = mem_disp.imm26 = 0x00;
      int_code[2] = mem_disp;
      code_position = 3;
   }else
      code_position = 1;
   
   /* processing bytecodes*/
   for ( pos_code = 0; pos_code < code->code_length; pos_code++ ){
      status = process_bytecode( code->code[pos_code] );
      if ( status < 0 ){
         printf("(bytecode: %s - line %d) ",bytecode_name[code->code[pos_code]], pos_code );
         if ( show_error( BYPASSER, 1 ) )
            return 1;
      }else if ( status > 0 ){
         printf("(bytecode: %s - line %d) ",bytecode_name[code->code[pos_code]], pos_code );
         if ( show_error( BYPASSER, 2 ) )
            return 1;
      }
   }
   
   /* processing delayed instructions*/
   if ( process_delayed( ) )
      if ( show_error( BYPASSER, 4 ) )
         return 1;
   
   /* writing translate code to file*/
   _txt_bytecodes = fopen( "output/intcode.dat", "w" );
   for ( i = 0; i < code_position; i++ )
      if ( int_code[i].type == RTYPE )
         fprintf( _txt_bytecodes, "%03d - %s: Ra = %d, Rb = %d, Rc = %d e imm16 = %d\n",i,
                                   opx_name[int_code[i].opx], int_code[i].ra,
                                   int_code[i].rb, int_code[i].rc, int_code[i].imm16 );
      else fprintf( _txt_bytecodes, "%03d - %s: Ra = %d, Rb = %d, Rc = %d e imm16 = %d\n",i,
                                     op_name[int_code[i].op], int_code[i].ra,
                                     int_code[i].rb, int_code[i].rc, int_code[i].imm16 );
   
   fclose( _txt_bytecodes );
   
   return 0;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/** Function to process a bytecode and create intermediate instructions
  * Returns: - 0, for successful execution;
             - 1, for fatal failure;
             - -1, for failure that can be hadled;
  * Arguments:
  * * opcode: the bytecode to be processed
  */
char process_bytecode ( u1 opcode ){
   TInstr instruction;
   int v1, v2, v3;
   char b1, b2;
   s2 offset_addr;
   
   /*printf("Opcode: 0x%X - %s\n",opcode,bytecode_name[opcode]);*/
   map_addr[pos_code] = code_position * 4;
   
   switch ( opcode ){
      /* bytecode NOP*/
      case NOP :  /*add 0, 0, 0*/
                  instruction.type = RTYPE;
                  instruction.ra = instruction.rb = instruction.rc = 0;
                  instruction.op = RTYPE_OP;
                  instruction.opx = ADD_OPX;
                  instruction.imm5 = instruction.imm16 = instruction.imm26 = 0x0;
                  int_code[code_position] = instruction;
                  code_position++;
                  break;
      /* bytecode ACONST_NULL*/
      case ACONST_NULL  :  operand_stack[stack_top].type = NULL_VALUE;
                           stack_top++;
                           /*movi aux_st, 0 -> addi aux_st, 0, 0*/
                           instruction.type = ITYPE;
                           instruction.ra = instruction.rc = 0;
                           instruction.rb = stack_top - 1 + OFFSET_AUX_VARS;
                           instruction.op = ADDI_OP;
                           instruction.opx = ITYPE_OPX;
                           instruction.imm16 = 0x00;
                           instruction.imm5 = instruction.imm26 = 0x00;
                           int_code[code_position] = instruction;
                           code_position++;
                           break;
      /* -------------------------------------------------------------------- */
      /* bytecode ICONST_M1*/
      case ICONST_M1 :  operand_stack[stack_top].value = -1;
                        operand_stack[stack_top].type = INT;
                        stack_top++;
                        /*movi aux_st, -1 -> addi aux_st, 0, -1*/
                        instruction.type = ITYPE;
                        instruction.ra = instruction.rc = 0;
                        instruction.rb = stack_top - 1 + OFFSET_AUX_VARS;
                        instruction.op = ADDI_OP;
                        instruction.opx = ITYPE_OPX;
                        instruction.imm16 = -1;
                        instruction.imm5 = instruction.imm26 = 0x00;
                        int_code[code_position] = instruction;
                        code_position++;
                        break;
      /* bytecode ICONST_0*/
      case ICONST_0  :  operand_stack[stack_top].value = 0;
                        operand_stack[stack_top].type = INT;
                        stack_top++;
                        /*movi aux_st, 0 -> addi aux_st, 0, 0*/
                        instruction.type = ITYPE;
                        instruction.ra = instruction.rc = 0;
                        instruction.rb = stack_top - 1 + OFFSET_AUX_VARS;
                        instruction.op = ADDI_OP;
                        instruction.opx = ITYPE_OPX;
                        instruction.imm16 = 0;
                        instruction.imm5 = instruction.imm26 = 0x00;
                        int_code[code_position] = instruction;
                        code_position++;
                        break;
      /* bytecode ICONST_1*/
      case ICONST_1  :  operand_stack[stack_top].value = 1;
                        operand_stack[stack_top].type = INT;
                        stack_top++;
                        /*movi aux_st, 1 -> addi aux_st, 0, 1*/
                        instruction.type = ITYPE;
                        instruction.ra = instruction.rc = 0;
                        instruction.rb = stack_top - 1 + OFFSET_AUX_VARS;
                        instruction.op = ADDI_OP;
                        instruction.opx = ITYPE_OPX;
                        instruction.imm16 = 1;
                        instruction.imm5 = instruction.imm26 = 0x00;
                        int_code[code_position] = instruction;
                        code_position++;
                        break;
      /* bytecode ICONST_2*/
      case ICONST_2  :  operand_stack[stack_top].value = 2;
                        operand_stack[stack_top].type = INT;
                        stack_top++;
                        /*movi aux_st, 2 -> addi aux_st, 0, 2*/
                        instruction.type = ITYPE;
                        instruction.ra = instruction.rc = 0;
                        instruction.rb = stack_top - 1 + OFFSET_AUX_VARS;
                        instruction.op = ADDI_OP;
                        instruction.opx = ITYPE_OPX;
                        instruction.imm16 = 2;
                        instruction.imm5 = instruction.imm26 = 0x00;
                        int_code[code_position] = instruction;
                        code_position++;
                        break;
      /* bytecode ICONST_3*/
      case ICONST_3  :  operand_stack[stack_top].value = 3;
                        operand_stack[stack_top].type = INT;
                        stack_top++;
                        /*movi aux_st, 3 -> addi aux_st, 0, 3*/
                        instruction.type = ITYPE;
                        instruction.ra = instruction.rc = 0;
                        instruction.rb = stack_top - 1 + OFFSET_AUX_VARS;
                        instruction.op = ADDI_OP;
                        instruction.opx = ITYPE_OPX;
                        instruction.imm16 = 3;
                        instruction.imm5 = instruction.imm26 = 0x00;
                        int_code[code_position] = instruction;
                        code_position++;
                        break;
      /* bytecode ICONST_4*/
      case ICONST_4  :  operand_stack[stack_top].value = 4;
                        operand_stack[stack_top].type = INT;
                        stack_top++;
                        /*movi aux_st, 4 -> addi aux_st, 0, 4*/
                        instruction.type = ITYPE;
                        instruction.ra = instruction.rc = 0;
                        instruction.rb = stack_top - 1 + OFFSET_AUX_VARS;
                        instruction.op = ADDI_OP;
                        instruction.opx = ITYPE_OPX;
                        instruction.imm16 = 4;
                        instruction.imm5 = instruction.imm26 = 0x00;
                        int_code[code_position] = instruction;
                        code_position++;
                        break;
      /* bytecode ICONST_5*/
      case ICONST_5  :  operand_stack[stack_top].value = 5;
                        operand_stack[stack_top].type = INT;
                        stack_top++;
                        /*movi aux_st, 5 -> addi aux_st, 0, 5*/
                        instruction.type = ITYPE;
                        instruction.ra = instruction.rc = 0;
                        instruction.rb = stack_top - 1 + OFFSET_AUX_VARS;
                        instruction.op = ADDI_OP;
                        instruction.opx = ITYPE_OPX;
                        instruction.imm16 = 5;
                        instruction.imm5 = instruction.imm26 = 0x00;
                        int_code[code_position] = instruction;
                        code_position++;
                        break;
      /* -------------------------------------------------------------------- */
      /* bytecode BIPUSH*/
      case BIPUSH :  pos_code++;
                     operand_stack[stack_top].value = (int) (s1) code->code[pos_code];
                     operand_stack[stack_top].type = INT;
                     stack_top++;
                     /*movi aux_st, value -> addi aux_st, 0, value*/
                     instruction.type = ITYPE;
                     instruction.ra = instruction.rc = 0;
                     instruction.rb = stack_top - 1 + OFFSET_AUX_VARS;
                     instruction.op = ADDI_OP;
                     instruction.opx = ITYPE_OPX;
                     instruction.imm16 = operand_stack[stack_top-1].value;
                     instruction.imm5 = instruction.imm26 = 0x00;
                     int_code[code_position] = instruction;
                     code_position++;
                     break;
      /* bytecode SIPUSH*/
      case SIPUSH :  v1 = (int) (short) ( ( code->code[++pos_code] << 8 ) | code->code[++pos_code] );
                     operand_stack[stack_top].value = v1;
                     operand_stack[stack_top].type = INT;
                     stack_top++;
                     /*movi aux_st, v1 -> addi aux_st, 0, v1*/
                     instruction.type = ITYPE;
                     instruction.ra = instruction.rc = 0;
                     instruction.rb = stack_top - 1 + OFFSET_AUX_VARS;
                     instruction.op = ADDI_OP;
                     instruction.opx = ITYPE_OPX;
                     instruction.imm16 = v1;
                     instruction.imm5 = instruction.imm26 = 0x00;
                     int_code[code_position] = instruction;
                     code_position++;
                     break;
      /* -------------------------------------------------------------------- */
      /* bytecode ILOAD*/
      case ILOAD  :  pos_code++;
                     v2 = (int) (s1) code->code[pos_code];
                     v1 = var[v2].value;
                     operand_stack[stack_top].value = v1;
                     operand_stack[stack_top].type = INT;
                     stack_top++;
                     /*mov aux_st, var_v2 -> add aux_st, 0, var_v2*/
                     instruction.type = RTYPE;
                     instruction.rc = stack_top - 1 + OFFSET_AUX_VARS;
                     instruction.ra = REG0;
                     instruction.rb = v2 + OFFSET_VARS;
                     instruction.op = RTYPE_OP;
                     instruction.opx = ADD_OPX;
                     instruction.imm5 = instruction.imm16 = instruction.imm26 = 0x00;
                     int_code[code_position] = instruction;
                     code_position++;
                     break;
      /* bytecode ILOAD_0*/
      case ILOAD_0   :  v1 = var[0].value;
                        operand_stack[stack_top].value = v1;
                        operand_stack[stack_top].type = INT;
                        stack_top++;
                        /*mov aux_st, var_0 -> add aux_st, 0, var_0*/
                        instruction.type = RTYPE;
                        instruction.rc = stack_top - 1 + OFFSET_AUX_VARS;
                        instruction.ra = REG0;
                        instruction.rb = 0 + OFFSET_VARS;
                        instruction.op = RTYPE_OP;
                        instruction.opx = ADD_OPX;
                        instruction.imm5 = instruction.imm16 = instruction.imm26 = 0x00;
                        int_code[code_position] = instruction;
                        code_position++;
                        break;
      /* bytecode ILOAD_1*/
      case ILOAD_1   :  v1 = var[1].value;
                        operand_stack[stack_top].value = v1;
                        operand_stack[stack_top].type = INT;
                        stack_top++;
                        /*mov aux_st, var_1 -> add aux_st, 0, var_1*/
                        instruction.type = RTYPE;
                        instruction.rc = stack_top - 1 + OFFSET_AUX_VARS;
                        instruction.ra = REG0;
                        instruction.rb = 1 + OFFSET_VARS;
                        instruction.op = RTYPE_OP;
                        instruction.opx = ADD_OPX;
                        instruction.imm5 = instruction.imm16 = instruction.imm26 = 0x00;
                        int_code[code_position] = instruction;
                        code_position++;
                        break;
      /* bytecode ILOAD_2*/
      case ILOAD_2   :  v1 = var[2].value;
                        operand_stack[stack_top].value = v1;
                        operand_stack[stack_top].type = INT;
                        stack_top++;
                        /*mov aux_st, var_2 -> add aux_st, 0, var_2*/
                        instruction.type = RTYPE;
                        instruction.rc = stack_top - 1 + OFFSET_AUX_VARS;
                        instruction.ra = REG0;
                        instruction.rb = 2 + OFFSET_VARS;
                        instruction.op = RTYPE_OP;
                        instruction.opx = ADD_OPX;
                        instruction.imm5 = instruction.imm16 = instruction.imm26 = 0x00;
                        int_code[code_position] = instruction;
                        code_position++;
                        break;
      /* bytecode ILOAD_3*/
      case ILOAD_3   :  v1 = var[3].value;
                        operand_stack[stack_top].value = v1;
                        operand_stack[stack_top].type = INT;
                        stack_top++;
                        /*mov aux_st, var_3 -> add aux_st, 0, var_3*/
                        instruction.type = RTYPE;
                        instruction.rc = stack_top - 1 + OFFSET_AUX_VARS;
                        instruction.ra = REG0;
                        instruction.rb = 3 + OFFSET_VARS;
                        instruction.op = RTYPE_OP;
                        instruction.opx = ADD_OPX;
                        instruction.imm5 = instruction.imm16 = instruction.imm26 = 0x00;
                        int_code[code_position] = instruction;
                        code_position++;
                        break;
      /* bytecode ALOAD*/
      case ALOAD     :  pos_code++;
                        v2 = (int) (s1) code->code[pos_code];
                        v1 = var[v2].value;  /*a reference*/
                        operand_stack[stack_top].value = v1;
                        operand_stack[stack_top].type = INT;
                        stack_top++;
                        /*mov aux_st, var_v2 -> add aux_st, 0, var_v2*/
                        instruction.type = RTYPE;
                        instruction.rc = stack_top - 1 + OFFSET_AUX_VARS;
                        instruction.ra = REG0;
                        instruction.rb = v2 + OFFSET_VARS;
                        instruction.op = RTYPE_OP;
                        instruction.opx = ADD_OPX;
                        instruction.imm5 = instruction.imm16 = instruction.imm26 = 0x00;
                        int_code[code_position] = instruction;
                        code_position++;
                        break;
      /* bytecode IALOAD*/
      case IALOAD    :  /* muli aux_st-1, aux_st-1, 4 (INT length*)*/
                        instruction.type = ITYPE;
                        instruction.ra = stack_top - 1 + OFFSET_AUX_VARS;
                        instruction.rb = stack_top - 1 + OFFSET_AUX_VARS;
                        instruction.rc = 0;
                        instruction.op = MULI_OP;
                        instruction.opx = ITYPE_OPX;
                        instruction.imm5 = instruction.imm26 = 0x00;
                        instruction.imm16 = 0x04;
                        int_code[code_position] = instruction;
                        code_position++;
                        /* add aux_st-1, aux_st-2, aux_st-1*/
                        instruction.type = RTYPE;
                        instruction.ra = stack_top - 2 + OFFSET_AUX_VARS;
                        instruction.rb = stack_top - 1 + OFFSET_AUX_VARS;
                        instruction.rc = stack_top - 1 + OFFSET_AUX_VARS;
                        instruction.op = RTYPE_OP;
                        instruction.opx = ADD_OPX;
                        instruction.imm5 = instruction.imm16 = instruction.imm26 = 0x00;
                        int_code[code_position] = instruction;
                        code_position++;
                        /*add aux_st-1, aux_st-1, REG_STRUCT*/
                        instruction.type = RTYPE;
                        instruction.ra = stack_top - 1 + OFFSET_AUX_VARS;
                        instruction.rb = DISP_REG_STRUCT;
                        instruction.rc = stack_top - 1 + OFFSET_AUX_VARS;
                        instruction.op = RTYPE_OP;
                        instruction.opx = ADD_OPX;
                        instruction.imm5 = instruction.imm16 = instruction.imm26 = 0x00;
                        int_code[code_position] = instruction;
                        code_position++;
                        /*ldw aux_st-2, 0(aux_st-1)*/
                        instruction.type = ITYPE;
                        instruction.ra = stack_top - 1 + OFFSET_AUX_VARS;
                        instruction.rb = stack_top - 2 + OFFSET_AUX_VARS;
                        instruction.rc = 0;
                        instruction.op = LDW_OP;
                        instruction.opx = ITYPE_OPX;
                        instruction.imm5 = instruction.imm16 = instruction.imm26 = 0x00;
                        int_code[code_position] = instruction;
                        code_position++;
                        stack_top--;
                        break;
      /* -------------------------------------------------------------------- */
      /* bytecode ISTORE*/
      case ISTORE :  pos_code++;
                     v2 = (int) code->code[pos_code];
                     v1 = operand_stack[--stack_top].value;
                     var[v2].value = v1;
                     var[v2].type = INT;
                     /*movi var_v2, aux_st -> add var_v2, 0, aux_st*/
                     instruction.type = RTYPE;
                     instruction.ra = REG0;
                     instruction.rb = stack_top + OFFSET_AUX_VARS;
                     instruction.rc = v2 + OFFSET_VARS;
                     instruction.op = RTYPE_OP;
                     instruction.opx = ADD_OPX;
                     instruction.imm5 = instruction.imm16 = instruction.imm26 = 0x00;
                     int_code[code_position] = instruction;
                     code_position++;
                     break;
      /* bytecode ISTORE_0*/
      case ISTORE_0  :  v1 = operand_stack[--stack_top].value;
                        var[0].value = v1;
                        var[0].type = INT;
                        /*movi var0, aux_st -> add var0, 0, aux_st*/
                        instruction.type = RTYPE;
                        instruction.ra = REG0;
                        instruction.rb = stack_top + OFFSET_AUX_VARS;
                        instruction.rc = 0 + OFFSET_VARS;
                        instruction.op = RTYPE_OP;
                        instruction.opx = ADD_OPX;
                        instruction.imm5 = instruction.imm16 = instruction.imm26 = 0x00;
                        int_code[code_position] = instruction;
                        code_position++;
                        break;
      /* bytecode ISTORE_1*/
      case ISTORE_1  :  v1 = operand_stack[--stack_top].value;
                        var[1].value = v1;
                        var[1].type = INT;
                        /*movi var1, aux_st -> add var1, 0, aux_st*/
                        instruction.type = RTYPE;
                        instruction.ra = REG0;
                        instruction.rb = stack_top + OFFSET_AUX_VARS;
                        instruction.rc = 1 + OFFSET_VARS;
                        instruction.op = RTYPE_OP;
                        instruction.opx = ADD_OPX;
                        instruction.imm5 = instruction.imm16 = instruction.imm26 = 0x00;
                        int_code[code_position] = instruction;
                        code_position++;
                        break;
      /* bytecode ISTORE_2*/
      case ISTORE_2  :  v1 = operand_stack[--stack_top].value;
                        var[2].value = v1;
                        var[2].type = INT;
                        /*movi var2, aux_st -> add var2, 0, aux_st*/
                        instruction.type = RTYPE;
                        instruction.ra = REG0;
                        instruction.rb = stack_top + OFFSET_AUX_VARS;
                        instruction.rc = 2 + OFFSET_VARS;
                        instruction.op = RTYPE_OP;
                        instruction.opx = ADD_OPX;
                        instruction.imm5 = instruction.imm16 = instruction.imm26 = 0x00;
                        int_code[code_position] = instruction;
                        code_position++;
                        break;
      /* bytecode ISTORE_3*/
      case ISTORE_3  :  v1 = operand_stack[--stack_top].value;
                        var[3].value = v1;
                        var[3].type = INT;
                        /*movi var3, aux_st -> add var3, 0, aux_st*/
                        instruction.type = RTYPE;
                        instruction.ra = REG0;
                        instruction.rb = stack_top + OFFSET_AUX_VARS;
                        instruction.rc = 3 + OFFSET_VARS;
                        instruction.op = RTYPE_OP;
                        instruction.opx = ADD_OPX;
                        instruction.imm5 = instruction.imm16 = instruction.imm26 = 0x00;
                        int_code[code_position] = instruction;
                        code_position++;
                        break;
      /* bytecode ASTORE*/
      case ASTORE    :  pos_code++;
                        v2 = (int) code->code[pos_code];
                        v1 = operand_stack[--stack_top].value;
                        var[v2].value = v1;
                        var[v2].type = INT;
                        /*movi var_v2, aux_st -> add var_v2, 0, aux_st*/
                        instruction.type = RTYPE;
                        instruction.ra = REG0;
                        instruction.rb = stack_top + OFFSET_AUX_VARS;
                        instruction.rc = v2 + OFFSET_VARS;
                        instruction.op = RTYPE_OP;
                        instruction.opx = ADD_OPX;
                        instruction.imm5 = instruction.imm16 = instruction.imm26 = 0x00;
                        int_code[code_position] = instruction;
                        code_position++;
                        break;
      /* bytecode IASTORE*/
      case IASTORE   :  /* muli aux_st-2, aux_st-2, 4 (INT length*)*/
                        instruction.type = ITYPE;
                        instruction.ra = stack_top - 2 + OFFSET_AUX_VARS;
                        instruction.rb = stack_top - 2 + OFFSET_AUX_VARS;
                        instruction.rc = 0;
                        instruction.op = MULI_OP;
                        instruction.opx = ITYPE_OPX;
                        instruction.imm5 = instruction.imm26 = 0x00;
                        instruction.imm16 = 0x04;
                        int_code[code_position] = instruction;
                        code_position++;
                        /* add aux_st, aux_st-2, aux_st-3*/
                        instruction.type = RTYPE;
                        instruction.ra = stack_top - 2 + OFFSET_AUX_VARS;
                        instruction.rb = stack_top - 3 + OFFSET_AUX_VARS;
                        instruction.rc = stack_top + OFFSET_AUX_VARS;
                        instruction.op = RTYPE_OP;
                        instruction.opx = ADD_OPX;
                        instruction.imm5 = instruction.imm16 = instruction.imm26 = 0x00;
                        int_code[code_position] = instruction;
                        code_position++;
                        /* add aux_st, aux_st, REG_STRUCT*/
                        instruction.type = RTYPE;
                        instruction.ra = stack_top + OFFSET_AUX_VARS;
                        instruction.rb = DISP_REG_STRUCT;
                        instruction.rc = stack_top + OFFSET_AUX_VARS;
                        instruction.op = RTYPE_OP;
                        instruction.opx = ADD_OPX;
                        instruction.imm5 = instruction.imm16 = instruction.imm26 = 0x00;
                        int_code[code_position] = instruction;
                        code_position++;
                        /* stw aux_st-1, 0(aux_st)*/
                        instruction.type = ITYPE;
                        instruction.ra = stack_top + OFFSET_AUX_VARS;
                        instruction.rb = stack_top - 1 + OFFSET_AUX_VARS;
                        instruction.rc = 0;
                        instruction.op = STW_OP;
                        instruction.opx = ITYPE_OPX;
                        instruction.imm5 = instruction.imm16 = instruction.imm26 = 0x00;
                        int_code[code_position] = instruction;
                        code_position++;
                        stack_top -= 3;
                        break;
      
      /* -------------------------------------------------------------------- */
      /* these 2 bytecodes may be used for code optimization*/
      /* bytecode POP2*/
      case POP2:  operand_stack[--stack_top].value = 0;
                  operand_stack[--stack_top].type = NULL_VALUE;
      /* bytecode POP*/
      case POP :  operand_stack[--stack_top].value = 0;
                  operand_stack[--stack_top].type = NULL_VALUE;
                  break;
      /* -------------------------------------------------------------------- */
      /* bytecode DUP*/
      case DUP :  operand_stack[stack_top] = operand_stack[stack_top-1];
                  stack_top++;
                  /*mov aux_st, aux_st+1 -> add aux_st+1, aux_st, 0*/
                  instruction.type = RTYPE;
                  instruction.ra = stack_top + OFFSET_AUX_VARS;
                  instruction.rb = REG0;
                  instruction.rc = stack_top + 1 + OFFSET_AUX_VARS;
                  instruction.op = RTYPE_OP;
                  instruction.opx = ADD_OPX;
                  instruction.imm5 = instruction.imm16 =  instruction.imm26 = 0x00;
                  int_code[code_position] = instruction;
                  code_position++;
                  break;
      /* bytecode DUP_X1*/
      case DUP_X1 :  
                     break;
      /* bytecode DUP_X2*/
      case DUP_X2 :  
                     break;
      /* bytecode DUP2*/
      case DUP2   :  /* for this implementation, there's only types on category 1*/
                     operand_stack[stack_top+1] = operand_stack[stack_top-1];
                     operand_stack[stack_top] = operand_stack[stack_top-2];
                     /*mov aux_st+1, aux_st-1 -> add aux_st+1, aux_st-1, 0*/
                     instruction.type = RTYPE;
                     instruction.ra = stack_top - 1 + OFFSET_AUX_VARS;
                     instruction.rb = REG0;
                     instruction.rc = stack_top + 1 + OFFSET_AUX_VARS;
                     instruction.op = RTYPE_OP;
                     instruction.opx = ADD_OPX;
                     instruction.imm5 = instruction.imm16 =  instruction.imm26 = 0x00;
                     int_code[code_position] = instruction;
                     code_position++;
                     /*mov aux_st-2, aux_st -> add aux_st, aux_st-2, 0*/
                     instruction.type = RTYPE;
                     instruction.ra = stack_top - 2 + OFFSET_AUX_VARS;
                     instruction.rb = REG0;
                     instruction.rc = stack_top + OFFSET_AUX_VARS;
                     instruction.op = RTYPE_OP;
                     instruction.opx = ADD_OPX;
                     instruction.imm5 = instruction.imm16 =  instruction.imm26 = 0x00;
                     int_code[code_position] = instruction;
                     code_position++;
                     stack_top += 2;
                     break;
      /* bytecode DUP2_X1*/
      case DUP2_X1:  
                     break;
      /* bytecode DUP2_X2*/
      case DUP2_X2:  
                     break;
      /* bytecode SWAP*/
      case SWAP   :  operand_stack[stack_top] = operand_stack[stack_top-1];
                     operand_stack[stack_top-1] = operand_stack[stack_top-2];
                     operand_stack[stack_top-2] = operand_stack[stack_top];
                     /*mov aux_st, aux_st-1 -> add aux_st, aux_st-1, 0*/
                     instruction.type = RTYPE;
                     instruction.ra = stack_top - 1 + OFFSET_AUX_VARS;
                     instruction.rb = REG0;
                     instruction.rc = stack_top + OFFSET_AUX_VARS;
                     instruction.op = RTYPE_OP;
                     instruction.opx = ADD_OPX;
                     instruction.imm5 = instruction.imm16 =  instruction.imm26 = 0x00;
                     int_code[code_position] = instruction;
                     code_position++;
                     /*mov aux_st-1, aux_st-2 -> add aux_st-1, aux_st-2, 0*/
                     instruction.type = RTYPE;
                     instruction.ra = stack_top - 2 + OFFSET_AUX_VARS;
                     instruction.rb = REG0;
                     instruction.rc = stack_top - 1 + OFFSET_AUX_VARS;
                     instruction.op = RTYPE_OP;
                     instruction.opx = ADD_OPX;
                     instruction.imm5 = instruction.imm16 =  instruction.imm26 = 0x00;
                     int_code[code_position] = instruction;
                     code_position++;
                     /*mov aux_st-2, aux_st -> add aux_st-2, aux_st, 0*/
                     instruction.type = RTYPE;
                     instruction.ra = stack_top + OFFSET_AUX_VARS;
                     instruction.rb = REG0;
                     instruction.rc = stack_top - 2 + OFFSET_AUX_VARS;
                     instruction.op = RTYPE_OP;
                     instruction.opx = ADD_OPX;
                     instruction.imm5 = instruction.imm16 =  instruction.imm26 = 0x00;
                     int_code[code_position] = instruction;
                     code_position++;
                     break;
      /* -------------------------------------------------------------------- */
      /* bytecode IADD*/
      case IADD:  if ( stack_top <= 1 )
                     return 1;
                  operand_stack[stack_top-2].value = operand_stack[stack_top-1].value + 
                                                     operand_stack[stack_top-2].value;
                  /*add aux_st-2, aux_st-1, aux_st-2*/
                  instruction.type = RTYPE;
                  instruction.ra = stack_top - 1 + OFFSET_AUX_VARS;
                  instruction.rb = instruction.rc = stack_top - 2 + OFFSET_AUX_VARS;
                  instruction.op = RTYPE_OP;
                  instruction.opx = ADD_OPX;
                  instruction.imm5 = instruction.imm16 = instruction.imm26 = 0x00;
                  int_code[code_position] = instruction;
                  code_position++;
                  /*stack top pointer update*/
                  stack_top--;
                  break;
      /* bytecode ISUB*/
      case ISUB:  if ( stack_top <= 1 )
                     return 1;
                  operand_stack[stack_top-2].value = operand_stack[stack_top-2].value -
                                                     operand_stack[stack_top-1].value;
                  /*sub aux_st-2, aux_st-2, aux_st-1*/
                  instruction.type = RTYPE;
                  instruction.rb = stack_top - 1 + OFFSET_AUX_VARS;
                  instruction.ra = instruction.rc = stack_top - 2 + OFFSET_AUX_VARS;
                  instruction.op = RTYPE_OP;
                  instruction.opx = SUB_OPX;
                  instruction.imm5 = instruction.imm16 = instruction.imm26 = 0x00;
                  int_code[code_position] = instruction;
                  code_position++;
                  /*stack top pointer update*/
                  stack_top--;
                  break;
      /* bytecode IMUL*/
      case IMUL:  if ( stack_top <= 1 )
                     return 1;
                  operand_stack[stack_top-2].value = operand_stack[stack_top-1].value * 
                                                     operand_stack[stack_top-2].value;
                  /*mul aux_st-2, aux_st-1, aux_st-2*/
                  instruction.type = RTYPE;
                  instruction.ra = stack_top - 1 + OFFSET_AUX_VARS;
                  instruction.rb = instruction.rc = stack_top - 2 + OFFSET_AUX_VARS;
                  instruction.op = RTYPE_OP;
                  instruction.opx = MUL_OPX;
                  instruction.imm5 = instruction.imm16 = instruction.imm26 = 0x00;
                  int_code[code_position] = instruction;
                  code_position++;
                  /*stack top pointer update*/
                  stack_top--;
                  break;
      /* bytecode IDIV*/
      case IDIV:  if ( stack_top <= 1 )
                     return 1;
                  /*operand_stack[stack_top-2].value = operand_stack[stack_top-2].value /
                                                     operand_stack[stack_top-1].value;*/
                  /*div aux_st-2, aux_st-2, aux_st-1*/
                  instruction.type = RTYPE;
                  instruction.rb = stack_top - 1 + OFFSET_AUX_VARS;
                  instruction.ra = instruction.rc = stack_top - 2 + OFFSET_AUX_VARS;
                  instruction.op = RTYPE_OP;
                  instruction.opx = DIV_OPX;
                  instruction.imm5 = instruction.imm16 = instruction.imm26 = 0x00;
                  int_code[code_position] = instruction;
                  code_position++;
                  /*stack top pointer update*/
                  stack_top--;
                  break;
      /* bytecode IREM*/
      case IREM:  if ( stack_top <= 1 )
                     return 1;
                  operand_stack[stack_top-2].value = operand_stack[stack_top-2].value %
                                                     operand_stack[stack_top-1].value;
                  /*div aux_st, aux_st-2, aux_st-1*/
                  instruction.type = RTYPE;
                  instruction.ra = stack_top - 2 + OFFSET_AUX_VARS;
                  instruction.rb = stack_top - 1 + OFFSET_AUX_VARS;
                  instruction.rc = stack_top + OFFSET_AUX_VARS;
                  instruction.op = RTYPE_OP;
                  instruction.opx = DIV_OPX;
                  instruction.imm5 = instruction.imm16 = instruction.imm26 = 0x00;
                  int_code[code_position] = instruction;
                  code_position++;
                  /*mul aux_st+1, aux_st, aux_st-1*/
                  instruction.type = RTYPE;
                  instruction.ra = stack_top + OFFSET_AUX_VARS;
                  instruction.rb = stack_top - 1 + OFFSET_AUX_VARS;
                  instruction.rc = stack_top + 1 + OFFSET_AUX_VARS;
                  instruction.op = RTYPE_OP;
                  instruction.opx = MUL_OPX;
                  instruction.imm5 = instruction.imm16 = instruction.imm26 = 0x00;
                  int_code[code_position] = instruction;
                  code_position++;
                  /*sub aux_st+1, aux_st-2, aux_st+1*/
                  instruction.type = RTYPE;
                  instruction.ra = stack_top - 2 + OFFSET_AUX_VARS;
                  instruction.rb = stack_top + 1 + OFFSET_AUX_VARS;
                  instruction.rc = stack_top + 1 + OFFSET_AUX_VARS;
                  instruction.op = RTYPE_OP;
                  instruction.opx = SUB_OPX;
                  instruction.imm5 = instruction.imm16 = instruction.imm26 = 0x00;
                  int_code[code_position] = instruction;
                  code_position++;
                  /*mov aux_st-2, aux_st+1 -> add aux_st-2, aux_st+1, 0*/
                  instruction.type = RTYPE;
                  instruction.ra = stack_top + 1 + OFFSET_AUX_VARS;
                  instruction.rb = REG0;
                  instruction.rc = stack_top - 2 + OFFSET_AUX_VARS;
                  instruction.op = RTYPE_OP;
                  instruction.opx = ADD_OPX;
                  instruction.imm5 = instruction.imm16 = instruction.imm26 = 0x00;
                  int_code[code_position] = instruction;
                  code_position++;
                  /*stack top pointer update*/
                  stack_top--;
                  break;
      /* bytecode INEG*/
      case INEG:  operand_stack[stack_top-1].value = -operand_stack[stack_top-1].value;
                  /*muli aux_st, aux_st, -1*/
                  instruction.type = ITYPE;
                  instruction.ra = instruction.rb = stack_top - 1 + OFFSET_AUX_VARS;
                  instruction.rc = 0;
                  instruction.op = MULI_OP;
                  instruction.opx = ITYPE_OPX;
                  instruction.imm5 = instruction.imm26 = 0x00;
                  instruction.imm16 = -1;
                  int_code[code_position] = instruction;
                  code_position++;
                  break;
      /* bytecode ISHL*/
      case ISHL:  if ( stack_top <= 1 )
                     return 1;
                  operand_stack[stack_top-2].value = operand_stack[stack_top-2].value <<
                                                     (operand_stack[stack_top-1].value & 0x1F);
                  /*rol aux_st-2, aux_st-2, aux_st-1(4..0)*/
                  instruction.type = RTYPE;
                  instruction.ra = instruction.rc = stack_top - 2 + OFFSET_AUX_VARS;
                  instruction.rb = stack_top - 1 + OFFSET_AUX_VARS;
                  instruction.op = RTYPE_OP;
                  instruction.opx = ROL_OPX;
                  instruction.imm5 = instruction.imm16 = instruction.imm26 = 0x00;
                  int_code[code_position] = instruction;
                  code_position++;
                  /*stack top pointer update*/
                  stack_top--;
                  break;
      /* bytecode ISHR*/
      case ISHR:  if ( stack_top <= 1 )
                     return 1;
                  operand_stack[stack_top-2].value = operand_stack[stack_top-2].value >>
                                                     (operand_stack[stack_top-1].value & 0x1F);
                  /*ror aux_st-2, aux_st-2, aux_st-1(4..0)*/
                  instruction.type = RTYPE;
                  instruction.ra = instruction.rc = stack_top - 2 + OFFSET_AUX_VARS;
                  instruction.rb = stack_top - 1 + OFFSET_AUX_VARS;
                  instruction.op = RTYPE_OP;
                  instruction.opx = ROR_OPX;
                  instruction.imm5 = instruction.imm16 = instruction.imm26 = 0x00;
                  int_code[code_position] = instruction;
                  code_position++;
                  /*stack top pointer update*/
                  stack_top--;
                  break;
      /* bytecode IUSHR*/
      case IUSHR  :  
                     break;
      /* -------------------------------------------------------------------- */
      /* bytecode IAND*/
      case IAND:  if ( stack_top <= 1 )
                     return 1;
                  operand_stack[stack_top-2].value = operand_stack[stack_top-1].value & 
                                                     operand_stack[stack_top-2].value;
                  /*and aux_st-2, aux_st-1, aux_st-2*/
                  instruction.type = RTYPE;
                  instruction.ra = stack_top - 1 + OFFSET_AUX_VARS;
                  instruction.rb = instruction.rc = stack_top - 2 + OFFSET_AUX_VARS;
                  instruction.op = RTYPE_OP;
                  instruction.opx = AND_OPX;
                  instruction.imm5 = instruction.imm16 = instruction.imm26 = 0x00;
                  int_code[code_position] = instruction;
                  code_position++;
                  /*stack top pointer update*/
                  stack_top--;
                  break;
      /* bytecode IOR*/
      case IOR :  if ( stack_top <= 1 )
                     return 1;
                  operand_stack[stack_top-2].value = operand_stack[stack_top-1].value | 
                                                     operand_stack[stack_top-2].value;
                  /*add aux_st-2, aux_st-1, aux_st-2*/
                  instruction.type = RTYPE;
                  instruction.ra = stack_top - 1 + OFFSET_AUX_VARS;
                  instruction.rb = instruction.rc = stack_top - 2 + OFFSET_AUX_VARS;
                  instruction.op = RTYPE_OP;
                  instruction.opx = OR_OPX;
                  instruction.imm5 = instruction.imm16 = instruction.imm26 = 0x00;
                  int_code[code_position] = instruction;
                  code_position++;
                  /*stack top pointer update*/
                  stack_top--;
                  break;
      /* bytecode IXOR*/
      case IXOR:  if ( stack_top <= 1 )
                     return 1;
                  operand_stack[stack_top-2].value = operand_stack[stack_top-1].value ^ 
                                                     operand_stack[stack_top-2].value;
                  /*add aux_st-2, aux_st-1, aux_st-2*/
                  instruction.type = RTYPE;
                  instruction.ra = stack_top - 1 + OFFSET_AUX_VARS;
                  instruction.rb = instruction.rc = stack_top - 2 + OFFSET_AUX_VARS;
                  instruction.op = RTYPE_OP;
                  instruction.opx = XOR_OPX;
                  instruction.imm5 = instruction.imm16 = instruction.imm26 = 0x00;
                  int_code[code_position] = instruction;
                  code_position++;
                  /*stack top pointer update*/
                  stack_top--;
                  break;
      /* bytecode IINC*/
      case IINC:  pos_code++;
                  v2 = (int) code->code[pos_code];
                  pos_code++;
                  v3 = (int) (s1) code->code[pos_code];
                  var[v2].value += v3;
                  /*addi var_v2, var_v2, v3*/
                  instruction.type = ITYPE;
                  instruction.rb = instruction.ra = v2 + OFFSET_VARS;
                  instruction.rc = 0;
                  instruction.op = ADDI_OP;
                  instruction.opx = ITYPE_OPX;
                  instruction.imm16 = v3;
                  instruction.imm5 = instruction.imm26 = 0x00;
                  int_code[code_position] = instruction;
                  code_position++;
                  break;
      /* -------------------------------------------------------------------- */
      /* bytecode INT2BYTE*/
      case INT2BYTE  :  
                        break;
      /* bytecode INT2CHAR*/
      case INT2CHAR  :  
                        break;
      /* bytecode INT2SHORT*/
      case INT2SHORT :  /* will be marked as implemented, as no effect is
                           expected after this bytecode execution*/
                        break;
      /* -------------------------------------------------------------------- */
      /* bytecode IFEQ*/
      case IFEQ:  pos_code++;
                  v1 = (int) (s1) code->code[pos_code];
                  pos_code++;
                  v2 = (int) (s1) code->code[pos_code];
                  offset_addr = ( v1 << 8 ) | v2;
                  /*compeq aux_st-1, aux_st-1, 0*/
                  instruction.type = RTYPE;
                  instruction.ra = instruction.rc = stack_top - 1 + OFFSET_AUX_VARS;
                  instruction.rb = REG0;
                  instruction.op = RTYPE_OP;
                  instruction.opx = CMPEQ_OPX;
                  instruction.imm5 = instruction.imm16 = instruction.imm16 = 0x00;
                  int_code[code_position] = instruction;
                  code_position++;
                  common_if( offset_addr );
                  /*stack top pointer update*/
                  stack_top--;
                  break;
      /* bytecode IFNE*/
      case IFNE:  pos_code++;
                  v1 = (int) (s1) code->code[pos_code];
                  pos_code++;
                  v2 = (int) (s1) code->code[pos_code];
                  offset_addr = ( v1 << 8 ) | v2;
                  /*compne aux_st-1, aux_st-1, 0*/
                  instruction.type = RTYPE;
                  instruction.ra = instruction.rc = stack_top - 1 + OFFSET_AUX_VARS;
                  instruction.rb = REG0;
                  instruction.op = RTYPE_OP;
                  instruction.opx = CMPNE_OPX;
                  instruction.imm5 = instruction.imm16 = instruction.imm16 = 0x00;
                  int_code[code_position] = instruction;
                  code_position++;
                  common_if( offset_addr );
                  /*stack top pointer update*/
                  stack_top--;
                  break;
      /* bytecode IFLT*/
      case IFLT:  pos_code++;
                  v1 = (int) (s1) code->code[pos_code];
                  pos_code++;
                  v2 = (int) (s1) code->code[pos_code];
                  offset_addr = ( v1 << 8 ) | v2;
                  /*complt aux_st-1, aux_st-1, 0*/
                  instruction.type = RTYPE;
                  instruction.ra = instruction.rc = stack_top - 1 + OFFSET_AUX_VARS;
                  instruction.rb = REG0;
                  instruction.op = RTYPE_OP;
                  instruction.opx = CMPLT_OPX;
                  instruction.imm5 = instruction.imm16 = instruction.imm16 = 0x00;
                  int_code[code_position] = instruction;
                  code_position++;
                  common_if( offset_addr );
                  /*stack top pointer update*/
                  stack_top--;
                  break;
      /* bytecode IFGE*/
      case IFGE:  pos_code++;
                  v1 = (int) (s1) code->code[pos_code];
                  pos_code++;
                  v2 = (int) (s1) code->code[pos_code];
                  offset_addr = ( v1 << 8 ) | v2;
                  /*compge aux_st-1, aux_st-1, 0*/
                  instruction.type = RTYPE;
                  instruction.ra = instruction.rc = stack_top - 1 + OFFSET_AUX_VARS;
                  instruction.rb = REG0;
                  instruction.op = RTYPE_OP;
                  instruction.opx = CMPGE_OPX;
                  instruction.imm5 = instruction.imm16 = instruction.imm16 = 0x00;
                  int_code[code_position] = instruction;
                  code_position++;
                  common_if( offset_addr );
                  /*stack top pointer update*/
                  stack_top--;
                  break;
      /* bytecode IFGT*/
      case IFGT:  pos_code++;
                  v1 = (int) (s1) code->code[pos_code];
                  pos_code++;
                  v2 = (int) (s1) code->code[pos_code];
                  offset_addr = ( v1 << 8 ) | v2;
                  /*complt aux_st-1, 0, aux_st-1*/
                  instruction.type = RTYPE;
                  instruction.rb = instruction.rc = stack_top - 1 + OFFSET_AUX_VARS;
                  instruction.ra = REG0;
                  instruction.op = RTYPE_OP;
                  instruction.opx = CMPLT_OPX;
                  instruction.imm5 = instruction.imm16 = instruction.imm16 = 0x00;
                  int_code[code_position] = instruction;
                  code_position++;
                  common_if( offset_addr );
                  /*stack top pointer update*/
                  stack_top--;
                  break;
      /* bytecode IFLE*/
      case IFLE:  pos_code++;
                  v1 = (int) (s1) code->code[pos_code];
                  pos_code++;
                  v2 = (int) (s1) code->code[pos_code];
                  offset_addr = ( v1 << 8 ) | v2;
                  /*compge aux_st-1, 0, aux_st-1*/
                  instruction.type = RTYPE;
                  instruction.rb = instruction.rc = stack_top - 1 + OFFSET_AUX_VARS;
                  instruction.ra = REG0;
                  instruction.op = RTYPE_OP;
                  instruction.opx = CMPGE_OPX;
                  instruction.imm5 = instruction.imm16 = instruction.imm16 = 0x00;
                  int_code[code_position] = instruction;
                  code_position++;
                  common_if( offset_addr );
                  /*stack top pointer update*/
                  stack_top--;
                  break;
      /* -------------------------------------------------------------------- */
      /* bytecode IF_ICMPEQ*/
      case IF_ICMPEQ :  pos_code++;
                        v1 = (int) (s1) code->code[pos_code];
                        pos_code++;
                        v2 = (int) (s1) code->code[pos_code];
                        offset_addr = ( v1 << 8 ) | v2;
                        /*compeq aux_st-2, aux_st-2, aux_st-1*/
                        instruction.type = RTYPE;
                        instruction.ra = instruction.rc = stack_top - 2 + OFFSET_AUX_VARS;
                        instruction.rb = stack_top - 1 + OFFSET_AUX_VARS;
                        instruction.op = RTYPE_OP;
                        instruction.opx = CMPEQ_OPX;
                        instruction.imm5 = instruction.imm16 = instruction.imm16 = 0x00;
                        int_code[code_position] = instruction;
                        code_position++;
                        common_if_icmp( offset_addr );
                        /*stack top pointer update*/
                        stack_top -= 2;
                        break;
      /* bytecode IF_ICMPNE*/
      case IF_ICMPNE :  pos_code++;
                        v1 = (int) (s1) code->code[pos_code];
                        pos_code++;
                        v2 = (int) (s1) code->code[pos_code];
                        offset_addr = ( v1 << 8 ) | v2;
                        /*compne aux_st-2, aux_st-2, aux_st-1*/
                        instruction.type = RTYPE;
                        instruction.ra = instruction.rc = stack_top - 2 + OFFSET_AUX_VARS;
                        instruction.rb = stack_top - 1 + OFFSET_AUX_VARS;
                        instruction.op = RTYPE_OP;
                        instruction.opx = CMPNE_OPX;
                        instruction.imm5 = instruction.imm16 = instruction.imm16 = 0x00;
                        int_code[code_position] = instruction;
                        code_position++;
                        common_if_icmp( offset_addr );
                        /*stack top pointer update*/
                        stack_top -= 2;
                        break;
      /* bytecode IF_ICMPLT*/
      case IF_ICMPLT :  pos_code++;
                        v1 = (int) (s1) code->code[pos_code];
                        pos_code++;
                        v2 = (int) (s1) code->code[pos_code];
                        offset_addr = ( v1 << 8 ) | v2;
                        /*complt aux_st-2, aux_st-2, aux_st-1*/
                        instruction.type = RTYPE;
                        instruction.ra = instruction.rc = stack_top - 2 + OFFSET_AUX_VARS;
                        instruction.rb = stack_top - 1 + OFFSET_AUX_VARS;
                        instruction.op = RTYPE_OP;
                        instruction.opx = CMPLT_OPX;
                        instruction.imm5 = instruction.imm16 = instruction.imm16 = 0x00;
                        int_code[code_position] = instruction;
                        code_position++;
                        common_if_icmp( offset_addr );
                        /*stack top pointer update*/
                        stack_top -= 2;
                        break;
      /* bytecode IF_ICMPGE*/
      case IF_ICMPGE :  pos_code++;
                        v1 = (int) (s1) code->code[pos_code];
                        pos_code++;
                        v2 = (int) (s1) code->code[pos_code];
                        offset_addr = ( v1 << 8 ) | v2;
                        /*compge aux_st-2, aux_st-2, aux_st-1*/
                        instruction.type = RTYPE;
                        instruction.ra = instruction.rc = stack_top - 2 + OFFSET_AUX_VARS;
                        instruction.rb = stack_top - 1 + OFFSET_AUX_VARS;
                        instruction.op = RTYPE_OP;
                        instruction.opx = CMPGE_OPX;
                        instruction.imm5 = instruction.imm16 = instruction.imm16 = 0x00;
                        int_code[code_position] = instruction;
                        code_position++;
                        common_if_icmp( offset_addr );
                        /*stack top pointer update*/
                        stack_top -= 2;
                        break;
      /* bytecode IF_ICMPGT*/
      case IF_ICMPGT :  pos_code++;
                        v1 = (int) (s1) code->code[pos_code];
                        pos_code++;
                        v2 = (int) (s1) code->code[pos_code];
                        offset_addr = ( v1 << 8 ) | v2;
                        /*complt aux_st-2, aux_st-1, aux_st-2*/
                        instruction.type = RTYPE;
                        instruction.rb = instruction.rc = stack_top - 2 + OFFSET_AUX_VARS;
                        instruction.ra = stack_top - 1 + OFFSET_AUX_VARS;
                        instruction.op = RTYPE_OP;
                        instruction.opx = CMPLT_OPX;
                        instruction.imm5 = instruction.imm16 = instruction.imm16 = 0x00;
                        int_code[code_position] = instruction;
                        code_position++;
                        common_if_icmp( offset_addr );
                        /*stack top pointer update*/
                        stack_top -= 2;
                        break;
      /* bytecode IF_ICMPLE*/
      case IF_ICMPLE :  pos_code++;
                        v1 = (int) (s1) code->code[pos_code];
                        pos_code++;
                        v2 = (int) (s1) code->code[pos_code];
                        offset_addr = ( v1 << 8 ) | v2;
                        /*compge aux_st-2, aux_st-1, aux_st-2*/
                        instruction.type = RTYPE;
                        instruction.rb = instruction.rc = stack_top - 2 + OFFSET_AUX_VARS;
                        instruction.ra = stack_top - 1 + OFFSET_AUX_VARS;
                        instruction.op = RTYPE_OP;
                        instruction.opx = CMPGE_OPX;
                        instruction.imm5 = instruction.imm16 = instruction.imm16 = 0x00;
                        int_code[code_position] = instruction;
                        code_position++;
                        common_if_icmp( offset_addr );
                        /*stack top pointer update*/
                        stack_top -= 2;
                        break;
      /* -------------------------------------------------------------------- */
      /* bytecode GOTO*/
      case GOTO:  pos_code++;
                  v1 = (int) (s1) code->code[pos_code];
                  pos_code++;
                  v2 = (int) (s1) code->code[pos_code];
                  offset_addr = ( v1 << 8 ) | v2;     
                  /*br (pos_code+offset_addr)*4*/
                  instruction.type = ITYPE;
                  instruction.ra = instruction.rb = instruction.rc = 0;
                  instruction.op = BR_OP;
                  instruction.opx = ITYPE_OPX;
                  instruction.imm5 = instruction.imm26 = 0x00;
                  if ( map_addr[pos_code-2+offset_addr] != -1 ){
                     instruction.imm16 = map_addr[pos_code-2+offset_addr] - ( code_position + 1 ) * 4;
                  }else{
                     instruction.imm16 = offset_addr;
                     instruction.imm26 = pos_code - 2;
                     /* store instruction index for later modification*/
                     delayed[delay_last] = code_position;
                     delay_last++;
                  }
                  int_code[code_position] = instruction;
                  code_position++;
                  break;
      
      /* -------------------------------------------------------------------- */
      /* these 2 are variable-length bytecodes*/
      /* bytecode TABLESWITCH*/
      case TABLESWITCH: 
                        break;
      /* bytecode LOOKUPSWITCH*/
      case LOOKUPSWITCH :  
                           break;
      
      /* -------------------------------------------------------------------- */
      /* bytecode RETURN*/
      case RETURN :  /*br the_end*/
                     instruction.type = ITYPE;
                     instruction.ra = instruction.rb = instruction.rc = 0;
                     instruction.op = BR_OP;
                     instruction.opx = ITYPE_OPX;
                     instruction.imm5 = 0x00;
                     instruction.imm16 = -1;
                     instruction.imm26 = pos_code;
                     /* store instruction index for later modification*/
                     delayed[delay_last] = code_position;
                     delay_last++;
                     int_code[code_position] = instruction;
                     code_position++;
                     break;
      /* -------------------------------------------------------------------- */
      /* bytecode NEWARRAY*/
      case NEWARRAY  :  pos_code++;
                        /* takes the array type from parameter*/
                        v2 = (int) code->code[pos_code];
                        /*...*/
                        /* MUST HAVE NEW CODE HERE WHEN NEW TYPES WERE SUPPORTED*/
                        /*...*/
                        /* takes the number of elements from the stack*/
                        v1 = operand_stack[stack_top].value;
                        /* muli aux_st-1, aux_st-1, 4 (len. INT)*/
                        instruction.type = ITYPE;
                        instruction.ra = stack_top - 1 + OFFSET_AUX_VARS;
                        instruction.rb = stack_top - 1 + OFFSET_AUX_VARS;
                        instruction.rc = 0;
                        instruction.op = MULI_OP;
                        instruction.opx = ITYPE_OPX;
                        instruction.imm5 = 0x00;
                        instruction.imm16 = 0x04;
                        instruction.imm26 = 0x00;
                        int_code[code_position] = instruction;
                        code_position++;
                        /* add aux_st, aux_st-1, array_length_reg*/
                        instruction.type = RTYPE;
                        instruction.ra = stack_top - 1 + OFFSET_AUX_VARS;
                        instruction.rb = ARRAY_LENGTH_REG;
                        instruction.rc = stack_top + OFFSET_AUX_VARS;
                        instruction.op = RTYPE_OP;
                        instruction.opx = ADD_OPX;
                        instruction.imm5 = 0x00;
                        instruction.imm16 = instruction.imm26 = 0x00;
                        int_code[code_position] = instruction;
                        code_position++;
                        /* movi array_length_reg, aux_st-1 -> addi array_length_reg, aux_st-1, 0*/
                        instruction.type = ITYPE;
                        instruction.ra = stack_top - 1 + OFFSET_AUX_VARS;
                        instruction.rb = ARRAY_LENGTH_REG;
                        instruction.rc = 0;
                        instruction.op = ADDI_OP;
                        instruction.opx = ITYPE_OPX;
                        instruction.imm5 = 0x00;
                        instruction.imm16 = instruction.imm26 = 0x00;
                        int_code[code_position] = instruction;
                        code_position++;
                        /* movi aux_st, aux_st+1 -> addi aux_st, aux_st+1, 0*/
                        instruction.type = ITYPE;
                        instruction.ra = stack_top - 1 + OFFSET_AUX_VARS;
                        instruction.rb = stack_top + OFFSET_AUX_VARS;
                        instruction.rc = 0;
                        instruction.op = ADDI_OP;
                        instruction.opx = ITYPE_OPX;
                        instruction.imm5 = 0x00;
                        instruction.imm16 = instruction.imm26 = 0x00;
                        int_code[code_position] = instruction;
                        code_position++;
                        break;
      /* bytecode MULTIANEWARRAY*/
      case MULTIANEWARRAY  :  
                              break;
      /* bytecode ARRAYLENGTH*/
      case ARRAYLENGTH  :  
                           break;
      /* bytecode IFNULL*/
      case IFNULL :  
                     break;
      /* bytecode IFNONNULL*/
      case IFNONNULL :  
                        break;
      /* -------------------------------------------------------------------- */
      /* bytecode WIDE*/
      case WIDE:  
                  break;
      /* bytecode GOTO_W*/
      case GOTO_W :  
                     break;
      /* -------------------------------------------------------------------- */
      default: return -1;
   }
   
   return 0;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/** Function to process the delayed instructions for correct addressing
  * Returns: - 0, for successful execution;
             - 1, for failure;
  * Arguments:
  * * [none]
  */
char process_delayed ( void ){
   register u4 i;
   
   for ( i = 0; i < delay_last; i++ ){
      if ( int_code[delayed[i]].imm16 == -1 ){
         /* the RETURN bytecode implementation*/
         if ( map_addr[int_code[delayed[i]].imm26] == -1 ) return 1;
         int_code[delayed[i]].imm16 = code_position * 4 - map_addr[int_code[delayed[i]].imm26];
      }else{
         /* other bytecodes implementations*/
         if ( map_addr[int_code[delayed[i]].imm26+int_code[delayed[i]].imm16] == -1 ) return 1;
         int_code[delayed[i]].imm16 = map_addr[int_code[delayed[i]].imm26+int_code[delayed[i]].imm16] - ( delayed[i] + 1 ) * 4;
      }
   }
   
   return 0;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/** Function to develop common operations over comparision bytecodes
  * Returns: [none]
  * Arguments:
  * * offset_addr: the offset adrress computed for the instruction
  */
void common_if_icmp ( s2 offset_addr ){
   TInstr instruction;
   
   /*movi aux_st-1, 1 -> addi aux_st-1, 0, 1*/
   instruction.type = ITYPE;
   instruction.ra = REG0;
   instruction.rb = stack_top - 1 + OFFSET_AUX_VARS;
   instruction.rc = 0;
   instruction.op = ADDI_OP;
   instruction.opx = ITYPE_OPX;
   instruction.imm5 = instruction.imm26 = 0x00;
   instruction.imm16 = 1;
   int_code[code_position] = instruction;
   code_position++;
   
   /*beq aux_st-2, aux_st-1, offset_addr*/
   instruction.type = ITYPE;
   instruction.ra = stack_top - 2 + OFFSET_AUX_VARS;
   instruction.rb = stack_top - 1 + OFFSET_AUX_VARS;
   instruction.rc = 0;
   instruction.op = BEQ_OP;
   instruction.opx = ITYPE_OPX;
   instruction.imm5 = instruction.imm26 = 0x00;
   if ( map_addr[pos_code-2+offset_addr] != -1 ){
      instruction.imm16 = map_addr[pos_code-2+offset_addr] - ( code_position + 1 ) * 4;
   }else{
      instruction.imm16 = offset_addr;
      instruction.imm26 = pos_code - 2;
      /* store instruction index for later modification*/
      delayed[delay_last] = code_position;
      delay_last++;
   }
   
   int_code[code_position] = instruction;
   code_position++;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/** Function to develop common operations over comparision with zero bytecodes
  * Returns: [none]
  * Arguments:
  * * offset_addr: the offset adrress computed for the instruction
  */
void common_if ( s2 offset_addr ){
   TInstr instruction;
   
   /*movi aux_st, 1 -> addi aux_st, 0, 1*/
   instruction.type = ITYPE;
   instruction.ra = REG0;
   instruction.rb = stack_top + OFFSET_AUX_VARS;
   instruction.rc = 0;
   instruction.op = ADDI_OP;
   instruction.opx = ITYPE_OPX;
   instruction.imm5 = instruction.imm26 = 0x00;
   instruction.imm16 = 1;
   int_code[code_position] = instruction;
   code_position++;
   
   /*beq aux_st-1, aux_st, offset_addr*/
   instruction.type = ITYPE;
   instruction.ra = stack_top - 1 + OFFSET_AUX_VARS;
   instruction.rb = stack_top + OFFSET_AUX_VARS;
   instruction.rc = 0;
   instruction.op = BEQ_OP;
   instruction.opx = ITYPE_OPX;
   instruction.imm5 = instruction.imm26 = 0x00;
   if ( map_addr[pos_code-2+offset_addr] != -1 ){
      instruction.imm16 = map_addr[pos_code-2+offset_addr] - ( code_position + 1 ) * 4;
   }else{
      instruction.imm16 = offset_addr;
      instruction.imm26 = pos_code - 2;
      /* store instruction index for later modification*/
      delayed[delay_last] = code_position;
      delay_last++;
   }
   
   int_code[code_position] = instruction;
   code_position++;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/** Function to return the intermediate code generated
  * Returns: the set of intemediate instructions
  * Arguments:
  * * *code_length: the code's length to be returned by reference
  */
TInstr* bypass_get_code ( u4 *code_length ){
   *code_length = code_position;
   return int_code;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/** Function to take off unnecessary instructions
  * Returns: [none]
  * Arguments:
  * * [none]
  */
void bypass_optimize ( void ){
   
}
