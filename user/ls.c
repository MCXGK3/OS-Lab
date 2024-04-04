#include "user_lib.h"
#include "util/string.h"
#include "util/types.h"

int main(int argc, char *argv[]) {
  int dir_fd;
  if(argc<=0)
  dir_fd = opendir_u(".");
  else{
    dir_fd=opendir_u(argv[0]);
  }
  if(dir_fd<0){
    setstatus(-1);
    printu("Open Failed\n");
    exit(0);
  }
  printu("---------- ls command -----------\n");
  printu("ls \"%s\":\n", ".");
  printu("[name]               [inode_num]\n");
  struct dir dir;
  int width = 20;
  while(readdir_u(dir_fd, &dir) == 0) {
    // we do not have %ms :(
    char name[width + 1];
    memset(name, ' ', width + 1);
    name[width] = '\0';
    if (strlen(dir.name) < width) {
      strcpy(name, dir.name);
      name[strlen(dir.name)] = ' ';
      if(dir.type==DIR_I)
      printu("\33[34m%s\33[37m %d\n", name, dir.inum);
      else 
      printu("%s %d\n",name, dir.inum);
    }
    else
      if(dir.type==DIR_I)
      printu("\33[34m%s\33[37m %d\n", dir.name, dir.inum);
      else
      printu("%s %d\n",dir.name,dir.inum);
  }
  printu("------------------------------\n");
  closedir_u(dir_fd);
  exit(0);
  return 0;
}