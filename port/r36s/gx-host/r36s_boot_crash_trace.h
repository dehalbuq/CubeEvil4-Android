// R36S experiment only: async-signal-safe crash location and map report.
// Does NOT try to recover from SIGSEGV; always terminates with 128+signal.
// Works without installing gdb on the DarkOSRE image.
#pragma once
#ifndef R36S_BOOT_CRASH_TRACE_H
#define R36S_BOOT_CRASH_TRACE_H
#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ucontext.h>
#include <sys/syscall.h>

namespace r36s_crash {
static const char* volatile stage="entry";
static volatile sig_atomic_t frame=0;
static volatile sig_atomic_t sequence=0;
static volatile sig_atomic_t entered=0;
static unsigned char alternate_stack[64*1024];

static void out(const char* s) {
  if(!s)return;
  size_t n=0;
  while(s[n] && n<2048)n++;
  while(n){ ssize_t written=write(STDERR_FILENO,s,n);
    if(written<=0)break;
    s+=written;n-=static_cast<size_t>(written);
  }
}
static void dec(unsigned long long n){
  char buf[32];int i=31;buf[i--]=0;
  do{buf[i--]=char('0'+(n%10));n/=10;}while(n && i>=0);
  out(buf+i+1);
}
static void hex(uintptr_t n){
  char b[2+sizeof(uintptr_t)*2+1];
  static const char d[]="0123456789abcdef";
  b[0]='0';b[1]='x';
  for(unsigned i=0;i<sizeof(uintptr_t)*2;i++){
    const unsigned bits=4*(sizeof(uintptr_t)*2-1-i);
    b[i+2]=d[(n>>bits)&15];
  }
  b[sizeof(b)-1]=0;
  out(b);
}
static void dumpMaps(){
  out("\nCRASH_PROCESS_MAPS_BEGIN\n");
  int fd=open("/proc/self/maps",O_RDONLY|O_CLOEXEC);
  if(fd>=0){
    char b[4096];
    ssize_t n;
    while((n=read(fd,b,sizeof(b)))>0){
      ssize_t off=0;
      while(off<n){
        ssize_t w=write(STDERR_FILENO,b+off,static_cast<size_t>(n-off));
        if(w<=0)break;
        off+=w;
      }
    }
    close(fd);
  }else out("MAPS_UNAVAILABLE\n");
  out("CRASH_PROCESS_MAPS_END\n");
}
static void handler(int sig,siginfo_t* info,void* ctx){
  if(entered++)_exit(128+sig);
  out("\n======== R36S_FRONTEND_CRASH ========\n");
  out("CRASH_SIGNAL=");dec(static_cast<unsigned>(sig));out("\n");
  out("CRASH_STAGE=");out(stage);out("\n");
  out("CRASH_RENDERED_FRAMES=");dec(static_cast<unsigned>(frame));out("\n");
  out("CRASH_GX_SEQUENCE=");dec(static_cast<unsigned>(sequence));out("\n");
  out("CRASH_FAULT_ADDRESS=");hex(reinterpret_cast<uintptr_t>(info?info->si_addr:nullptr));out("\n");
  out("CRASH_THREAD_ID=");dec(static_cast<unsigned long long>(syscall(SYS_gettid)));out("\n");
#if defined(__aarch64__)
  const auto* u=static_cast<const ucontext_t*>(ctx);
  if(u){
    out("CRASH_PC=");hex(static_cast<uintptr_t>(u->uc_mcontext.pc));out("\n");
    out("CRASH_LR=");hex(static_cast<uintptr_t>(u->uc_mcontext.regs[30]));out("\n");
    out("CRASH_SP=");hex(static_cast<uintptr_t>(u->uc_mcontext.sp));out("\n");
    for(int n=0;n<8;n++){
      out("CRASH_X");dec(n);out("=");hex(static_cast<uintptr_t>(u->uc_mcontext.regs[n]));out("\n");
    }
  }
#endif
  dumpMaps();
  out("CRASH_RESULT=FRONTEND_SIGNAL_CAUGHT\n");
  _exit(128+sig);
}
static bool install(){
  stack_t st{};
  st.ss_sp=alternate_stack;
  st.ss_size=sizeof(alternate_stack);
  st.ss_flags=0;
  if(sigaltstack(&st,nullptr)!=0)return false;
  struct sigaction action{};
  action.sa_sigaction=handler;
  sigemptyset(&action.sa_mask);
  action.sa_flags=SA_SIGINFO|SA_ONSTACK|SA_RESETHAND;
  for(int sig: {SIGSEGV,SIGBUS,SIGILL,SIGABRT}){
    if(sigaction(sig,&action,nullptr)!=0)return false;
  }
  return true;
}
static void mark(const char* s,unsigned count,unsigned seq){
  stage=s; frame=static_cast<sig_atomic_t>(count);
  sequence=static_cast<sig_atomic_t>(seq);
}
} // namespace r36s_crash
#endif
