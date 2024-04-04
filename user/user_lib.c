/*
 * The supporting library for applications.
 * Actually, supporting routines for applications are catalogued as the user 
 * library. we don't do that in PKE to make the relationship between application 
 * and user library more straightforward.
 */

#include "user_lib.h"
#include "util/types.h"
#include "util/snprintf.h"
#include "kernel/syscall.h"
#define PGSIZE 4096
memory_block* free_blocks= (memory_block*)NULL;
uint64 lastaddr=0;


uint64 do_user_call(uint64 sysnum, uint64 a1, uint64 a2, uint64 a3, uint64 a4, uint64 a5, uint64 a6,
                 uint64 a7) {
  int ret;

  // before invoking the syscall, arguments of do_user_call are already loaded into the argument
  // registers (a0-a7) of our (emulated) risc-v machine.
  asm volatile(
      "ecall\n"
      "sw a0, %0"  // returns a 32-bit value
      : "=m"(ret)
      :
      : "memory");

  return ret;
}
void LibInit(){
  free_blocks=NULL;
  lastaddr=0;
}
//
// printu() supports user/lab1_1_helloworld.c
//
int printu(const char* s, ...) {
  va_list vl;
  va_start(vl, s);

  char out[256];  // fixed buffer size.
  int res = vsnprintf(out, sizeof(out), s, vl);
  va_end(vl);
  const char* buf = out;
  size_t n = res < sizeof(out) ? res : sizeof(out);

  // make a syscall to implement the required functionality.
  return do_user_call(SYS_user_print, (uint64)buf, n, 0, 0, 0, 0, 0);
}

int print_with_hartid(const char* s, ...){
  va_list vl;
  va_start(vl, s);

  char out[256];  // fixed buffer size.
  int res = vsnprintf(out, sizeof(out), s, vl);
  va_end(vl);
  const char* buf = out;
  size_t n = res < sizeof(out) ? res : sizeof(out);

  // make a syscall to implement the required functionality.
  return do_user_call(SYS_user_print, (uint64)buf, n, 1, 0, 0, 0, 0);
}

//
// applications need to call exit to quit execution.
//
int exit(int code) {
  return do_user_call(SYS_user_exit, code, 0, 0, 0, 0, 0, 0); 
}

//
// lib call to naive_malloc
//
void* naive_malloc() {
  lastaddr=do_user_call(SYS_user_allocate_page, 0, 0, 0, 0, 0, 0, 0);
  lastaddr+=PGSIZE;
  return (void*)(lastaddr-PGSIZE);
}

//
// lib call to naive_free
//
void naive_free(void* va) {
  do_user_call(SYS_user_free_page, (uint64)va, 0, 0, 0, 0, 0, 0);
}

//
// lib call to naive_fork
int fork() {
  return do_user_call(SYS_user_fork, 0, 0, 0, 0, 0, 0, 0);
}

//
// lib call to yield
//
void yield() {
  do_user_call(SYS_user_yield, 0, 0, 0, 0, 0, 0, 0);
}

//
// lib call to open
//
int open(const char *pathname, int flags) {
  return do_user_call(SYS_user_open, (uint64)pathname, flags, 0, 0, 0, 0, 0);
}

//
// lib call to read
//
int read_u(int fd, void * buf, uint64 count){
  return do_user_call(SYS_user_read, fd, (uint64)buf, count, 0, 0, 0, 0);
}

//
// lib call to write
//
int write_u(int fd, void *buf, uint64 count) {
  return do_user_call(SYS_user_write, fd, (uint64)buf, count, 0, 0, 0, 0);
}

//
// lib call to seek
// 
int lseek_u(int fd, int offset, int whence) {
  return do_user_call(SYS_user_lseek, fd, offset, whence, 0, 0, 0, 0);
}

//
// lib call to read file information
//
int stat_u(int fd, struct istat *istat) {
  return do_user_call(SYS_user_stat, fd, (uint64)istat, 0, 0, 0, 0, 0);
}

//
// lib call to read file information from disk
//
int disk_stat_u(int fd, struct istat *istat) {
  return do_user_call(SYS_user_disk_stat, fd, (uint64)istat, 0, 0, 0, 0, 0);
}

//
// lib call to open dir
//
int opendir_u(const char *dirname) {
  return do_user_call(SYS_user_opendir, (uint64)dirname, 0, 0, 0, 0, 0, 0);
}

//
// lib call to read dir
//
int readdir_u(int fd, struct dir *dir) {
  return do_user_call(SYS_user_readdir, fd, (uint64)dir, 0, 0, 0, 0, 0);
}

//
// lib call to make dir
//
int mkdir_u(const char *pathname) {
  return do_user_call(SYS_user_mkdir, (uint64)pathname, 0, 0, 0, 0, 0, 0);
}

//
// lib call to close dir
//
int closedir_u(int fd) {
  return do_user_call(SYS_user_closedir, fd, 0, 0, 0, 0, 0, 0);
} 

