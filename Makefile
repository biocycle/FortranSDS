ifeq ($(COMPILER),pgi)
	F90 = pgf90
	FFLAGS = -g

	NC_ROOT = /usr/local/netcdf4-pgi
	HDF_ROOT = /usr/local/hdf5-pgi
else
	F90 = gfortran
	FFLAGS = -g -Wall

	NC_ROOT = /usr/local/netcdf4-gcc
	HDF_ROOT = /usr/local/hdf5-gcc
endif

FFLAGS += -I$(NC_ROOT)/include
LDFLAGS = -L$(HDF_ROOT)/lib -L$(NC_ROOT)/lib -lnetcdff -lnetcdf -lhdf5_hl -lhdf5 -lz

.SUFFIXES:
.SUFFIXES: .f90 .F90 .o

.PHONY: all test

.F90.o:
	$(F90) $(FFLAGS) -c $< -o $@

.f90.o:
	$(F90) $(FFLAGS) -c $< -o $@

all: test

test: test/lowlevel_nc

test/lowlevel_nc: simple_netcdf.o test/lowlevel_nc.o
	$(F90) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *~ *.o *.mod test/*.o test/lowlevel_nc
