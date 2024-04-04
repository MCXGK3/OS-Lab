#include "user_lib.h"
#include "util/types.h"
#include "util/string.h"

int main(int argc,char* argv[]) {
    if(argc==0){
      setstatus(-1);
        printu("Need Arguments!!!\n");
    }
    else{
  int fd=open(argv[0],O_RDONLY);
  if(fd<0){
    setstatus(-1);
    printu("Open Failed!!!\n");
  }
  else{
  char buf[4000];
  read_u(fd,buf,4000);
    // strcpy(buf,"123456789123456789123456789");
  int off=0;
  while (off<strlen(buf))
  {
    printu("%s",buf+off);
    off+=255;
  }
  printu("\n");
  close(fd);
  }
    }
  
  exit(0);
  return 0;
}