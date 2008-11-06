#ifndef __ERR_H__
#define __ERR_H__

//#define ERR_VER	0x

#define	ERROR_OK		0
#define ERROR_UNK		-1
//bad exception code
#define ERROR_BAD_EXCODE	-50	

//no exception handler for that code
#define	ERROR_EXCODE_NOTFOUND	-51	

//already used
#define	ERROR_USED_EXCODE	-52	
#define	ERROR_INTR_CONTEXT	-100
#define ERROR_DOES_EXIST	-104
#define ERROR_DOESNOT_EXIST	-105
#define ERROR_NO_TIMER		-150
#define ERROR_NOT_IRX		-201
#define ERROR_FILE_NOT_FOUND	-203
#define ERROR_FILE_ERROR	-204
#define ERROR_NO_MEM		-400
#define ERROR_SEMACOUNT_ZERO	-400

// what should these be? Used in intrman.c
#define ERROR_ILLEGAL_INTRCODE ERROR_UNK
#define ERROR_CPUDI ERROR_UNK

#endif//__ERR_H__
