#include <simple_netcdf.inc>

subroutine read_netcdf
    type(SNCFile) :: file
    type(SNCVar) :: t_var
    integer :: lat_size, lon_size, time_size
    real*4, pointer :: t(:,:)
    character(32) :: calendar, units, long_name

    ! Open the file for reading.
    file = snc_open("some.nc")

    ! Read a global attribute
    call snc_get_global_att(file, "calendar", calendar)

    ! If you need to, get the dimensions of the file.
    lat_size = snc_get_dim(file, "lat")
    lon_size = snc_get_dim(file, "lon")
    time_size = snc_get_dim(file, "time")

    ! Find the variable in the file by name.  Optionally retrieve its units.
    t_var = snc_inq_var(file, "t", units)
    ! The number and size of t's dimensions is automatically read:
    print *, "t has", t_var%ndims, "dimensions"
    print *, "t's dimension sizes are:", t_var%dims(1:t_var%ndims)

    ! Read the variable data into t.
    call snc_read(file, t_var, t)

    ! Remember to deallocate t when you're done with it
    deallocate(t)

    ! Close the file once we're done with it.
    call snc_close(file)

end subroutine read_netcdf
