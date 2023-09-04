#include "kernel/types.h"
#include "user/user.h"
#include "kernel/syscall.h"

int main(int argc, char* argv[])
{
  if (argc <= 1)
  {
    fprintf(stderr, "usage: %s <amount>\n", argv[0]);
    exit(1);
  }
  int sleep_amnt = atoi(argv[1]);
  exit(sleep(sleep_amnt));
}
