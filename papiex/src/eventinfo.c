#include "papiex_internal.h"

static char *is_derived(PAPI_event_info_t *info)
{
  if (strlen(info->derived) == 0)
    return("No");
  else if (strcmp(info->derived,"NOT_DERIVED") == 0)
    return("No");
  else if (strcmp(info->derived,"DERIVED_CMPD") == 0)
    return("No");
  else
    return("Yes");
}

static void print_notes(FILE *out, int eventcode, PAPI_event_info_t *info, int verbose)
{
  if (!verbose)
    {
      fprintf(out,"%-30s: %s\n",info->symbol,info->long_descr);
      return;
    }

  fprintf(out,"%-30s: %s\n",(eventcode & PAPI_PRESET_MASK ? "Preset Event" : "Native Event"),info->symbol);
  if (strlen(info->short_descr))
    fprintf(out,"%-30s: %s\n","Short Description", info->short_descr);
  if (strlen(info->long_descr))
    fprintf(out,"%-30s: %s\n","Long Description", info->long_descr);
  if (strlen(info->note))
    fprintf(out,"%-30s: %s\n","Notes", info->note);
  if (eventcode & PAPI_PRESET_MASK) {
    fprintf(out,"%-30s: %s\n","Derived",is_derived(info));
    fprintf(out,"%-30s: %s\n", "Derived type", info->derived);
    fprintf(out,"%-30s: %s\n","Postfix processing string", info->postfix);
  }
}

void _papiex_dump_event_info(FILE *out, int eventcode, int verbose)
{
  int j,k,retval;
  PAPI_event_info_t n_info;
  PAPI_event_info_t info;
  char tmp[LINE_MAX];

  retval = PAPI_get_event_info(eventcode, &info);
  if (retval != PAPI_OK)
    PAPIEX_PAPI_ERROR("PAPI_get_event_info",retval);
  
  print_notes(out,eventcode,&info,verbose);
  if (verbose == 0)
    return;

  if (eventcode & PAPI_PRESET_MASK) 
    {
      for (j=0;j<(int)info.count;j++) 
	{
	  sprintf(tmp,"Native name[%d]",j);
	  fprintf(out,"%-30s: %s\n",tmp,info.name[j]);
	  retval = PAPI_get_event_info(info.code[j], &n_info);
	  if (retval != PAPI_OK)
	    PAPIEX_PAPI_ERROR("PAPI_get_event_info",retval);
	  fprintf(out,"%-30s: %d\n","Number of register values", n_info.count);
	  for (k=0;k<(int)n_info.count;k++)
	    {
	      sprintf(tmp,"Register name[%d]",k);
	      fprintf(out,"%-30s: %s\n",tmp, n_info.name[k]);
	      sprintf(tmp,"Register code[%d]",k);
	      fprintf(out,"%-30s: 0x%-10x\n",tmp, n_info.code[k]);
	    }
	  fprintf(out,"%-30s: %s\n","Native event description", n_info.long_descr);
	}
    }
  else 
    { 
      for (k=0;k<(int)info.count;k++) {
	sprintf(tmp,"Register name[%d]",k);
	fprintf(out,"%-30s: %s\n",tmp, info.name[k]);
	sprintf(tmp,"Register code[%d]",k);
	fprintf(out,"%-30s: 0x%-10x\n",tmp, info.code[k]);
      }
      /* If there's a unit mask specified first, then just print that unit mask */
      /* If not, then print all the available unit masks */
      if (strchr(info.symbol,':'))
	return;
      
      while (PAPI_enum_event(&eventcode, PAPI_NTV_ENUM_UMASKS) == PAPI_OK) {
	retval = PAPI_get_event_info(eventcode, &info);
	if (retval != PAPI_OK)
	  PAPIEX_PAPI_ERROR("PAPI_get_event_info",retval);

	print_notes(out,eventcode,&info,verbose);
	for (k=0;k<(int)info.count;k++) {
	  sprintf(tmp,"Register name[%d]",k);
	  fprintf(out,"%-30s: %s\n",tmp, info.name[k]);
	  sprintf(tmp,"Register code[%d]",k);
	  fprintf(out,"%-30s: 0x%-10x\n",tmp, info.code[k]);
	}
      }
    }
}
