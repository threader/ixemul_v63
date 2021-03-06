/*
 * Copyright (c) 1985 Regents of the University of California.
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
static char sccsid[] = "@(#)getusershell.c      5.7 (Berkeley) 2/23/91";
#endif /* LIBC_SCCS and not lint */

#define _KERNEL
#include "ixemul.h"
#include <memory.h>
#include <ctype.h>
#include <stdio.h>
#include <sys/stat.h>

#define SHELLS "etc:shells"

/*
 * Do not add local shells here.  They should be added in /etc/shells
 */
static const char *okshells[] =
    { "/bin/sh", "/bin/csh", 0 };

static char **shells = NULL, *strings = NULL;
static char **curshell = NULL;
static char **initshells(void);

/*
 * Get a list of shells from SHELLS, if it exists.
 */
char *
getusershell(void) {
    char *ret;

    if (curshell == NULL)
	curshell = initshells();

    ret = *curshell;
    if (ret != NULL)
	curshell++;

    return (ret);
}

void
endusershell(void) {
    if (shells != NULL)
	syscall(SYS_free,(char *)shells);

    shells = NULL;
    if (strings != NULL)
	syscall(SYS_free,strings);

    strings = NULL;
    curshell = NULL;
}

void
setusershell(void) {
    curshell = initshells();
}

static char **
initshells(void) {
    register char **sp, *cp;
    register FILE *fp;
    struct stat statb;

    if (shells != NULL)
	syscall(SYS_free,(char *)shells);

    shells = NULL;
    if (strings != NULL)
	syscall(SYS_free,strings);

    strings = NULL;
    if ((fp = (FILE *)syscall(SYS_fopen,SHELLS, "r")) == (FILE *)0)
	return((char **)okshells);

    if (syscall(SYS_fstat,fileno(fp), &statb) == -1) {
	(void)syscall(SYS_fclose,fp);
	return((char **)okshells);
    }

    if ((strings = (char *)syscall(SYS_malloc,(unsigned)statb.st_size + 1)) == NULL) {
	(void)syscall(SYS_fclose,fp);
	return((char **)okshells);
    }

    shells = (char **)syscall(SYS_calloc,(unsigned)statb.st_size / 3, sizeof (char *));
    if (shells == NULL) {
	(void)syscall(SYS_fclose,fp);
	syscall(SYS_free,strings);
	strings = NULL;
	return((char **)okshells);
    }

    sp = shells;
    cp = strings;
    while (syscall(SYS_fgets,cp, MAXPATHLEN + 1, fp) != NULL) {
	while (isspace(*cp) && *cp != '\0')
	    cp++;

	if (*cp == '#' || *cp == '\0')
	    continue;

	*sp++ = cp;
	while (!isspace(*cp) && *cp != '#' && *cp != '\0')
	    cp++;

	*cp++ = '\0';
    }
    *sp = (char *)0;
    (void)syscall(SYS_fclose,fp);
    return (shells);
}
