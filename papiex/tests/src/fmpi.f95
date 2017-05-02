program main

  implicit none

  include 'mpif.h'

  integer ret
  integer iam
  integer namelen
  
  integer :: size
  
  integer(8) :: looplen = 10000000

  character(len=256):: name

  name = ' '

  call MPI_Init (ret)
  call MPI_Comm_rank (MPI_COMM_WORLD, iam, ret)
  write(6,*) "Hello from mpi task number ", iam
  
  call MPI_Comm_size (MPI_COMM_WORLD, size, ret)
  write(6,*) "There are ", size, " tasks in this communicator"

  call MPI_Get_processor_name (name, namelen, ret)
  write(6,*) "Task ", iam, " is running on proc ", name

  call do_some_work (iam, size, looplen)
  call MPI_Finalize (ret)
end program main

subroutine do_some_work (iam, size, looplen)
  implicit none

  integer, intent(in) :: iam
  integer, intent(in) :: size
  integer(8), intent(in) :: looplen

  include 'mpif.h'

  integer :: i
  integer :: ierr
  integer :: junk=11
  integer :: junkt
  integer :: left,right
  integer :: stat (MPI_STATUS_SIZE)

  left = iam - 1
  if (left < 0) then
    left = size - 1
  end if
  right = iam + 1
  if (right > size - 1) then
    right = 0
  end if

  do i=1,looplen
    call mpi_sendrecv (junk, 1, MPI_INTEGER, right, 1, &
                       junkt, 1, MPI_INTEGER, left, 1, MPI_COMM_WORLD, stat, ierr)
  end do

  return
end subroutine do_some_work
