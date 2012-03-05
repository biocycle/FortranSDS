! write then read a simple netcdf file with one 2-dimensional variable
program timestep_nc
    use SimpleNetCDF
    implicit none

    integer, parameter :: N_LON = 4, N_LAT = 3, N_TIME = 2

    type(SNCFile) :: file
    type(SNCVar) :: t_var
    real*4 :: t(N_LON, N_LAT, N_TIME)
    real*4, pointer :: t2(:,:)
    integer :: lat_id, lon_id, time_id
    real*4 :: f
    integer :: i, j, k

    f = 1.2
    do k = 1, N_TIME
        do j = 1, N_LAT
            do i = 1, N_LON
                t(i, j, k) = f
                f = f + 1.1
            end do
            f = f + 5.2
        end do
        f = f - (N_LON * N_LAT) + 11.7
    end do

    ! CREATE THE FILE

    file = snc_create("timestep.nc", overwrite = .true.)
    lat_id  = snc_def_dim(file, "lat", 3)
    lon_id  = snc_def_dim(file, "lon", 4)
    time_id = snc_def_dim(file, "time", SNC_UNLIMITED)
    t_var = snc_def_var(file, "t", SNC_FLOAT, (/lon_id, lat_id, time_id/))
    call snc_enddef(file)
    ! write by individual timestep
    do k = 1, N_TIME
        call snc_write(file, t_var, t(:,:,k), k)
    end do
    call snc_close(file)


    ! READ THE FILE

    file = snc_open("timestep.nc")

    t_var = snc_inq_var(file, "t")

    ! read by individual timestop
    do k = 1, N_TIME
        call snc_read(file, t_var, t2, k)

        do j = 1, N_LAT
            do i = 1, N_LON
                if (t2(i, j) /= t(i,j,k)) then
                    print *, "t at timestep ", k, " should be ", t(:,:,k)
                    print *, "but is                                ", t2
                    stop
                end if
            end do
        end do

        deallocate(t2)
    end do

    call snc_close(file)

    print *, "success!"
end program timestep_nc
