#include "user_lib.h"
#include "util/types.h"
#include "util/string.h"

int main(int argc,char* argv[]) {
    if(argc==0){
        printu("Need Arguments!!!\n");
    }
    else{
  int fd=open(argv[0],O_RDONLY);
  char buf[4000];
  read_u(fd,buf,256);
    // strcpy(buf,"123456789123456789123456789");
  int off=0;
  while (off<strlen(buf))
  {
    printu("%s",buf);
    off+=256;
  }
  printu("\n");
    }
  exit(0);
  return 0;
}