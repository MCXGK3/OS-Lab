#include "user_lib.h"
#include "util/string.h"
#include "util/types.h"


int main(int argc, char *argv[]) {
  if(argc<=0){
    printu("Need Arguments!!!\n");
  }
  else{
  mkdir_u(argv[0]);
  }

  exit(0);
  return 0;
}
