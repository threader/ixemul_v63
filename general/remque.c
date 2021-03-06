/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/* remque(entry) */

#include "defs.h"

#ifndef mc68000

struct node2 {
    struct node2 *next;
    struct node2 *prev;
};

void remque(struct node2 *entry)
{
  struct node2 *p = entry->next;
  entry = entry->prev;
  p->prev = entry;
  entry->next = p;
}

#else

ENTRY(remque)
asm(" \n\
	movl    sp@(4),a0 \n\
	movl    a0@,a1 \n\ 
	movl    a0@(4),a0 \n\
	movl    a0,a1@(4) \n\
	movl    a1,a0@ \n\
	rts \n\
");

#endif
