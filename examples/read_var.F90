#include <simple_netcdf.inc>

! Open a NetCDF file and read a variable from it in one shot.
program read_var
    use SimpleNetCDF
    implicit none

    ! The array to read the NetCDF data into.  It must be a pointer, and the
    ! number of dimensions must equal that of the variable in the file or
    ! one less than that if you read a timestep.
    real*4, pointer :: var(:,:,:), var2(:,:)
    ! String to hold variable's units.
    character(32) :: units

    ! This opens the some.nc file and reads the variable varname into var.
    ! It also reads the variable's units.  You do not have to read units.
    call snc_open_var("some.nc", "varname", var, units = units)

    ! This opens the some.nc file and reads the variable varname into var2.
    ! In this case, it reads the second timestep, i.e. a subset of varname,
    ! that is one dimension less than the whole variable.
    ! Note that units is not given since it is optional, but we have given
    ! the optional timestep argument.
    call snc_open_var("same.nc", "varname", var2, timestep = 2)
end program open_var
