/*  Support stuff for doing low level debugging of the library and/or
 *  runtime startup code using KPrintF.
 *
 *  Compile with CFLAGS="-O2 -DDEBUG_VERSION" to get debugging version.
 *
 *  FIXME: For some reason the -O2 is required, otherwise the system
 *  crashes.
 */

/*
 *  Use as:     KPRINTF (("foo = %s\n", bar));
 *  Prints:     /mysource/project/bell.c:45: foo = bell
 */

#ifndef __KPRINTF_H__
#define __KPRINTF_H__
//#define DEBUG_VERSION
#ifdef DEBUG_VERSION

#ifdef NATIVE_MORPHOS
#define KPrintF dprintf
#endif

#include <proto/exec___.h>
#include <exec/execbase.h>

extern void KPrintF(char *, ...);

#define KPRINTF_WHERE           do { KPrintF ("%08lx:%s:%ld: ", SysBase->ThisTask, __FILE__, __LINE__); } while (0)
#define KPRINTF_ARGS(a)         do { KPrintF a; } while (0)
#ifdef _KERNEL
#define KPRINTF(a)              do { if (ixemulbase && ixemulbase->ix_debug_flag) {KPRINTF_WHERE; KPRINTF_ARGS(a);} } while (0)
#else
#define KPRINTF(a)              do { KPRINTF_WHERE; KPRINTF_ARGS(a); } while (0)
#endif
#define KPRINTF_DISABLED(a)     do { Disable (); KPRINTF (a); Enable (); } while (0)

#define KPRINTF_ARGV(name,argv) \
  do { int argi; \
    for (argi = 0; (argv)[argi] != NULL; argi++) \
      KPRINTF (("%s[%ld] = [%s]\n", (name), argi, (argv)[argi])); \
  } while (0)

#else   /* not DEBUG_VERSION */

#define KPRINTF_WHERE                   /* Expands to nothing */
#define KPRINTF_ARGS(a)                 /* Expands to nothing */
#define KPRINTF(a)                      /* Expands to nothing */
#define KPRINTF_DISABLED(a)             /* Expands to nothing */
#define KPRINTF_ARGV(name,argv)         /* Expands to nothing */

#endif  /* DEBUG_VERSION */

#endif
