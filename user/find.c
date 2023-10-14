#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "user/user.h"

char *stem(char *path)
{
  uint last_sep = 0;
  for (uint curr = 0; path[curr] != '\0'; curr++)
  {
    if (path[curr] == '/')
      last_sep = curr;
  }
  if (last_sep == 0)
    return path;
  return path + last_sep + 1;

}

int find(char *path, char *pattern)
{
  int fd;
  struct dirent de;
  struct stat st;
  char buf[512], *p;
  if ((fd = open(path, 0)) < 0) {
    fprintf(2, "find: cannot open %s\n", path);
    return -1;
  }
  if (fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return -1;
  }
  switch (st.type)
  {
  case T_DEVICE:
  case T_FILE:
    if (!strcmp(stem(path), pattern))
      printf("%s\n", path);
    break;
  case T_DIR:
    if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)) {
      fprintf(2, "find: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';
    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
      if (de.inum == 0)
        continue;
      if ((!strcmp(de.name, ".")) || (!strcmp(de.name, "..")))
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      find(buf, pattern);
    }
    break;
  }
  close(fd);
  return 0;
}


int main(int argc, char *argv[])
{
  if (argc < 3)
  {
    printf("Usage: %s <search_dir> <pattern>\n", argv[0]);
    return 1;
  }

  return find(argv[1], argv[2]);
}
