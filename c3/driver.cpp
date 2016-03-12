#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include "c3.h"
#include "common.h"

static char buf[DISK_CAP * 4]; // 4X disk capacity
Disk disk;

// communicates with the kernel virtual disk driver
void start_io_log(void) {
  int fd = open("/dev/c3disk", O_RDWR);
  ioctl(fd, IOCTL_TXBEGIN, buf);
  close(fd);
}

void fetch_io_log(void) {
  memset(buf, 0, sizeof(buf));
  int fd = open("/dev/c3disk", O_RDWR);
  ioctl(fd, IOCTL_TXEND, buf);
  close(fd);

  log_disk *d = (log_disk*)buf;
  disk.size = d->nbytes;
  disk.data = d->data;
  disk.ops.clear();

  DiskOp last(d->log);

  for (log_io *i = d->log; i; i = i->next) {
    DiskOp op = DiskOp(i);
    if (op.is_write() && op.nbytes == 0) continue;
    if (op.is_flush()) {
      if (!last.is_flush()) {
        disk.ops.push_back(new DiskOp(i));
      }
      // printf("[log] ------------\n");
    } else {
      assert(op.nbytes % SECT_SIZE == 0);
      // decompose each long request into sector-size small requests
      // printf("[log] Write %d for %d bytes %08x\n", op.offset / 512, op.nbytes, *(int*)op.data);
      for (int off = 0; off < op.nbytes; off += SECT_SIZE) {
        DiskOp *nop = new DiskOp(i);
        assert(nop);
        nop->offset += off;
        nop->nbytes = SECT_SIZE;
        nop->data += off;
        disk.ops.push_back(nop);
      }
    }
    last = op;
  }
}

