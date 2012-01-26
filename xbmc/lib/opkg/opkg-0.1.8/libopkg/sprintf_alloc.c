/* sprintf_alloc.c -- like sprintf with memory allocation

   Carl D. Worth

   Copyright (C) 2001 University of Southern California

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*/

#include <stdarg.h>

#include "sprintf_alloc.h"
#include "libbb/libbb.h"

int sprintf_alloc(char **str, const char *fmt, ...)
{
    va_list ap;
    int n;
    unsigned size = 100;

    if (!str) {
      opkg_msg(ERROR, "Internal error: str=NULL.\n");
      return -1;
    }
    if (!fmt) {
      opkg_msg(ERROR, "Internal error: fmt=NULL.\n");
      return -1;
    }

    /* On x86_64 systems, any strings over 100 were segfaulting.  
       It seems that the ap needs to be reinitalized before every
       use of the v*printf() functions. I pulled the functionality out
       of vsprintf_alloc and combined it all here instead. 
    */
    

    /* ripped more or less straight out of PRINTF(3) */

    *str = xcalloc(1, size);

    while(1) {
      va_start(ap, fmt);
      n = vsnprintf (*str, size, fmt, ap);
      va_end(ap);
      /* If that worked, return the size. */
      if (n > -1 && n < size)
	return n;
	/* Else try again with more space. */
	if (n > -1)    /* glibc 2.1 */
	    size = n+1; /* precisely what is needed */
	else           /* glibc 2.0 */
	    size *= 2;  /* twice the old size */
	*str = xrealloc(*str, size);
    }

    return -1; /* Just to be correct - it probably won't get here */
}
