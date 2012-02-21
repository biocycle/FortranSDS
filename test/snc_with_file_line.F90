#include <simple_netcdf.inc>

program with_file_line
    use SimpleNetcdf
    implicit none

    type(SNCFile) :: file
    integer :: dimid

    file%ncid = -1
    file%name = "nonexistent_file.nc"

    dimid = SNC_GET_DIM(file, "some_dim")

    print *, dimid
end program with_file_line
