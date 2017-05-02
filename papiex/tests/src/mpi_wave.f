      program main

c*********************************************************************72
c
cc MAIN is the main program for WAVE_MPI.
c
c  Discussion:
c
c    WAVE_MPI solves the wave equation in parallel using MPI.
c
c    Discretize the equation for u(x,t):
c      d^2 u/dt^2 - c^2 * d^2 u/dx^2 = 0  for 0 < x < 1, 0 < t
c    with boundary conditions:
c      u(0,t) = u0(t) = sin ( 2 * pi * ( 0 - c * t ) )
c      u(1,t) = u1(t) = sin ( 2 * pi * ( 1 - c * t ) )
c    and initial conditions:
c         u(x,0) = g(x,t=0) =                sin ( 2 * pi * ( x - c * t ) )
c      dudt(x,0) = h(x,t=0) = - 2 * pi * c * cos ( 2 * pi * ( x - c * t ) ) 
c
c    by:
c
c      alpha = c * dt / dx.
c
c      U(x,t+dt) = 2 U(x,t) - U(x,t-dt) 
c        + alpha^2 ( U(x-dx,t) - 2 U(x,t) + U(x+dx,t) ).
c
c  Licensing:
c
c    This code is distributed under the GNU LGPL license. 
c
c  Modified:
c
c    17 November 2013
c
c  Author:
c
c    John Burkardt
c
c  Reference:
c
c    Geoffrey Fox, Mark Johnson, Gregory Lyzenga, Steve Otto, John Salmon, 
c    David Walker,
c    Solving problems on concurrent processors, 
c    Volume 1: General Techniques and Regular Problems,
c    Prentice Hall, 1988,
c    ISBN: 0-13-8230226,
c    LC: QA76.5.F627.
c
c  Local parameters:
c
c    Local, double precision DT, the time step.
c
c    Local, integer ID, the MPI process ID.
c
c    Local, integer N_GLOBAL, the total number of points.
c
c    Local, integer N_LOCAL, the number of points visible to 
c    this process.
c
c    Local, integer NSTEPS, the number of time steps.
c
c    Local, integer P, the number of MPI processes.
c
      implicit none

      include 'mpif.h'

      integer n_global
      parameter ( n_global = 401 )

      double precision dt
      parameter ( dt = 0.00125 )
      integer error
      integer i_global_hi
      integer i_global_lo
      integer id
      integer n_local
      integer nsteps
      parameter ( nsteps = 4000 )
      integer p
c
c  I really need an allocatable array here.  
c  Just to get it done, I'll settle for using N_GLOBAL as the dimension.
c
      double precision u1_local(n_global)
      double precision wtime
c
c  Initialize MPI.
c
      call MPI_Init ( error )

      call MPI_Comm_rank ( MPI_COMM_WORLD, id, error )

      call MPI_Comm_size ( MPI_COMM_WORLD, p, error )

      if ( id .eq. 0 ) then
        call timestamp ( )
        write ( *, '(a)' ) ''
        write ( *, '(a)' ) 'MPI_WAVE:'
        write ( *, '(a)' ) '  FORTRAN77 version.'
        write ( *, '(a)' ) 
     &    '  Estimate a solution of the wave equation using MPI.'
        write ( *, '(a)' ) ''
        write ( *, '(a,i2,a)' ) '  Using ', p, ' processes.'
        write ( *, '(a,i6,a)' ) 
     &    '  Using a total of ', n_global, ' points.'
        write ( *, '(a,i6,a,g14.6)' ) 
     &    '  Using ', nsteps, ' time steps of size ', dt
        write ( *, '(a,g14.6)' ) 
     &    '  Computing final solution at time ', dt * nsteps
      end if

      wtime = MPI_Wtime ( )
c
c  Determine N_LOCAL
c
      i_global_lo = (   id       * ( n_global - 1 ) ) / p
      i_global_hi = ( ( id + 1 ) * ( n_global - 1 ) ) / p
      if ( 0 .lt. id ) then
        i_global_lo = i_global_lo - 1
      end if

      n_local = i_global_hi + 1 - i_global_lo
c
c  Update N_LOCAL values.
c
      call update ( id, p, n_global, n_local, nsteps, dt, u1_local )
c
c  Collect local values into global array.
c
      call collect ( id, p, n_global, n_local, nsteps, dt, u1_local )
c
c  Report elapsed wallclock time.
c
      wtime = MPI_Wtime ( ) - wtime

      if ( id .eq. 0 ) then
        write ( *, '(a)' ) ''
        write ( *, '(a,g14.6,a)' ) 
     &    '  Elapsed wallclock time was ', wtime, ' seconds.'
      end if
c
c  Terminate MPI.
c
      call MPI_Finalize ( error )
