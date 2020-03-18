/*
 * inttypes.h
 * 
 */
#ifdef WIN32

#ifndef INTTYPES_WIN
#define INTTYPES_WIN
typedef unsigned char   uint8_t;
typedef unsigned short  uint16_t;
typedef unsigned int    uint32_t;
/*
#if __WORDSIZE == 64
typedef unsigned long int   uint64_t; 
#else
typedef unsigned long long  uint64_t;
#endif
*/
typedef uint32_t   socklen_t;

#ifndef __attribute__
#define __attribute__(Spec) /*empty*/
#endif
#endif

#else  
/* Linux */
#include <inttypes.h>
#endif
/* end of inttypes.h */
