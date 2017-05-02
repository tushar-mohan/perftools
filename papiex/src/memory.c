#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include "papiex_internal.h"

/*
 * This call has been superseded by PAPI_get_dmem_info
 * and should not be used
 *
static void get_memory_info(long *size, long *resident, long *shared, long *text, long *library, long *heap, long *locked, long *stack)
{
  char fn[PATH_MAX], tmp[PATH_MAX];
  FILE *f;
  int ret;
  long sz = 0, lck = 0, res = 0, shr = 0, stk = 0, txt = 0, dat = 0, dum = 0, lib = 0;

  sprintf(fn,"/proc/%ld/status",(long)getpid());
  f = fopen(fn,"r");
  if (f == NULL)
    {
      fprintf(stderr,"fopen(%s): %s\n",fn,strerror(errno));
      return;
    }
  while (1)
    {
      if (fgets(tmp,PATH_MAX,f) == NULL)
	break;
      if (strspn(tmp,"VmSize:") == strlen("VmSize:"))
	{
	  sscanf(tmp+strlen("VmSize:"),"%ld",&sz);
	  *size = sz;
	  continue;
	}
      if (strspn(tmp,"VmLck:") == strlen("VmLck:"))
	{
	  sscanf(tmp+strlen("VmLck:"),"%ld",&lck);
	  *locked = lck;
	  continue;
	}
      if (strspn(tmp,"VmRSS:") == strlen("VmRSS:"))
	{
	  sscanf(tmp+strlen("VmRSS:"),"%ld",&res);
	  *resident = res;
	  continue;
	}
      if (strspn(tmp,"VmData:") == strlen("VmData:"))
	{
	  sscanf(tmp+strlen("VmData:"),"%ld",&dat);
	  *heap = dat;
	  continue;
	}
      if (strspn(tmp,"VmStk:") == strlen("VmStk:"))
	{
	  sscanf(tmp+strlen("VmStk:"),"%ld",&stk);
	  *stack = stk;
	  continue;
	}
      if (strspn(tmp,"VmExe:") == strlen("VmExe:"))
	{
	  sscanf(tmp+strlen("VmExe:"),"%ld",&txt);
	  *text = txt;
	  continue;
	}
      if (strspn(tmp,"VmLib:") == strlen("VmLib:"))
	{
	  sscanf(tmp+strlen("VmLib:"),"%ld",&lib);
	  *library = lib;
	  continue;
	}
    }
  fclose(f);

  sprintf(fn,"/proc/%ld/statm",(long)getpid());
  f = fopen(fn,"r");
  if (f == NULL)
    {
      fprintf(stderr,"fopen(%s): %s\n",fn,strerror(errno));
      return;
    }
  ret = fscanf(f,"%ld %ld %ld %ld %ld %ld %ld",&dum,&dum,&shr,&dum,&dum,&dat,&dum);
  if (ret != 7)
    {
      fprintf(stderr,"fscanf(7 items): %d\n",ret);
      return;
    }
  *shared = (shr * getpagesize())/1024;
  fclose(f);

  return;
}
*
* End commented out function -- use PAPI_get_dmem_info instead of the above
*/

void _papiex_dump_memory_info(FILE *output)
{
/* This portion is commented out, as we are now
 * using PAPI_get_dmem_info call (below commented code) 
 *
  long a,b,c,d,e,f,g,h;
  get_memory_info(&a,&b,&c,&d,&e,&f,&g,&h);
  fprintf(output,"Mem Size:\t\t%ld\n",a);
  fprintf(output,"Mem Resident:\t\t%ld\n",b);
  fprintf(output,"Mem Shared:\t\t%ld\n",c);
  fprintf(output,"Mem Text:\t\t%ld\n",d);
  fprintf(output,"Mem Library:\t\t%ld\n",e);
  fprintf(output,"Mem Heap:\t\t%ld\n",f);
  fprintf(output,"Mem Locked:\t\t%ld\n",g);
  fprintf(output,"Mem Stack:\t\t%ld\n",h);
 *
 * End commented section
 */

  PAPI_dmem_info_t dmem_info;
   int retval = PAPI_get_dmem_info(&dmem_info);

  if (PAPI_OK == retval) {
    pretty_printl(output, "[PROCESS] Mem. virtual peak KB" , 0, dmem_info.peak);
    pretty_printl(output, "[PROCESS] Mem. resident peak KB", 0, dmem_info.high_water_mark);
    pretty_printl(output, "[PROCESS] Mem. text KB", 0, dmem_info.text);
    pretty_printl(output, "[PROCESS] Mem. library KB", 0, dmem_info.library);
    pretty_printl(output, "[PROCESS] Mem. heap KB", 0, dmem_info.heap);
    pretty_printl(output, "[PROCESS] Mem. stack KB", 0, dmem_info.stack);
    pretty_printl(output, "[PROCESS] Mem. shared KB", 0, dmem_info.shared);
    pretty_printl(output, "[PROCESS] Mem. locked KB", 0, dmem_info.locked);
  }
  else {
    fprintf(output, "PAPI_get_dmem_info failed with error code: %d", retval);
  }
}
