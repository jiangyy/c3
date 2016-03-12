#include "c3.h"

RandomGen::RandomGen() {
  remain = nvalidate;
}

DiskImage *RandomGen::next() {
  int limit, i = 0;

  if (remain <= 0) return NULL;
  remain --;

  limit = (unsigned int)rand() % (1 + disk.ops.size());

  std::vector<DiskOp*> ops, cnt;
  for (DiskOp *io: disk.ops) {
    if (i == limit) break;
    if (io->is_flush()) {
      for (DiskOp *op: cnt) ops.push_back(op);
      cnt.clear();
    } else {
      cnt.push_back(io);
    }
    i ++;
  }

  for (DiskOp *io: cnt) {
    if (rand() % 2 == 0) { // partial write back
      ops.push_back(io);
    }
  }

  return mount_disk_img(ops);
}


