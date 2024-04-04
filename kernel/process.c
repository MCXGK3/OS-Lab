/*
 * Utility functions for process management. 
 *
 * Note: in Lab1, only one process (i.e., our user application) exists. Therefore, 
 * PKE OS at this stage will set "current" to the loaded user application, and also
 * switch to the old "current" process after trap handling.
 */

#include "riscv.h"
#include "strap.h"
#include "config.h"
#include "process.h"
#include "elf.h"
#include "string.h"
#include "vmm.h"
#include "pmm.h"
#include "memlayout.h"
#include "sched.h"
#include "spike_interface/spike_utils.h"
#include "util/functions.h"
#include "spike_interface/atomic.h"

//Two functions defined in kernel/usertrap.S
extern char smode_trap_vector[];
extern void return_to_user(trapframe *, uint64 satp);

spinlock_t alloc_proc_lock=SPINLOCK_INIT;


// trap_sec_start points to the beginning of S-mode trap segment (i.e., the entry point
// of S-mode trap vector).
extern char trap_sec_start[];

// process pool. added @lab3_1
process procs[NPROC];

// current points to the currently running user-mode application.
process* current[NCPU] = {NULL};

//
// switch to a user-mode process
//
void switch_to(process* proc) {
  assert(proc);
  current[read_tp()] = proc;

  // write the smode_trap_vector (64-bit func. address) defined in kernel/strap_vector.S
  // to the stvec privilege register, such that trap handler pointed by smode_trap_vector
  // will be triggered when an interrupt occurs in S mode.
  write_csr(stvec, (uint64)smode_trap_vector);

  // set up trapframe values (in process structure) that smode_trap_vector will need when
  // the process next re-enters the kernel.
  proc->trapframe->kernel_sp = proc->kstack;      // process's kernel stack
  proc->trapframe->kernel_satp = read_csr(satp);  // kernel page table
  proc->trapframe->kernel_trap = (uint64)smode_trap_handler;

  // SSTATUS_SPP and SSTATUS_SPIE are defined in kernel/riscv.h
  // set S Previous Privilege mode (the SSTATUS_SPP bit in sstatus register) to User mode.
  unsigned long x = read_csr(sstatus);
  x &= ~SSTATUS_SPP;  // clear SPP to 0 for user mode
  x |= SSTATUS_SPIE;  // enable interrupts in user mode

  // write x back to 'sstatus' register to enable interrupts, and sret destination mode.
  write_csr(sstatus, x);

  // set S Exception Program Counter (sepc register) to the elf entry pc.
  write_csr(sepc, proc->trapframe->epc);

  // make user page table. macro MAKE_SATP is defined in kernel/riscv.h. added @lab2_1
  uint64 user_satp = MAKE_SATP(proc->pagetable);

  // return_to_user() is defined in kernel/strap_vector.S. switch to user mode with sret.
  // note, return_to_user takes two parameters @ and after lab2_1.
  // if(proc->pid==2) printerr("%p %p\n",proc->trapframe,user_satp);
  return_to_user(proc->trapframe, user_satp);
}

//
// initialize process pool (the procs[] array). added @lab3_1
//
void init_proc_pool() {
  memset( procs, 0, sizeof(process)*NPROC );
  sprint("process size=%d\n",sizeof(process));

  for (int i = 0; i < NPROC; ++i) {
    procs[i].status = FREE;
    procs[i].pid = i;
  }
}

