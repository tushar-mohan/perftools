#define INT32 int
#define INT64 long long
#define bool int
#define PATHSCALE_NONE  0x0
#define PATHSCALE_LOOPS 0x1
#define PATHSCALE_FUNCS 0x2
#define PATHSCALE_ALL   0x3

typedef struct pathscale_pu {
  char *fname;
  char *pu_name;
  int *loops;
  int *calls;
  int num_loops;
  int num_calls;
  int pusize;
  long pc; 
  int checksum;
} pathscale_pu_t;

extern void papiex_pathscale_gate(int);
extern void papiex_pathscale_thread_init(papiex_perthread_data_t *thread_data);
extern void papiex_pathscale_thread_shutdown(papiex_perthread_data_t *thread_data);
