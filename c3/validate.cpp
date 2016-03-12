#include "c3.h"

DiskImage *buggy = NULL;
static int bug_num = 0;

bool validate(DiskImage *cdi) {
  if (edi_hash.find(cdi->hashcode()) != edi_hash.end()) {
    return true;
  }

  int mind = 1000000000;

  for (DiskImage *edi: edis) {
    int d = 0;
    for (int i = 0; i < 256; i ++) {
      if (edi->bytes[i] > cdi->bytes[i]) {
        d += edi->bytes[i] - cdi->bytes[i];
      }
    }
    if (d == 0) return true;
    mind = min(d, mind);
  }
  if (mind < 32) {
    return true;
  } else {
    if (mind > bug_num) {
      bug_num = mind;
      if (buggy != NULL) {
        delete buggy;
      }
      buggy = new DiskImage(cdi->path.c_str());
      buggy->incons_nbytes = mind;
    }
    return false;
  }
}

