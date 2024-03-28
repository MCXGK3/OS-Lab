#include "user_lib.h"
#include "util/string.h"
#include "util/types.h"

void pwd() {
  char path[30];
  read_cwd(path);
  printu("cwd:%s\n", path);
}


int main(int argc, char *argv[]) {

  pwd();
  
  exit(0);
  return 0;
}