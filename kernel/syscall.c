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
#include "spike_interface/spike_utils.h"

//
// implement the SYS_user_print syscall
//
ssize_t sys_user_print(const char* buf, size_t n) {
  // buf is now an address in user space of the given app's user stack,
  // so we have to transfer it into phisical address (kernel is running in direct mapping).
  assert( current );
  char* pa = (char*)user_va_to_pa((pagetable_t)(current->pagetable), (void*)buf);
  sprint(pa);
  return 0;
}

//
// implement the SYS_user_exit syscall
//
ssize_t sys_user_exit(uint64 code) {
  sprint("User exit with code:%d.\n", code);
  // in lab1, PKE considers only one app (one process). 
  // therefore, shutdown the system when the app calls exit()
  shutdown(code);
}

//
// maybe, the simplest implementation of malloc in the world ... added @lab2_2
//
uint64 sys_user_allocate_page() {
  void* pa = alloc_page();
  uint64 va = g_ufree_page;
  g_ufree_page += PGSIZE;
  user_vm_map((pagetable_t)current->pagetable, va, PGSIZE, (uint64)pa,
         prot_to_type(PROT_WRITE | PROT_READ, 1));

  return va;
}

//
// reclaim a page, indicated by "va". added @lab2_2
//
uint64 sys_user_free_page(uint64 va) {
  user_vm_unmap((pagetable_t)current->pagetable, va, PGSIZE, 1);
  return 0;
}

void insert_mblist(memblo m){
  if(memsnum>=32){return ;}
  bool flag=0;
  for(int i=memsnum-1;i>=0;i--){
    if(m.size>mems[i].size){
      mems[i+1]=m;
      flag=1;
      break;
    }
    else{
      mems[i+1]=mems[i];
    }
  }
  if(!flag)mems[0]=m;
  memsnum++;
  return;
}

uint64 should_malloc(uint64 n){
  int num=(n-1)/PGSIZE;
  uint64 va=sys_user_allocate_page();
  for(int i=0;i<num;i++) sys_user_allocate_page();
    usedmems[usedmemsnum].address=va;
    usedmems[usedmemsnum].size=n;
    usedmemsnum++;
  if(n%PGSIZE!=0){
    memblo new={va+n,PGSIZE-(n%PGSIZE)};
    insert_mblist(new);
  }
  return va;
}

uint64 haven_malloc(int i,uint64 n){
  memblo new=mems[i];
  if(new.size<n){return -1;}
  else {
    for(int j=i;j<memsnum-1;j++){
      mems[i]=mems[i+1];
    }
    memsnum--;
  if(new.size>n){
    memblo new2={new.address+n,new.size-n};
    insert_mblist(new2);
  }
  new.size=n;
  usedmems[usedmemsnum]=new;
  usedmemsnum++;
  }
  
  return new.address;
}

uint64 add_malloc(uint64 n){
  if(memsnum==0) return -1;
  memblo new=mems[memsnum-1];
  memsnum--;
  uint64 add=n-new.size;
  new.size=n;
  usedmems[usedmemsnum]=new;
  usedmemsnum++;

  int num=(add-1)/PGSIZE;
  uint64 va=sys_user_allocate_page();
  for(int i=0;i<num;i++) sys_user_allocate_page();
  if(add%PGSIZE!=0){
    memblo new={va+n,PGSIZE-(n%PGSIZE)};
    insert_mblist(new);
  }
  return new.address;

}

uint64 sys_better_malloc(uint64 n){
  if(memsnum==0) return should_malloc(n);
  else{
    for(int i=0;i<memsnum;i++){
      if(mems[i].size>=n){
        return haven_malloc(i,n);
      }
    }
    return add_malloc(n);
  }
}

uint64 sys_better_free(uint64 va){
  for(int i=0;i<usedmemsnum;i++){
    if(usedmems[i].address==va){
      memblo new=usedmems[i];
      for(int j=i;j<usedmemsnum-1;j++){
        usedmems[j]=usedmems[j+1];
      }
      insert_mblist(new);
      return 0;
    }
  }
  return -1;
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
      return sys_better_malloc(a1);
    case SYS_user_free_page:
      return sys_better_free(a1);
    default:
      panic("Unknown syscall %ld \n", a0);
  }
}
