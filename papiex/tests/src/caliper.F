#include "papiex.h"
      
      program zero

      real a, b, c;
      a = 0.1
      b = 1.1
      c = 2.1

#if defined(FULL_CALIPER)
      PAPIEX_START_ARG(1,"write")
#endif

      print *, "Doing 10000000 iters. of a += b * c on doubles."

#if defined(FULL_CALIPER)
      PAPIEX_STOP_ARG(1)
#endif

#if !defined(FULL_CALIPER)
      PAPIEX_START()
#else
      PAPIEX_START_ARG(2,"do loop")
#endif

      do i=1,100000000
         a = a + b * c
      end do

#if !defined(FULL_CALIPER)
      PAPIEX_STOP()
#else
      PAPIEX_STOP_ARG(2)
#endif

      end

