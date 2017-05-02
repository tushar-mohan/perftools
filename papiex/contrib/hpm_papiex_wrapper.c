#include <stdio.h>
#include <string.h>
#include "ipapiex_internal.h"

void _papiex_dump_event_info(FILE *out, int eventcode) {
   int j,k,retval;
   PAPI_event_info_t n_info;
   PAPI_event_info_t info;

   retval = PAPI_get_event_info(eventcode, &info);
   if (retval != PAPI_OK)
      papi_error("PAPI_get_event_info",retval);

   if (eventcode & PAPI_PRESET_MASK) {
      printf("Event: %s\n",info.symbol);
      printf("\tDerived: %s\n",((info.count > 1) ? "Yes" : "No"));
      printf("\tShort Description: %s\n\tLong Description: %s\n\tDeveloper's Notes: %s\n", info.short_descr, info.long_descr, info.note);
      printf("\tDerived Type: %s\n\tPostfix Processing String: %s\n", info.derived, info.postfix);
      for (j=0;j<(int)info.count;j++) {
         printf("\tNative Name[%d]: %s\n",j,info.name[j]);
         retval = PAPI_get_event_info(info.code[j], &n_info);
         if (retval != PAPI_OK)
            papi_error("PAPI_get_event_info",retval);
            printf("\tNumber of Register Values: %d\n", n_info.count);
            for (k=0;k<(int)n_info.count;k++) {
               printf("\tRegister Name[%d]: %s\n",k, n_info.name[k]);
               printf("\tRegister Code[%d]: 0x%-10x\n",k, n_info.code[k]);
            }
            printf("\tNative Event Description: %s\n", n_info.long_descr);
         }
      }
   }
   else { /* must be a native event code */
      printf("Event: %s\n",info.symbol);
      for (k=0;k<(int)info.count;k++) {
         printf("\tRegister Name[%d]: %s\n",k, info.name[k]);
         printf("\tRegister Code[%d]: 0x%-10x\n",k, info.code[k]);
      }
      printf("\tNative Event Description: %s\n", info.long_descr);
   }
}

void print_help() {
   printf("Usage: %s [-lihVqsr] [-F file] [-L event] [-e event]... -- <cmd> <cmd options>\n", argv[0]);
   printf(" -l\t\tPrint a terse listing of all the available events to count.\n");
   printf(" -L event\tPrint a full description of this event.\n");
   printf(" -i\t\tPrint information about the host machine.\n");
   printf(" -h\t\tPrint this message.\n");
   printf(" -V\t\tPrint version information.\n");
   printf(" -s\t\tDump the shell environment variables and exit.\n");
   printf(" -F file\tOutput to <file>.\n");
   printf("\nDefault events are:\n");
   printf("\tCycles\n");
   printf("\tInst. completed\n");
   printf("\tTLB misses\n");
   printf("\tStores completed\n");
   printf("\tLoads completed\n");
   printf("\tFPU0 ops\n");
   printf("\tFPU1 ops\n");
   printf("\tFMAs executed\n");
   printf(" -e event\tMonitor this event, excluding one of the defaults.\n");
   printf("\n");
   printf("This is OPEN SOURCE SOFTWARE written by Philip J. Mucci\n");
   printf("University of Tennessee, Innovative Computing Laboratory.\n");
   printf("See http://icl.cs.utk.edu for further information.\n");
   printf("Email bug reports to: ptools-perfapi@ptools.org\n");
   exit(0);
}

