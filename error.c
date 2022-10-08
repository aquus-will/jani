/* File with implementations for the error and debugs handler functions
   Created: 20-jan-2008
   Last modified: 15-jul-2008
   Author: Willian dos Santos Lima*/

/* ========================================================================= */

#include "error.h"

/** Function for showing an error
  * Returns: 0, for common error; 1, for fatal error
  * Arguments:
  * * p: the compiler phase where the error ocurred
  * * id: the identification of the error inside its phase
  */
char show_error ( TPhase p, u1 id ){
   printf("ERROR");
   
   switch ( p ){
      /* .class file recognizer*/
      case RECOGNIZER:
         switch ( id ){
            case 1: printf(" [FATAL]: Cannot open .class file.\n");
                    return 1;
            case 2: printf(" [FATAL]: A fail ocurred when reading .class file.\n");
                    return 1;
            case 3: printf(" [FATAL]: Cannot find method \"main\" or its code.\n");
                    return 1;
            case 4: printf(" [FATAL]: The .class file is invalid. (wrong magic number)\n");
                    return 1;
            case 5: printf(" [FATAL]: A fail ocurred when trying to allocate memory.\n");
                    return 1;
            case 6: printf(" [FATAL]: Null pointer!\n");
                    return 1;
            /* unknown id*/
            default: UNKNOWN_ERROR;
                     return 0;
         }
      /* bytecodes bypasser*/
      case BYPASSER:
         switch ( id ){
            case 1: printf(": Not able to process a bytecode.\n");
                    return 0;
            case 2: printf(" [FATAL]: Not able to process a bytecode.\n");
                    return 1;
            case 3: printf(" [FATAL]: A fail ocurred when trying to allocate memory.\n");
                    return 1;
            case 4: printf(" [FATAL]: A fail ocurred when mapping instructions addresses.\n");
                    return 1;
            case 5: printf(" [FATAL]: Null pointer!\n");
                    return 1;
            /* unknown id*/
            default: UNKNOWN_ERROR;
                     return 0;
         }
      /* graph generator*/
      case GRAPH_GEN:
         switch ( id ){
            case 1: printf(" [FATAL]: (At CFG-gen) Unable to allocate memory for CFG.\n");
                    return 1;
            case 2: printf(" [FATAL]: (At CFG-gen) A fail ocurred when trying to get basic blocks.\n");
                    return 1;
            case 3: printf(" [FATAL]: (At DFG-gen) Unable to allocate memory for DFG.\n");
                    return 1;
            case 4: printf(" [FATAL]: (At CDG-gen) Unable to allocate memory for CDG.\n");
                    return 1;
            default: UNKNOWN_ERROR;
                     return 0;
         }
      /* register allocator*/
      case REGISTER_ALLOC:
         switch ( id ){
            case 1: printf(" [FATAL]: Error building interference graph.\n");
                    return 1;
            default: UNKNOWN_ERROR;
                     return 0;
         }
      /* code generator*/
      case CODE_GEN:
         switch ( id ){
            case 1: printf(" [FATAL]: Instruction fetching failure.\n");
                    return 1;
            case 2: printf(" [FATAL]: Error in recording word on binary file.\n");
                    return 1;
            /* unknown id*/
            default: UNKNOWN_ERROR;
                     return 0;
         }
      /* unknown phase*/
      default: UNKNOWN_ERROR;
   }
   
   return 0;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/** Function for showing an error
  * Returns: [none]
  * Arguments:
  * * *fname: source's file name
  * * ln: the line where debug command is
  */
void lndb ( char *fname, u4 ln ){
   printf("DEBUG INFO: At '%s' on line %u.\n",fname,ln);
}
