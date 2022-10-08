/* Header file for the .class file recognizing.
   Created: 23-apr-2007
   Last modified: 19-feb-2008
   Author: Willian dos Santos Lima*/

/* ========================================================================= */
#ifndef RECOG
#define RECOG

/* ------------------------------------------------------------------------- */

/* function to open the file which will be compiled and initialize processing,
   this functon must extract only the bytecodes to be compiled and show the data
   at the processed .class file (printed to a file .info)*/
char recognize ( char* file_name );

/* function to identify all fields from "constant pool" of the .class file*/
char recog_constant_pool ( u2 psize, FILE *_fp );

/* function to identify all fields from "fields" of the .class file*/
char recog_fields ( u2 fsize, FILE *_fp );

/* function to identify all fields from "methods" of the .class file*/
char recog_methods ( u2 msize, FILE *_fp );

/* function to identify all fields from "attributes" of the .class file*/
char recog_attributes ( TAttributeInfo *attr, u2 asize, FILE *_fp );

/* function to get the bytecodes (Code attribute), given method "main"'s
   location and which of its attributes correspondes to the code*/
char recog_code ( u2 met_index, u2 attr_met_index );

/* ------------------------------------------------------------------------- */

/* function for complete memory disallocation refering to TClassFile structure*/
void recog_mem_free_cf ( TClassFile *c );

/* function for complete memory disallocation refering to TCodeAttribute
   structure*/
void recog_mem_free_code ( TCodeAttribute *c );

/* ------------------------------------------------------------------------- */

/* function to build a string from constant pool's Utf8 representation*/
char recog_utf8_str ( u2 index, char *str );

/* function to build .info information file*/
void recog_build_info ( const TClassFile *c, char *file_name );

/* function to show an entry from constant pool*/
void recog_show_cp_entry ( u2 index, FILE *_ip );

/* ------------------------------------------------------------------------- */
/* function to get the class file internal representation*/
TClassFile* recog_get_class_file ( void );

/* function to get the main code obtained by recognizer*/
TCodeAttribute* recog_get_main_code ( void );

/* ========================================================================= */
/* error codes for this phase:
   - 1, for file opening failure;
   - 2, for failure in file processing;
   - 3, didn't find method "main" or its code;
   - 4, wrong magic number;
   - 5, memory allocation failure
   - 6, null pointer*/

/* ========================================================================= */

#endif
