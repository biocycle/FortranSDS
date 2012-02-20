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

    integer, parameter :: SNC_UNLIMITED = NF90_UNLIMITED
    !integer, parameter :: SNC_BYTE = NF90_BYTE
    !integer, parameter :: SNC_CHAR = NF90_CHAR
    !integer, parameter :: SNC_SHORT = NF90_SHORT
    integer, parameter :: SNC_INT = NF90_INT
    integer, parameter :: SNC_FLOAT = NF90_FLOAT
    integer, parameter :: SNC_DOUBLE = NF90_DOUBLE

    integer, parameter :: READ_MODE = NF90_SHARE
#ifndef HAVE_NETCDF4
    integer, parameter :: WRITE_MODE = NF90_NOCLOBBER
#else
    integer, parameter :: WRITE_MODE = IOR(NF90_NOCLOBBER, NF90_HDF5)
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

    function snc_get_dim(file, dimname)
        type(SNCFile), intent(in) :: file
        character(*), intent(in) :: dimname
        integer :: snc_get_dim
        integer :: dimid
        character(700) :: err_msg

        write(err_msg, "('getting id for dimension ''',A,''' in ',A)") trim(dimname), trim(file%name)
        call snc_handle_error(nf90_inq_dimid(file%ncid, dimname, dimid), err_msg)

        write(err_msg, "('getting length for dimension ''',A,''' in ',A)") trim(dimname), trim(file%name)
        call snc_handle_error(nf90_inquire_dimension(file%ncid, dimid, len = snc_get_dim), err_msg)
    end function snc_get_dim

    function snc_inq_var(file, varname)
        type(SNCFile), intent(in) :: file
        character(*), intent(in) :: varname
        type(SNCVar) :: snc_inq_var
        character(700) :: err_msg
        integer :: status, i, dimids(4)

        snc_inq_var%name = varname

        write(err_msg, "('inquiring variable''s id ''',A,''' in ',A)") trim(varname), trim(file%name)
        status = nf90_inq_varid(file%ncid, varname, snc_inq_var%id)
        call snc_handle_error(status, err_msg)

        write(err_msg, "('inquiring about variable ''',A,''' in ',A)") trim(varname), trim(file%name)
        status = nf90_inquire_variable(file%ncid, snc_inq_var%id, &
            ndims = snc_inq_var%ndims, dimids = dimids)
        call snc_handle_error(status, err_msg)
        if (snc_inq_var%ndims > size(dimids)) then
            print "('Too many dimensions in variable ''',A,''' in ',A)", trim(varname), trim(file%name)
            stop
        end if

        do i = 1, snc_inq_var%ndims
            write(err_msg, "('getting length for dimension ',I1,' of ''',A,''' in ',A)") i, trim(varname), trim(file%name)
            call snc_handle_error(nf90_inquire_dimension(file%ncid, dimids(i), len = snc_inq_var%dims(i)), err_msg)
        end do
    end function snc_inq_var


    subroutine snc_get_att(file, var, attname, attvalue)
        type(SNCFile), intent(in) :: file
        type(SNCVar), intent(in) :: var
        character(*), intent(in) :: attname
        character(*), intent(out) :: attvalue
        character(800) :: err_msg

        write(err_msg, "('getting attribute ''',A,':',A,''' in ',A)") &
            trim(var%name), trim(attname), trim(file%name)
        call snc_handle_error(nf90_get_att(file%ncid, var%id, attname, attvalue), err_msg)
    end subroutine snc_get_att

    subroutine snc_read2f(file, var, data)
       type(SNCFile), intent(in) :: file
        type(SNCVar), intent(in) :: var
        real*4, pointer :: data(:,:)
        character(700) :: err_msg

        allocate(data(var%dims(1), var%dims(2)))
        write(err_msg, "('reading variable ''',A,''' in ',A)") trim(var%name), trim(file%name)
        call snc_handle_error(nf90_get_var(file%ncid, var%id, data), err_msg)
    end subroutine snc_read2f

    ! Open a NetCDF file for writing
    function snc_create(filename)
        character(*), intent(in) :: filename
        type(SNCFile) :: snc_create
        character(530) :: err_msg

        write(err_msg, "('creating ',A)") trim(filename)
        call snc_handle_error(nf90_create(filename, WRITE_MODE, snc_create%ncid), err_msg)
        snc_create%name = filename
    end function snc_create

    function snc_def_dim(file, dimname, dimsize)
        type(SNCFile), intent(in) :: file
        character(*), intent(in) :: dimname
        integer, intent(in) :: dimsize
        integer :: snc_def_dim
        integer :: dimid
        character(700) :: err_msg

        write(err_msg, "('defining dimension ''',A,''' = ',I6,'in ',A)") trim(dimname), dimsize, trim(file%name)
        call snc_handle_error(nf90_def_dim(file%ncid, dimname, dimsize, dimid), err_msg)
        snc_def_dim = dimid
    end function snc_def_dim

    function snc_def_var(file, varname, vartype, dimids)
        type(SNCFile), intent(in) :: file
        character(*), intent(in) :: varname
        integer, intent(in) :: vartype
        integer, intent(in), dimension(:) :: dimids
        type(SNCVar) :: snc_def_var
        character(700) :: err_msg

        write(err_msg, "('defining variable ''',A,''' in ',A)") trim(varname), trim(file%name)
        call snc_handle_error(nf90_def_var(file%ncid, varname, vartype, dimids, snc_def_var%id), err_msg)
        snc_def_var%name = varname
    end function snc_def_var

    subroutine snc_put_att(file, var, attname, attvalue)
        type(SNCFile), intent(in) :: file
        type(SNCVar), intent(in) :: var
        character(*), intent(in) :: attname, attvalue
        character(700) :: err_msg

        write(err_msg, "('creating attribute ''',A,''' for variable ',A,' in ',A)") &
            trim(attname), trim(var%name), trim(file%name)
        call snc_handle_error(nf90_put_att(file%ncid, var%id, attname, attvalue), err_msg)
    end subroutine snc_put_att

    subroutine snc_enddef(file)
        type(SNCFile), intent(in) :: file
        character(550) :: err_msg

        write(err_msg, "('Ending header definition for ',A)") trim(file%name)
        call snc_handle_error(nf90_enddef(file%ncid), err_msg)
    end subroutine snc_enddef

    subroutine snc_write2f(file, var, data)
        type(SNCFile), intent(in) :: file
        type(SNCVar), intent(in) :: var
        real, intent(in), dimension(:,:) :: data
        character(700) :: err_msg

        write(err_msg, "('writing 2d float variable ''',A,''' to ',A)") trim(var%name), trim(file%name)
        call snc_handle_error(nf90_put_var(file%ncid, var%id, data), err_msg)
    end subroutine snc_write2f

    subroutine snc_close(file)
        type(SNCFile), intent(in) :: file
        character(520) :: err_msg

        write(err_msg, "('closing ',A)") trim(file%name)
        call snc_handle_error(nf90_close(file%ncid), err_msg)
    end subroutine snc_close

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
