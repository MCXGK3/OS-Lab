/*
 * contains the implementation of all syscalls.
 */

#include <stdint.h>
#include <errno.h>

#include "util/types.h"
#include "syscall.h"
#include "string.h"
#include "process.h"
#include "util/functions.h"
#include "pmm.h"
#include "vmm.h"
#include "sched.h"
#include "proc_file.h"
#include "elf.h"

#include "spike_interface/spike_utils.h"
#include "spike_interface/spike_htif.h"
#include "sync_utils.h"
#include "spike_interface/atomic.h"

int exit_barrier=0;

int sem_light[32];
int semnum=0;
process* sem_pro[32];

spinlock_t sem_lock=SPINLOCK_INIT;
//
// implement the SYS_user_print syscall
//
ssize_t sys_user_print(const char* buf, size_t n) {
  // buf is now an address in user space of the given app's user stack,
  // so we have to transfer it into phisical address (kernel is running in direct mapping).
  assert( current[read_tp()] );
  char* pa = (char*)user_va_to_pa((pagetable_t)(current[read_tp()]->pagetable), (void*)buf);
  printerr(pa);
  return n;
}

//
// implement the SYS_user_exit syscall
//
ssize_t sys_user_exit(uint64 code) {
  int tp=read_tp();
  sprint("User exit with code:%d.\n", code);
  // reclaim the current process, and reschedule. added @lab3_1
  process *parent=current[tp]->parent;
  if(current[tp]->wait){
    insert_to_ready_queue(parent);
  }

  free_process( current[read_tp()] );
  schedule();
  return 0;
}

//
// maybe, the simplest implementation of malloc in the world ... added @lab2_2
//
uint64 sys_user_allocate_page() {
  void* pa = alloc_page();
  int tp=read_tp();
  uint64 va;
  // if there are previously reclaimed pages, use them first (this does not change the
  // size of the heap)
  if (current[tp]->user_heap.free_pages_count > 0) {
    va =  current[tp]->user_heap.free_pages_address[--current[tp]->user_heap.free_pages_count];
    assert(va < current[tp]->user_heap.heap_top);
  } else {
    // otherwise, allocate a new page (this increases the size of the heap by one page)
    va = current[tp]->user_heap.heap_top;
    current[tp]->user_heap.heap_top += PGSIZE;

    current[tp]->mapped_info[HEAP_SEGMENT].npages++;
  }
  user_vm_map((pagetable_t)current[tp]->pagetable, va, PGSIZE, (uint64)pa,
         prot_to_type(PROT_WRITE | PROT_READ, 1));

  return va;
}

//
// reclaim a page, indicated by "va". added @lab2_2
//
uint64 sys_user_free_page(uint64 va) {
  int tp=read_tp();
  user_vm_unmap((pagetable_t)current[tp]->pagetable, va, PGSIZE, 1);
  // add the reclaimed page to the free page list
  current[tp]->user_heap.free_pages_address[current[tp]->user_heap.free_pages_count++] = va;
  return 0;
}

//
// kerenl entry point of naive_fork
//
ssize_t sys_user_fork() {
  sprint("User call fork.\n");
  return do_fork( current[read_tp()] );
}

//
// kerenl entry point of yield. added @lab3_2
//
ssize_t sys_user_yield() {
  // TODO (lab3_2): implment the syscall of yield.
  // hint: the functionality of yield is to give up the processor. therefore,
  // we should set the status of currently running process to READY, insert it in
  // the rear of ready queue, and finally, schedule a READY process to run.
  //panic( "You need to implement the yield syscall in lab3_2.\n" );
  current[read_tp()]->status=READY;
  insert_to_ready_queue(current[read_tp()]);
  schedule();
  return 0;
}

//
// open file
//
ssize_t sys_user_open(char *pathva, int flags) {
  char* pathpa = (char*)user_va_to_pa((pagetable_t)(current[read_tp()]->pagetable), pathva);
  return do_open(pathpa, flags);
}

