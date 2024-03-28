#include "user_lib.h"
#include "util/string.h"
#include "util/types.h"


void cd(const char *path) {
  if (change_cwd(path) != 0)
    printu("No Such Path!!!\n");
}

int main(int argc, char *argv[]) {

  if(argc<=0){
    printu("Need Arguments!!!\n");
  }
  else{
    cd(argv[0]);
  }
  exit(0);
  return 0;
}