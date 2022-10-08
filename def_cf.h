/* File containing types and definitions used in class file.
   Created: 24-apr-2007
   Last modified: 23-jan-2008
   Author: Willian dos Santos Lima*/

/* ========================================================================= */

/* definitions are valid when not reached yet by any other part of this project*/
#ifndef DEF_CF
#define DEF_CF

/* ------------------------------------------------------------------------- */

#define MAGIC_NUMBER 0xCAFEBABE

/* constants for field tag, used inside CPINFO*/
#define CONSTANT_Class 7
#define CONSTANT_Fieldref 9
#define CONSTANT_Methodref 10
#define CONSTANT_InterfaceMethodref 11
#define CONSTANT_String 8
#define CONSTANT_Integer 3
#define CONSTANT_Float 4
#define CONSTANT_Long 5
#define CONSTANT_Double 6
#define CONSTANT_NameAndType 12
#define CONSTANT_Utf8 1

/* ------------------------------------------------------------------------- */

/* constant_pool struct*/
typedef struct CPINFO{
   u1 tag;
   u1 *info;
}TCpInfo;

/* attribute_info struct*/
typedef struct ATTRIBUTEINFO{
   u2 attribute_name_index;
   u4 attribute_length;
   u1 *info;
}TAttributeInfo;

/* field_info struct*/
typedef struct FIELDINFO{
   u2 access_flags;
   u2 name_index;
   u2 descriptor_index;
   u2 attributes_count;
   TAttributeInfo *attributes;
}TFieldInfo;

/* method_info struct*/
typedef struct METHODINFO{
   u2 access_flags;
   u2 name_index;
   u2 descriptor_index;
   u2 attributes_count;
   TAttributeInfo *attributes;
}TMethodInfo;

/* ------------------------------------------------------------------------- */

/* struct for exception table*/
typedef struct EXCEPTIONENTRY{
   u2 start_pc;
   u2 end_pc;
   u2 handler_pc;
   u2 catch_type;
}TExceptionEntry;

/* struct for code attribute*/
typedef struct CODEATTRIBUTE{
   u2 attribute_name_index;
   u4 attribute_length;
   /* the fields above are common*/
   u2 max_stack;
   u2 max_locals;
   u4 code_length;
   u1 *code;      /* the so called 'bytecodes'!*/
   u2 exception_table_length;
   TExceptionEntry *exception_table;
   u2 attributes_count;
   TAttributeInfo *attributes;
}TCodeAttribute;

/* FURTHER IMPLEMENTATION: support other attributes:
   ConstantValue, Exceptions, InnerClasses, Synthetic, SourceFile,
   LineNumberTable, LocalVariableTable, Deprecated*/

/* ------------------------------------------------------------------------- */

/* struct for class file data storage*/
typedef struct CLASSFILE{
   u4 magic;
   u2 minor_version;
   u2 major_version;
   u2 constant_pool_count;
   TCpInfo *constant_pool;
   u2 access_flags;
   u2 this_class;
   u2 super_class;
   u2 interfaces_count;
   u2 *interfaces;
   u2 fields_count;
   TFieldInfo *fields;
   u2 methods_count;
   TMethodInfo *methods;
   u2 attributes_count;
   TAttributeInfo *attributes;
}TClassFile;

#endif