//
// read file
//
ssize_t sys_user_read(int fd, char *bufva, uint64 count) {
  int i = 0;
  while (i < count) { // count can be greater than page size
    uint64 addr = (uint64)bufva + i;
    uint64 pa = (uint64)lookup_pa((pagetable_t)current[read_tp()]->pagetable,addr);
    uint64 off = addr - ROUNDDOWN(addr, PGSIZE);
    uint64 len = count - i < PGSIZE - off ? count - i : PGSIZE - off;
    uint64 r = do_read(fd, (char *)pa + off, len);
    i += r; if (r < len) return i;
  }
  return count;
}

//
// write file
//
ssize_t sys_user_write(int fd, char *bufva, uint64 count) {
  int i = 0;
  while (i < count) { // count can be greater than page size
    uint64 addr = (uint64)bufva + i;
    uint64 pa = (uint64)lookup_pa((pagetable_t)current[read_tp()]->pagetable,addr);
    uint64 off = addr - ROUNDDOWN(addr, PGSIZE);
    uint64 len = count - i < PGSIZE - off ? count - i : PGSIZE - off;
    uint64 r = do_write(fd, (char *)pa + off, len);
    i += r; if (r < len) return i;
  }
  return count;
}

//
// lseek file
//
ssize_t sys_user_lseek(int fd, int offset, int whence) {
  return do_lseek(fd, offset, whence);
}

//
// read vinode
//
ssize_t sys_user_stat(int fd, struct istat *istat) {
  struct istat * pistat = (struct istat *)user_va_to_pa((pagetable_t)(current[read_tp()]->pagetable), istat);
  return do_stat(fd, pistat);
}

//
// read disk inode
//
ssize_t sys_user_disk_stat(int fd, struct istat *istat) {
  struct istat * pistat = (struct istat *)user_va_to_pa((pagetable_t)(current[read_tp()]->pagetable), istat);
  return do_disk_stat(fd, pistat);
}

//
// close file
//
ssize_t sys_user_close(int fd) {
  return do_close(fd);
}

//
// lib call to opendir
//
ssize_t sys_user_opendir(char * pathva){
  char * pathpa = (char*)user_va_to_pa((pagetable_t)(current[read_tp()]->pagetable), pathva);
  return do_opendir(pathpa);
}

//
// lib call to readdir
//
ssize_t sys_user_readdir(int fd, struct dir *vdir){
  struct dir * pdir = (struct dir *)user_va_to_pa((pagetable_t)(current[read_tp()]->pagetable), vdir);
  return do_readdir(fd, pdir);
}

//
// lib call to mkdir
//
ssize_t sys_user_mkdir(char * pathva){
  char * pathpa = (char*)user_va_to_pa((pagetable_t)(current[read_tp()]->pagetable), pathva);
  return do_mkdir(pathpa);
}

//
// lib call to closedir
//
ssize_t sys_user_closedir(int fd){
  return do_closedir(fd);
}

//
// lib call to link
//
ssize_t sys_user_link(char * vfn1, char * vfn2){
  char * pfn1 = (char*)user_va_to_pa((pagetable_t)(current[read_tp()]->pagetable), (void*)vfn1);
  char * pfn2 = (char*)user_va_to_pa((pagetable_t)(current[read_tp()]->pagetable), (void*)vfn2);
  return do_link(pfn1, pfn2);
}

//
// lib call to unlink
//
ssize_t sys_user_unlink(char * vfn){
  char * pfn = (char*)user_va_to_pa((pagetable_t)(current[read_tp()]->pagetable), (void*)vfn);
  return do_unlink(pfn);
}

int load_other_program_with_para(process* proc,char* pathname){

  int ret=load_bincode_from_other_elf_with_para(proc,pathname);
  return ret;
}

