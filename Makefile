# Makefile for Simple SDS
#
# Environment Variables:
#
# - COMPILER=pgi: select the Portland Group compilers
# - COMPILER=gcc: select the GNU Compiler Collection (default)
#
# - NC4=true: build with NetCDF4 instead of NetCDF3
# - H4=true: build with HDF version 4 support
# - H5=true: build with HDF version 5 support

ifeq ($(H5),true)
	need_h5=true
endif
ifeq ($(NC4),true)
	need_h5=true
endif

CPP = cpp

ifeq ($(COMPILER),pgi)
	PFX = pgi

	CC = pgcc
	CFLAGS = -g
	F90 = pgf90
	FFLAGS = -g

else
	PFX = gcc

	CC = gcc
	CFLAGS = -g -Wall -std=c99 -pedantic
	F90 = gfortran
	FFLAGS = -g -Wall
endif

ifeq ($(NC4),true)
	NC_ROOT = /usr/local/netcdf4-$(PFX)
else
	NC_ROOT = /usr/local/netcdf3-$(PFX)
endif
CFLAGS += -I. -I$(NC_ROOT)/include -Ic99/
FFLAGS += -I. -I$(NC_ROOT)/include
LDFLAGS = -L$(NC_ROOT)/lib

H4_ROOT = /usr/local/hdf4-$(PFX)

ifeq ($(H4),true)
	CFLAGS += -I$(H4_ROOT)/include
	LDFLAGS += -L$(H4_ROOT)/lib -ldf -lmfhdf -ljpeg -lz
endif

ifeq ($(NC4),true)
	CFLAGS += -DHAVE_NETCDF4
	FFLAGS += -DHAVE_NETCDF4
	LDFLAGS += -lnetcdff -lnetcdf
else
	LDFLAGS += -lnetcdf
endif

H5_ROOT = /usr/local/hdf5-$PFX
ifeq ($(need_h5),true)
	CFLAGS += -I$(H5_ROOT)/include
	LDFLAGS += -L$(H5_ROOT)/lib -lhdf5_hl -lhdf5 -lz
endif

C_LDFLAGS = $(LDFLAGS) -Lc99 -lm

# ---

C99_OBJS = \
	c99/sds.o \
	c99/sds_sort.o \
	c99/util.o \
	c99/sds.o \
	c99/sds_nc.o \
	c99/sds_hdf4.o

NC2CODE_OBJS = \
	nc2code/nc2code.o \
	nc2code/f90.o \
	nc2code/generate_f90.o \
	nc2code/generate_simple_f90.o 

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

all: test c99/libsimplesds.a nc2code/nc2code doc

simple_netcdf.F90: simple_netcdf.F90.erb
	tools/ferb $< >$@

c99/libsimplesds.a: $(C99_OBJS)
	ar -ru $@ $^

nc2code/nc2code: $(C99_OBJS) $(NC2CODE_OBJS)
	$(CC) -o $@ $^ $(C_LDFLAGS)

dump_hdf: c99/libsimplesds.a dump_hdf.o
	$(CC) -o $@ dump_hdf.o -lsimplesds $(C_LDFLAGS)

doc: doc/simple_netcdf.html

doc/simple_netcdf.html: simple_netcdf.F90
	tools/fdoc/bin/fdoc -t "Simple SDS NetCDF Module" $^ > $@

test: test/lowlevel_nc test/snc_with_file_line test/snc_no_file_line \
	test/cf_nc test/cf_nc_z test/timestep_nc test/open_var

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

test/timestep_nc: simple_netcdf.o test/timestep_nc.o
	$(F90) -o $@ $^ $(LDFLAGS)

test/open_var: simple_netcdf.o test/open_var.o
	$(F90) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *~ *.o *.mod simple.nc cf.nc
	rm -f c99/*.o c99/lib*.a
	rm -f test/*.o test/lowlevel_nc test/snc_*_file_line test/cf_nc test/timestep_nc
	rm -f nc2code/*.o nc2code/*~


# deps
c99/sds.c: c99/sds.h
c99/sds_hdf.c: c99/sds.h
c99/sds_nc.c: c99/sds.h
c99/sds_sort.c: c99/sds.h
c99/util.c: c99/util.h
