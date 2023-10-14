#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
  int p_to_c[2];
  int c_to_p[2];
  char parent_buf = 0xae;
  char parent_result;
  pipe(p_to_c);
  pipe(c_to_p);
  if (fork() == 0)
  {
    char child_buf;
    if (read(p_to_c[0], &child_buf, 1) != 1)
    {
      printf("ping error! read failed\n");
      return 1;
    }
    close(p_to_c[0]);
    if (child_buf != parent_buf)
    {
      printf("ping error! got %d\n", child_buf);
      return 1;
    }
    printf("%d: received ping\n", getpid());
    write(c_to_p[1], &child_buf, 1);
    close(c_to_p[1]);
    return 0;
  }
  write(p_to_c[1], &parent_buf, 1);
  close(p_to_c[1]);
  if (read(c_to_p[0], &parent_result, 1) != 1)
  {
    printf("pong error! read failed\n");
    return 1;
  }
  close(c_to_p[0]);
  if (parent_result != parent_buf)
  {
    printf("pong error! got %d\n", parent_result);
    return 1;
  }
  printf("%d: received pong\n", getpid());
  return 0;
}