ssize_t sys_user_exec(const char* pathname,int n,char ** paraname){
  int tp=read_tp();
  char path[512];
  char para[n][30];
  char *pathpa=(char *)user_va_to_pa(current[tp]->pagetable,(void*)pathname);
  for(int i=0;i<n;i++){
    char **paraspa=(char **)user_va_to_pa(current[tp]->pagetable,(void*)(paraname+i));
    char *parapa=(char*)user_va_to_pa(current[tp]->pagetable,*paraspa);
    strcpy(para[i],parapa);
  }
  
  strcpy(path,pathpa);
  realloc_proc(current[read_tp()]);
  int ret=load_other_program_with_para(current[tp],path);
  if(ret==-1){
    printerr("Execute Failed\n");
    sys_user_exit(0); 
    return 0;}
  char** prava=(char**)sys_user_allocate_page();
  char** prapa=user_va_to_pa(current[tp]->pagetable,prava);
  int offset=n*8;
  for(int i=0;i<n;i++){
    strcpy((char*)(prapa)+offset,para[i]);
    *(prapa+i)=(char*)(prava)+offset;
    offset+=strlen(para[i])+1;
  }
  current[tp]->trapframe->regs.a0=n;
  current[tp]->trapframe->regs.a1=(uint64)prava;
  return n;
}

ssize_t sys_user_wait(int id){
  return do_wait(id);

}


ssize_t sys_user_scan(char *bufva){
  char buf[1024];
  char* bufpa=user_va_to_pa(current[read_tp()]->pagetable,bufva);
  sprint(">>>");
  sprint("\b \b");
  //sprint("");
  spike_file_read(stdin,buf,1024);
  for(int i=0;i<1024;i++){
    if(buf[i]=='\n'){
      buf[i]=0;
      break;
    }
  }
  strcpy(bufpa,buf);
  return 0;
}

ssize_t sys_user_getchar(){
  return getchar();
}

int funcname_printer(uint64 ret_addr) {
  //sprint("%d\n",sym_count);
  for(int i=0;i<current[read_tp()]->sym_count;i++){
    //sprint("%d %d %d\n", ret_addr, symbols[i].st_value, symbols[i].st_size);
    if(ret_addr >=current[read_tp()]->values[i][0]&&ret_addr<=current[read_tp()]->values[i][1]){
      printerr("%s\n",current[read_tp()]->sym_names[i]);
      if(strcmp(current[read_tp()]->sym_names[i],"main")==0) return 0;
      return 1;
    }
  }
  return 1;
}
ssize_t sys_user_backtrace(int layer){
  uint64 trace_sp = current[read_tp()]->trapframe->regs.sp + 32;
  uint64 trace_ra = trace_sp + 8;
  trace_ra=(uint64)user_va_to_pa(current[read_tp()]->pagetable,(void*)trace_ra);
  uint64 nowlay;
  for(nowlay=0; nowlay<layer; nowlay++) {
    //sprint("%d\n",nowlay);
    if(funcname_printer(*(uint64*)trace_ra) == 0) return nowlay;
    trace_ra += 16;
  }
  return nowlay;
}



ssize_t sys_user_sem_p(uint64 id){
  if(id>semnum)return -1;
  sem_light[id]--;
  if(sem_light[id]<0){
    current[read_tp()]->status=BLOCKED;
    current[read_tp()]->queue_next=NULL;
    if(sem_pro[id]==NULL){
      sem_pro[id]=current[read_tp()];
    }
    else{
      process* p;
      for(p=sem_pro[id];p->queue_next!=NULL;p=p->queue_next){
      }
      p->queue_next=current[read_tp()];
    }
    schedule();
  }
  return 0;
}

ssize_t sys_user_sem_v(uint64 id){
  if(id>semnum) return -1;
  sem_light[id]++;
  if(sem_light[id]<=0){
    process* p=sem_pro[id];
    sem_pro[id]=p->queue_next;
    p->status=READY;
    insert_to_ready_queue(p);
  }
  return 0;
}

