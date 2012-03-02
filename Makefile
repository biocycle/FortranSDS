# Makefile for Simple SDS

CPP = cpp

ifeq ($(COMPILER),pgi)
	CC = pgcc
	CFLAGS = -g
	F90 = pgf90
	FFLAGS = -g

	NC_ROOT = /usr/local/netcdf4-pgi
	HDF_ROOT = /usr/local/hdf5-pgi
else
	CC = gcc
	CFLAGS = -g -Wall -ansi -pedantic
	F90 = gfortran
	FFLAGS = -g -Wall

	NC_ROOT = /usr/local/netcdf4-gcc
	HDF_ROOT = /usr/local/hdf5-gcc
endif

ifeq ($(NC4),true)
	FFLAGS += -DHAVE_NETCDF4
endif

CFLAGS += -I. -I$(NC_ROOT)/include
FFLAGS += -I. -I$(NC_ROOT)/include
LDFLAGS = -L$(HDF_ROOT)/lib -L$(NC_ROOT)/lib -lnetcdff -lnetcdf -lhdf5_hl -lhdf5 -lz
C_LDFLAGS = $(LDFLAGS) -lm

NC2CODE_OBJS = nc2code.o generate_f90.o sds_nc.o skiplist.o util.o

.SUFFIXES:
.SUFFIXES: .c .f90 .F90 .o

.PHONY: all test

.c.o:
	$(CC) $(CFLAGS) -c $*.c -o $*.o

.F90.o:
	$(CPP) $(FFLAGS) -w $< $*.f90
	$(F90) $(FFLAGS) -c $*.f90 -o $@ || (rm $*.f90; false)
	rm $*.f90

.f90.o:
	$(F90) $(FFLAGS) -c $< -o $@

all: test

nc2code: $(NC2CODE_OBJS)
	$(CC) -o $@ $^ $(C_LDFLAGS)

test: test/lowlevel_nc test/snc_with_file_line test/snc_no_file_line \
	test/cf_nc test/cf_nc_z

test/lowlevel_nc: simple_netcdf.o test/lowlevel_nc.o
	$(F90) -o $@ $^ $(LDFLAGS)

test/snc_with_file_line: simple_netcdf.o test/snc_with_file_line.o
	$(F90) -o $@ $^ $(LDFLAGS)

test/snc_no_file_line: simple_netcdf.o test/snc_no_file_line.o
	$(F90) -o $@ $^ $(LDFLAGS)

test/cf_nc: simple_netcdf.o test/cf_nc.o
	$(F90) -o $@ $^ $(LDFLAGS)

test/cf_nc_z: simple_netcdf.o test/cf_nc_z.o
	$(F90) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *~ *.o *.mod simple.nc cf.nc
	rm -f test/*.o test/lowlevel_nc test/snc_*_file_line test/cf_nc