//
// lib call to link
//
int link_u(const char *fn1, const char *fn2){
  return do_user_call(SYS_user_link, (uint64)fn1, (uint64)fn2, 0, 0, 0, 0, 0);
}

//
// lib call to unlink
//
int unlink_u(const char *fn){
  return do_user_call(SYS_user_unlink, (uint64)fn, 0, 0, 0, 0, 0, 0);
}

//
// lib call to close
//
int close(int fd) {
  return do_user_call(SYS_user_close, fd, 0, 0, 0, 0, 0, 0);
}

int exec(const char *pathname,int n,char **paraname){
  return do_user_call(SYS_user_exec,(uint64)pathname,n,(uint64)paraname,0,0,0,0);
}

int wait(int id){
  return do_user_call(SYS_user_wait,id,0,0,0,0,0,0);
}

int scanu(char* buf){
  return do_user_call(SYS_user_scan,(uint64)buf,0,0,0,0,0,0);
}

char getcharu(){
  char c=do_user_call(SYS_user_getchar,0,0,0,0,0,0,0);
  // printu("%d ",c);
  return c;
}

int print_backtrace(int layer){
  return do_user_call(SYS_user_backtrace, layer, 0, 0, 0, 0, 0, 0); 
}

void test(){
  for(memory_block*p=free_blocks;p!=NULL;p=p->next){
    printu("%p next is %p, size is %d\n",p,p->next,p->size);
  }
}
void* better_malloc(int size){
  //test();
  size=(((size-1)/8)+1)*8;
  int relsize=0;
  if((void*)free_blocks==NULL){
    size+=sizeof(memory_block);
    // printu("%d\n",size);
    int num=(size-1)/PGSIZE;
    // printu("num is %d\n",num);
    uint64 lastpage;
    memory_block* new=(memory_block*)naive_malloc();
    for(int i=0;i<num;i++){
      lastpage=(uint64)naive_malloc();
    }
    if(size%PGSIZE>(PGSIZE-sizeof(memory_block))){
      relsize=(((size-1)/PGSIZE)+1)*PGSIZE;
    }
    else{
      relsize=size;
      memory_block* newfree=(memory_block*)((char*)new+size);
      newfree->next=free_blocks;
      newfree->size=PGSIZE-sizeof(memory_block)-(size%PGSIZE);
      free_blocks=newfree;
    }
    new->size=relsize-sizeof(memory_block);
    //printu("new is %p\n",new);
    // printu("memory_block size is%d\n",sizeof(memory_block));
    return (void*)(new+1);
  }
  else{
    if(free_blocks->size>size+sizeof(memory_block)){
      memory_block* new=free_blocks;
      memory_block* newfree=(memory_block*)((char*)free_blocks+size+sizeof(memory_block));
      newfree->size=free_blocks->size-size-sizeof(memory_block);
      newfree->next=free_blocks->next;
      free_blocks->size=size;
      free_blocks=newfree;
      return (void*)(new+1);
    }
    else if(free_blocks->size>size){
      memory_block* new=free_blocks;
      free_blocks=free_blocks->next;
      return (void*)(new+1);
    }
    for(memory_block* p=free_blocks;(void*)(p->next)!=NULL;p=p->next){
      if(p->next->size>size+sizeof(memory_block)){
          memory_block* new=p->next;
          memory_block* newfree=(memory_block*)((char*)new+size+sizeof(memory_block));
          newfree->size=new->size-size-sizeof(memory_block);
          newfree->next=new->next;
          p->next->size=size;
          p->next=newfree;
          return (void*)(new+1);
      }
      else if (p->next->size>size)
      {
        memory_block* new=p->next;
        p->next=new->next;
        return (void*)(new+1);
      } 
    }
    memory_block* p;
    if(free_blocks->next!=NULL){
    for(p=free_blocks;(void*)(p->next->next)!=NULL;p=p->next);
    
    
    if((uint64)(p->next)+sizeof(memory_block)+p->next->size==lastaddr){
    memory_block* new=p->next;
    relsize=size-p->next->size;
    int num=(relsize-1)/PGSIZE;
    uint64 lastpage=0;
    for(int i=0;i<num;i++) lastpage=(uint64)naive_malloc();
    if(relsize%PGSIZE>PGSIZE-sizeof(memory_block)){
      p->next->size+=num*PGSIZE;
      p->next=p->next->next;
    }
    else{
      memory_block* newfree=(memory_block*)(lastpage+(relsize%PGSIZE));
      newfree->size=PGSIZE-(relsize%PGSIZE)-sizeof(memory_block);
      newfree->next=p->next->next;
      new->size+=relsize;
      p->next=newfree;
    }
    return (void*)(new+1);
    }
    else{
    size+=sizeof(memory_block);
    int num=(size-1)/PGSIZE;
    uint64 lastpage;
    memory_block* new=(memory_block*)naive_malloc();
    for(int i=0;i<num;i++){
      lastpage=(uint64)naive_malloc();
    }
    if(size%PGSIZE>(PGSIZE-sizeof(memory_block))){
      relsize=(((size-1)/PGSIZE)+1)*PGSIZE;
    }
    else{
      relsize=size;
      memory_block* newfree=(memory_block*)((char*)new+size);
      newfree->next=p->next->next;
      newfree->size=PGSIZE-sizeof(memory_block)-(size%PGSIZE);
      p->next->next=newfree;
    }
    new->size=relsize-sizeof(memory_block);
    //printu("new is %p\n",new);
    // printu("memory_block size is%d\n",sizeof(memory_block));
    return (void*)(new+1);
    }
    }
    else{
      if(((uint64)free_blocks)+sizeof(memory_block)+free_blocks->size==lastaddr){
        memory_block* new=free_blocks;
        relsize=size-free_blocks->size;
        int num=(relsize-1)/PGSIZE;
        uint64 lastpage=0;
        for(int i=0;i<num;i++) lastpage=(uint64)naive_malloc();
        if(relsize%PGSIZE>PGSIZE-sizeof(memory_block)){
          free_blocks->size+=num*PGSIZE;
        }
    else{
      memory_block* newfree=(memory_block*)(lastpage+(relsize%PGSIZE));
      newfree->size=PGSIZE-(relsize%PGSIZE)-sizeof(memory_block);
      newfree->next=free_blocks->next;
      new->size+=relsize;
      free_blocks=newfree;
    }
    return (void*)(new+1);
      }
    else{
      size+=sizeof(memory_block);
    // printu("%d\n",size);
    int num=(size-1)/PGSIZE;
    // printu("num is %d\n",num);
    uint64 lastpage;
    memory_block* new=(memory_block*)naive_malloc();
    for(int i=0;i<num;i++){
      lastpage=(uint64)naive_malloc();
    }
    if(size%PGSIZE>(PGSIZE-sizeof(memory_block))){
      relsize=(((size-1)/PGSIZE)+1)*PGSIZE;
    }
    else{
      relsize=size;
      memory_block* newfree=(memory_block*)((char*)new+size);
      newfree->next=free_blocks->next;
      newfree->size=PGSIZE-sizeof(memory_block)-(size%PGSIZE);
      free_blocks->next=newfree;
    }
    new->size=relsize-sizeof(memory_block);
    //printu("new is %p\n",new);
    // printu("memory_block size is%d\n",sizeof(memory_block));
    return (void*)(new+1);
    }
    }
  }
}