c
c  Terminate.
c
      if ( id .eq. 0 ) then
        write ( *, '(a)' ) ''
        write ( *, '(a)' ) 'WAVE_MPI:'
        write ( *, '(a)' ) '  Normal end of execution.'
        write ( *, '(a)' ) ''
        call timestamp ( )
      end if

      stop
      end
      subroutine update ( id, p, n_global, n_local, nsteps, dt, 
     &  u1_local ) 

c*********************************************************************72
c
cc UPDATE advances the solution a given number of time steps.
c
c  Licensing:
c
c    This code is distributed under the GNU LGPL license. 
c
c  Modified:
c
c    17 November 2013
c
c  Author:
c
c    John Burkardt
c
c  Parameters:
c
c    Input, integer ID, the identifier of this process.
c
c    Input, integer P, the number of processes.
c
c    Input, integer N_GLOBAL, the total number of points.
c
c    Input, integer N_LOCAL, the number of points visible to 
c    this process.
c
c    Input, integer NSTEPS, the number of time steps.
c
c    Input, double precision DT, the size of the time step.
c
c    Output, double precision U1_LOCAL(N_LOCAL), the portion of the solution
c    at the last time, as evaluated by this process.
c
      implicit none

      include 'mpif.h'

      integer n_local

      double precision alpha
      double precision alpha2
      double precision c
      double precision dt
      double precision dudt
      double precision dx
      integer error
      double precision exact
      integer i
      integer i_global
      integer i_global_hi
      integer i_global_lo
      integer i_local
      integer i_local_hi
      integer i_local_lo
      integer id
      integer j
      integer ltor
      parameter ( ltor = 20 )
      integer n_global
      integer nsteps
      integer p
      integer rtol
      parameter ( rtol = 10 )
      integer status(MPI_STATUS_SIZE)
      double precision t
      double precision u0_local(n_local)
      double precision u1_local(n_local)
      double precision u2_local(n_local)
      double precision x
c
c  Determine the value of ALPHA.
c
      c = 1.0D+00
      dx = 1.0D+00 / dble ( n_global - 1 )
      alpha = c * dt / dx
      alpha2 = alpha * alpha

      if ( 1.0D+00 .le. abs ( alpha ) ) then

        if ( id .eq. 0 ) then
          write ( *, '(a)' ) ''
          write ( *, '(a)' ) 'UPDATE - Warning!'
          write ( *, '(a)' ) '  1 <= |ALPHA| = | C * dT / dX |.'
          write ( *, '(a,g14.6)' ) '  C = ', c
          write ( *, '(a,g14.6)' ) '  dT = ', dt
          write ( *, '(a,g14.6)' ) '  dX = ', dx
          write ( *, '(a,g14.6)' ) '  ALPHA = ', alpha
          write ( *, '(a)' ) '  Computation will not be stable!'
        end if

        call MPI_Finalize ( error )
        stop 1

      end if
c
c  The global array of N_GLOBAL points must be divided up among the processes.
c  Each process stores about 1/P of the total + 2 extra slots.
c
      i_global_lo = (   id       * ( n_global - 1 ) ) / p
      i_global_hi = ( ( id + 1 ) * ( n_global - 1 ) ) / p
      if ( 0 .lt. id ) then
        i_global_lo = i_global_lo - 1
      end if

      i_local_lo = 0
      i_local_hi = i_global_hi - i_global_lo

      t = 0.0D+00
      do i_global = i_global_lo, i_global_hi
        x = dble ( i_global ) / dble ( n_global - 1 )
        i_local = i_global - i_global_lo
        u1_local(i_local+1) = exact ( x, t )
      end do

      do i_local = i_local_lo, i_local_hi
        u0_local(i_local+1) = u1_local(i_local+1)
      end do
c
c  Take NSTEPS time steps.
c
      do i = 1, nsteps

        t = dt * dble ( i )
c 
c  For the first time step, we need to use the initial derivative information.
c
        if ( i .eq. 1 ) then

          do i_local = i_local_lo + 1, i_local_hi - 1
            i_global = i_global_lo + i_local
            x = dble ( i_global ) / dble ( n_global - 1 )
            u2_local(i_local+1) = 
     &        +         0.5D+00 * alpha2   * u1_local(i_local-1+1) 
     &        + ( 1.0D+00 -       alpha2 ) * u1_local(i_local+0+1) 
     &        +         0.5D+00 * alpha2   * u1_local(i_local+1+1) 
     &        +                         dt * dudt ( x, t )
          end do
