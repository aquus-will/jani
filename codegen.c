/* File with implementations for the code generator functions.
   Created: 26-feb-2007
   Last modified: 08-jul-2008
   Author: Willian dos Santos Lima*/

/* ========================================================================= */

#include "def.h"
#include "error.h"
#include "codegen.h"

/** Function to generate instruction words
  * Returns: - 0, for successful execution;
             - 1, for failure;
  * Arguments:
  * * *int_code: the intermediate code
  * * code_length: the intermediate code's length
  */
char code_gen ( const TInstr *int_code, u4 code_length ){
   register u4 i;
   u4 word, tmp_u4;
   u2 tmp_u2;
   u1 tmp_u1;
   FILE* _out;
   
   _out = fopen( output_, "wb" );
   
   for ( i = 0; i < code_length; i++ ){
      /* verifying instruction type to assemble word*/
      if ( int_code[i].type == ITYPE ){
         /*ITYPE word -> A, B, IMMED16, OP*/
         tmp_u4 = int_code[i].ra;
         word = ( tmp_u4 << 27 );
         tmp_u4 = int_code[i].rb;
         tmp_u4 = ( tmp_u4 << 22 );
         word = word + tmp_u4;
         tmp_u2 = int_code[i].imm16;
         tmp_u4 = ( tmp_u2 << 6 );
         word = word + tmp_u4;
         word = word + int_code[i].op;
      }else if ( int_code[i].type == JTYPE ){
         /*JTYPE word -> IMMED26, OP*/
         tmp_u4 = int_code[i].imm16;
         tmp_u4 = ( tmp_u4 << 6 );
         word = tmp_u4;
         word = word + int_code[i].op;
      }else if ( int_code[i].type == RTYPE ){
         /*RTYPE word -> A, B, C, OPX, IMM5, OP*/
         tmp_u4 = int_code[i].ra;
         word = ( tmp_u4 << 27 );
         tmp_u4 = int_code[i].rb;
         tmp_u4 = ( tmp_u4 << 22 );
         word = word + tmp_u4;
         tmp_u4 = int_code[i].rc;
         tmp_u4 = ( tmp_u4 << 17 );
         word = word + tmp_u4;
         tmp_u4 = int_code[i].opx;
         tmp_u4 = ( tmp_u4 << 11 );
         word = word + tmp_u4;
         tmp_u4 = int_code[i].imm5;
         tmp_u4 = ( tmp_u4 << 6 );
         word = word + tmp_u4;
         word = word + int_code[i].op;
      }else{
         /* no instruction type fetched*/
         if ( show_error( CODE_GEN, 1 ) )
            return 1;
      }
      
      /* recording inverted word in binary file*/
      tmp_u1 = word >> 24;
      if ( fwrite( &tmp_u1, sizeof(u1), 1, _out ) != 1 )
         if ( show_error( CODE_GEN, 2 ) )
            return 1;
      
      tmp_u1 = ( word << 8 ) >> 24;
      if ( fwrite( &tmp_u1, sizeof(u1), 1, _out ) != 1 )
         if ( show_error( CODE_GEN, 2 ) )
            return 1;
      
      tmp_u1 = ( word << 16 ) >> 24;
      if ( fwrite( &tmp_u1, sizeof(u1), 1, _out ) != 1 )
         if ( show_error( CODE_GEN, 2 ) )
            return 1;
      
      tmp_u1 = ( word << 24 ) >> 24;
      if ( fwrite( &tmp_u1, sizeof(u1), 1, _out ) != 1 )
         if ( show_error( CODE_GEN, 2 ) )
            return 1;
   }
   
   fclose( _out );
   
   return 0;
}
