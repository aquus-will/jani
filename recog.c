/* File with implementations for .class file recognizing functions.
   Created: 23-apr-2007
   Last modified: 20-feb-2008
   Author: Willian dos Santos Lima*/

/* ========================================================================= */

#include "def.h"
#include "fileman.h"
#include "error.h"
#include "recog.h"

/* recognizer global variables declaration*/
TClassFile *recog_cf;
TCodeAttribute *recog_main_code;

/** Function for .class file recognizing
  * Returns: - 0, for successful execution;
             - 1, for failure;
  * Arguments:
  * * file_name: .class file name
  */
char recognize ( char* file_name ){
   register u4 i, j;
   u2 cp_index;
   char aux[U2_MAX+1], re;
   char found_main, found_code;
   FILE *_class;
   TClassFile cf;
   
   /* opening .class file*/
   if ( (_class = fopen( file_name, "rb" )) == NULL ){
      err_++;
      if ( show_error( RECOGNIZER, 1 ) )
         return 1;
   }
   
   recog_cf = malloc( sizeof(TClassFile) );
   if ( recog_cf == NULL ){   /* null pointer ----*/
      err_++;
      if ( show_error( RECOGNIZER, 5 ) )
         return 1;
   }/* -------------------------------------------*/
   cf = *recog_cf;
   
   re = 0;  /* reading errors identification*/
   
   /* reading the first 4 fields from .class file*/
   if ( !re ) re = fread4( &(cf.magic), _class );
   /* magic number validation*/
   if ( cf.magic != MAGIC_NUMBER ){
      if ( fclose( _class ) ){
         printf("WARNING: Cannot close .class file.\n"); /* file closing failure*/
         warn_++;
      }
      recog_mem_free_cf( recog_cf );
      err_++;
      if ( show_error( RECOGNIZER, 4 ) )
         return 1;
   }
   
   if ( !re ) re = fread2( &(cf.minor_version), _class );
   if ( !re ) re = fread2( &(cf.major_version), _class );
   if ( !re ) re = fread2( &(cf.constant_pool_count), _class );
   
   /* reading "constant pool"*/
   if ( !re ){
      *recog_cf = cf;
      recog_cf->constant_pool = malloc( cf.constant_pool_count * sizeof(TCpInfo) );
      if ( recog_cf->constant_pool == NULL ){   /* null pointer ----*/
         err_++;
         if ( show_error( RECOGNIZER, 5 ) )
            return 1;
      }/* ----------------------------------------------------------*/
      re = recog_constant_pool( cf.constant_pool_count, _class );
      if ( !re ) cf = *recog_cf;
   }
   
   /* reading more 3 simple fields from .class file*/
   if ( !re ) re = fread2( &(cf.access_flags), _class );
   if ( !re ) re = fread2( &(cf.this_class), _class );
   if ( !re ) re = fread2( &(cf.super_class), _class );
   
   /* reading "interfaces_count" and "interfaces"*/
   if ( !re ) re = fread2( &(cf.interfaces_count), _class );
   if ( !re ){
      cf.interfaces = malloc( cf.interfaces_count * sizeof(u2) );
      if ( cf.interfaces == NULL ){   /* null pointer ----*/
         err_++;
         if ( show_error( RECOGNIZER, 5 ) )
            return 1;
      }/* ------------------------------------------------*/
   }
   for ( i = 0; i < cf.interfaces_count && !re; i++ )
      re = fread2( &(cf.interfaces[i]), _class );
   
   /* reading "fields_count" and "fields"*/
   if ( !re ) re = fread2( &(cf.fields_count), _class );
   if ( !re ){
      *recog_cf = cf;
      recog_cf->fields = malloc( cf.fields_count * sizeof(TFieldInfo) );
      if ( recog_cf->fields == NULL ){   /* null pointer ----*/
         err_++;
         if ( show_error( RECOGNIZER, 5 ) )
            return 1;
      }/* ---------------------------------------------------*/
      re = recog_fields( cf.fields_count, _class );
      if ( !re ) cf = *recog_cf;
   }
   
   /* reading "methods_count" and "methods"*/
   if ( !re ) re = fread2( &(cf.methods_count), _class );
   if ( !re ){
      *recog_cf = cf;
      recog_cf->methods = malloc( cf.methods_count * sizeof(TMethodInfo) );
      if ( recog_cf->methods == NULL ){   /* null pointer ----*/
         err_++;
         if ( show_error( RECOGNIZER, 5 ) )
            return 1;
      }/* ----------------------------------------------------*/
      re = recog_methods( cf.methods_count, _class );
      if ( !re ) cf = *recog_cf;
   }
   
   /* reading "attributes_count" and "attributes"*/
   if ( !re ) re = fread2( &(cf.attributes_count), _class );
   if ( !re ){
      cf.attributes = malloc( cf.attributes_count * sizeof(TAttributeInfo) );
      if ( cf.attributes == NULL ){   /* null pointer ----*/
         err_++;
         if ( show_error( RECOGNIZER, 5 ) )
            return 1;
      }/* ------------------------------------------------*/
   }
   if ( !re ){
      *recog_cf = cf;
      re = recog_attributes( recog_cf->attributes, cf.attributes_count, _class );
      if ( !re ) cf = *recog_cf;
   }
   
   /* if there were no errors, it might print data*/
   if ( gravar_info_ && !re )
      recog_build_info( &cf, file_name );
   
   /* closing .class file*/
   if ( fclose( _class ) ){
      printf("WARNING: Cannot close .class file.\n"); /* file closing failure*/
      warn_++;
   }
   
   /* EXTRACTING BYTECODES FROM METHOD "main" ------------------------------*/
   if ( !re ){
      /* finding method whose name is "main"*/
      found_main = FALSE;
      for ( i = 0; i < cf.methods_count && !found_main; i++ ){
         cp_index = cf.methods[i].name_index;
         if ( recog_utf8_str( cp_index, aux ) ) continue;
         /* verifying whether this method is named "main"*/
         if ( s_eq( aux, "main" ) ){
            /* finding code attribute of method "main"*/
            found_code = FALSE;
            for ( j = 0; j < cf.methods[i].attributes_count && !found_code; j++ ){
               cp_index = cf.methods[i].attributes[j].attribute_name_index;
               if ( recog_utf8_str( cp_index, aux ) ) continue;
               /* verifying whether this attribute is named "Code"*/
               if ( s_eq( aux, "Code" ) ){
                  recog_main_code = malloc( sizeof(TCodeAttribute) );
                  if ( recog_main_code == NULL ){   /* null pointer ----*/
                     err_++;
                     if ( show_error( RECOGNIZER, 5 ) )
                        return 1;
                  }/* --------------------------------------------------*/
                  recog_code( i, j );
                  found_code = TRUE;
               }
            }
            /* no need to continue searching*/
            found_main = TRUE;
         }
      }
      if ( !found_main || !found_code ){
         recog_mem_free_cf( recog_cf );
         err_++;
         if ( show_error( RECOGNIZER, 3 ) )
            return 1;
      }
   }
   /* ----------------------------------------------------------------------*/
   
   /* freeing memory - maybe it will be delayed untill bypass phase finishes*/
   recog_mem_free_cf( recog_cf );   /* also disallocates dinamic structures of cf*/
   
   /* verifying whether all read operations were successful*/
   if ( re > 0 ){
      err_++;
      if ( show_error( RECOGNIZER, 2 ) )
         return 1;
   }else if ( re < 0 ){
      err_++;
      if ( show_error( RECOGNIZER, 5 ) )
         return 1;
   }
   
   return 0;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/** Function for constant_pool array recognizing
  * Returns: 0, for success; 1, for failure; -1, for malloc failure
  * Arguments:
  * * psize: size of constant_pool
  * * _fp: pointer for .class file
  */
char recog_constant_pool ( u2 psize, FILE *_fp ){
   register u2 i;
   u1 tag;
   char re = 0;   /* reading errors identification*/
   /* variables for use inside switch --------------------------------------*/
   u1 itag;
   u2 name_index, class_index, descriptor_index;
   u2 name_and_type_index, string_index;
   u4 bytes, high_bytes, low_bytes;
   u2 size, length;
   /* ----------------------------------------------------------------------*/
   
   i = 0;
   while ( ++i <= psize-1 && !re ){
      if ( fread( &tag, 1, 1, _fp ) != 1 ) re = 1;
      recog_cf->constant_pool[i].tag = tag;
      itag = tag;
      switch ( tag ){
         case CONSTANT_Class: {
            /* u1 tag; u2 name_index;*/
            if ( !re ) re = fread2( &name_index, _fp );
            size = sizeof( struct{u2 ni;} );
            recog_cf->constant_pool[i].info = malloc( size );
            if ( recog_cf->constant_pool[i].info == NULL )
               return -1;
            recog_cf->constant_pool[i].info[0] = (u1) name_index >> 8;   /* high order bits*/
            recog_cf->constant_pool[i].info[1] = (u1) name_index;        /* low order bits*/
            break;
         }
         case CONSTANT_Fieldref:
         case CONSTANT_Methodref:
         case CONSTANT_InterfaceMethodref: {
            /* u1 tag; u2 class_index; u2 name_and_type_index;*/
            if ( !re ) re = fread2( &class_index, _fp );
            if ( !re ) re = fread2( &name_and_type_index, _fp );
            size = sizeof( struct{u2 ci; u2 nti;} );
            recog_cf->constant_pool[i].info = malloc( size );
            if ( recog_cf->constant_pool[i].info == NULL )
               return -1;
            recog_cf->constant_pool[i].info[0] = (u1) class_index >> 8;   /* high order bits*/
            recog_cf->constant_pool[i].info[1] = (u1) class_index;        /* low order bits*/
            recog_cf->constant_pool[i].info[2] = (u1) name_and_type_index >> 8;   /* high order bits*/
            recog_cf->constant_pool[i].info[3] = (u1) name_and_type_index;        /* low order bits*/
            break;
         }
         case CONSTANT_String: {
            /* u1 tag; u2 string_index;*/
            if ( !re ) re = fread2( &string_index, _fp );
            size = sizeof( struct{u2 si;} );
            recog_cf->constant_pool[i].info = malloc( size );
            if ( recog_cf->constant_pool[i].info == NULL )
               return -1;
            recog_cf->constant_pool[i].info[0] = (u1) string_index >> 8;   /* high order bits*/
            recog_cf->constant_pool[i].info[1] = (u1) string_index;        /* low order bits*/
            break;
         }
         case CONSTANT_Integer:
         case CONSTANT_Float: {
            /* u1 tag; u4 bytes;*/
            if ( !re ) re = fread4( &bytes, _fp );
            size = sizeof( struct{u4 b;} );
            recog_cf->constant_pool[i].info = malloc( size );
            if ( recog_cf->constant_pool[i].info == NULL )
               return -1;
            recog_cf->constant_pool[i].info[0] = (u1) bytes >> 24;     /* highest order bits*/
            recog_cf->constant_pool[i].info[1] = (u1) bytes >> 16;
            recog_cf->constant_pool[i].info[2] = (u1) bytes >> 8;
            recog_cf->constant_pool[i].info[3] = (u1) bytes;           /* lowest order bits*/
            break;
         }
         case CONSTANT_Long:
         case CONSTANT_Double: {
            /* u1 tag; u4 high_bytes; u4 low_bytes;*/
            if ( !re ) re = fread4( &high_bytes, _fp );
            if ( !re ) re = fread4( &low_bytes, _fp );
            size = sizeof( struct{u4 hb; u4 lb;} );
            recog_cf->constant_pool[i].info = malloc( size );
            if ( recog_cf->constant_pool[i].info == NULL )
               return -1;
            recog_cf->constant_pool[i].info[0] = (u1) high_bytes >> 24;     /* highest order bits*/
            recog_cf->constant_pool[i].info[1] = (u1) high_bytes >> 16;
            recog_cf->constant_pool[i].info[2] = (u1) high_bytes >> 8;
            recog_cf->constant_pool[i].info[3] = (u1) high_bytes;           /* lowest order bits*/
            recog_cf->constant_pool[i].info[4] = (u1) low_bytes >> 24;     /* highest order bits*/
            recog_cf->constant_pool[i].info[5] = (u1) low_bytes >> 16;
            recog_cf->constant_pool[i].info[6] = (u1) low_bytes >> 8;
            recog_cf->constant_pool[i].info[7] = (u1) low_bytes;           /* lowest order bits*/
            /* corresponds to 2 table entries, increment i*/
            i++;
            break;
         }
         case CONSTANT_NameAndType: {
            /* u1 tag; u2 name_index; u2 descriptor_index;*/
            if ( !re ) re = fread2( &name_index, _fp );
            if ( !re ) re = fread2( &descriptor_index, _fp );
            size = sizeof( struct{u2 ni; u2 di;} );
            recog_cf->constant_pool[i].info = malloc( size );
            if ( recog_cf->constant_pool[i].info == NULL )
               return -1;
            recog_cf->constant_pool[i].info[0] = (u1) name_index >> 8;   /* high order bits*/
            recog_cf->constant_pool[i].info[1] = (u1) name_index;        /* low order bits*/
            recog_cf->constant_pool[i].info[2] = (u1) descriptor_index >> 8;   /* high order bits*/
            recog_cf->constant_pool[i].info[3] = (u1) descriptor_index;        /* low order bits*/
            break;
         }
         case CONSTANT_Utf8: {
            /* u1 tag; u2 length; u1 bytes[length]*/
            if ( !re ) re = fread2( &length, _fp );
            size = sizeof( struct{u2 l; u1 b[length];} );
            recog_cf->constant_pool[i].info = malloc( size );
            if ( recog_cf->constant_pool[i].info == NULL )
               return -1;
            recog_cf->constant_pool[i].info[0] = (u1) length >> 8;   /* high order bits*/
            recog_cf->constant_pool[i].info[1] = (u1) length;        /* low order bits*/
            if ( fread( &(recog_cf->constant_pool[i].info[2]), 1, length, _fp ) != length ) re = 1;
            break;
         }
      }
   }
   
   if ( re ) return 1;
   
   return 0;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/** Function for fields array recognizing
  * Returns: 0, for success; 1, for failure; -1, for malloc failure
  * Arguments:
  * * fsize: size of fields
  * * _fp: pointer for .class file
  */
char recog_fields ( u2 fsize, FILE *_fp ){
   register u2 i;
   u2 at_count;
   char re = 0;   /* reading errors identification*/
   
   i = 0;
   for (; i < fsize && !re; i++ ){
      if ( !re ) re = fread2( &(recog_cf->fields[i].access_flags), _fp );
      if ( !re ) re = fread2( &(recog_cf->fields[i].name_index), _fp );
      if ( !re ) re = fread2( &(recog_cf->fields[i].descriptor_index), _fp );
      if ( !re ) re = fread2( &(recog_cf->fields[i].attributes_count), _fp );
      if ( !re ){
         at_count = recog_cf->fields[i].attributes_count;
         recog_cf->fields[i].attributes = malloc( at_count * sizeof(TAttributeInfo) );
         if ( recog_cf->fields[i].attributes == NULL )
            return -1;
         re = recog_attributes ( recog_cf->fields[i].attributes, at_count, _fp );
      }
   }
   
   if ( re ) return 1;
   
   return 0;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/** Function for methods array recognizing
  * Returns: 0, for success; 1, for failure; -1, for malloc failure
  * Arguments:
  * * msize: size of methods
  * * _fp: pointer for .class file
  */
char recog_methods ( u2 msize, FILE *_fp ){
   register u2 i;
   u2 at_count;
   char re = 0;   /* reading errors identification*/
   
   i = 0;
   for (; i < msize && !re; i++ ){
      if ( !re ) re = fread2( &(recog_cf->methods[i].access_flags), _fp );
      if ( !re ) re = fread2( &(recog_cf->methods[i].name_index), _fp );
      if ( !re ) re = fread2( &(recog_cf->methods[i].descriptor_index), _fp );
      if ( !re ) re = fread2( &(recog_cf->methods[i].attributes_count), _fp );
      if ( !re ){
         at_count = recog_cf->methods[i].attributes_count;
         recog_cf->methods[i].attributes = malloc( at_count * sizeof(TAttributeInfo) );
         if ( recog_cf->methods[i].attributes == NULL )
            return -1;
         re = recog_attributes ( recog_cf->methods[i].attributes, at_count, _fp );
      }
   }
   
   if ( re ) return 1;
   
   return 0;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/** Function for attributes array recognizing
  * Returns: 0, for success; 1, for failure; -1, for malloc failure
  * Arguments:
  * * attr: attributes vector to be filled
  * * asize: size of attr
  * * _fp: pointer for .class file
  */
char recog_attributes ( TAttributeInfo *attr, u2 asize, FILE *_fp ){
   register u2 i, j;
   char re = 0;   /* reading errors identification*/
   
   i = 0;
   for (; i < asize && !re; i++ ){
      if ( !re ) re = fread2( &(attr[i].attribute_name_index), _fp );
      if ( !re ) re = fread4( &(attr[i].attribute_length), _fp );
      if ( !re ){
         attr[i].info = malloc( attr[i].attribute_length );
         if ( attr[i].info == NULL )
            return -1;
         for ( j = 0; j < attr[i].attribute_length && !re; j++ )
            if ( fread( &(attr[i].info[j]), 1, 1, _fp ) != 1 ) re = 1;
      }
   }
   
   if ( re ) return 1;
   
   return 0;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/** Function to get Code attribute
  * Returns: 0, for success; 1, for failure; -1, for malloc failure
  * Arguments:
  * * met_index: index of the method which bytecodes would be extracted
  * * attr_met_index: index inside "methods" where resides Code attribute
  */
char recog_code ( u2 met_index, u2 attr_met_index ){
   register u4 i, j;
   u4 tmp4, num4;
   
   /* getting the simple fields:*/
   /* the first 6 bytes that are common*/
   recog_main_code->attribute_name_index = recog_cf->methods[met_index].attributes[attr_met_index].attribute_name_index;
   recog_main_code->attribute_length = recog_cf->methods[met_index].attributes[attr_met_index].attribute_length;
   
   /* max_stack*/
   recog_main_code->max_stack = recog_cf->methods[met_index].attributes[attr_met_index].info[0];
   recog_main_code->max_stack = recog_main_code->max_stack << 8;
   recog_main_code->max_stack += recog_cf->methods[met_index].attributes[attr_met_index].info[1];
   
   /* max_locals*/
   recog_main_code->max_locals = recog_cf->methods[met_index].attributes[attr_met_index].info[2];
   recog_main_code->max_locals = recog_main_code->max_locals << 8;
   recog_main_code->max_locals += recog_cf->methods[met_index].attributes[attr_met_index].info[3];
   
   /* code_length*/
   recog_main_code->code_length = recog_cf->methods[met_index].attributes[attr_met_index].info[4];
   recog_main_code->code_length = recog_main_code->code_length << 24;
   num4 = recog_cf->methods[met_index].attributes[attr_met_index].info[5];
   recog_main_code->code_length += num4 << 16;
   num4 = recog_cf->methods[met_index].attributes[attr_met_index].info[6];
   recog_main_code->code_length += num4 << 8;
   recog_main_code->code_length += recog_cf->methods[met_index].attributes[attr_met_index].info[7];
   
   /* the code*/
   recog_main_code->code = malloc( recog_main_code->code_length );
   if ( recog_main_code->code == NULL )
      return -1;
   for ( i = 0; i < recog_main_code->code_length; i++ )
      recog_main_code->code[i] = recog_cf->methods[met_index].attributes[attr_met_index].info[8+i];
   
   tmp4 = 8 + i;
   
   /* exception_table_length*/
   recog_main_code->exception_table_length = recog_cf->methods[met_index].attributes[attr_met_index].info[tmp4++];
   recog_main_code->exception_table_length = recog_main_code->exception_table_length << 8;
   recog_main_code->exception_table_length += recog_cf->methods[met_index].attributes[attr_met_index].info[tmp4++];
   
   /* reading array referent to exception_table*/
   for ( i = 0; i < recog_main_code->exception_table_length; i++ ){
      /* start_pc*/
      recog_main_code->exception_table[i].start_pc = recog_cf->methods[met_index].attributes[attr_met_index].info[tmp4++];
      recog_main_code->exception_table[i].start_pc = recog_main_code->exception_table[i].start_pc << 8;
      recog_main_code->exception_table[i].start_pc += recog_cf->methods[met_index].attributes[attr_met_index].info[tmp4++];
      /* end_pc*/
      recog_main_code->exception_table[i].end_pc = recog_cf->methods[met_index].attributes[attr_met_index].info[tmp4++];
      recog_main_code->exception_table[i].end_pc = recog_main_code->exception_table[i].end_pc << 8;
      recog_main_code->exception_table[i].end_pc += recog_cf->methods[met_index].attributes[attr_met_index].info[tmp4++];
      /* handler_pc*/
      recog_main_code->exception_table[i].handler_pc = recog_cf->methods[met_index].attributes[attr_met_index].info[tmp4++];
      recog_main_code->exception_table[i].handler_pc = recog_main_code->exception_table[i].handler_pc << 8;
      recog_main_code->exception_table[i].handler_pc += recog_cf->methods[met_index].attributes[attr_met_index].info[tmp4++];
      /* catch_type*/
      recog_main_code->exception_table[i].catch_type = recog_cf->methods[met_index].attributes[attr_met_index].info[tmp4++];
      recog_main_code->exception_table[i].catch_type = recog_main_code->exception_table[i].catch_type << 8;
      recog_main_code->exception_table[i].catch_type += recog_cf->methods[met_index].attributes[attr_met_index].info[tmp4++];
   }
   
   /* informations about the code attributes*/
   /* attributes_count*/
   recog_main_code->attributes_count = recog_cf->methods[met_index].attributes[attr_met_index].info[tmp4++];
   recog_main_code->attributes_count = recog_main_code->attributes_count << 8;
   recog_main_code->attributes_count += recog_cf->methods[met_index].attributes[attr_met_index].info[tmp4++];
   
   /* reading array referent to attributes*/
   recog_main_code->attributes = malloc( recog_main_code->attributes_count * sizeof(TAttributeInfo) );
   if ( recog_main_code->attributes == NULL )
      return -1;
   for ( i = 0; i < recog_main_code->attributes_count; i++ ){
      /* attribute_name_index*/
      recog_main_code->attributes[i].attribute_name_index = recog_cf->methods[met_index].attributes[attr_met_index].info[tmp4++];
      recog_main_code->attributes[i].attribute_name_index = recog_main_code->attributes[i].attribute_name_index << 8;
      recog_main_code->attributes[i].attribute_name_index += recog_cf->methods[met_index].attributes[attr_met_index].info[tmp4++];
      /* attribute_length*/
      recog_main_code->attributes[i].attribute_length = recog_cf->methods[met_index].attributes[attr_met_index].info[tmp4++];
      recog_main_code->attributes[i].attribute_length = recog_main_code->attributes[i].attribute_length << 24;
      num4 = recog_cf->methods[met_index].attributes[attr_met_index].info[tmp4++];
      recog_main_code->attributes[i].attribute_length += num4 << 16;
      num4 = recog_cf->methods[met_index].attributes[attr_met_index].info[tmp4++];
      recog_main_code->attributes[i].attribute_length += num4 << 8;
      recog_main_code->attributes[i].attribute_length += recog_cf->methods[met_index].attributes[attr_met_index].info[tmp4++];
      /* info*/
      recog_main_code->attributes[i].info = malloc( recog_main_code->attributes[i].attribute_length );
      if ( recog_main_code->attributes[i].info == NULL )
         return -1;
      for ( j = 0; j < recog_main_code->attributes[i].attribute_length; j++ )
         recog_main_code->attributes[i].info[j] = recog_cf->methods[met_index].attributes[attr_met_index].info[tmp4++];
   }
   
   return 0;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/** Function for complete destruction of TClassFile* structure
  * Returns: (void)
  * Arguments:
  * * c: structure to be destroyed
  */
void recog_mem_free_cf ( TClassFile *c ){
   register u4 i, j;
   
   /* destroying constant_pool structure*/
   for ( i = 1; i < c->constant_pool_count; i++ )
      free( c->constant_pool[i].info );
   free( c->constant_pool );
   
   /* destroying interfaces structure*/
   free( c->interfaces );
   
   /* destroying fields structure*/
   for ( i = 0; i < c->fields_count; i++ ){
      for ( j = 0; j < c->fields[i].attributes_count; j++ )
         free( c->fields[i].attributes[j].info );
      free( c->fields[i].attributes );
   }
   free( c->fields );
   
   /* destroying methods structure*/
   for ( i = 0; i < c->methods_count; i++ ){
      for ( j = 0; j < c->methods[i].attributes_count; j++ )
         free( c->methods[i].attributes[j].info );
      free( c->methods[i].attributes );
   }
   free( c->methods );
   
   /* destroying attributes structure*/
   for ( i = 0; i < c->attributes_count; i++ )
      free( c->attributes[i].info );
   free( c->attributes );
   
   /* destroying primary structure*/
   free( c );
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/** Function get Utf8 string from constant_pool
  * Returns: 0, for success; 1, for failure
  * Arguments:
  * * index: the index in constant_pool table
  * * str: the desired string
  */
char recog_utf8_str ( u2 index, char *str ){
   u2 size;
   u4 i;
   
   if ( recog_cf->constant_pool[index].tag != CONSTANT_Utf8 ){
      strcpy( str, "" );
      return 1;
   }
   
   size = recog_cf->constant_pool[index].info[0];
   size = size << 8;
   size += recog_cf->constant_pool[index].info[1];
   for ( i = 0; i < size; i++ )
      str[i] = recog_cf->constant_pool[index].info[2+i];
   str[i] = 0;
   
   return 0;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/** Function to build info file for .class file
  * Returns: (void)
  * Arguments:
  * * c: class file structure to be printed
  * * file_name: .class file name
  */
void recog_build_info ( const TClassFile *c, char *file_name ){
   register u4 i, j;
   char aux[U1_MAX+1];
   FILE *_info;
   
   /* opening file where informations go*/
   strcpy( aux, file_name );
   strcat( aux, ".info" );
   if ( (_info = fopen( aux, "w" )) == NULL ){
      printf("WARNING: Cannot write .info file for .class file.\n");
      warn_++;
   }else printf("Generating .info file for .class file...\n");
   
   /* printing ClassFile*/
   fprintf(_info, "Class file properties:\n");
   fprintf(_info, "'magic = 0x%X\n",c->magic);
   fprintf(_info, "'version = %u.%u\n",c->major_version,c->minor_version);
   fprintf(_info, "'constant_pool_count = %u\n",c->constant_pool_count);
   fprintf(_info, "'constant_pool:\n");
   for ( i = 1; i < c->constant_pool_count; i++ )
      recog_show_cp_entry( i, _info );
   fprintf(_info, "'access_flags = 0x%04X\n",c->access_flags);
   fprintf(_info, "'this_class = %u\n",c->this_class);
   fprintf(_info, "'super_class = %u\n",c->super_class);
   fprintf(_info, "'interfaces_count = %u\n",c->interfaces_count);
   fprintf(_info, "'interfaces:\n");
   for ( i = 0; i < c->interfaces_count; i++ )
      fprintf(_info, "'%3u: cf.interfaces[%u] = %u\n",i,i,c->interfaces[i]);
   fprintf(_info, "'fields_count = %u\n",c->fields_count);
   for ( i = 0; i < c->fields_count; i++ ){
      fprintf(_info, "'%3u: cf.fields[%u].access_flags = %u\n",i,i,c->fields[i].access_flags);
      fprintf(_info, "'%3u: cf.fields[%u].name_index = %u\n",i,i,c->fields[i].name_index);
      fprintf(_info, "'%3u: cf.fields[%u].descriptor_index = %u\n",i,i,c->fields[i].descriptor_index);
      fprintf(_info, "'%3u: cf.fields[%u].attributes_count = %u\n",i,i,c->fields[i].attributes_count);
      for ( j = 0; j < c->fields[i].attributes_count; j++ ){
         fprintf(_info, "'%6u: cf.fields[%u].attributes[%u].attr_name_index = %u\n",i,i,j,c->fields[i].attributes[j].attribute_name_index);
         fprintf(_info, "'%6u: cf.fields[%u].attributes[%u].attr_length = %u\n",i,i,j,c->fields[i].attributes[j].attribute_length);
         /*fiels info, HERE*/
      }
   }
   fprintf(_info, "'methods_count = %u\n",c->methods_count);
   for ( i = 0; i < c->methods_count; i++ ){
      fprintf(_info, "'%3u: cf.methods[%u].access_flags = %u\n",i,i,c->methods[i].access_flags);
      fprintf(_info, "'%3u: cf.methods[%u].name_index = %u\n",i,i,c->methods[i].name_index);
      fprintf(_info, "'%3u: cf.methods[%u].descriptor_index = %u\n",i,i,c->methods[i].descriptor_index);
      fprintf(_info, "'%3u: cf.methods[%u].attributes_count = %u\n",i,i,c->methods[i].attributes_count);
      for ( j = 0; j < c->methods[i].attributes_count; j++ ){
         fprintf(_info, "'%6u: cf.methods[%u].attributes[%u].attr_name_index = %u\n",i,i,j,c->methods[i].attributes[j].attribute_name_index);
         fprintf(_info, "'%6u: cf.methods[%u].attributes[%u].attr_length = %u\n",i,i,j,c->methods[i].attributes[j].attribute_length);
         /*field info, HERE*/
      }
   }
   fprintf(_info, "'attributes_count = %u\n",c->attributes_count);
   for ( i = 0; i < c->attributes_count; i++ ){
      fprintf(_info, "'%3u: cf.attributes[%u].attr_name_index = %u\n",i,i,c->attributes[i].attribute_name_index);
      fprintf(_info, "'%3u: cf.attributes[%u].attr_length = %u\n",i,i,c->attributes[i].attribute_length);
      /*field info, HERE*/
   }
   
   /* closing file .class.info*/
   if ( fclose( _info ) ){
      printf("WARNING: Cannot close .info file for .class file.\n"); /* file closing failure*/
      warn_++;
   }
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/** Function for constant_pool fields exhibition
  * Returns: (void)
  * Arguments:
  * * index: constant_pool index to be showed
  * * _ip: information file pointer
  */
void recog_show_cp_entry ( u2 index, FILE *_ip ){
   u1 tag = recog_cf->constant_pool[index].tag;
   /* variables for use inside switch --------------------------------------*/
   register u4 j;
   u2 num2;
   u4 num4, tmp4;
   char aux[U2_MAX+1];
   /* ----------------------------------------------------------------------*/
   
   fprintf(_ip, "'%3u: cf.constant_pool[%u].tag = %u\n",index,index,tag);
   switch ( tag ){
      /* CONSTANT_Class_info*/
      case  CONSTANT_Class:
               fprintf(_ip, "\t''this entry is a class info with properties:\n");
               num2 = recog_cf->constant_pool[index].info[0];
               num2 = num2 << 8;
               num2 += recog_cf->constant_pool[index].info[1]; /*!*/
               fprintf(_ip, "\t''> name index: %u\n",num2);
               break;
      /* CONSTANT_Fieldref_info*/
      case  CONSTANT_Fieldref:
               fprintf(_ip, "\t''this entry is a field info with properties:\n");
               num2 = recog_cf->constant_pool[index].info[0];
               num2 = num2 << 8;
               num2 += recog_cf->constant_pool[index].info[1]; /*!*/
               fprintf(_ip, "\t''> class index: %u\n",num2);
               num2 = recog_cf->constant_pool[index].info[2];
               num2 = num2 << 8;
               num2 += recog_cf->constant_pool[index].info[3]; /*!*/
               fprintf(_ip, "\t''> name and type index: %u\n",num2);
               break;
      /* CONSTANT_Methodref_info*/
      case  CONSTANT_Methodref:
               fprintf(_ip, "\t''this entry is a method info with properties:\n");
               num2 = recog_cf->constant_pool[index].info[0];
               num2 = num2 << 8;
               num2 += recog_cf->constant_pool[index].info[1]; /*!*/
               fprintf(_ip, "\t''> class index: %u\n",num2);
               num2 = recog_cf->constant_pool[index].info[2];
               num2 = num2 << 8;
               num2 += recog_cf->constant_pool[index].info[3]; /*!*/
               fprintf(_ip, "\t''> name and type index: %u\n",num2);
               break;
      /* CONSTANT_InterfaceMethodref_info*/
      case  CONSTANT_InterfaceMethodref:
               fprintf(_ip, "\t''this entry is an interface method info with properties:\n");
               num2 = recog_cf->constant_pool[index].info[0];
               num2 = num2 << 8;
               num2 += recog_cf->constant_pool[index].info[1]; /*!*/
               fprintf(_ip, "\t''> class index: %u\n",num2);
               num2 = recog_cf->constant_pool[index].info[2];
               num2 = num2 << 8;
               num2 += recog_cf->constant_pool[index].info[3]; /*!*/
               fprintf(_ip, "\t''> name and type index: %u\n",num2);
               break;
      /* CONSTANT_String_info*/
      case  CONSTANT_String:
               fprintf(_ip, "\t''this entry is a string info with properties:\n");
               num2 = recog_cf->constant_pool[index].info[0];
               num2 = num2 << 8;
               num2 += recog_cf->constant_pool[index].info[1]; /*!*/
               fprintf(_ip, "\t''> string index: %u\n",num2);
               break;
      /* CONSTANT_Integer_info*/
      case  CONSTANT_Integer:
               fprintf(_ip, "\t''this entry is an integer number with properties:\n");
               num4 = recog_cf->constant_pool[index].info[0];
               num4 = num4 << 24;
               tmp4 = recog_cf->constant_pool[index].info[1];
               num4 += tmp4 << 16;
               tmp4 = recog_cf->constant_pool[index].info[2];
               num4 += tmp4 << 8;
               num4 += recog_cf->constant_pool[index].info[3]; /*!*/
               fprintf(_ip, "\t''> bytes: %u\n",num4);
               break;
      /* CONSTANT_Float_info: to be implemented*/
      case  CONSTANT_Float:
               /* further implementation*/
               fprintf(_ip, "\t''this entry is a float info.\n");
               break;
      /* CONSTANT_Long_info: to be implemented*/
      case  CONSTANT_Long:
               /* further implementation*/
               fprintf(_ip, "\t''this entry is a long info.\n");
               break;
      /* CONSTANT_Double_info: to be implemented*/
      case  CONSTANT_Double:
               /* further implementation*/
               fprintf(_ip, "\t''this entry is a double info.\n");
               break;
      /* CONSTANT_NameAndType_info*/
      case  CONSTANT_NameAndType:
               fprintf(_ip, "\t''this entry is a name and type info with properties:\n");
               num2 = recog_cf->constant_pool[index].info[0];
               num2 = num2 << 8;
               num2 += recog_cf->constant_pool[index].info[1]; /*!*/
               fprintf(_ip, "\t''> name index: %u\n",num2);
               num2 = recog_cf->constant_pool[index].info[2];
               num2 = num2 << 8;
               num2 += recog_cf->constant_pool[index].info[3]; /*!*/
               fprintf(_ip, "\t''> descriptor index: %u\n",num2);
               break;
      /* CONSTANT_Utf8_info*/
      case  CONSTANT_Utf8:
               fprintf(_ip, "\t''this entry is a string Ut8 with properties:\n");
               fprintf(_ip, "\t''> length: %u\n",recog_cf->constant_pool[index].info[1]);
               fprintf(_ip, "\t''> string: ");
               recog_utf8_str( index, aux );
               /*for ( j = 0; j < recog_cf->constant_pool[index].info[1]; j++ )
                  fprintf(_ip, "%c",recog_cf->constant_pool[index].info[2+j]);*/
               fprintf(_ip, "%s\n", aux);
               break;
   }

}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/** Function for getting the class file information
  * Returns: the class file in internal format
  * Arguments:
  * * [none]
  */
TClassFile* recog_get_class_file ( void ){
   return recog_cf;
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

/** Function for getting the main code
  * Returns: the main code represented by a code attribute structure
  * Arguments:
  * * [none]
  */
TCodeAttribute* recog_get_main_code ( void ){
   return recog_main_code;
}