c
c  After the first time step, we can use the previous two solution estimates.
c
        else

          do i_local = i_local_lo + 1, i_local_hi - 1
            u2_local(i_local+1) = 
     &        +               alpha2   * u1_local(i_local-1+1) 
     &        + 2.0 * ( 1.0 - alpha2 ) * u1_local(i_local+0+1) 
     &        +               alpha2   * u1_local(i_local+1+1) 
     &        -                          u0_local(i_local+0+1)
          end do

        end if
c
c  Exchange data with "left-hand" neighbor. 
c
        if ( 0 .lt. id ) then
          call MPI_Send ( u2_local(i_local_lo+2), 1, 
     &      MPI_DOUBLE_PRECISION, id - 1, rtol, MPI_COMM_WORLD, error )
          call MPI_Recv ( u2_local(i_local_lo+1), 1, 
     &      MPI_DOUBLE_PRECISION, id - 1, ltor, MPI_COMM_WORLD, status, 
     &      error )
        else
          x = 0.0D+00
          u2_local(i_local_lo+1) = exact ( x, t )
        end if
c
c  Exchange data with "right-hand" neighbor.
c
        if ( id .lt. p - 1 ) then
          call MPI_Send ( u2_local(i_local_hi), 1, MPI_DOUBLE_PRECISION, 
     &      id + 1, ltor, MPI_COMM_WORLD, error )
          call MPI_Recv ( u2_local(i_local_hi+1),   1, 
     &      MPI_DOUBLE_PRECISION, id + 1, rtol, MPI_COMM_WORLD, status, 
     &      error )
        else
          x = 1.0D+00
          u2_local(i_local_hi+1) = exact ( x, t )
        end if
c
c  Shift data for next time step.
c
        do i_local = i_local_lo, i_local_hi
          u0_local(i_local+1) = u1_local(i_local+1)
          u1_local(i_local+1) = u2_local(i_local+1)
        end do

      end do

      return
      end
      subroutine collect ( id, p, n_global, n_local, nsteps, dt, 
     &  u_local ) 

c*********************************************************************72
c
cc COLLECT has workers send results to the master, which prints them.
c
c  Licensing:
c
c    This code is distributed under the GNU LGPL license. 
c
c  Modified:
c
c    17 November 2013
c
c  Author:
c
c    John Burkardt
c
c  Parameters:
c
c    Input, integer ID, the identifier of this process.
c
c    Input, integer P, the number of processes.
c
c    Input, integer N_GLOBAL, the total number of points.
c
c    Input, integer N_LOCAL, the number of points visible
c    to this process.
c
c    Input, integer NSTEPS, the number of time steps.
c
c    Input, double precision DT, the size of the time step.
c
c    Input, double precision U_LOCAL(N_LOCAL), the final solution estimate 
c    computed by this process.
c
      implicit none

      include 'mpif.h'

      integer n_global
      integer n_local

      integer buffer(2)
      integer collect1
      parameter ( collect1 = 10 )
      integer collect2
      parameter ( collect2 = 20 )
      double precision dt
      integer error
      double precision exact
      integer i
      integer i_global
      integer i_global_hi
      integer i_global_lo
      integer i_local
      integer i_local_hi
      integer i_local_lo
      integer id
      integer j
      integer n_local2
      integer nsteps
      integer p
      integer status(MPI_STATUS_SIZE)
      double precision t
      double precision u_global(n_global)
      double precision u_local(n_local)
      double precision x

      i_global_lo = (   id       * ( n_global - 1 ) ) / p
      i_global_hi = ( ( id + 1 ) * ( n_global - 1 ) ) / p
      if ( 0 < id ) then
        i_global_lo = i_global_lo - 1
      end if

      i_local_lo = 0
      i_local_hi = i_global_hi - i_global_lo
c
c  Master collects worker results into the U_GLOBAL array.
c
      if ( id .eq. 0 ) then
c
c  Copy the master's results into the global array.
c
        do i_local = i_local_lo, i_local_hi
          i_global = i_global_lo + i_local - i_local_lo
          u_global(i_global+1) = u_local(i_local+1)
        end do
c
c  Contact each worker.
c
        do i = 1, p - 1
c
c  Message "collect1" contains the global index and number of values.
c
          call MPI_Recv ( buffer, 2, MPI_INTEGER, i, collect1, 
     &      MPI_COMM_WORLD, status, error )

          i_global_lo = buffer(1)
          n_local2 = buffer(2)

          if ( i_global_lo < 0 ) then
            write ( *, '(a,i6)' ) 
     &        '  Illegal I_GLOBAL_LO = ', i_global_lo
            call MPI_Finalize ( error )
            stop 1
          else if ( n_global <= i_global_lo + n_local2 - 1 ) then
            write ( *, '(a,i6)' ) 
     &        '  Illegal I_GLOBAL_LO + N_LOCAL = ',
     &        i_global_lo + n_local2
            call MPI_Finalize ( error )
            stop 1
          end if
