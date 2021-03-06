/*
 * Copyright (c) 1983, 1988 Regents of the University of California.
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
static char sccsid[] = "@(#)syslog.c    5.34 (Berkeley) 6/26/91";
#endif /* LIBC_SCCS and not lint */

#define _KERNEL
#include "ixnet.h"
#include "my_varargs.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/syslog.h>
#include <sys/uio.h>
#include <sys/errno.h>
#include <netdb.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <paths.h>
#include <stdio.h>
#include <amitcp/socketbasetags.h>

/*
 * syslog, vsyslog --
 *      print message on log file; output is intended for syslogd(8).
 */

#ifndef NATIVE_MORPHOS

void
vsyslog(int pri, const char *fmt, va_list ap)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;

    if (network_protocol == IX_NETWORK_AMITCP) {
	TCP_SyslogA(pri,fmt,ap);
    }
    else if (network_protocol == IX_NETWORK_AS225) {
	static char buf[1024]; /* hopefully this is enough */

	vsprintf(buf,fmt,ap);
	SOCK_syslog(pri,buf);
    }
}

#else

#include <sys/syscall.h>

void
vsyslog(int pri, const char *fmt, va_list ap)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;

    char buf[1024 + 1];

    vsnprintf(buf,sizeof(buf) - 1,fmt,ap);

    buf[sizeof(buf) - 1] = '\0';

    if (network_protocol == IX_NETWORK_AMITCP)
	TCP_SyslogA(pri,buf,NULL);
    else
	SOCK_syslog(pri,buf);
}

void
_varargs68k_vsyslog(int pri, const char *fmt, char *ap)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;

    if (network_protocol == IX_NETWORK_AMITCP) {
	TCP_SyslogA(pri,fmt,ap);
    }
    else if (network_protocol == IX_NETWORK_AS225) {
	char buf[1024 + 1];

	/* call the 68k vsnprintf */

	int (*_sc)()=(void *)(&((char *)ixemulbase)[-((SYS_vsnprintf)+4)*6]);

	/* code for:
		movem.l d0-d3,-(sp)
		jsr     (a0)
		lea.l   16(sp),sp
		rts
	*/
	static const UWORD gate[] = {
	    0x48E7,0xF000,0x4E90,0x4FEF,0x0010,0x4E75
	};

	REG_D0 = (ULONG)buf;
	REG_D1 = (ULONG)(sizeof(buf) - 1);
	REG_D2 = (ULONG)fmt;
	REG_D3 = (ULONG)ap;
	REG_A0 = (ULONG)_sc;
	(void)MyEmulHandle->EmulCallDirect68k((APTR)gate);

	buf[sizeof(buf) - 1] = '\0';

	SOCK_syslog(pri,buf);
    }
}

#endif

void
openlog(const char *ident, int logstat, int logfac)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;

    if (network_protocol == IX_NETWORK_AMITCP) {
	struct TagItem list[] = {
	    {SBTM_SETVAL(SBTC_LOGTAGPTR), (ULONG)ident},
	    {SBTM_SETVAL(SBTC_LOGSTAT), logstat},
	    {SBTM_SETVAL(SBTC_LOGFACILITY), logfac},
	    {TAG_END}
	};
	TCP_SocketBaseTagList(list);
    }
}

void
closelog(void)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;

    if (network_protocol == IX_NETWORK_AMITCP) {
	struct TagItem list[] = {
	    {SBTM_SETVAL(SBTC_LOGTAGPTR), NULL},
	    {TAG_END}
	};
	TCP_SocketBaseTagList(list);
    }
}

/* setlogmask -- set the log mask level */
int
setlogmask(int pmask)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;

    if (network_protocol == IX_NETWORK_AMITCP) {
	struct TagItem lst[3]= {
	    {SBTM_GETVAL(SBTC_LOGMASK),NULL},
	    {SBTM_SETVAL(SBTC_LOGMASK), pmask},
	    {TAG_END}
	};

	TCP_SocketBaseTagList(lst);
	return (int)lst[0].ti_Data;
    }
    return 0;
}
