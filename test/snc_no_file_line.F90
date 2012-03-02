program no_file_line
    use SimpleNetcdf
    implicit none

    type(SNCFile) :: file
    integer :: dim

    file%ncid = -1
    file%name = "nonexistent_file.nc"

    dim = snc_get_dim(file, "some_dim")

    print *, dim
end program no_file_line
