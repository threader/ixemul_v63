# Generated automatically from Makefile.in by configure.
#### Start of system configuration section. ####

srcdir =        .

ifeq ($(srcdir),.)
srcdir = ../../..
endif

VPATH :=        $(srcdir)

# Common prefix for machine-independent installed files.
prefix =        /usr/local

# Common prefix for machine-dependent installed files.
exec_prefix =   ${prefix}

bindir =        $(exec_prefix)/bin
libdir =        $(exec_prefix)/Sys/libs

INSTALL =       /usr/bin/install -c
INSTALL_DATA =  ${INSTALL} -m 644

CC =            gcc

CFLAGS =        -O2
LDFLAGS =       

RANLIB =        ranlib
AR =            ar

NATIVE_CC =     gcc

NATIVE_CFLAGS = -O2
NATIVE_LDFLAGS= 

DO_CATENATE =   no

#### End system configuration section ####

# I *love* GNU make!
define catenate
/bin/echo -n creating $@...
/bin/echo "$(^:%=#include \"%\"\n)" >$@
/bin/echo done
endef

ifeq ($(BASE), morphos)
FLAVOR_CFLAGS = -mcpu=$(FPU)
FIXED_A4 = -ffixed-r13
else
FLAVOR_CFLAGS = -m$(CPU) -m$(FPU)
FIXED_A4 = -ffixed-a4
endif

INCS = -I$(srcdir) -I$(srcdir)/../library -I$(srcdir)/../include -I.

STD_CFLAGS = $(CFLAGS) $(INCS) $(OTHER_CFLAGS)

ALL_CFLAGS = $(FLAVOR_CFLAGS) $(STD_CFLAGS)

ifeq ($(OS),morphos)
NATIVE_FLAGS = $(NATIVE_CFLAGS) $(OTHER_CFLAGS) $(filter -DDEBUG_VERSION -DNOTRAP, $(ALL_CFLAGS)) -DNATIVE_MORPHOS -DCPU_$(FPU)
else
NATIVE_FLAGS = $(NATIVE_CFLAGS) $(OTHER_CFLAGS) $(filter -DDEBUG_VERSION -DNOTRAP, $(ALL_CFLAGS)) -DCPU_$(CPU) -DFPU_$(FPU)
endif

LIB =           library.a

# Strip the executable (-s), don't look for standard libraries (-nostdlib)
# don't link in crt0.o (-nostartfiles) and tell the linker explicitly to
# load the module from the library that defines the symbol "___load_seg"
# (-Xlinker -u -Xlinker ___load_seg).

ifeq ($(CPU), powerpc)
LINKFLAGS =     -nostdlib -nostartfiles -L/gg/ppc-morphos/lib/libnix/ -Xlinker -u -Xlinker __load_seg
else
LINKFLAGS =     -nostdlib -nostartfiles -Xlinker -u -Xlinker ___load_seg
endif

.c.o:
		$(CC) $(ALL_CFLAGS) -c $< -o $@

ifeq ($(CPU), powerpc)
all:            ixemul.library
else
all:            ixemul.library ixemul.trace ixemul.debug
endif


SRC_MISC =      parse_version.c create_header.c tracecntl.c
SRC_OTHERS =    hwck.c trap.c ix_timer.c debugstub.c gates.c start.c

ifeq ($(CPU), powerpc)
OBJ_OTHERS =    $(SRC_OTHERS:.c=.o)
SRC_A4 =        ix_startup.c _main.c ix_exec_entry.c exit.c ix_resident.c machdep.c
else
OBJ_OTHERS =    $(filter-out gates.o start.o, $(SRC_OTHERS:.c=.o))
SRC_A4 =        ix_startup_68k.c _main.c ix_exec_entry.c exit.c ix_resident_68k.c machdep_68k.c
endif

