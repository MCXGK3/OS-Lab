#include "kernel/riscv.h"
#include "kernel/process.h"
#include "spike_interface/spike_utils.h"
#include "util/string.h"

static void handle_instruction_access_fault() { panic("Instruction access fault!"); }

static void handle_load_access_fault() { panic("Load access fault!"); }

static void handle_store_access_fault() { panic("Store/AMO access fault!"); }

static void handle_illegal_instruction() { panic("Illegal instruction!"); }

static void handle_misaligned_load() { panic("Misaligned Load!"); }

static void handle_misaligned_store() {panic("Misaligned AMO!"); }

// added @lab1_3
static void handle_timer() {
  int cpuid = 0;
  // setup the timer fired at next time (TIMER_INTERVAL from now)
  *(uint64*)CLINT_MTIMECMP(cpuid) = *(uint64*)CLINT_MTIMECMP(cpuid) + TIMER_INTERVAL;

  // setup a soft interrupt in sip (S-mode Interrupt Pending) to be handled in S-mode
  write_csr(sip, SIP_SSIP);
}


void debug(int mcause){
  sprint("mcause is %d\n",mcause);
  uint64 epc=read_csr(mepc);
  for(int i=0;i<current[read_tp()]->line_ind;i++){
    if(epc==(current[read_tp()]->line+i)->addr){
      addr_line temp=*(current[read_tp()]->line+i);
      char dir[100];
      strcpy(dir,*(current[read_tp()]->dir+(current[read_tp()]->file+temp.file)->dir));
      int len=strlen(dir);
      int line=temp.line;
      dir[len]='/';
      strcpy(dir+len+1,(current[read_tp()]->file+temp.file)->file);
      printerr("Runtime error at %s:%d\n",dir,temp.line);
      spike_file_t* f = spike_file_open(dir, O_RDONLY, 0);
      char content[2048];
      spike_file_read(f,content,sizeof(content));
      for(int i=0,k=1;i<sizeof(content);i++){
        if(k==line){printerr("%c",content[i]);}
        if(content[i]=='\n'){
          k++;
        }
        if(k>line){break;}
      }
      spike_file_close(f);
      break;
    }
  }
  return ;
}
//
// handle_mtrap calls a handling function according to the type of a machine mode interrupt (trap).
//
void handle_mtrap() {
  uint64 mcause = read_csr(mcause);
  // sprint("mcause is %d\n",mcause);
  
  switch (mcause) {
    case CAUSE_MTIMER:
      handle_timer();
      break;
    case CAUSE_FETCH_ACCESS:
      debug(mcause);
      handle_instruction_access_fault();
      break;
    case CAUSE_LOAD_ACCESS:
      debug(mcause);  
      handle_load_access_fault();
    case CAUSE_STORE_ACCESS:
      debug(mcause);
      handle_store_access_fault();
      break;
    case CAUSE_ILLEGAL_INSTRUCTION:
      // TODO (lab1_2): call handle_illegal_instruction to implement illegal instruction
      // interception, and finish lab1_2.
      //panic( "call handle_illegal_instruction to accomplish illegal instruction interception for lab1_2.\n" );
      debug(mcause);
      handle_illegal_instruction();
      break;
    case CAUSE_MISALIGNED_LOAD:
      debug(mcause);
      handle_misaligned_load();
      break;
    case CAUSE_MISALIGNED_STORE:
      debug(mcause);
      handle_misaligned_store();
      break;

    default:
      sprint("machine trap(): unexpected mscause %p\n", mcause);
      sprint("            mepc=%p mtval=%p\n", read_csr(mepc), read_csr(mtval));
      panic( "unexpected exception happened in M-mode.\n" );
      break;
  }
}
