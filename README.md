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

A secondary component of this project is a tool, nc2code, which will
examine arbitrary NetCDF files and generate code to read them in one
of several languages.  Incidentally, it uses a high level C wrapper to
examine a NetCDf file which may prove useful if you are writing NetCDF
handling code in C.


Examples
========

Reading variables from a NetCDF file is quite simple.  You open the file, look up the variable then read it into an appropriate pointer array before closing the file:

    type(SNCFile) :: file
    type(SNCVar) :: t_var
    real*4, pointer :: t(:,:)
    character(32) :: units

    file = snc_open("some.nc")
    t_var = snc_inq_var(file, "t", units) ! units is optional here
    call snc_read(file, t_var, t)
    call snc_close(file)


Writing a CF-compliant NetCDF file is a little more complex:

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


For more examples, see the files in the examples/ directory.
