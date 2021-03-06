/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)setenv.c    5.6 (Berkeley) 6/4/91";
#endif /* LIBC_SCCS and not lint */

#define _KERNEL
#include "ixemul.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define environ (*u.u_environ)

/*
 * setenv --
 *      Set the value of the environmental variable "name" to be
 *      "value".  If rewrite is set, replace any current value.
 */
int setenv(const char *name, const char *value, int rewrite)
{
	usetup;
	register char *C;
	int l_value, offset;
	
	if (*value == '=')                      /* no `=' in value */
		++value;
	l_value = strlen(value);
	if ((C = _findenv(environ, name, &offset))) {   /* find if already exists */
		if (!rewrite)
			return (0);
		if (strlen(C) >= l_value) {     /* old larger; copy over */
			while ((*C++ = *value++)) ;
			return (0);
		}
	} else {                                        /* create new slot */
		register int    cnt;
		register char   **P;

		for (P = environ, cnt = 0; *P; ++P, ++cnt);
		if (u.u_env_alloced) {                  /* just increase size */
			environ = (char **)realloc((char *)environ,
			    (size_t)(sizeof(char *) * (cnt + 2)));
			if (!environ)
				return (-1);
		}
		else {                          /* get new space */
			u.u_env_alloced = 1;            /* copy old entries into it */
			P = (char **)malloc((size_t)(sizeof(char *) * (cnt + 2)));
			if (!P)
				return (-1);
			bcopy(environ, P, cnt * sizeof(char *));
			environ = P;
		}
		environ[cnt + 1] = NULL;
		offset = cnt;
	}
	for (C = (char *)name; *C && *C != '='; ++C);   /* no `=' in name */
	if (!(environ[offset] =                 /* name + `=' + value */
	    malloc((size_t)((int)(C - name) + l_value + 2))))
		return (-1);
	for (C = environ[offset]; (*C = *name++) && *C != '='; ++C)
		;
	for (*C++ = '='; (*C++ = *value++); )
		;
	return (0);
}

/*
 * unsetenv(name) --
 *      Delete environmental variable "name".
 */
void unsetenv(const char *name)
{
	usetup;
	register char **P;
	int offset;

	while (_findenv(environ, name, &offset))                /* if set multiple times */
		for (P = &environ[offset];; ++P)
			if (!(*P = *(P + 1)))
				break;
}