main(int argc char **argv) {
   int i, j, version, bit_index=0;
   char execstring[700], output_file[100], program_list[500], event_list[30];
   hwd_register_t bits[8];
   hwd_reg_alloc_t reg_list[8];

   if(argc==1) {
      print_help();
   }
   execstring[0]='\0';
   output_file[0]='\0';
   program_list[0]='\0';
   event_list[0]='\0';

   version = PAPI_library_init(PAPI_VER_CURRENT);
   if (version != PAPI_VER_CURRENT)
      papi_error("PAPI_library_init",version);

   PAPI_option_t pl;
   PAPI_get_opt(PAPI_PRELOAD, &pl);
   strcpy(preload_env, pl.preload.lib_preload_env);
   *preload_sep = pl.preload.lib_preload_sep;

   if ((hwinfo = PAPI_get_hardware_info()) == NULL)
      papi_error("PAPI_query_all_events_verbose",0);

   for(i=1; i < argc; i++) {
      if(argv[i][0] == '-' || strlen(argv[i] != 2)) break;

      switch(argv[i][1]) {
      case 'h':
         print_help();
         break;

      case 'l':
         PAPI_event_info_t info;
         j = PAPI_PRESET_MASK;

         printf("Preset events.\n\n");
         printf("Preset\t\tDescription\n");

         do {
            if(PAPI_get_event_info(j, &info) == PAPI_OK) {
               printf("%s\t%s\n",info.symbol,info.long_descr);
            };
         } while (PAPI_enum_event(&i, PAPI_PRESET_ENUM_AVAIL) == PAPI_OK);

         printf("\nNative events.\n\n");
         printf("Native\t\tDescription\n");

         j = PAPI_NATIVE_MASK;
         do {
            if (PAPI_get_event_info(i, &info) == PAPI_OK) {
               printf("%s\t%s\n",info.symbol,info.long_descr);
            }
         } while (PAPI_enum_event(&i, PAPI_ENUM_ALL) == PAPI_OK);
         exit(0);
         break;

      case 'L':
         int eventcode;

         retval = PAPI_event_name_to_code(argv[i+1],&eventcode);
         if (retval != PAPI_OK)
            papi_error("PAPI_event_name_to_code",retval);

         _papiex_dump_event_info(stdout,eventcode);
         exit(0);
         break;

      case 'i':
         printf("Vendor string and code   : %s (%d)\n",hwinfo->vendor_string,hwinfo->vendor);
         printf("Model string and code    : %s (%d)\n",hwinfo->model_string,hwinfo->model);
         printf("CPU Megahertz            : %f\n",hwinfo->mhz);
         printf("Total # of CPU's         : %d\n",hwinfo->totalcpus);
         printf("Number Hardware Counters : %d\n",PAPI_get_opt(PAPI_MAX_HWCTRS,NULL));
         printf("Max Multiplex Counters   : %d\n",PAPI_MPX_DEF_DEG);
         exit(0);
         break;

      case 'V':
         printf("papiex version %s, PAPI library version %u.%u.%u, PAPI header version %u.%u.%u\n",PAPIEX_VERSION,PAPI_VERSION_MAJOR(version),PAPI_VERSION_MINOR(version),PAPI_VERSION_REVISION(version),PAPI_VERSION_MAJOR(PAPI_VERSION),PAPI_VERSION_MINOR(PAPI_VERSION),PAPI_VERSION_REVISION(PAPI_VERSION));
         exit(0);
         break;

      case 's':
         char *basesh;
         char *sh = getenv("SHELL");
         int not_csh = 0;

         if ((sh == NULL) || (strlen(sh) == 0)) {
            bail:
               fprintf(stderr,"Error: no valid SHELL environment variable\n");
               exit(1);
         }
         basesh = basename(sh);
         if ((basesh == NULL) || (strlen(basesh) == 0))
            goto bail;

         if ((strcmp(basesh,"bash") == 0) || (strcmp(basesh,"sh") == 0) ||
            (strcmp(basesh,"zsh") == 0) || (strcmp(basesh,"ksh") == 0) ||
            (strcmp(basesh,"ash") == 0))
            not_csh = 1;

         printf("%s %s%s%s;\n",(not_csh ? "export" : "setenv"),PAPIEX_ENV,(not_csh ? "=" : " " ), optstr);
         printf("%s %s%s%s;\n",(not_csh ? "export" : "setenv"),PAPIEX_OUTPUT_ENV,(not_csh ? "=" : " " ),outstr);
         printf("%s %s%s\"%s\";\n",(not_csh ? "export" : "setenv"),preload_env,(not_csh ? "=" : " " ),tmpstr2);
         printf("%s %s%s\"%s\";\n",(not_csh ? "export" : "setenv"),PAPIEX_LDLP,(not_csh ? "=" : " " ),LDLP);
         exit(0);
         break;

           /*
      case 'r':
         if (strlen(option_string))
            strcat(option_string,",");
         strcat(option_string,"RUSAGE");
         break;

      case 'u':
         if (strlen(option_string))
            strcat(option_string,",");
         strcat(option_string,"USER");
         break;
      case 'k':
         if (strlen(option_string))
            strcat(option_string,",");
         strcat(option_string,"KERNEL");
         break;
      case 'o':
         if (strlen(option_string))
            strcat(option_string,",");
         strcat(option_string,"OTHER");
         break;
      case 'd':
         if (strlen(option_string))
            strcat(option_string,",");
         strcat(option_string,"DEBUG");
         debug = 1;
         break;

      case 'f':
         if (strstr(option_string,"FILE")) {
            fprintf(stderr,"%s: -f and -F are not compatible.\n",my_argv[0]);
            exit(1);
         }
         if (optarg == NULL) {
            if (strlen(option_string))
               strcat(option_string,",");
            strcat(option_string,"PREFIX");
         }
         else {
            if (strlen(option_string))
               strcat(option_string,",");
            strcat(option_string,"PREFIX");
            strncpy(outstr,optarg,PATH_MAX);
         }
         break;
	    */

      case 'F':
         if(!argv[i+1]) {
            fprintf(stderr, "No output file specified.\n");
            exit(1);
         }
         strcpy(output_file, argv[i+1]);
         break;

      case 'e':
         int eventcode;
         if(argv[i+1]) retval = PAPI_event_name_to_code(argv[i+1],&eventcode);
         if (retval != PAPI_OK)
            papi_error("PAPI_event_name_to_code",retval);

         if(eventcode && PAPI_PRESET_MASK) {
            eventcode &= PAPI_PRESET_AND_MASK;
            if(_papi_hwi_presets.data[eventcode].derived == NOT_DERIVED) {
               printf("The preset event you specified: %s maps ", argv[i+1]);
               printf("to the following native event:\n");
            }
            if(_papi_hwi_presets.data[eventcode].derived == DERIVED_ADD) {
               printf("The preset event you specified: %s maps ", argv[i+1]);
               printf("to the addition of the following native event:\n");
            }
            else if(_papi_hwi_presets.data[eventcode].derived == DERIVED_SUB) {
               printf("The preset event you specified: %s maps ", argv[i+1]);
               printf("to the subtraction of the second native event from the first event:\n");
            }
            for(j=0; _papi_hwi_presets.data[eventcode].native[j]!=PAPI_NULL; j++) {
               native_code=_papi_hwi_presets.data[eventcode].native[j];
               printf("%s\n", _papi_hwd_ntv_code_to_name(native_code));
               if(bit_index < 8) {
                  _papi_hwd_ntv_code_to_bits(native_code, bits[bit_index]);
                  bit_index++;
               }
               else {
                  fprintf(stderr,"An attempt to add too many events.\n");
                  exit(1);
               }
            }
            break;
         }
         bits[bit_index] = &native_table[eventcode & PAPI_NATIVE_AND_MASK].resources;
         bit_index++;
         break;

      case '-':
         for(j=i+1; j<argc; j++) {
            strcat(program_list, argv[j]);
            strcat(program_list, " ");
         }
         break;

         default:
         error:
            fprintf(stderr,"Try `%s -h' for more information.\n",argv[0]);
            exit(1);
      }
   }
   PAPI_shutdown();
   if(program_list[0] == '\0') {
      fprintf(stderr,"No executable given.\n");
      fprintf(stderr,"Try `%s -h' for more information.\n",argv[0]);
      exit(1);
   }
   strcpy(execstring, "hpmcount ");
   if(output_file[0] != '\0') {
      strcat(execstring, "-o ");
      strcat(execstring, output_file);
      strcat(execstring, " ");
   }
   /* Build event list */
   for (i = 0; i < bit_index; i++) {
      reg_list[i].ra_position = -1;
      for (j = 0; j < 8; j++) {
         reg_list[i].ra_counter_cmd[j] = bits[i].counter_cmd[j];
      }
      for (j = 0; j < 2; j++) {
         reg_list[i].ra_group[j] = bits[i].group[j];
      }
   }
   if ((group = do_counter_allocation(event_list, natNum)) >= 0) {
      fprintf(stderr,"The given events cannot be counted simultaneously.\n");
      exit(1);
   }
   sprintf(event_list, "-g %d, ", group);
   strcat(execstring, event_list);

   strcat(execstring, program_list);
   strcat(execstring, "\n");
   system(execstring);
   exit(0);
}
