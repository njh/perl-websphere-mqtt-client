#/****************************************************************************/
#/*                                                                          */
#/* Program name: MQIsdp protocol C Language implementation                  */
#/*                                                                          */
#/* Description: This makefile compiles the provided code on Windows, Linux  */
#/*              AIX, Sun Solaris and HP-UX                                  */
#/*                                                                          */
#/*  Statement:  Licensed Materials - Property of IBM                        */
#/*                                                                          */
#/*              MQSeries SupportPac IA93                                    */
#/*              (C) Copyright IBM Corp. 2002                                */
#/*                                                                          */
#/****************************************************************************/
#/*                                                                          */
#/* Function:                                                                */
#/*                                                                          */
#/* Requires GNU Make Version 3.77 or later.                                 */
#/* Build output is produced in the current directory                        */
#/* To build multithreaded Execute:                                          */
#/*          make -f <Makefile relative path>/Makefile                       */
#/* To build single threaded Execute:                                        */
#/*          make -f <Makefile relative path>/Makefile BT=SINGLE             */
#/*                                                                          */
#/****************************************************************************/
#/*                                                                          */
#/* Change history:                                                          */
#/*                                                                          */
#/* V1.0   19-02-2003  IRH  Initial release                                  */
#/* V1.1   18-08-2003  IRH  LIB makefile variable seems to clash with LIB    */
#/*                         environment variable on Windows XP. Changed      */
#/*                         makefile variable name to LIBEXT.                */
#/* V1.2   30-11-2003  IRH  Added mspfp.c implementation of persistence.     */                           
#/*                         Added publish.c and subscribe.c samples.         */
#/*                         Removed demo.c and democ.c samples.              */
#/*                         Renamed shared library produced to wmqtt.[dll|so]*/
#/*==========================================================================*/
#/* Module Name: Makefile                                                    */
STARTDIR = $(CURDIR)
ifndef SBD
  SBD = $(STARTDIR)
endif

ifeq ($(OS),Windows_NT)
  OSTYPE = $(OS)
  UNIX = 0
else
  OSTYPE = $(shell uname -s)
  UNIX = 1
endif

ifeq ($(OS),Windows_NT)
  LIBEXT = .lib
  DLL = .dll
  EXE = .exe
  OBJ = .obj
  CC = cl
  LD = link
#  CPPFLAGS = /D "WIN32" /D "_DEBUG"
#  DEBUGCFLAGS = /MDd /Zi
#  DEBUGLINKFLAGS = /debug
  CPPFLAGS = /D "WIN32"
  DEBUGCFLAGS = /MD /GD
  DEBUGLINKFLAGS =

  CFLAGS = $(DEBUGCFLAGS) /nologo /c /Od /W3 /Fd /I $(SBD)
  SINGLEPROC = /D "MSP_SINGLE_THREAD"
  LINKFLAGS = $(DEBUGLINKFLAGS) /nologo /machine:I386 /pdbtype:sept 
  LIBLINKFLAGS = /dll
  MAINLINKFLAGS = /subsystem:console
  NTLIBS = user32.lib kernel32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib
  IMP = /implib:$(@:.dll=.lib)
  LIBPDB = /pdb:$(@:.dll=.pdb)
  LIBMAP = /map:$(@:.dll=.map)
  EXEPDB = /pdb:$(@:.exe=.pdb)
  EXEMAP = /map:$(@:.exe=.map)
  OUT = /out:
  OBJOUT = /Fo
  LNKFLAG =
  # Flags to link C libraries
  LIBLDFLAGS = $(LINKFLAGS) $(LIBLINKFLAGS) $(NTLIBS) $(IMP) $(LIBPDB) $(LIBMAP)
  # Flags to link C programs
  MAINLDFLAGS = $(LINKFLAGS) $(MAINLINKFLAGS) $(EXEPDB) $(EXEMAP) $(NTLIBS)
  LIBPREF =
endif  

# Generic UNIX build flags
ifeq ($(UNIX),1)
  # These -o options deliberately have a space on the end!!! Do not
  # remove the space.
  CC = gcc
  LD = gcc
  OUT = -o 
  OBJOUT = -o 
  OBJ = .o
  CPPFLAGS = -I$(SBD)
  CFLAGS = -c
  LIBEXT =
  EXE =
  LIBPREF = lib
  LNKFLAG = -l
  SINGLEPROC = -DMSP_SINGLE_THREAD
endif

# Linux specifics
ifeq ($(OSTYPE),Linux)
  DLL = .so
  CC = gcc
  LD = gcc
  UNIX_LIBPATH = -Wl,-rpath,.:/usr/lib:/lib
  LINUXLIBS = -lpthread -lc -lnsl -ldl
  EXTRACPPFLAGS = -DTHREADS -D_POSIX_PTHREAD_SEMANTICS -D_REENTRANT
  CPPFLAGS = -ansi -g -fpic -DUNIX -DLINUX -D_XOPEN_SOURCE -D_XOPEN_SOURCE_EXTENDED -DUSESIGACTION $(EXTRACPPFLAGS) -I$(SBD)
  CFLAGS = -c
  # Flags to link C libraries
  LIBLDFLAGS = -shared -s $(UNIX_LIBPATH) $(LINUXLIBS)
  # Flags to link C programs
  MAINLDFLAGS = $(UNIX_LIBPATH) -s -L. $(LINUXLIBS)
