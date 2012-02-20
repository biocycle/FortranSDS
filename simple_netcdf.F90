! NetCDF wrapper functions to make it easier to read and write NetCDF files
! especially CF-compliant files and give more useful error messages.
module SimpleNetCDF
    use netcdf
    implicit none

    type SNCFile
        integer :: ncid
        character(512) :: name
    end type SNCFile

    type SNCVar
        integer :: id
        character(128) :: name
        integer :: ndims, dims(4)
    end type SNCVar

    ! read 2-dimension variables of type integer, float (real*4)
    ! or double (real*8).
    interface snc_read2
        module procedure snc_read2i, snc_read2f, snc_read2d
    end interface snc_read2

    ! write 2-dimension variables of type integer, float or double.
    interface snc_write2
        module procedure snc_write2i, snc_write2f, snc_write2d
    end interface snc_write2

    interface snc_get_att
        module procedure snc_get_att_str, snc_get_atti, snc_get_attf, snc_get_attd
    end interface snc_get_att

    interface snc_put_att
        module procedure snc_put_att_str, snc_put_atti, snc_put_attf, snc_put_attd
    end interface snc_put_att

    integer, parameter :: SNC_UNLIMITED = NF90_UNLIMITED
    integer, parameter :: SNC_INT       = NF90_INT
    integer, parameter :: SNC_FLOAT     = NF90_FLOAT
    integer, parameter :: SNC_DOUBLE    = NF90_DOUBLE

    integer, parameter :: READ_MODE = NF90_SHARE
#ifndef HAVE_NETCDF4
    integer, parameter :: WRITE_MODE = NF90_NOCLOBBER
#else
    integer, parameter :: WRITE_MODE = IOR(NF90_NOCLOBBER, NF90_HDF5)
    integer, parameter :: SNC_DEFAULT_DEFLATE = 6
#endif

