# $Id$
# Define C-compiler and loader
CC=gcc
LD=gcc
OPTIMIZATION =  -O2 -g

# Other libs
LIBS= -lm

# For some Unix
BUILD-LINUX=build-linux
LEXT=
LFC=g77
LAR=ar
LRANLIB=ranlib
LINCDIR='-I/usr/include -I$(LIBSRC)/'
LLIBDIR=-L/usr/lib

# For MingW
BUILD-WIN32=build-win32
WFC=/usr/local/bin/i386-mingw32-g77
WAR=/usr/local/bin/i386-mingw32-ar
WRANLIB=/usr/local/bin/i386-mingw32-ranlib
WEXT=.exe
WINCDIR='-I/usr/local/i386-mingw32/include -I$(LIBSRC)/'
WLIBDIR=-L/usr/local/i386-mingw32/lib
WEXT=.exe

# NetCDF libraray
NETCDF_LIB = -lnetcdf

# Construct compiler flags
CFLAGS = $(OPTIMIZATION) $(NETCDF_INCLUDE)
LIBNETCDF = $(NETCDF_LIBDIR) $(NETCDF_LIB)

# Commands
MKDIR=mkdir -p
PROJECT=ecpack
LS=ls
SED=sed
RM=rm -f
RMDIR=rm -rf
MV=mv


all: win32 linux 



win32: 
	($(MKDIR) $(BUILD-WIN32) ; cd $(BUILD-WIN32) ; make -f ../Makefile FC=$(WFC) AR=$(WAR) RANLIB=$(WRANLIB) EXT=$(WEXT) INCDIR=$(WINCDIR) csi2ncdf.exe)

linux: 
	($(MKDIR) $(BUILD-LINUX) ; cd $(BUILD-LINUX) ; make -f ../Makefile FC=$(LFC) AR=$(LAR) RANLIB=$(LRANLIB) EXT=$(LEXT) INCDIR=$(LINCDIR) csi2ncdf)


csi2ncdf.o: ../csi2ncdf.c ../ncdef.h ../csicond.h ../csibin.h ../error.h ../in_out.h
	$(CC) $(CFLAGS) -c ../csi2ncdf.c

csi2ncdf$(EXT): csi2ncdf.o
	$(LD) -o csi2ncdf$(EXT) csi2ncdf.o  $(LIBNETCDF) $(LIBS)

