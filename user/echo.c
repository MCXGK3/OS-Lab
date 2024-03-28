#include "user_lib.h"
#include "util/string.h"
#include "util/types.h"

int main(int argc, char *argv[]) {
  int fd;
  printu("argc  %d\n",argc);
  for(int i=0;i<argc;i++){
    printu("%s\n",argv[i]);
  }
  exit(0);
  return 0;
}
