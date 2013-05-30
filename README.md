Simple SDS
==========

The Simple SDS project attempts to make
[NetCDF](http://www.unidata.ucar.edu/software/netcdf/) (and
eventually, [HDF](http://www.hdfgroup.org/)) easier to code for
atmospheric science code.  The major component is a set of Fortran 90
modules that wrap NetCDF routines to add better error reporting and
automatically stop the program on error.  The module also has
convenience routines to help you write
[CF-compliant](http://cf-pcmdi.llnl.gov/) files.


Improved Error Handling and Reporting
=====================================

A major aim of Simple SDS is to improve the error reporting and
handling of the NetCDF library.  Normally you will need to check every
NetCDF function's status return code to see if an error occurred.  Example:

    call check( nf90_inq_dimid(ncid, "some_dim", dimid) )
    .
    .
    .
    subroutine check(status)
      .
      .
      .

If the call results in an error, you can get a description like
"NetCDF: Not a valid ID" using nf90_strerror(status), but this has no
context and leaves something to be desired.  The Simple SDS module
keeps track of enough information to report the file, action attempted
and other relevant information when reporting an error.  Example:

Source code:

     some_dimid = snc_get_dimid(file, "some_dim")

Resulting error in terminal:

     NetCDF: Not a valid ID
       While: getting id for dimension 'some_dim' in nonexistent_file.nc

With a little extra work, it can also report the source file and line
number in your code that resulted in a NetCDF error:

    test/snc_with_file_line.F90 line   13: NetCDF: Not a valid ID
       While: getting id for dimension 'some_dim' in nonexistent_file.nc

To do this properly requires the C preprocessor and some Makefile
magic.  See below for more.

In any event, when a Simple SDS routine encounters an error, it is
reported and +stop+ is called to halt the program.  Because you call
Simple SDS routines directly, there is no extra code to check the
error status.


Using Simple SDS
================

To use the Simple SDS NetCDF Fortran module, download this code to
your computer, edit the Makefile if needed and get the code built.
You may want to run the tests in the test/ directory.  Next, copy the
simple_netcdf.F90 file into an appropriate place in your source tree.
Then write or rewrite your NetCDF library calls with the replacement
functions in the Simple SDS module.

You may want to use the macros in simple_netcdf.inc to automatically
add src_file=/src_line= arguments to Simple SDS function and
subroutine calls.  These optional arguments let the library report
where in your code an error came from.  If this sounds useful to you,
there are additional steps you will need to take to use it in your
program.  First you will need to add the simple_netcdf.inc header file to each of the source files that call Simple SDS routines:

    #include "simple_netcdf.inc"

This defines C Preprocessor macros that automatically add

    src_file = __FILE__, src_line = __LINE__

arguments to Simple SDS routines.  The C Preprocessor (for __FILE__
and __LINE__ macros) has proven tricky to get to work with Fortran
source code.  The Makefile here shows one solution:

    .F90.o:
    	$(CPP) $(FFLAGS) -w $< $*.f90
    	$(F90) $(FFLAGS) -c $*.f90 -o $@ || (rm $*.f90; false)
    	rm $*.f90

This takes source files with one of the standard pre-processed
extensions .F90 (commonly .F, .F90, .F95, .F03, .FOR, .FPP), runs it
through the C PreProcessor writing the output to the non-preprocessed
extension .f90, compiles the .f90 file, then makes sure the .f90 file
gets deleted and the proper success/failure exit code is seen by make
with some shell code.  Not overly difficult, but it requires that if
you have a file with the .F90 extension, there must not also be a file
of that name with the .f90 extension, which further assumes a
case-sensitive filesystem.


Examples
========

Often, you'd just like to read a variable in from a NetCDF file and that's it.  Well, there's a function for that!

    ! optional
    #include "simple_netcdf.inc"

    use SimpleNetCDF
    implicit none

    real*4, pointer :: var(:,:,:)

    call snc_open_var("some.nc", "varname", var)

If you'd also like the units for that variable, that's a simple shortcut:

    call snc_open_var("some.nc", "varname", var, units = units)

What if you need just one timestep's worth of the variable?  That's another shortcut:

    real*4, pointer :: var2(:,:)

    call snc_open_var("same.nc", "varname", var2, timestep = 2)

Note that this is a new variable with 2 instead of 3 dimensions, because one of the dimensions for the full variable is time.

What if we want to do a little more?  There are more function calls, but it's still fairly simple and has good error reporting:

    ! optional
    #include "simple_netcdf.inc"

    use SimpleNetCDF
    implicit none

    type(SNCFile) :: file
    type(SNCVar) :: t_var, u_var
    real*4, pointer :: t(:,:), u(:,:)
    character(32) :: units

    file = snc_open("some.nc")

    t_var = snc_inq_var(file, "t", units) ! units is optional here
    call snc_read(file, t_var, t)

    t_var = snc_inq_var(file, "u")        ! see? optional
    call snc_read(file, u_var, u)

    call snc_close(file)

Writing is always more work, but making a CF-compliant NetCDF file is not as bad as it could be:

    use SimpleNetCDF
    implicit none

    character(*) :: filename
    type(SNCFile) :: file
    type(SNCVar) :: var
    integer, parameter :: N_LAT = 3, N_LON = 4, N_LEVELS = 2, N_TIME = 2
    real*4 :: lat(N_LAT), lon(N_LON), time(N_TIME)
    real*4 :: d(N_LON, N_LAT, N_LEVELS, N_TIME)
    character(10), parameter :: t_unit = "seconds"
    character(60) :: time_units
    
    write(time_units, "A,' since ',I4.4,'-',I2.2,'-',I2.2)") &
        t_unit, year, month, day

    file = snc_cf_grid_create(filename, N_LON, N_LAT, &
        time_units, SNC_STANDARD_CAL, &
        ! optional
        title = "What Kind Of Data", source = "MyShinyProgram 1.0", &
        ! optional, helps with debugging
        src_file = __FILE__, src_line = __LINE__)

    call snc_cf_grid_vertical(file, N_LEVELS, "levels", "Levels", "hPa", "down", &
        src_file = __FILE__, src_line = __LINE__)

    var = snc_cf_def_var(file, "co2", "CO2 flux", SNC_FLOAT, "kg m-2 s-1", &
        ! optional
        missing_value = -999.0, &
        src_file = __FILE__, src_line = __LINE__)

    call snc_enddef(file)

    call snc_cf_write_coords(file, lon, lat, levels, time)
    call snc_write(file, var, d)

    call snc_close(file)


For more details, see the files in the examples/ directory.
