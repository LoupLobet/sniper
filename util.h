#ifndef _UTIL_H_
#define _UTIL_H_

#include <string.h>
#include <stdio.h>

#define MIN(A, B) (((A) > (B)) ? (B) : (A))
#define MAX(A, B) (((A) < (B)) ? (B) : (A))
#define LEN(A) (sizeof(A)/sizeof(*A))

#define nil (void *)0

#define ARG (argc > 1 ? (argc--, *(++argv)): nil)
#define OPTS(body) \
	_setprogname(&progname, *argv);\
	argc--;\
	argv++;\
	while (argc > 0) {\
		if (**argv != '-')\
			break;\
		else {\
			for (char *opts = *argv + 1; *opts; opts++) {\
				switch (*opts) {\
					body\
				}\
			}\
			argc--;\
			argv++;\
		}\
	}

typedef long long vlong;

typedef signed char int8;
typedef signed short int16;
typedef signed int int32;
typedef signed long long int64;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long uvlong;

void	 sysfatal(const char *, ...);
void	 syslog(const char *, ...);
void	*emalloc(uint);
void	*erealloc(void *, uint);
long	 estrtol(const char *nptr, int base);

void	 _setprogname(char *);

extern char *_progname;

#endif