ssize_t sys_user_sem_new(uint64 num){
  spinlock_lock(&sem_lock);
  if(semnum==32) return -1;
  if(num<0) return -1;
  sem_light[semnum]=num;
  sem_pro[semnum]=NULL;
  semnum++;
  int copynum=semnum;
  spinlock_unlock(&sem_lock);
  return copynum-1;
}

ssize_t sys_user_printpa(uint64 va)
{
  // printerr("%lx\n",va);
  uint64 pa = (uint64)user_va_to_pa((pagetable_t)(current[read_tp()]->pagetable), (void*)va);
  printerr("%lx\n", pa);
  return 0;
}

ssize_t sys_user_rcwd(char * pathva){
  char * pathpa=(char*)user_va_to_pa(current[read_tp()]->pagetable,(void*)pathva);
  return do_rcwd(pathpa);
}

ssize_t sys_user_ccwd(char * pathva){
  char * pathpa=(char*)user_va_to_pa(current[read_tp()]->pagetable,(void*)pathva);
  //sprint("%s %d\n",pathpa,i);
  return do_ccwd(pathpa);
}
//
// [a0]: the syscall number; [a1] ... [a7]: arguments to the syscalls.
// returns the code of success, (e.g., 0 means success, fail for otherwise)
//
long do_syscall(long a0, long a1, long a2, long a3, long a4, long a5, long a6, long a7) {
  switch (a0) {
    case SYS_user_print:
      return sys_user_print((const char*)a1, a2);
    case SYS_user_exit:
      return sys_user_exit(a1);
    // added @lab2_2
    case SYS_user_allocate_page:
      return sys_user_allocate_page();
    case SYS_user_free_page:
      return sys_user_free_page(a1);
    case SYS_user_fork:
      return sys_user_fork();
    case SYS_user_yield:
      return sys_user_yield();
    // added @lab4_1
    case SYS_user_open:
      return sys_user_open((char *)a1, a2);
    case SYS_user_read:
      return sys_user_read(a1, (char *)a2, a3);
    case SYS_user_write:
      return sys_user_write(a1, (char *)a2, a3);
    case SYS_user_lseek:
      return sys_user_lseek(a1, a2, a3);
    case SYS_user_stat:
      return sys_user_stat(a1, (struct istat *)a2);
    case SYS_user_disk_stat:
      return sys_user_disk_stat(a1, (struct istat *)a2);
    case SYS_user_close:
      return sys_user_close(a1);
    // added @lab4_2
    case SYS_user_opendir:
      return sys_user_opendir((char *)a1);
    case SYS_user_readdir:
      return sys_user_readdir(a1, (struct dir *)a2);
    case SYS_user_mkdir:
      return sys_user_mkdir((char *)a1);
    case SYS_user_closedir:
      return sys_user_closedir(a1);
    // added @lab4_3
    case SYS_user_link:
      return sys_user_link((char *)a1, (char *)a2);
    case SYS_user_unlink:
      return sys_user_unlink((char *)a1);
    case SYS_user_exec:
      return sys_user_exec((char *)a1,a2,(char **)a3);
    case SYS_user_wait:
      return sys_user_wait(a1);
    case SYS_user_scan:
      return sys_user_scan((char *)a1);
    case SYS_user_getchar:
      return sys_user_getchar();
    case SYS_user_backtrace:
      return sys_user_backtrace(a1);
    case SYS_user_sem_P:
      return sys_user_sem_p(a1);
    case SYS_user_sem_V:
      return sys_user_sem_v(a1);
    case SYS_user_sem_new:
      return sys_user_sem_new(a1);
    case SYS_user_printpa:
      return sys_user_printpa(a1);
    // added @lab4_challenge1
    case SYS_user_rcwd:
      return sys_user_rcwd((char *)a1);
    case SYS_user_ccwd:
      return sys_user_ccwd((char *)a1);
    default:
      panic("Unknown syscall %ld \n", a0);
  }
}
