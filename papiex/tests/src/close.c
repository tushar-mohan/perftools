#include <unistd.h>
int main()
{
  int i;
  for (i=1;i<256;i++) {
    close(i);
  }
  return 0;
}
