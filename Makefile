CFLAGS = -g -ggdb3

all: csi2ncdf 

csi2ncdf.o: csi2ncdf.c ncdef.h csicond.h 
	gcc $(CFLAGS) -c csi2ncdf.c

csi2ncdf: csi2ncdf.o
	gcc -o csi2ncdf.exe csi2ncdf.o  -lnetcdf
	
