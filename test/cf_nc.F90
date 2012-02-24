#include <simple_netcdf.inc>

program cf_nc
    use SimpleNetCDF
    implicit none

    type(SNCFile) :: file
    type(SNCVar) :: var
    integer, parameter :: N_LAT = 3, N_LON = 4, N_TIME = 2
    integer :: i, j, k
    real*4 :: lat(N_LAT), lon(N_LON), time(N_TIME)
    real*4 :: f, t(N_LON, N_LAT, N_TIME)
    real*4, pointer :: t2(:,:,:)
    character(50) :: time_units

    lat = (/0.0, 0.5, 1.0/)
    lon = (/0.0, 1.0, 2.0, 3.0/)
    time = (/0, 3/)

    f = 23.0
    do k = 1,N_TIME
        do j = 1,N_LAT
            do i = 1,N_LON
                t(i, j, k) = f
                f = f * 1.22
            end do
            f = f + 22.7
        end do
        f = f + 8.9
    end do
    time_units = "hours since 2012-02-03"

    file = snc_cf_grid_create("cf.nc", N_LON, N_LAT, time_units, &
        SNC_STANDARD_CAL, title = "test file", &
        source = "some test program")
    var = snc_cf_def_var(file, "t", "temperature", SNC_FLOAT, "K", &
        standard_name = "air_temperature", missing_value = -999.0)
    call SNC_ENDDEF(file)
    call SNC_CF_WRITE_COORDS(file, lon, lat, time)
    call SNC_WRITE(file, var, t)
    call SNC_CLOSE(file)

    ! verify dimensions, global attributes, variables and their attributes
end program cf_nc