endif

# AIX specifics
ifeq ($(OSTYPE),AIX)
  DLL = .a
  CC = xlc_r
  LD = xlc_r
  UNIX_LIBPATH = -blibpath:.:/usr/lib:/lib
  AIXLIBS = -lpthread -lc_r
  EXTRACPPFLAGS = -DTHREADS
  # The following flags are significant when comiling in and linking (in CPPFLAGS, LIBLDFLAGS and MAINLDFLAGS)
  # The presence of -g causes debug objects to be built
  # The presence of -s causes the symbol table to be stripped, significantly reducing the size,
  #     but making it difficult to debug.
  CPPFLAGS = -s -DUNIX -DAIX $(EXTRACPPFLAGS) -I$(SBD)
  CFLAGS = -c
  # Flags to link C libraries
  LIBLDFLAGS = -G -s $(UNIX_LIBPATH) $(AIXLIBS)
  # Flags to link C programs
  MAINLDFLAGS = -bdynamic -s -L. $(AIXLIBS)
endif

# SUN SOLARIS specifics
ifeq ($(OSTYPE),SunOS)
  DLL = .so
  CC = /opt/SUNWspro/bin/cc
  LD = /opt/SUNWspro/bin/cc
  UNIX_LIBPATH = -R .:/usr/lib:/lib
  SOLLIBS = -lpthread -lc -lsocket -lnsl
  EXTRACPPFLAGS = -DTHREADS -D_POSIX_PTHREAD_SEMANTICS
  # The following flags are significant when comiling in and linking (in CPPFLAGS, LIBLDFLAGS and MAINLDFLAGS)
  # The presence of -g causes debug objects to be built
  # The presence of -s causes the symbol table to be stripped, significantly reducing the size,
  #     but making it difficult to debug.
  CPPFLAGS = -mt -KPIC -s -DUNIX -DSOLARIS $(EXTRACPPFLAGS) -I$(SBD)
  CFLAGS = -Aa -c
  # Flags to link C libraries
  LIBLDFLAGS = -G -Bdynamic -s $(UNIX_LIBPATH) $(SOLLIBS)
  # Flags to link C programs
  MAINLDFLAGS = -mt -L. $(UNIX_LIBPATH) $(SOLLIBS)
endif

# HP-UX specifics
ifeq ($(OSTYPE),HP-UX)
  DLL = .sl
  CC = /opt/ansic/bin/cc
  LD = ld
  UNIX_LIBPATH = +b .:/usr/lib:/lib
  HPUXLIBS = -lpthread -lc
  EXTRACPPFLAGS = -DTHREADS -D_REENTRANT
  # The following flags are significant when comiling in and linking (in CPPFLAGS, LIBLDFLAGS and MAINLDFLAGS)
  # The presence of -g causes debug objects to be built
  # The presence of -s causes the symbol table to be stripped, significantly reducing the size,
  #     but making it difficult to debug.
  CPPFLAGS = -DUNIX -DHPUX $(EXTRACPPFLAGS) -I$(SBD)
  CFLAGS = +DAportable +z -Aa -D_HPUX_SOURCE -s -c
  # Flags to link C libraries
  LIBLDFLAGS = -b -s +s $(UNIX_LIBPATH) $(HPUXLIBS)
  # Flags to link C programs
  MAINLDFLAGS = -L. $(UNIX_LIBPATH) $(HPUXLIBS) +s -s /usr/ccs/lib/crt0.o
endif

VPATH = $(SBD)


# Objects required for the daemon library
MQISDPLIBOBJECTS = \
mspipc$(OBJ)   \
mspsh$(OBJ)    \
msputils$(OBJ) \
mspscada$(OBJ) \
msptcp$(OBJ)   \
msphash$(OBJ)  \
mspclnt$(OBJ)  \
mspdmn$(OBJ)   \
mspstart$(OBJ)

# MQISDPLIBNAME - Main part of library name
# MQISDPLDLIB   - Name to provide the linker when linking mains with this library
# MQISDPLIB     - Name of the library to build
MQISDPLIBNAME = wmqtt
MQISDPLDLIB = $(LNKFLAG)$(MQISDPLIBNAME)$(LIBEXT)
MQISDPLIB = $(LIBPREF)$(MQISDPLIBNAME)$(DLL)

# Client applications to build
MQISDPMAIN = \
publish$(EXE)   \
subscribe$(EXE)

MQISDPMAINOBJECTS = \
mspfp$(OBJ)

# If we are building the protocol and application as a single process do the following
ifeq ($(BT),SINGLE)
  CPPFLAGS_TMP = $(CPPFLAGS)
  CPPFLAGS := $(CPPFLAGS_TMP) $(SINGLEPROC)
endif

default: $(MQISDPLIB) $(MQISDPMAIN)


%$(OBJ): %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(OBJOUT)$@

$(MQISDPMAIN): %$(EXE): %$(OBJ) $(MQISDPMAINOBJECTS)
	$(LD) $(MAINLDFLAGS) $(MQISDPLDLIB) $(OUT)$@ $(basename $@)$(OBJ) $(MQISDPMAINOBJECTS)

$(MQISDPLIB): $(MQISDPLIBOBJECTS)
	$(LD) $(LIBLDFLAGS) $(OUT)$(MQISDPLIB) $(MQISDPLIBOBJECTS)
