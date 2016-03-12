#include "c3.h"
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>

// =========== CDI Collection ============

void Runner::init() {
  start_io_log(); // tell the driver to collect I/O log
}

void Runner::finalize() {
  if (nsleep > 0) {
    fprintf(stderr, "Wait for FS daemon write back... "); fflush(stderr);
    sleep(nsleep);  // wait for FS daemon flush data
  }
  fetch_io_log(); // get I/O log from driver
}

void Runner::do_before(int syscall, int arg1, int arg2, int arg3) {
  
}

// =========== EDI Collection ============
static bool is_edi_call(int syscall_id) {
  static int FS_CALLS[] = {
    SYS_rmdir,
    SYS_mkdir,
    SYS_close,
    SYS_sync,
    SYS_syncfs,
    SYS_fsync,
    SYS_unlink,
    SYS_link,
    SYS_rename,
  };
  static std::set<int> fs_calls = std::set<int>(FS_CALLS, FS_CALLS + sizeof(FS_CALLS) / sizeof(int));

  return fs_calls.find(syscall_id) != fs_calls.end();
}


void Profiler::init() {
  get_edi();
  snapshot_next = false;
}

void Profiler::finalize() {
  get_edi();
}

void Profiler::do_before(int syscall, int arg1, int arg2, int arg3) {
  if (snapshot_next) {
    get_edi();
    snapshot_next = false;
  }
  if (is_edi_call(syscall)) {
    snapshot_next = true;
  }
}



// ============= Atomicity Breaker ==================
static bool is_am_call(int syscall_id, int arg1, int arg2, int arg3) {
  static int FS_CALLS[] = {
    SYS_open,
    SYS_ftruncate,
    SYS_ftruncate64,
    SYS_creat,
    SYS_unlink,
  };
  static std::set<int> fs_calls = std::set<int>(FS_CALLS, FS_CALLS + sizeof(FS_CALLS) / sizeof(int));

  // write a chunk
  if (syscall_id == SYS_write) {
    int count = arg3;
    if (count > 512) {
      return true;
    }
  }
  // open with write
  if (syscall_id == SYS_open) {
    int flags = arg2;
    return (flags & O_WRONLY) || (flags & O_RDWR);
  }
  // or in the system call list
  return fs_calls.find(syscall_id) != fs_calls.end();
}

void Amplifier::init() {
  snapshot_next = false;
  start_io_log(); // tell the driver to collect I/O log
}

void Amplifier::finalize() {
  _system("sync");
  fetch_io_log(); // get I/O log from driver
}

void Amplifier::do_before(int syscall, int arg1, int arg2, int arg3) {
  if (snapshot_next) {
    snapshot_next = false;
    _system("sync");
  }
  if (is_am_call(syscall, arg1, arg2, arg3)) {
    snapshot_next = true;
  }


}


