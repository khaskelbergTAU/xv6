#include "kernel/types.h"
#include "user/user.h"

#define MAX_LINE 512
/**
 * this program reads line by line from stdin, and executes its arguments with the arguments from stdin appended.
*/
#define min(a,b) ((a) < (b) ? (a) : (b))


char WHITESPACE_CHARS[] = { ' ', '\t', '\n' };

int is_whitespace(char c)
{
  for (uint indx = 0; indx < sizeof(WHITESPACE_CHARS); indx++)
  {
    if (c == WHITESPACE_CHARS[indx]) return 1;
  }
  return 0;
}

int not_whitespace(char c)
{
  for (uint indx = 0; indx < sizeof(WHITESPACE_CHARS); indx++)
  {
    if (c == WHITESPACE_CHARS[indx]) return 0;
  }
  return 1;
}

int skip_whitespace(char *text, char **result)
{
  char *start = text;
  while ((*text != 0) && is_whitespace(*text))
    text++;
  *result = text;
  return start - text;
}

int skip_arg(char *text, char **result)
{
  char *start = text;
  while ((*text != 0) && not_whitespace(*text))
    text++;
  *result = text;
  return text - start;
}

int count_args(char *line)
{
  int arg_count = 0;
  skip_whitespace(line, &line);
  while (*line != 0)
  {
    arg_count++;
    skip_arg(line, &line);
    skip_whitespace(line, &line);
  }
  return arg_count;
}

int parse_args(char *line, int old_argc, char **old_argv, char ***new_argv_result)
{
  int line_arg_amnt = count_args(line);
  int new_argc = old_argc + line_arg_amnt;
  char **new_argv = malloc((new_argc + 1) * sizeof(char *));
  for (int indx = 0; indx < old_argc; indx++)
  {
    new_argv[indx] = malloc(strlen(old_argv[indx]) * sizeof(char));
    strcpy(new_argv[indx], old_argv[indx]);
  }
  char *current_arg = line;
  skip_whitespace(current_arg, &current_arg);
  for (int indx = old_argc; indx < new_argc; indx++)
  {
    char *arg_start = current_arg;
    int len = skip_arg(current_arg, &current_arg);
    skip_whitespace(current_arg, &current_arg);
    new_argv[indx] = malloc((len + 1) * sizeof(char));
    strncpy(new_argv[indx], arg_start, len);
    new_argv[indx][len] = '\0';
  }
  new_argv[new_argc] = 0;
  *new_argv_result = new_argv;
  return new_argc;
}


void free_argv(char **argv, int argc)
{
  for (int indx = 0; indx < argc; indx++)
  {
    free(argv[indx]);
  }
  free(argv);
}

void *realloc(void *mem, int old_size, int new_size)
{
  void *new_mem = malloc(new_size);
  memmove(new_mem, mem, min(old_size, new_size));
  free(mem);
  return new_mem;
}

int getline(char **line, int fd)
{
  uint capacity = 10;
  char *func_line = malloc(capacity);
  uint len = 0;
  char c = 'a';
  do
  {
    if (read(fd, &c, 1) < 1) break;
    if (len + 1 == capacity)
    {
      uint new_capacity = capacity * 2;
      func_line = realloc(func_line, capacity, new_capacity);
      capacity = new_capacity;
    }
    func_line[len++] = c;
  } while (c != '\n' && c != '\0');
  if (c == '\0')
  {
    len--;
  }
  else
  {
    func_line[len] = '\0';
  }
  *line = func_line;
  return len;
}

int xargs(int argc, char *argv[])
{
  argv++;
  argc--; // ignore executable name
  char *line;
  while (getline(&line, stdin) > 0)
  {
    if (line[0] == 0) break;
    char **new_argv;
    int new_argc = parse_args(line, argc, argv, &new_argv);
    free(line);
    if (fork() == 0)
    {
      int retval = exec(new_argv[0], new_argv);
      fprintf(stderr, "exec failed %d :(\n", retval);
      exit(-1);
    }
    wait(0);
    free_argv(new_argv, new_argc);
  }
  return 0;
}

int main(int argc, char *argv[])
{
  return xargs(argc, argv);
}
