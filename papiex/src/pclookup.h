int bfd_open_executable (char *filename);
int
bfd_find_src_loc (void *i_addr_hex, char **o_file_str, int *o_lineno,
		  char **o_funct_str);
void
bfd_close_executable ();