SRC =           $(filter-out $(SRC_A4) $(SRC_OTHERS) $(SRC_MISC),$(notdir $(wildcard $(srcdir)/*.c)))

# These are object files which explicitly get linked into the ixemul.library
# file, in the order specified

OBJ_N =         start.o tracecntl.o

OBJ_T =         start.ot tracecntl.ot

IXLIBS =        $(LIB) \
		../../../../string/$(CPU)/$(FPU)/$(OS)/libstring.a \
		../../../../general/$(CPU)/$(FPU)/$(OS)/libgeneral.a \
		../../../../stdlib/$(CPU)/$(FPU)/$(OS)/libstdlib.a \
		../../../../stdio/$(CPU)/$(FPU)/$(OS)/libstdio.a \
		../../../../wchar/$(CPU)/$(FPU)/$(OS)/libwchar.a

ifeq ($(OS),pos)
LIBS =          -lgcc /pos/pos_dev/poslib/List.lib
else
ifeq ($(OS),morphos)
LIBS =          -lgcc -labox -lsyscall -laboxstubs -lmoto -lm
else
LIBS =          -lgcc -lamiga -ldebug
endif
endif


ifeq ($(DO_CATENATE), yes)

SRC___ =        $(filter _%,$(SRC))
SRC_AH =        $(filter a% b% c% d% e% f% g% h%,$(SRC))
SRC_II =        $(filter i%,$(SRC))
SRC_JO =        $(filter j% k% l% m% n% o%,$(SRC))
SRC_PR =        $(filter p% q% r%,$(SRC))
SRC_SS =        $(filter s%,$(SRC))
SRC_TZ =        $(filter t% u% v% w% x% y% z%,$(SRC))

$(LIB) :        a4.o __.o ah.o ii.o jo.o pr.o ss.o tz.o $(OBJ_OTHERS)
		rm -f $@
		$(AR) rv $@ $^
		$(RANLIB) $@

a4.o:           a4.c
		$(CC) $(ALL_CFLAGS) $(FIXED_A4) -c $< -o $@

a4.c:           $(SRC_A4)
		@$(catenate)

__.c:           $(SRC___)
		@$(catenate)

ah.c:           $(SRC_AH)
		@$(catenate)

ii.c:           $(SRC_II)
		@$(catenate)

jo.c:           $(SRC_JO)
		@$(catenate)

pr.c:           $(SRC_PR)
		@$(catenate)

ss.c:           $(SRC_SS)
		@$(catenate)

tz.c:           $(SRC_TZ)
		@$(catenate)

ii.o tz.o:      version.h

# ix_open.c (and thus ii.o) depends on the size of struct user

ii.o:           $(srcdir)/../include/user.h

else

OBJS= $(SRC:.c=.o)
OBJS_A4= $(SRC_A4:.c=.o)

$(LIB):         $(OBJS_A4) $(OBJS) $(OBJ_OTHERS)
		rm -f $@
		$(AR) rv $@ $^
		$(RANLIB) $@

$(OBJS_A4): %.o: %.c
		$(CC) $(ALL_CFLAGS) $(FIXED_A4) -c $< -o $@

# ix_open.c (and thus ii.o) depends on the size of struct user
ifeq ($(CPU), powerpc)
ix_open.o:      $(srcdir)/../include/user.h
else
ix_open_68k.o:      $(srcdir)/../include/user.h
endif

ix_settings.o vfork.o: version.h

endif


ixemul.library: $(OBJ_N) $(IXLIBS)
		$(CC) $(LINKFLAGS) $(OBJ_N) $(IXLIBS) $(LIBS) -o $@

ixemul.trace:   $(OBJ_T) $(IXLIBS)
		$(CC) $(LINKFLAGS) $(OBJ_T) $(IXLIBS) $(LIBS) -o $@

ixemul.debug:   $(OBJ_N) $(IXLIBS)
		$(CC) -g $(LINKFLAGS) $(OBJ_N) $(IXLIBS) $(LIBS) -O0 -DDEBUG_VERSION -o $@

clean:
		rm -rf 680?0 notrap powerpc
		rm -f create_header parse_version

clobber:        clean
		rm -f Makefile

ifeq ($(CPU), powerpc)

start.o:        start.c version.h $(srcdir)/../include/sys/syscall.def ix_internals.h

start.ot:       start.c version.h $(srcdir)/../include/sys/syscall.def ix_internals.h
		$(CC) $(STD_CFLAGS) -DTRACE_LIBRARY -c $< -o $@

else

start.o:        start.S version.h $(srcdir)/../include/sys/syscall.def ix_internals.h
		$(CC) $(STD_CFLAGS) -traditional -c -m68000 -msoft-float $< -o $@

start.ot:       start.S version.h $(srcdir)/../include/sys/syscall.def ix_internals.h
		$(CC) $(STD_CFLAGS) -DTRACE_LIBRARY -traditional -c -m68000 -msoft-float $< -o $@

endif

# Force -m68020 & -m68881 so the assembler won't bitch about fpu
# instructions.  This means we have to leave out FLAVOR_CFLAGS.

ifneq ($(CPU), powerpc)

trap.o:         trap.S ix_internals.h
		$(CC) $(STD_CFLAGS) -traditional -c -m68020 -m68881 $< -o $@

endif

# Never omit the frame pointer.

ix_timer.o:     ix_timer.c
		$(CC) $(ALL_CFLAGS) -fno-omit-frame-pointer -c $< -o $@

# hwck.c checks for defines like mc68020 and __HAVE_68881__, but it also
# must be compiled with -m68000. So we first preprocess the source with
# standard flavor flags, and then we compile the resulting source with
# -m68000. This ensures that when we try to use a 68020 ixemul.library on
# a 68000 Amiga, we get a correct message and not a crash.

ifneq ($(CPU), powerpc)

hwck.o:         hwck.c
		$(CC) $(ALL_CFLAGS) -E $< -o x.c
		$(CC) $(STD_CFLAGS) -m68000 -msoft-float -c x.c -o $@
		rm -f x.c

endif

debugstub.o:    $(srcdir)/../stack/debugstub.c
		$(CC) $(ALL_CFLAGS) -c $< -o $@

tracecntl.o:    tracecntl.c
		$(CC) $(ALL_CFLAGS) $(FIXED_A4)  -c $< -o $@

tracecntl.ot:   tracecntl.c
		$(CC) $(ALL_CFLAGS) -DTRACE_LIBRARY $(FIXED_A4) -c -O0 $< -o $@

ifeq ($(CPU), powerpc)
ix_internals.h: create_header.o
		echo -e -n >$@ "/* This header has been generated by the create_header tool.\n   DO NOT EDIT! */\n\n"

		FOO=`ppc-morphos-objdump -D create_header.o | grep -A 1 MY_SIZEOF_STRUCTUSER | tail -n 1 | sed "s/.*\.long //"`; echo -e -n >>$@ "/* The size of struct user is $$FOO bytes. */\n\n"

		FOO=`ppc-morphos-objdump -D create_header.o | grep -A 1 MY_P_SIGMASK_OFFSET | tail -n 1 | sed "s/.*\.long //"`; echo -e -n >>$@ "#define P_SIGMASK_OFFSET $$FOO\n\n"

		FOO=`ppc-morphos-objdump -D create_header.o | grep -A 1 MY_U_ONSTACK_OFFSET | tail -n 1 | sed "s/.*\.long //"`; echo -e -n >>$@ "#define U_ONSTACK_OFFSET $$FOO\n"
		FOO=`ppc-morphos-objdump -D create_header.o | grep -A 1 MY_P_FLAG_OFFSET | tail -n 1 | sed "s/.*\.long //"`; echo -e -n >>$@ "#define P_FLAG_OFFSET $$FOO\n"

		FOO=`ppc-morphos-objdump -D create_header.o | grep -A 1 MY_IXBASE_FLAGS | tail -n 1 | sed "s/.*\.long //"`; echo -e -n >>$@ "#define IXBASE_FLAGS $$FOO\n"
		FOO=`ppc-morphos-objdump -D create_header.o | grep -A 1 MY_IXBASE_NEGSIZE | tail -n 1 | sed "s/.*\.long //"`; echo -e -n >>$@ "#define IXBASE_NEGSIZE $$FOO\n"
		FOO=`ppc-morphos-objdump -D create_header.o | grep -A 1 MY_IXBASE_POSSIZE | tail -n 1 | sed "s/.*\.long //"`; echo -e -n >>$@ "#define IXBASE_POSSIZE $$FOO\n"
		FOO=`ppc-morphos-objdump -D create_header.o | grep -A 1 MY_IXBASE_VERSION | tail -n 1 | sed "s/.*\.long //"`; echo -e -n >>$@ "#define IXBASE_VERSION $$FOO\n"
		FOO=`ppc-morphos-objdump -D create_header.o | grep -A 1 MY_IXBASE_REVISION | tail -n 1 | sed "s/.*\.long //"`; echo -e -n >>$@ "#define IXBASE_REVISION $$FOO\n"
		FOO=`ppc-morphos-objdump -D create_header.o | grep -A 1 MY_IXBASE_IDSTRING | tail -n 1 | sed "s/.*\.long //"`; echo -e -n >>$@ "#define IXBASE_IDSTRING $$FOO\n"
		FOO=`ppc-morphos-objdump -D create_header.o | grep -A 1 MY_IXBASE_SUM | tail -n 1 | sed "s/.*\.long //"`; echo -e -n >>$@ "#define IXBASE_SUM $$FOO\n"
		FOO=`ppc-morphos-objdump -D create_header.o | grep -A 1 MY_IXBASE_OPENCNT | tail -n 1 | sed "s/.*\.long //"`; echo -e -n >>$@ "#define IXBASE_OPENCNT $$FOO\n"
		FOO=`ppc-morphos-objdump -D create_header.o | grep -A 1 MY_IXBASE_LIBRARY | tail -n 1 | sed "s/.*\.long //"`; echo -e -n >>$@ "#define IXBASE_LIBRARY $$FOO\n"

		FOO=`ppc-morphos-objdump -D create_header.o | grep -A 1 MY_IXBASE_SIZEOF | tail -n 1 | sed "s/.*\.long //"`; echo -e -n >>$@ "#define IXBASE_SIZEOF (IXBASE_C_PRIVATE + $$FOO)\n"
		FOO=`ppc-morphos-objdump -D create_header.o | grep -A 1 MY_IXFAKEBASE_SIZE | tail -n 1 | sed "s/.*\.long //"`; echo -e -n >>$@ "#define IXFAKEBASE_SIZE $$FOO\n"

		FOO=`ppc-morphos-objdump -D create_header.o | grep -A 1 MY_USERPTR_OFFSET | tail -n 1 | sed "s/.*\.long //"`; echo -e -n >>$@ "#define USERPTR_OFFSET $$FOO\n"
		FOO=`ppc-morphos-objdump -D create_header.o | grep -A 1 MY_IDNESTPTR_OFFSET | tail -n 1 | sed "s/.*\.long //"`; echo -e -n >>$@ "#define IDNESTPTR_OFFSET $$FOO\n"
		FOO=`ppc-morphos-objdump -D create_header.o | grep -A 1 MY_SPREGPTR_OFFSET | tail -n 1 | sed "s/.*\.long //"`; echo -e -n >>$@ "#define SPREGPTR_OFFSET $$FOO\n"
endif

ifneq ($(CPU), powerpc)

ix_internals.h:	create_header
		#gcc ./create_header >$@

create_header:	create_header.c $(srcdir)/../include/user.h ixemul.h
		$(CC) $(CFLAGS) $(INCS) $(OTHER_CFLAGS) $(DEFS) $< -o $@
endif

create_header.o:        create_header.c $(srcdir)/../include/user.h ixemul.h
		$(NATIVE_CC) $(ALL_CFLAGS) -c $< -o $@

parse_version:  parse_version.o
		$(NATIVE_CC) $(NATIVE_LDFLAGS) $< -o $@

parse_version.o:  parse_version.c
		$(NATIVE_CC) $(NATIVE_FLAGS) -c $< -o $@

version.h:      parse_version $(srcdir)/../version.in
		./parse_version $(srcdir) >$@

gates.o:        ../include/sys/syscall.def

