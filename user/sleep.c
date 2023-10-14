#include "kernel/types.h"
#include "user/user.h"
#include "kernel/syscall.h"

int main(int argc, char* argv[])
{
  if (argc != 2)
  {
    fprintf(stderr, "usage: %s <cpu_ticks>\n", argv[0]);
    exit(1);
  }
  int sleep_amnt = atoi(argv[1]); // in cpu clock cycles
  exit(sleep(sleep_amnt));
}
