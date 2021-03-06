/*
 * Copyright (c) 1983 Regents of the University of California.
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
static char sccsid[] = "@(#)getnetent.c 5.8 (Berkeley) 2/24/91";
#endif /* LIBC_SCCS and not lint */

#define _KERNEL
#include "ixnet.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXALIASES      35

void
setnetent(int f)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;

    if (network_protocol == IX_NETWORK_AS225) {
	SOCK_setnetent(f);
	return;
    }
    else /* if (network_protocol == IX_NETWORK_AMITCP) */ {
	if (u.u_net_fp == NULL)
	    u.u_net_fp = fopen(_TCP_PATH_NETWORKS, "r" );
	else
	    rewind(u.u_net_fp);
	u.u_net_stayopen |= f;
    }
}

void
endnetent(void)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;

    if (p->u_networkprotocol == IX_NETWORK_AS225) {
	SOCK_endnetent();
	return;
    }

    if (u.u_net_fp) {
	fclose(u.u_net_fp);
	u.u_net_fp = NULL;
    }
    u.u_net_stayopen = 0;
}

struct netent *
getnetent(void)
{
    usetup;
    char *s;
    register char *cp, **q;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;

    if (network_protocol == IX_NETWORK_AS225) {
	return SOCK_getnetent();
    }
    else /* if (network_protocol == IX_NETWORK_AMITCP) */ {
	if (u.u_net_line == NULL)
	  u.u_net_line = malloc(BUFSIZ + 1);
	if (u.u_net_aliases == NULL)
	  u.u_net_aliases = malloc(MAXALIASES * sizeof(char *));
	if (u.u_net_line == NULL || u.u_net_aliases == NULL)
	  {
	    errno = ENOMEM;
	    return NULL;
	  }
	if (u.u_net_fp == NULL && (u.u_net_fp = fopen(_TCP_PATH_NETWORKS, "r" )) == NULL)
	    return (NULL);
again:
	if ((s = fgets(u.u_net_line, BUFSIZ, u.u_net_fp)) == NULL)
	    return (NULL);

	if (*s == '#')
	    goto again;

	cp = strpbrk(s, "#\n");
	if (cp == NULL)
	    goto again;

	*cp = '\0';
	u.u_net.n_name = s;
	cp = strpbrk(s, " \t");
	if (cp == NULL)
	    goto again;

	*cp++ = '\0';
	while (*cp == ' ' || *cp == '\t')
	    cp++;

	s = strpbrk(cp, " \t");
	if (s != NULL)
	    *s++ = '\0';

	u.u_net.n_net = ntohl(inet_makeaddr(inet_network(cp), 0).s_addr);
	u.u_net.n_addrtype = AF_INET;
	q = u.u_net.n_aliases = u.u_net_aliases;
	if (s != NULL) {
	    cp = s;
	    while (cp && *cp) {
		if (*cp == ' ' || *cp == '\t') {
		    cp++;
		    continue;
		}
		if (q < &u.u_net_aliases[MAXALIASES - 1])
		    *q++ = cp;

		cp = strpbrk(cp, " \t");
		if (cp != NULL)
		    *cp++ = '\0';
	    }
	}
	*q = NULL;
	return (&u.u_net);
    }
}
