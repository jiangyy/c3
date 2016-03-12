#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>

#include <sys/ptrace.h>
#include <sys/reg.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include "c3.h"

static const int PT_OPTIONS = PTRACE_O_TRACEFORK | PTRACE_O_TRACECLONE;

static int child_process(const char *run_mode) {
  const char *args[] = {
    "python",
    runner_path(),
    subject_path(),
    run_mode,
    NULL,
  };

  ptrace(PTRACE_TRACEME);
  kill(getpid(), SIGSTOP);
  execvp(args[0], (char**)args);
  assert(0);
}

static int trace_process(pid_t traced_pid, Instrument *ins) {
  int status, retval;

  waitpid(-1, &status, __WALL);

  ptrace(PTRACE_SETOPTIONS, traced_pid, NULL, PT_OPTIONS);
  ptrace(PTRACE_SYSCALL, traced_pid, NULL, NULL);
  
  while (1) {
    pid_t p = waitpid(-1, &status, __WALL);

    if (WIFEXITED(status)) {
      if (p == traced_pid) break; // main process terminated
    }

    int flags = status >> 16;
    if (flags == PTRACE_EVENT_FORK || flags == PTRACE_EVENT_CLONE) {
      int child_pid;
      ptrace(PTRACE_GETEVENTMSG, p, NULL, (long)&child_pid);
      ptrace(PTRACE_SYSCALL, child_pid, NULL, NULL);
    }

    int syscall_id = ptrace(PTRACE_PEEKUSER, p, sizeof(long)*ORIG_EAX);
    int arg1 = ptrace(PTRACE_PEEKUSER, p, sizeof(long)*EBX);
    int arg2 = ptrace(PTRACE_PEEKUSER, p, sizeof(long)*ECX);
    int arg3 = ptrace(PTRACE_PEEKUSER, p, sizeof(long)*EDX);

    if (ins != NULL) {
      ins->do_before(syscall_id, arg1, arg2, arg3);
    }

    ptrace(PTRACE_SYSCALL, p, NULL, NULL);
  }

  return 0;
}

void ptrace_program(Instrument *ins) {
  prepare_run();

  if (ins) { ins->init(); }

  pid_t child_pid = fork();
  if (child_pid == 0) {
    child_process("run");
  } else {
    trace_process(child_pid, ins);
  }

  if (ins) { ins->finalize(); }
}

