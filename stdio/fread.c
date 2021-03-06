/*      $NetBSD: fread.c,v 1.6 1995/02/02 02:09:34 jtc Exp $    */

/*-
 * Copyright (c) 1990, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
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
#if 0
static char sccsid[] = "@(#)fread.c     8.2 (Berkeley) 12/11/93";
#endif
static char rcsid[] = "$NetBSD: fread.c,v 1.6 1995/02/02 02:09:34 jtc Exp $";
#endif /* LIBC_SCCS and not lint */

#define _KERNEL
#include "ixemul.h"

#include <stdio.h>
#include <string.h>

#define LARGEREADS	0

//#if LARGEREADS
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "local.h"

int
lflush(fp)
	FILE *fp;
{

	if ((fp->_flags & (__SLBF|__SWR)) == (__SLBF|__SWR))
	{
		return (__sflush(fp));
	}
	return (0);
}

//#endif

size_t
fread(buf, size, count, fp)
	void *buf;
	size_t size, count;
	register FILE *fp;
{
	register size_t resid;
	register char *p;
	register int r;
	size_t total;
	#if LARGEREADS
	usetup;
	#endif

	/*
	 * The ANSI standard requires a return value of 0 for a count
	 * or a size of 0.  Peculiarily, it imposes no such requirements
	 * on fwrite; it only requires fread to be broken.
	 */
	if ((resid = count * size) == 0)
		return (0);
	if (fp->_r < 0)
		fp->_r = 0;
	total = resid;
	p = buf;
#if LARGEREADS
	r = fp->_r;
	/* TODO: should limit to specific buffering modes? - Piru */
	if (resid >= r + fp->_bf._size) {

		/* empty current buffer first, if any - Piru */
		if (r) {
			(void)memcpy((void *)p, (void *)fp->_p, r);
			fp->_r = 0;
			fp->_p += r;
			p += r;
			resid -= r;
		}

		/* If still unfilled area, continue */
		if (resid) {
			int actual;

			/* SysV does not make this test; take it out for compatibility */
			if (fp->_flags & __SEOF)
				return ((total - resid) / size);

			/* if not already reading, have to be reading and writing */
			if ((fp->_flags & __SRD) == 0) {
				if ((fp->_flags & __SRW) == 0) {
					errno = EBADF;
					return ((total - resid) / size);
				}
				/* switch to reading */
				if (fp->_flags & __SWR) {
					if (__sflush(fp))
						return ((total - resid) / size);
					fp->_flags &= ~__SWR;
					fp->_w = 0;
					fp->_lbfsize = 0;
				}
				fp->_flags |= __SRD;
			} else {
				/*
				 * We were reading.  If there is an ungetc buffer,
				 * we must have been reading from that. Drop it,
				 * restoring the previous buffer (if any). If there
				 * is anything in that buffer, read from it.
				 */
				if (HASUB(fp)) {
					FREEUB(fp);
					if ((fp->_r = fp->_ur) != 0) {
						fp->_p = fp->_up;

						r = fp->_r < resid ? fp->_r : resid;
						(void)memcpy((void *)p, (void *)fp->_p, r);
						fp->_r -= r;
						fp->_p += r;
						p += r;
						resid -= r;
					}
				}
			}

			/* If still unfilled area, continue */
			if (resid) {
				/*
				 * Before reading from a line buffered or unbuffered file,
				 * flush all line buffered output files, per the ANSI C
				 * standard.
				 */
				if (fp->_flags & (__SLBF|__SNBF)) {
					(void) _fwalk(lflush);
				}

				/*
				 * Now do the large read operation
				 */
				actual = (*fp->_read)(fp->_cookie, (char *)p, resid);
				if (actual <= 0) {
					if (actual == 0) {
						fp->_flags |= __SEOF;
					}
					else {
						fp->_flags |= __SERR;
					}
				}
				else {
					resid -= actual;
				}

				return ((total - resid) / size);
			}
		}
	}
	else {
#endif
	while (resid > (r = fp->_r)) {
		(void)memcpy((void *)p, (void *)fp->_p, (size_t)r);
		fp->_p += r;
		/* fp->_r = 0 ... done in __srefill */
		p += r;
		resid -= r;
		if (__srefill(fp)) {
			/* no more input: return partial result */
			return ((total - resid) / size);
		}
	}
	(void)memcpy((void *)p, (void *)fp->_p, resid);
	fp->_r -= resid;
	fp->_p += resid;
#if LARGEREADS
	}
#endif
	return (count);
}
