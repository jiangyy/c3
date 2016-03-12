#include "c3.h"
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>
#include <vector>
#include <set>

DiskImage *CDIGenerator::mount_disk_img(std::vector<DiskOp*> ops) {
  static char crash_disk[DISK_CAP], ret;

  memcpy(crash_disk, disk.data, DISK_CAP);
  for (DiskOp *op: ops) {
//    printf("Write to %d (offset %d) %08x\n", op->offset / 512, op->nbytes / 512, 
//      *(int*)op->data);
    memcpy(crash_disk + op->offset, op->data, op->nbytes);
  }

  _system("mkdir -p /mnt/recover; umount /mnt/recover");

  FILE *fp = fopen("/tmp/disk.img", "w");
  size_t nwrite = fwrite(crash_disk, DISK_CAP, 1, fp);
  fclose(fp);
  assert(nwrite == 1);

  _system("mount /tmp/disk.img /mnt/recover");
  if (ret != 0) {
    fprintf(stderr, "Fatal error: Cannot mount crash disk image.");
    assert(0);
  }

  DiskImage *cdi = new DiskImage(("/mnt/recover/" + subject).c_str());

  return cdi;
}

static std::set<std::string> visited;

void init_srand() {
  int fd = open("/dev/urandom", O_RDONLY), seed;
  assert(sizeof(int) == read(fd, &seed, sizeof(int)));
  close(fd);
  srand(seed);
}

bool check_consistency(CDIGenerator *gen) {
  bool valid = true;
  static int counter = 0;

  for (DiskImage *cdi = gen->next(); cdi; cdi = gen->next()) {
    std::string hash = cdi->hashcode();
    bool validated = false;

    stats.cdi();

    if (visited.find(cdi->hashcode()) == visited.end()) {
      // if this is an unexplored CDI
      visited.insert(cdi->hashcode());
      validated = validate(cdi);
    } else {
      validated = true;
    }

    delete cdi;
    if (!validated) {
      valid = false;
    } else {
      // proceed to next check
    }
  }
  return valid;
}