c
c  Message "collect2" contains the values.
c
          call MPI_Recv ( u_global(i_global_lo+1), n_local2, 
     &      MPI_DOUBLE_PRECISION, i, collect2, MPI_COMM_WORLD, 
     &      status, error )

        end do
c
c  Print the results.
c
        t = dt * dble ( nsteps )
        write ( *, '(a)' ) ''
        write ( *, '(a)' ) '    I      X     F(X)   Exact'
        write ( *, '(a)' ) ''
        do i_global = 0, n_global - 1
          x = dble ( i_global ) / dble ( n_global - 1 )
          write ( *, '(2x,i3,2x,f6.3,2x,f6.3,2x,f6.3)' ) 
     &      i_global, x, u_global(i_global+1), exact ( x, t )
        end do
c
c  Workers send results to process 0.
c
      else
c
c  Message "collect1" contains the global index and number of values.
c
        buffer(1) = i_global_lo
        buffer(2) = n_local
        call MPI_Send ( buffer, 2, MPI_INTEGER, 0, collect1, 
     &    MPI_COMM_WORLD, error )
c
c  Message "collect2" contains the values.
c
        call MPI_Send ( u_local, n_local, MPI_DOUBLE_PRECISION, 0, 
     &    collect2, MPI_COMM_WORLD, error )

      end if

      return
      end
      function exact ( x, t )

c*********************************************************************72
c
cc EXACT evaluates the exact solution
c
c  Licensing:
c
c    This code is distributed under the GNU LGPL license. 
c
c  Modified:
c
c    17 November 2013
c
c  Author:
c
c    John Burkardt
c
c  Parameters:
c
c    Input, double precision X, the location.
c
c    Input, double precision T, the time.
c
c    Output, double precision EXACT, the value of the exact solution.
c
      implicit none

      double precision c
      parameter ( c = 1.0D+00 )
      double precision exact
      double precision pi
      parameter ( pi = 3.141592653589793D+00 )
      double precision t
      double precision x

      exact = sin ( 2.0D+00 * pi * ( x - c * t ) )

      return
      end
      function dudt ( x, t )

c*********************************************************************72
c
cc DUDT evaluates the partial derivative dudt.
c
c  Licensing:
c
c    This code is distributed under the GNU LGPL license. 
c
c  Modified:
c
c    17 November 2013
c
c  Author:
c
c    John Burkardt
c
c  Parameters:
c
c    Input, double precision X, the location.
c
c    Input, double precision T, the time.
c
c    Output, double precision DUDT, the value of the time derivative of 
c    the solution.
c
      implicit none

      double precision c
      parameter ( c = 1.0D+00 )
      double precision dudt
      double precision pi
      parameter ( pi = 3.141592653589793D+00 )
      double precision t
      double precision x

      dudt = - 2.0D+00 * pi * c * cos ( 2.0D+00 * pi * ( x - c * t ) )

      return
      end
      subroutine timestamp ( )

c*********************************************************************72
c
cc TIMESTAMP prints out the current YMDHMS date as a timestamp.
c
c  Licensing:
c
c    This code is distributed under the GNU LGPL license.
c
c  Modified:
c
c    12 January 2007
c
c  Author:
c
c    John Burkardt
c
c  Parameters:
c
c    None
c
      implicit none

      character * ( 8 ) ampm
      integer d
      character * ( 8 ) date
      integer h
      integer m
      integer mm
      character * ( 9 ) month(12)
      integer n
      integer s
      character * ( 10 ) time
      integer y

      save month

      data month /
     &  'January  ', 'February ', 'March    ', 'April    ',
     &  'May      ', 'June     ', 'July     ', 'August   ',
     &  'September', 'October  ', 'November ', 'December ' /

      call date_and_time ( date, time )

      read ( date, '(i4,i2,i2)' ) y, m, d
      read ( time, '(i2,i2,i2,1x,i3)' ) h, n, s, mm

      if ( h .lt. 12 ) then
        ampm = 'AM'
      else if ( h .eq. 12 ) then
        if ( n .eq. 0 .and. s .eq. 0 ) then
          ampm = 'Noon'
        else
          ampm = 'PM'
        end if
      else
        h = h - 12
        if ( h .lt. 12 ) then
          ampm = 'PM'
        else if ( h .eq. 12 ) then
          if ( n .eq. 0 .and. s .eq. 0 ) then
            ampm = 'Midnight'
          else
            ampm = 'AM'
          end if
        end if
      end if

      write ( *,
     &  '(i2,1x,a,1x,i4,2x,i2,a1,i2.2,a1,i2.2,a1,i3.3,1x,a)' )
     &  d, month(m), y, h, ':', n, ':', s, '.', mm, ampm

      return
      end
