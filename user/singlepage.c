/*
 * Below is the given application for lab2_challenge2_singlepageheap.
 * This app performs malloc memory.
 */

#include "user_lib.h"
//#include "util/string.h"

typedef unsigned long long uint64;

char* strcpy(char* dest, const char* src) {
  char* d = dest;
  while ((*d++ = *src++))
    ;
  return dest;
}
int main(void) {
  LibInit();
  char str[20] = "hello, world!!!";
  char *m = (char *)better_malloc(100);
  char *p = (char *)better_malloc(50);
  printu("m=%p\np=%p\n",m,p);
  if((uint64)p - (uint64)m > 512 ){
    setstatus(-1);
    printu("you need to manage the vm space precisely!");
    exit(-1);
  }
  better_free((void *)m);
  strcpy(p,str);
  printu("%s\n",p);
  char *n = (char *)better_malloc(50);
  char* te[100];
  for(int i=0;i<100;i++) te[i]=better_malloc(50);
  for(int i=0;i<100;i++) better_free(te[i]);
  if(m != n)
  {
    setstatus(-1);
    printu("your malloc is not complete.\n");
    exit(-1);
  }
//  else{
//    printu("0x%lx 0x%lx\n", m, n);
//  }
  exit(0);
  return 0;
}