//
// allocate an empty process, init its vm space. returns the pointer to
// process strcuture. added @lab3_1
//
process* alloc_process() {
  // locate the first usable process structure
  int i;
  spinlock_lock(&alloc_proc_lock);
  sprint("");
  for( i=0; i<NPROC; i++ )
    if( procs[i].status == FREE ) break;

  if( i>=NPROC ){
    panic( "cannot find any free process structure.\n" );
    spinlock_unlock(&alloc_proc_lock);
    return 0;
  }
  procs[i].status=UNAVAILABLE;
  spinlock_unlock(&alloc_proc_lock);
  sprint("%d\n",i);

  // init proc[i]'s vm space
  procs[i].trapframe = (trapframe *)alloc_page();  //trapframe, used to save context
  memset(procs[i].trapframe, 0, sizeof(trapframe));

  // page directory
  procs[i].pagetable = (pagetable_t)alloc_page();
  memset((void *)procs[i].pagetable, 0, PGSIZE);

  procs[i].kstack = (uint64)alloc_page() + PGSIZE;   //user kernel stack top
  uint64 user_stack = (uint64)alloc_page();       //phisical address of user stack bottom
  procs[i].trapframe->regs.sp = USER_STACK_TOP;  //virtual address of user stack top
  procs[i].stack_least=USER_STACK_TOP-20*PGSIZE;
  // allocates a page to record memory regions (segments)
  procs[i].mapped_info = (mapped_region*)alloc_page();
  memset( procs[i].mapped_info, 0, PGSIZE );

  // map user stack in userspace
  user_vm_map((pagetable_t)procs[i].pagetable, USER_STACK_TOP - PGSIZE, PGSIZE,
    user_stack, prot_to_type(PROT_WRITE | PROT_READ, 1));
  procs[i].mapped_info[STACK_SEGMENT].va = USER_STACK_TOP - PGSIZE;
  procs[i].mapped_info[STACK_SEGMENT].npages = 1;
  procs[i].mapped_info[STACK_SEGMENT].seg_type = STACK_SEGMENT;

  // map trapframe in user space (direct mapping as in kernel space).
  user_vm_map((pagetable_t)procs[i].pagetable, (uint64)procs[i].trapframe, PGSIZE,
    (uint64)procs[i].trapframe, prot_to_type(PROT_WRITE | PROT_READ, 0));
  procs[i].mapped_info[CONTEXT_SEGMENT].va = (uint64)procs[i].trapframe;
  procs[i].mapped_info[CONTEXT_SEGMENT].npages = 1;
  procs[i].mapped_info[CONTEXT_SEGMENT].seg_type = CONTEXT_SEGMENT;

  // map S-mode trap vector section in user space (direct mapping as in kernel space)
  // we assume that the size of usertrap.S is smaller than a page.
  user_vm_map((pagetable_t)procs[i].pagetable, (uint64)trap_sec_start, PGSIZE,
    (uint64)trap_sec_start, prot_to_type(PROT_READ | PROT_EXEC, 0));
  procs[i].mapped_info[SYSTEM_SEGMENT].va = (uint64)trap_sec_start;
  procs[i].mapped_info[SYSTEM_SEGMENT].npages = 1;
  procs[i].mapped_info[SYSTEM_SEGMENT].seg_type = SYSTEM_SEGMENT;

  sprint("in alloc_proc. user frame 0x%lx, user stack 0x%lx, user kstack 0x%lx \n",
    procs[i].trapframe, procs[i].trapframe->regs.sp, procs[i].kstack);

  // initialize the process's heap manager
  procs[i].user_heap.heap_top = USER_FREE_ADDRESS_START;
  procs[i].user_heap.heap_bottom = USER_FREE_ADDRESS_START;
  procs[i].user_heap.free_pages_count = 0;

  // map user heap in userspace
  procs[i].mapped_info[HEAP_SEGMENT].va = USER_FREE_ADDRESS_START;
  procs[i].mapped_info[HEAP_SEGMENT].npages = 0;  // no pages are mapped to heap yet.
  procs[i].mapped_info[HEAP_SEGMENT].seg_type = HEAP_SEGMENT;

  procs[i].total_mapped_region = 4;
  procs[i].wait=0;

  // initialize files_struct
  procs[i].pfiles = init_proc_file_management();
  sprint("in alloc_proc. build proc_file_management successfully.\n");

  // return after initialization.
  return &procs[i];
}

//
// reclaim a process. added @lab3_1
//
int free_process( process* proc ) {
  // we set the status to ZOMBIE, but cannot destruct its vm space immediately.
  // since proc can be current process, and its user kernel stack is currently in use!
  // but for proxy kernel, it (memory leaking) may NOT be a really serious issue,
  // as it is different from regular OS, which needs to run 7x24.
  proc->status = ZOMBIE;

  return 0;
}

