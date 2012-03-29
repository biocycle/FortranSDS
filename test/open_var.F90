#include <simple_netcdf.inc>

program open_var
    use SimpleNetCDF
    implicit none

    real*4, pointer :: t(:,:,:), t1(:,:)
    character(32) :: units

    call snc_open_var("cf.nc", "t", t, units = units)
    if (units /= "K") stop "Bad units"
    print *, "t = ", t
    

    call snc_open_var("cf.nc", "t", t1, timestep = 2)
    print *, "t1 = ", t1
end program open_var
