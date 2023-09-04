#include "kernel/types.h"
#include "user/user.h"

#define MAX_PRIME 35

void pipe_stage(int prev_pipe[2])
{
  int prime;
  if (read(prev_pipe[0], &prime, 4) <= 0)
  {
    close(prev_pipe[0]);
    return;
  }
  printf("prime %d\n", prime);
  int next_pipe[2];
  pipe(next_pipe);
  if (fork() == 0)
  {
    close(next_pipe[1]);
    pipe_stage(next_pipe);
    exit(0);
  }
  else
  {
    close(next_pipe[0]);
    int curr_num;
    while (read(prev_pipe[0], &curr_num, 4) > 0)
    {
      if (curr_num % prime != 0)
      {
        write(next_pipe[1], &curr_num, 4);
      }
    }
    close(prev_pipe[0]);
    close(next_pipe[1]);
    wait(0);
  }
}


int main(int argc, char *argv[])
{
  int first_pipe[2];
  pipe(first_pipe);
  if (fork() == 0)
  {
    close(first_pipe[1]);
    pipe_stage(first_pipe);
    exit(0);
  }
  else
  {
    close(first_pipe[0]);
    for (int val = 2; val < MAX_PRIME; val++)
    {
      write(first_pipe[1], &val, 4);
    }
    close(first_pipe[1]);
    wait(0);
  }
  return 0;
}




