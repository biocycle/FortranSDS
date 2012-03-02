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
    real*4, pointer :: t2(:,:,:), coord(:)
    integer :: lat_size, lon_size, time_size
    character(50) :: time_units
    character(150) :: attv
    real*4 :: attvf

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
        source = "some test program", src_file = __FILE__, src_line = __LINE__)
    var = snc_cf_def_var(file, "t", "temperature", SNC_FLOAT, "K", &
        standard_name = "air_temperature", missing_value = -999.0)
    call snc_enddef(file)
    call snc_cf_write_coords(file, lon, lat, time)
    call snc_write(file, var, t)
    call snc_close(file)

    ! verify dimensions, global attributes, variables and their attributes
    file = snc_open("cf.nc")

    call snc_get_global_att(file, "Conventions", attv)
    if (attv /= "CF-1.6") print *, "Bad 'Conventions' value", trim(attv)

    call snc_get_global_att(file, "title", attv)
    if (attv /= "test file")  print *, "Bad 'title' value '", trim(attv), "'"

    call snc_get_global_att(file, "source", attv)
    if (attv /= "some test program")  print *, "Bad 'source' value '", trim(attv), "'"

    lat_size = snc_get_dim(file, "lat")
    lon_size = snc_get_dim(file, "lon")
    time_size = snc_get_dim(file, "time")
    if (lat_size /= N_LAT) print *, "Bad lat size", lat_size
    if (lon_size /= N_LON) print *, "Bad lon size", lon_size
    if (time_size /= N_TIME) print *, "Bad time size", time_size

    var =  snc_inq_var(file, "lon")
    if (var%ndims /= 1) print *, "Bad lon dim count", var%ndims
    if (var%dims(1) /= lon_size) print *, "Bad lon var size", var%dims(1)

    call snc_get_att(file, var, "long_name", attv)
    if (trim(attv) /= "longitude") print *, "Bad lon:long_name value", attv
    call snc_get_att(file, var, "units", attv)
    if (trim(attv) /= "degrees_east") print *, "Bad lon:units value", attv
    call snc_get_att(file, var, "standard_name", attv)
    if (trim(attv) /= "longitude") print *, "Bad lon:standard_name value", attv
    call snc_get_att(file, var, "axis", attv)
    if (trim(attv) /= "X") print *, "Bad lon:axis value", attv

    call snc_read(file, var, coord)
    print *, 'lon = ', coord
    deallocate(coord)

    var =  snc_inq_var(file, "lat")
    if (var%ndims /= 1) print *, "Bad lat dim count", var%ndims
    if (var%dims(1) /= lat_size) print *, "Bad lat var size", var%dims(1)

    call snc_get_att(file, var, "long_name", attv)
    if (trim(attv) /= "latitude") print *, "Bad lat:long_name value", attv
    call snc_get_att(file, var, "units", attv)
    if (trim(attv) /= "degrees_north") print *, "Bad lat:units value", attv
    call snc_get_att(file, var, "standard_name", attv)
    if (trim(attv) /= "latitude") print *, "Bad lat:standard_name value", attv
    call snc_get_att(file, var, "axis", attv)
    if (trim(attv) /= "Y") print *, "Bad lat:axis value", attv

    call snc_read(file, var, coord)
    print *, 'lat = ', coord
    deallocate(coord)

    var =  snc_inq_var(file, "time")
    if (var%ndims /= 1) print *, "Bad time dim count", var%ndims
    if (var%dims(1) /= time_size) print *, "Bad time var size", var%dims(1)

    call snc_get_att(file, var, "long_name", attv)
    if (trim(attv) /= "time") print *, "Bad time:long_name value", attv
    call snc_get_att(file, var, "units", attv)
    if (trim(attv) /= "hours since 2012-02-03") print *, "Bad time:units value", attv
    call snc_get_att(file, var, "axis", attv)
    if (trim(attv) /= "T") print *, "Bad time:axis value", attv
    call snc_get_att(file, var, "calendar", attv)
    if (trim(attv) /= "standard") print *, "Bad time:calendar value", attv

    call snc_read(file, var, coord)
    print *, 'time = ', coord
    deallocate(coord)

    var =  snc_inq_var(file, "t")
    if (var%ndims /= 3) print *, "Bad lon dim count", var%ndims
    if (var%dims(1) /= N_LON) print *, "Bad t dimension 1", var%dims(1)
    if (var%dims(2) /= N_LAT) print *, "Bad t dimension 2", var%dims(2)
    if (var%dims(3) /= N_TIME) print *, "Bad t dimension 3", var%dims(3)

    call snc_get_att(file, var, "long_name", attv)
    if (trim(attv) /= "temperature") print *, "Bad t:long_name value", attv
    call snc_get_att(file, var, "units", attv)
    if (trim(attv) /= "K") print *, "Bad t:units value", attv
    call snc_get_att(file, var, "standard_name", attv)
    if (trim(attv) /= "air_temperature") print *, "Bad t:standard_name value", attv
    call snc_get_att(file, var, "missing_value", attvf)
    if (attvf /= -999.0) print *, "Bad t:missing_value value", attvf

    call snc_read(file, var, t2)
    print *, 't2 = ', t2
    deallocate(t2)

    call snc_close(file)
end program cf_nc