//
// implements fork syscal in kernel. added @lab3_1
// basic idea here is to first allocate an empty process (child), then duplicate the
// context and data segments of parent process to the child, and lastly, map other
// segments (code, system) of the parent to child. the stack segment remains unchanged
// for the child.
//
int do_fork( process* parent)
{
  sprint( "will fork a child from parent %d.\n", parent->pid );
  process* child = alloc_process();
  int hartid=read_tp();
  strcpy(child->appname,parent->appname);
  // printerr("total region is%d\n",parent->total_mapped_region);
  for( int i=0; i<parent->total_mapped_region; i++ ){
    // browse parent's vm space, and copy its trapframe and data segments,
    // map its code segment.
    switch( parent->mapped_info[i].seg_type ){
      case CONTEXT_SEGMENT:
        *child->trapframe = *parent->trapframe;
        break;
      case STACK_SEGMENT:{
        // printerr("STACK\n");
        child->mapped_info[STACK_SEGMENT].va=parent->mapped_info[STACK_SEGMENT].va;
        child->mapped_info[STACK_SEGMENT].npages=parent->mapped_info[STACK_SEGMENT].npages;
      for(int j=0;j<parent->mapped_info[STACK_SEGMENT].npages;j++){
        if(user_va_to_pa(child->pagetable,(void*)child->mapped_info[STACK_SEGMENT].va+j*PGSIZE)==NULL){
          void *pa=alloc_page();
          user_vm_map(child->pagetable,child->mapped_info[STACK_SEGMENT].va+j*PGSIZE,PGSIZE,(uint64)pa,prot_to_type(PROT_READ|PROT_WRITE,1));
        }
        sprint("stack_segment:%p %p\n",child->mapped_info[STACK_SEGMENT].va+j*PGSIZE,user_va_to_pa(child->pagetable,(void*)child->mapped_info[STACK_SEGMENT].va+j*PGSIZE));
        memcpy( (void*)user_va_to_pa(child->pagetable,(void*)child->mapped_info[STACK_SEGMENT].va+j*PGSIZE),
          (void*)user_va_to_pa(parent->pagetable,(void*)parent->mapped_info[i].va+j*PGSIZE), PGSIZE );
      }
      }
        break;
      case HEAP_SEGMENT:{
        // printerr("HEAP\n");
        // build a same heap for child process.

        // convert free_pages_address into a filter to skip reclaimed blocks in the heap
        // when mapping the heap blocks
        int free_block_filter[MAX_HEAP_PAGES];
        memset(free_block_filter, 0, MAX_HEAP_PAGES);
        uint64 heap_bottom = parent->user_heap.heap_bottom;
        for (int j = 0; j < parent->user_heap.free_pages_count; j++) {
          int index = (parent->user_heap.free_pages_address[j] - heap_bottom) / PGSIZE;
          free_block_filter[index] = 1;
        }

        // copy and map the heap blocks
        for (uint64 heap_block = current[hartid]->user_heap.heap_bottom;
             heap_block < current[hartid]->user_heap.heap_top; heap_block += PGSIZE) {
          if (free_block_filter[(heap_block - heap_bottom) / PGSIZE])  // skip free blocks
            continue;

          // void* child_pa = alloc_page();
          // memcpy(child_pa, (void*)user_va_to_pa(parent->pagetable,(void*)heap_block), PGSIZE);

          void* pa=user_va_to_pa(parent->pagetable,(void*)heap_block);
          user_vm_map((pagetable_t)child->pagetable, heap_block, PGSIZE, (uint64)pa,
                      prot_to_type(PROT_READ, 1));
          pte_t* pte=page_walk((pagetable_t)child->pagetable,heap_block,0);
            *pte|=1<<8;
        }

        child->mapped_info[HEAP_SEGMENT].npages = parent->mapped_info[HEAP_SEGMENT].npages;

        // copy the heap manager from parent to child
        memcpy((void*)&child->user_heap, (void*)&parent->user_heap, sizeof(parent->user_heap));
        break;
      }
      case CODE_SEGMENT:
      // printerr("CODE\n");
        // TODO (lab3_1): implment the mapping of child code segment to parent's
        // code segment.
        // hint: the virtual address mapping of code segment is tracked in mapped_info
        // page of parent's process structure. use the information in mapped_info to
        // retrieve the virtual to physical mapping of code segment.
        // after having the mapping information, just map the corresponding virtual
        // address region of child to the physical pages that actually store the code
        // segment of parent process.
        // DO NOT COPY THE PHYSICAL PAGES, JUST MAP THEM.
        for(uint64 j=0;j<parent->mapped_info[CODE_SEGMENT].npages;j++)
        map_pages(child->pagetable,parent->mapped_info[CODE_SEGMENT].va+j*PGSIZE,PGSIZE,(uint64)user_va_to_pa(parent->pagetable,(void*)parent->mapped_info[CODE_SEGMENT].va+j*PGSIZE),prot_to_type(PROT_READ|PROT_EXEC,1));
        //panic( "You need to implement the code segment mapping of child in lab3_1.\n" );
        sprint("do_fork map code segment at pa:%lx of parent to child at va:%lx.\n",user_va_to_pa(parent->pagetable,(void*)parent->mapped_info[CODE_SEGMENT].va),parent->mapped_info[CODE_SEGMENT].va);
        // after mapping, register the vm region (do not delete codes below!)
        child->mapped_info[CODE_SEGMENT].va = parent->mapped_info[CODE_SEGMENT].va;
        child->mapped_info[CODE_SEGMENT].npages =
          parent->mapped_info[CODE_SEGMENT].npages;
        child->mapped_info[CODE_SEGMENT].seg_type = CODE_SEGMENT;
        child->total_mapped_region++;
        break;
      case DATA_SEGMENT:{
        // printerr("DATA\n");
        // void* childp=alloc_page();
        // memcpy(childp, user_va_to_pa(parent->pagetable, (void*)(parent->mapped_info[i].va)), PGSIZE);
        for(int j=0;j<parent->mapped_info[DATA_SEGMENT].npages;j++){
// printerr("va %p %d\n",parent->mapped_info[DATA_SEGMENT].va,parent->mapped_info[DATA_SEGMENT].npages);
          
        user_vm_map((pagetable_t)child->pagetable, parent->mapped_info[DATA_SEGMENT].va+PGSIZE*j, PGSIZE, (uint64)user_va_to_pa(parent->pagetable, (void*)(parent->mapped_info[DATA_SEGMENT].va+PGSIZE*j)),
                      prot_to_type(PROT_READ, 1));
        pte_t* pte=page_walk(child->pagetable,parent->mapped_info[DATA_SEGMENT].va+PGSIZE*j,0);
        
        *pte|=PTE_F;
        }
        child->mapped_info[DATA_SEGMENT].va=parent->mapped_info[DATA_SEGMENT].va;
        child->mapped_info[DATA_SEGMENT].npages=parent->mapped_info[DATA_SEGMENT].npages;
        child->mapped_info[DATA_SEGMENT].seg_type=DATA_SEGMENT;
        child->total_mapped_region++;
        break;
      }
    }
    child->pfiles->cwd=parent->pfiles->cwd;

  }

  child->status = READY;
  child->trapframe->regs.a0 = 0;
  child->parent = parent;
  insert_to_ready_queue( child );

  return child->pid;
}