void better_free(void* addr){
  // printu("better_free %p\n",addr);
  // printu("%p\n",free_blocks);
  //test();
  
  memory_block* now=(memory_block*)((uint64)addr-sizeof(memory_block));
  if((uint64)now<(uint64)free_blocks){
    now->next=free_blocks;
    free_blocks=now;
    return;
  }
  else{
    memory_block*p;
    for(p=free_blocks;(void*)(p->next)!=NULL;p=p->next){
      if((uint64)now>(uint64)p && (uint64)now<(uint64)p->next){
        now->next=p->next;
        p->next=now;
        return;
      }
    }
    now->next=p->next;
    p->next=now;
    return;
  }

}


void sem_P(int id){
  do_user_call(SYS_user_sem_P,id,0,0,0,0,0,0);
}

void sem_V(int id){
  do_user_call(SYS_user_sem_V,id,0,0,0,0,0,0);
}

int sem_new(int num){
  return do_user_call(SYS_user_sem_new,num,0,0,0,0,0,0);
}

void printpa(void* va)
{
  do_user_call(SYS_user_printpa, (uint64)va, 0, 0, 0, 0, 0, 0);
}
int read_cwd(char *path) {
  return do_user_call(SYS_user_rcwd, (uint64)path, 0, 0, 0, 0, 0, 0);
}

//
// lib call to change pwd
//
int change_cwd(const char *path) {
  return do_user_call(SYS_user_ccwd, (uint64)path, 0, 0, 0, 0, 0, 0);
}

int puttask(int pid){
  return do_user_call(SYS_user_puttask,pid,0,0,0,0,0,0);}
int checktask(){
  return do_user_call(SYS_user_checktask,0,0,0,0,0,0,0);
}
int gettask(){
  return do_user_call(SYS_user_gettask,0,0,0,0,0,0,0);
}

int kill(int pid){
  return do_user_call(SYS_user_kill,pid,0,0,0,0,0,0);
}

void setstatus(int status){
  do_user_call(SYS_user_procstatus,status,0,0,0,0,0,0);
  return;
}

int getstatus(){
  return do_user_call(SYS_user_getstatus,0,0,0,0,0,0,0);
}

void shutdown(){
  do_user_call(SYS_user_shutdown,0,0,0,0,0,0,0);
}
void ps(){
  do_user_call(SYS_user_ps,0,0,0,0,0,0,0);
}