contains

    ! Open a NetCDF file for reading
    function snc_open(filename)
        character(*), intent(in) :: filename
        type(SNCFile) :: snc_open
        character(530) :: err_msg

        write(err_msg, "('opening ',A)") trim(filename)
        call snc_handle_error(nf90_open(filename, READ_MODE, snc_open%ncid), err_msg)
        snc_open%name = filename
    end function snc_open

    ! Open a NetCDF file for writing
    function snc_create(filename)
        character(*), intent(in) :: filename
        type(SNCFile) :: snc_create
        character(530) :: err_msg

        write(err_msg, "('creating ',A)") trim(filename)
        call snc_handle_error(nf90_create(filename, WRITE_MODE, snc_create%ncid), err_msg)
        snc_create%name = filename
    end function snc_create



    ! DIMENSIONS SECTION ---


    ! Returns the size of the given dimension
    function snc_get_dim(file, dimname)
        type(SNCFile), intent(in) :: file
        character(*), intent(in) :: dimname
        integer :: snc_get_dim
        integer :: dimid
        character(700) :: err_msg

        write(err_msg, "('getting id for dimension ''',A,''' in ',A)") &
            trim(dimname), trim(file%name)
        call snc_handle_error(nf90_inq_dimid(file%ncid, dimname, dimid), err_msg)

        write(err_msg, "('getting length for dimension ''',A,''' in ',A)") &
            trim(dimname), trim(file%name)
        call snc_handle_error(nf90_inquire_dimension(file%ncid, dimid, len = snc_get_dim), err_msg)
    end function snc_get_dim

    ! Defines a dimension with the given name and size, then returns its id
    ! for use in defining variables later.
    function snc_def_dim(file, dimname, dimsize)
        type(SNCFile), intent(in) :: file
        character(*), intent(in) :: dimname
        integer, intent(in) :: dimsize
        integer :: snc_def_dim
        integer :: dimid
        character(700) :: err_msg

        write(err_msg, "('defining dimension ''',A,''' = ',I6,'in ',A)") &
            trim(dimname), dimsize, trim(file%name)
        call snc_handle_error(nf90_def_dim(file%ncid, dimname, dimsize, dimid), err_msg)
        snc_def_dim = dimid
    end function snc_def_dim



    ! VARIABLES SECTION


    ! Looks for the given variable name in the NetCDF file and gets its
    ! dimensions.
    function snc_inq_var(file, varname)
        type(SNCFile), intent(in) :: file
        character(*), intent(in) :: varname
        type(SNCVar) :: snc_inq_var
        character(700) :: err_msg
        integer :: status, i, dimids(4)

        snc_inq_var%name = varname

        write(err_msg, "('inquiring variable''s id ''',A,''' in ',A)") &
            trim(varname), trim(file%name)
        status = nf90_inq_varid(file%ncid, varname, snc_inq_var%id)
        call snc_handle_error(status, err_msg)

        write(err_msg, "('inquiring about variable ''',A,''' in ',A)") &
            trim(varname), trim(file%name)
        status = nf90_inquire_variable(file%ncid, snc_inq_var%id, &
            ndims = snc_inq_var%ndims, dimids = dimids)
        call snc_handle_error(status, err_msg)
        if (snc_inq_var%ndims > size(dimids)) then
            print "('Too many dimensions in variable ''',A,''' in ',A)", &
                trim(varname), trim(file%name)
            stop
        end if

        do i = 1, snc_inq_var%ndims
            write(err_msg, "('getting length for dimension ',I1,' of ''',A,''' in ',A)") &
                i, trim(varname), trim(file%name)
            call snc_handle_error(nf90_inquire_dimension(file%ncid, dimids(i), &
                len = snc_inq_var%dims(i)), err_msg)
        end do
    end function snc_inq_var

    ! Define a variable for the NetCDF file with the given name and type; the
    ! dimids are the values returned by snc_def_dim.
    function snc_def_var(file, varname, vartype, dimids)
        type(SNCFile), intent(in) :: file
        character(*), intent(in) :: varname
        integer, intent(in) :: vartype
        integer, intent(in), dimension(:) :: dimids
        type(SNCVar) :: snc_def_var
        character(700) :: err_msg

        write(err_msg, "('defining variable ''',A,''' in ',A)") trim(varname), trim(file%name)
        call snc_handle_error( &
            nf90_def_var(file%ncid, varname, vartype, dimids, snc_def_var%id &
#ifdef HAVE_NETCDF4
              , deflate_level = SNC_DEFAULT_DEFLATE &
#endif
            ), err_msg)
        snc_def_var%name = varname
    end function snc_def_var

    ! Read a variable's data into a 2-dimensional array, then return it to the user.
    subroutine snc_read2i(file, var, data)
       type(SNCFile), intent(in) :: file
        type(SNCVar), intent(in) :: var
        integer, pointer :: data(:,:)
        character(700) :: err_msg

        allocate(data(var%dims(1), var%dims(2)))
        write(err_msg, "('reading 2d int variable ''',A,''' in ',A)") &
            trim(var%name), trim(file%name)
        call snc_handle_error(nf90_get_var(file%ncid, var%id, data), err_msg)
    end subroutine snc_read2i

    subroutine snc_read2f(file, var, data)
       type(SNCFile), intent(in) :: file
        type(SNCVar), intent(in) :: var
        real*4, pointer :: data(:,:)
        character(700) :: err_msg

        allocate(data(var%dims(1), var%dims(2)))
        write(err_msg, "('reading 2d float variable ''',A,''' in ',A)") &
            trim(var%name), trim(file%name)
        call snc_handle_error(nf90_get_var(file%ncid, var%id, data), err_msg)
    end subroutine snc_read2f

    subroutine snc_read2d(file, var, data)
       type(SNCFile), intent(in) :: file
        type(SNCVar), intent(in) :: var
        real*8, pointer :: data(:,:)
        character(700) :: err_msg

        allocate(data(var%dims(1), var%dims(2)))
        write(err_msg, "('reading 2d double variable ''',A,''' in ',A)") &
            trim(var%name), trim(file%name)
        call snc_handle_error(nf90_get_var(file%ncid, var%id, data), err_msg)
    end subroutine snc_read2d

    ! Write the given variable's data to a NetCDF file.
    subroutine snc_write2i(file, var, data)
        type(SNCFile), intent(in) :: file
        type(SNCVar), intent(in) :: var
        integer, intent(in), dimension(:,:) :: data
        character(700) :: err_msg

        write(err_msg, "('writing 2d int variable ''',A,''' to ',A)") &
            trim(var%name), trim(file%name)
        call snc_handle_error(nf90_put_var(file%ncid, var%id, data), err_msg)
    end subroutine snc_write2i

    subroutine snc_write2f(file, var, data)
        type(SNCFile), intent(in) :: file
        type(SNCVar), intent(in) :: var
        real*4, intent(in), dimension(:,:) :: data
        character(700) :: err_msg

        write(err_msg, "('writing 2d float variable ''',A,''' to ',A)") &
            trim(var%name), trim(file%name)
        call snc_handle_error(nf90_put_var(file%ncid, var%id, data), err_msg)
    end subroutine snc_write2f

    subroutine snc_write2d(file, var, data)
        type(SNCFile), intent(in) :: file
        type(SNCVar), intent(in) :: var
        real*8, intent(in), dimension(:,:) :: data
        character(700) :: err_msg

        write(err_msg, "('writing 2d double variable ''',A,''' to ',A)") &
            trim(var%name), trim(file%name)
        call snc_handle_error(nf90_put_var(file%ncid, var%id, data), err_msg)
    end subroutine snc_write2d



    !  ATTRIBUTES SECTION ---


    ! Read a character attribute's value from the NetCDF file.
    subroutine snc_get_att_str(file, var, attname, attvalue)
        type(SNCFile), intent(in) :: file
        type(SNCVar), intent(in) :: var
        character(*), intent(in) :: attname
        character(*), intent(out) :: attvalue
        character(800) :: err_msg

        write(err_msg, "('getting string attribute ''',A,':',A,''' in ',A)") &
            trim(var%name), trim(attname), trim(file%name)
        call snc_handle_error(nf90_get_att(file%ncid, var%id, attname, attvalue), err_msg)
    end subroutine snc_get_att_str

    subroutine snc_get_atti(file, var, attname, attvalue)
        type(SNCFile), intent(in) :: file
        type(SNCVar), intent(in) :: var
        character(*), intent(in) :: attname
        integer, intent(out) :: attvalue(:)
        character(800) :: err_msg

        write(err_msg, "('getting integer attribute ''',A,':',A,''' in ',A)") &
            trim(var%name), trim(attname), trim(file%name)
        call snc_handle_error(nf90_get_att(file%ncid, var%id, attname, attvalue), err_msg)
    end subroutine snc_get_atti

    subroutine snc_get_attf(file, var, attname, attvalue)
        type(SNCFile), intent(in) :: file
        type(SNCVar), intent(in) :: var
        character(*), intent(in) :: attname
        real*4, intent(out) :: attvalue
        character(800) :: err_msg

        write(err_msg, "('getting float attribute ''',A,':',A,''' in ',A)") &
            trim(var%name), trim(attname), trim(file%name)
        call snc_handle_error(nf90_get_att(file%ncid, var%id, attname, attvalue), err_msg)
    end subroutine snc_get_attf

    subroutine snc_get_attd(file, var, attname, attvalue)
        type(SNCFile), intent(in) :: file
        type(SNCVar), intent(in) :: var
        character(*), intent(in) :: attname
        real*8, intent(out) :: attvalue
        character(800) :: err_msg

        write(err_msg, "('getting double attribute ''',A,':',A,''' in ',A)") &
            trim(var%name), trim(attname), trim(file%name)
        call snc_handle_error(nf90_get_att(file%ncid, var%id, attname, attvalue), err_msg)
    end subroutine snc_get_attd

    ! Write an attribute to the NetCDF file.
    subroutine snc_put_att_str(file, var, attname, attvalue)
        type(SNCFile), intent(in) :: file
        type(SNCVar), intent(in) :: var
        character(*), intent(in) :: attname, attvalue
        character(700) :: err_msg

        write(err_msg, "('creating string attribute ''',A,''' for variable ',A,' in ',A)") &
            trim(attname), trim(var%name), trim(file%name)
        call snc_handle_error(nf90_put_att(file%ncid, var%id, attname, attvalue), err_msg)
    end subroutine snc_put_att_str

    subroutine snc_put_atti(file, var, attname, attvalue)
        type(SNCFile), intent(in) :: file
        type(SNCVar), intent(in) :: var
        character(*), intent(in) :: attname
        integer, intent(in) :: attvalue
        character(700) :: err_msg

        write(err_msg, "('creating int attribute ''',A,''' for variable ',A,' in ',A)") &
            trim(attname), trim(var%name), trim(file%name)
        call snc_handle_error(nf90_put_att(file%ncid, var%id, attname, attvalue), err_msg)
    end subroutine snc_put_atti

    subroutine snc_put_attf(file, var, attname, attvalue)
        type(SNCFile), intent(in) :: file
        type(SNCVar), intent(in) :: var
        character(*), intent(in) :: attname
        real*4, intent(in) :: attvalue
        character(700) :: err_msg

        write(err_msg, "('creating float attribute ''',A,''' for variable ',A,' in ',A)") &
            trim(attname), trim(var%name), trim(file%name)
        call snc_handle_error(nf90_put_att(file%ncid, var%id, attname, attvalue), err_msg)
    end subroutine snc_put_attf

    subroutine snc_put_attd(file, var, attname, attvalue)
        type(SNCFile), intent(in) :: file
        type(SNCVar), intent(in) :: var
        character(*), intent(in) :: attname
        real*8, intent(in) :: attvalue
        character(700) :: err_msg

        write(err_msg, "('creating attribute ''',A,''' for variable ',A,' in ',A)") &
            trim(attname), trim(var%name), trim(file%name)
        call snc_handle_error(nf90_put_att(file%ncid, var%id, attname, attvalue), err_msg)
    end subroutine snc_put_attd



    ! Finish the NetCDF data definition and prepare for reading. Call when
    ! you are ready to write variables to the file.
    subroutine snc_enddef(file)
        type(SNCFile), intent(in) :: file
        character(550) :: err_msg

        write(err_msg, "('Ending header definition for ',A)") trim(file%name)
        call snc_handle_error(nf90_enddef(file%ncid), err_msg)
    end subroutine snc_enddef

    ! Closes the NetCDF file after you are done reading from or writing to it.
    subroutine snc_close(file)
        type(SNCFile), intent(in) :: file
        character(520) :: err_msg

        write(err_msg, "('closing ',A)") trim(file%name)
        call snc_handle_error(nf90_close(file%ncid), err_msg)
    end subroutine snc_close



    ! Check the return status of a NetCDF function.  If there is an error,
    ! it prints the NetCDF error string, the error message if given, then
    ! stops the program.
    subroutine snc_handle_error(status, message)
        integer, intent(in) :: status
        character(*), intent(in), optional :: message

        if (status /= NF90_NOERR) then
            print *, trim(nf90_strerror(status))
            if (present(message)) print "('   While: ',A)", trim(message)
            stop
        end if
    end subroutine snc_handle_error

end module SimpleNetCDF
