# $Id$
# Define C-compiler and loader
CC=gcc
LD=gcc
OPTIMIZATION = -O3

# Other libs
LIBS= -lm

# NetCDF libraray
# Location of includes (only define when not automatically found)
## NETCDF_INCLUDE = -I$(HOME)/include
# Location of NetCDF lib (only define when nog automatically found)
## NETCDF_LIBDIR = -L$(HOME)/lib
# Name of NetCDF lib
NETCDF_LIB = -lnetcdf

# Construct compiler flags
CFLAGS = $(OPTIMIZATION) $(NETCDF_INCLUDE)
LIBNETCDF = $(NETCDF_LIBDIR) $(NETCDF_LIB)

all: csi2ncdf

csi2ncdf.o: csi2ncdf.c ncdef.h csicond.h csibin.h error.h in_out.h
	$(CC) $(CFLAGS) -c csi2ncdf.c

csi2ncdf: csi2ncdf.o
	$(LD) -o csi2ncdf csi2ncdf.o  $(LIBNETCDF) $(LIBS)

