#include "user_lib.h"
#include "util/string.h"
#include "util/types.h"


int main(int argc, char *argv[]) {
  if(argc<=0){
    setstatus(-1);
    printu("Need Arguments!!!\n");
  }
  else{
    for(int i=0;i<argc;i++){
  int ret=mkdir_u(argv[i]);
  if(ret<0){
    setstatus(-1);
    printu("%s mkdir failed\n",argv[i]);
  }
    }
  }

  exit(0);
  return 0;
}
