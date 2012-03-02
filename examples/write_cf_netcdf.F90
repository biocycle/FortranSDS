#include <simple_netcdf.inc>

! Write a CF-compliant gridded NetCDF file
subroutine cf_netcdf
    use SimpleNetCDF
    implicit none

    character(*) :: filename

    ! variables to keep track of NetCDF things
    type(SNCFile) :: file
    type(SNCVar) :: var

    ! grid and coordinate variables
    integer, parameter :: N_LAT = 3, N_LON = 4, N_LEVELS = 2, N_TIME = 2
    real*4 :: lat(N_LAT), lon(N_LON), time(N_TIME)
    real*4 :: d(N_LON, N_LAT, N_LEVELS, N_TIME)
    integer :: year, month, day, hour, minute, second, tz_hours
    character(*) :: t_unit
    character(60) :: time_units


    ! -- model writes data to variable d --


    ! Fill in the timestamp for the start of the data in this file.
    ! t_unit is usually one of: 'seconds', 'hours', 'days'.

    ! Shortened UTC version, example: "seconds since 2012-02-03"
    write(time_units, "A,' since ',I4.4,'-',I2.2,'-',I2.2)") &
        t_unit, year, month, day

    ! longer example with timezone offset (local time = MST), example:
    ! "hours since 2012-02-03 03:00:00 -07:00"
    write(time_units, "(A,' since ',I4.4,'-',I2.2,'-',I2.2,' ',I2.2,':',I2.2,':',I2.2,' ',I3,':00')") &
        t_unit, year, month, day, hour, minute, second, tz_hours

    ! Create the file.  Arguments: file name, grid cell count in the X
    ! and Y dimensions, start time of the file data, the calendar to
    ! use, optional file title and source, and source file and line number
    ! preprocessor magic to help error reporting.
    file = snc_cf_grid_create(filename, N_LON, N_LAT, &
        time_units, SNC_STANDARD_CAL, &
        ! optional
        title = "What Kind Of Data", source = "MyShinyProgram 1.0", &
        ! optional, helps with debugging
        src_file = __FILE__, src_line = __LINE__)

    ! Define the vertical dimension.
    ! Arguments: type(SNCFile) from snc_cf_grid_create, vertical dimension size,
    ! short and long names for the variable, vertical units, does the variable
    ! increase going "down" or "up"? and optional standard name and debugging
    ! info.
    call snc_cf_grid_vertical(file, N_LEVELS, "levels", "Levels", "hPa", "down", &
        ! optional, helps with debugging
        src_file = __FILE__, src_line = __LINE__)

    ! Define a variable for the file to store.  You can have more than one of
    ! these.  You will need to create separate type(SNCVar) variables for them.
    ! Arguments: type(SNCFile) var from snc_cf_grid_create, short variable name,
    ! longer variable name, type (SNC_INT, SNC_FLOAT or SNC_DOUBLE), units,
    ! and some optional arguments.
    var = snc_cf_def_var(file, "co2", "CO2 flux", SNC_FLOAT, "kg m-2 s-1", &
        ! optional
        missing_value = -999.0, &
        ! optional, helps with debugging
        src_file = __FILE__, src_line = __LINE__)

    ! Define additional attributes here as needed, for example:
    call snc_put_att(file, var, "TracerNum", 1)

    ! when you're done definining variables and attributes, call this
    call snc_enddef(file)

    ! now you need to write the coordinate variable data describing the grid
    call snc_cf_write_coords(file, lon, lat, levels, time)

    ! write the variable data.  If you defined more than one variable above,
    ! add other snc_write() statements for them too.
    call snc_write(file, var, d)

    ! and finally, close the file
    call snc_close(file)
end subroutine cf_netcdf
