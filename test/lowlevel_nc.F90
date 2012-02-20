! write then read a simple netcdf file with one 2-dimensional variable
program lowlevel_nc
    use SimpleNetCDF
    implicit none

    integer, parameter :: N_LAT = 3, N_LON = 4

    type(SNCFile) :: file
    type(SNCVar) :: t_var, u_var
    real*4 :: t(N_LON, N_LAT), u(N_LON, N_LAT)
    real*4, pointer :: t2(:,:), u2(:,:)
    integer :: lat_id, lon_id, time_id
    integer :: lat_size, lon_size, time_size
    character(32) :: units
    real*4 :: datt

    t(:,1) = (/1.2, 2.2, 3.2, 4.2/)
    t(:,2) = (/7.4, 7.3, 7.2, 7.1/)
    t(:,3) = (/6.8, 6.9, 6.0, 6.1/)

    u(:,1) = (/5.5, 6.4, 7.7, 4.5/)
    u(:,2) = (/1.0, 3.68, 7.45, 99.94/)
    u(:,3) = (/2309, 24088, 18773, 10028/)

    datt = 777.66


    ! CREATE THE FILE

    file = snc_create("simple.nc")

    if (file%name /= "simple.nc") stop "didn't copy filename"

    ! set dimensions lat, lon, time (-1 = unlimited)
    lat_id  = snc_def_dim(file, "lat", 3)
    lon_id  = snc_def_dim(file, "lon", 4)
    time_id = snc_def_dim(file, "time", SNC_UNLIMITED)

    ! define variables and their attributes
    t_var = snc_def_var(file, "t", SNC_FLOAT, (/lon_id, lat_id, time_id/))
    call snc_put_att(file, t_var, "units", "K")
    u_var = snc_def_var(file, "u", SNC_DOUBLE, (/lon_id, lat_id, time_id/))
    call snc_put_att(file, u_var, "thing", datt)

    ! write header, prepare to write data
    call snc_enddef(file)

    ! write variables
    call snc_write2(file, t_var, t)
    call snc_write2(file, u_var, u)

    call snc_close(file)



    ! READ THE FILE

    file = snc_open("simple.nc")

    lat_size = snc_get_dim(file, "lat")
    lon_size = snc_get_dim(file, "lon")
    time_size = snc_get_dim(file, "time")
    print *, lat_size, lon_size, time_size

    if (lat_size /= N_LAT) stop "bad lat size"
    if (lon_size /= N_LON) stop "bad lon size"
    if (time_size /= 1) stop "bad time size"

    t_var = snc_inq_var(file, "t")
    print *, trim(t_var%name), t_var%ndims, t_var%dims
    if (trim(t_var%name) /= "t") stop "didn't copy var name"
    if (t_var%ndims /= 3) stop "wrong number of var dimensions"

    call snc_get_att(file, t_var, "units", units)
    print *, units
    if (trim(units) /= "K") stop "bad units"

    call snc_read2(file, t_var, t2)
    print *, "t2 = ", t2

    u_var = snc_inq_var(file, "u")
    print *, trim(u_var%name), u_var%ndims, u_var%dims
    if (trim(u_var%name) /= "u") stop "didn't copy var name"
    if (u_var%ndims /= 3) stop "wrong number of var dimensions"

    call snc_get_att(file, u_var, "thing", datt)
    print *, datt
    if (datt /= 777.66) stop "bad 'thing' attribute"

    call snc_read2(file, u_var, u2)
    print *, "u2 = ", u2

    call snc_close(file)

end program lowlevel_nc