int realloc_proc(process* proc,bool free){
  for(int i=0;i<proc->mapped_info[CODE_SEGMENT].npages;i++)
  user_vm_unmap(proc->pagetable,proc->mapped_info[CODE_SEGMENT].va+i*PGSIZE,PGSIZE,0);
  int hartid=read_tp();

  for(int i=ROUNDDOWN(proc->mapped_info[STACK_SEGMENT].va,PGSIZE);i<USER_STACK_TOP;i+=PGSIZE){
    user_vm_unmap(proc->pagetable,i,PGSIZE,1);
  }
  // for(int i=ROUNDDOWN(proc->trapframe->regs.sp,PGSIZE);i<USER_STACK_TOP;i+=PGSIZE){
  //   user_vm_unmap(proc->pagetable,i,PGSIZE,1);
  // }


  int free_block_filter[MAX_HEAP_PAGES];
  memset(free_block_filter, 0, MAX_HEAP_PAGES);
  uint64 heap_bottom = proc->user_heap.heap_bottom;
  for (int i = 0; i < proc->user_heap.free_pages_count; i++) {
    int index = (proc->user_heap.free_pages_address[i] - heap_bottom) / PGSIZE;
    free_block_filter[index] = 1;
  }

  // copy and map the heap blocks
  for (uint64 heap_block = proc->user_heap.heap_bottom;
        heap_block < proc->user_heap.heap_top; heap_block += PGSIZE) {
    if (free_block_filter[(heap_block - heap_bottom) / PGSIZE])  // skip free blocks
      continue;
    pte_t* pte=page_walk(proc->pagetable,heap_block,0);
    user_vm_unmap(proc->pagetable,heap_block,PGSIZE,(*pte&&PTE_F)?0:1);
  }
  
  for(uint64 i=0,addr=proc->mapped_info[DATA_SEGMENT].va;i<proc->mapped_info[DATA_SEGMENT].npages;i++,addr+=PGSIZE){
    uint64 add=ROUNDDOWN(addr,PGSIZE);
    pte_t* pte=page_walk(proc->pagetable,add,0);
    // printerr("add is %p\n",add);
    user_vm_unmap(proc->pagetable,add,PGSIZE,(*pte&&PTE_F)?0:1);
    // if(*pte&&PTE_F) printerr("free ok\n");
  }
  user_vm_unmap((pagetable_t)proc->pagetable, proc->mapped_info[DATA_SEGMENT].va,PGSIZE,0);
  free_page((void*)ROUNDDOWN(proc->kstack-1,PGSIZE));
 

  if(free){
    free_page(proc->mapped_info);
    free_page(proc->pfiles);
    user_vm_unmap(proc->pagetable,(uint64)proc->trapframe,PGSIZE,1);
    free_page(proc->pagetable);
    proc->parent=NULL;
    proc->wait=0;
    proc->status=FREE;
  }
  else{
  memset(proc->trapframe, 0, sizeof(trapframe));
  

  // page directory
  //proc->pagetable = (pagetable_t)alloc_page();
  //memset((void *)proc->pagetable, 0, PGSIZE);

  proc->kstack = (uint64)alloc_page() + PGSIZE;   //user kernel stack top
  uint64 user_stack = (uint64)alloc_page();       //phisical address of user stack bottom
  proc->trapframe->regs.sp = USER_STACK_TOP;  //virtual address of user stack top

  // allocates a page to record memory regions (segments)
  memset( proc->mapped_info, 0, PGSIZE );

  // map user stack in userspace
  user_vm_map((pagetable_t)proc->pagetable, USER_STACK_TOP - PGSIZE, PGSIZE,
    user_stack, prot_to_type(PROT_WRITE | PROT_READ, 1));
  proc->mapped_info[STACK_SEGMENT].va = USER_STACK_TOP - PGSIZE;
  proc->mapped_info[STACK_SEGMENT].npages = 1;
  proc->mapped_info[STACK_SEGMENT].seg_type = STACK_SEGMENT;

  proc->mapped_info[CONTEXT_SEGMENT].va = (uint64)proc->trapframe;
  proc->mapped_info[CONTEXT_SEGMENT].npages = 1;
  proc->mapped_info[CONTEXT_SEGMENT].seg_type = CONTEXT_SEGMENT;

  // map S-mode trap vector section in user space (direct mapping as in kernel space)
  // we assume that the size of usertrap.S is smaller than a page.
  proc->mapped_info[SYSTEM_SEGMENT].va = (uint64)trap_sec_start;
  proc->mapped_info[SYSTEM_SEGMENT].npages = 1;
  proc->mapped_info[SYSTEM_SEGMENT].seg_type = SYSTEM_SEGMENT;


  // initialize the process's heap manager
  proc->user_heap.heap_top = USER_FREE_ADDRESS_START;
  proc->user_heap.heap_bottom = USER_FREE_ADDRESS_START;
  proc->user_heap.free_pages_count = 0;

  // map user heap in userspace
  proc->mapped_info[HEAP_SEGMENT].va = USER_FREE_ADDRESS_START;
  proc->mapped_info[HEAP_SEGMENT].npages = 0;  // no pages are mapped to heap yet.
  proc->mapped_info[HEAP_SEGMENT].seg_type = HEAP_SEGMENT;

  proc->total_mapped_region = 4;
  }
  //proc->wait=0;


  return 0;
}

int do_kill(int pid){
  for(int i=0;i<NPROC;i++){
    if(procs[i].pid==pid){
      if(procs[i].parent==current[read_tp()]){
        if(procs[i].status!=ZOMBIE) return -1;
        realloc_proc(&procs[i],1);
        return 0;
      }
      return -2;
    }
  }
  return -3;
}

void do_ps(){
  for(int i=0;i<NPROC;i++){
    if(procs[i].status!=FREE){
      
      printerr("%d\t",procs[i].pid);
      switch (procs[i].status)
      {
      case FREE:
      printerr("FREE\t");
      break;
      case RUNNING:
      printerr("RUNNING\t");
      break;
      case BLOCKED:
      printerr("BLOCKED\t");
      break;
      case ZOMBIE:
      printerr("ZOMBIE\t");
      break;
      case UNAVAILABLE:
      printerr("UNAVAILABLE\t");
      break;
      default:
        break;
      }
      printerr("%s\n",procs[i].appname);
    }
  }
}