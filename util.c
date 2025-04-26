#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#include "util.h"

char *_progname = "";

void
sysfatal(const char *fmt, ...)
{
		va_list ap;

		va_start(ap, fmt);
		fprintf(stderr, "%s: ", _progname);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
		fputc('\n', stderr);
		exit(1);
}

void
syslog(const char *fmt, ...)
{
		va_list ap;

		va_start(ap, fmt);
		fprintf(stderr, "%s: ", _progname);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
		fputc('\n', stderr);
}

void *
emalloc(uint n)
{
		void *p = NULL;

		p = malloc(n);
		if (p == NULL)
			sysfatal("malloc");
		memset(p, 0, n);
		return p;
}

void *
erealloc(void *q, uint n)
{
		void *p = NULL;

		p = realloc(q, n);
		if (p == NULL)
			sysfatal("realloc");
		return p;
}

long
estrtol(const char *nptr, int base)
{
		char *endptr = NULL;

		errno = 0;
		long n;

		n = strtol(nptr, &endptr, base);
		if ((endptr == nptr) || ((n == LONG_MAX || n == LONG_MIN)
		&& errno == ERANGE))
			sysfatal("invalid integer: %s", nptr);
		return n;
}

void
_setprogname(char *s)
{
		_progname = s;
		if (!strncmp(s, "./", 2))
			*_progname += 2;
}
