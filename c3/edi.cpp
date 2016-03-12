#include "c3.h"

std::set<std::string> edi_hash;
std::vector<DiskImage*> edis;

void get_edi(void) {
  DiskImage *edi = new DiskImage();

  stats.edi();
  if (edi_hash.find(edi->hashcode()) == edi_hash.end() &&
      edi->nbytes > 0) {
    edis.push_back(edi);
    edi_hash.insert(edi->hashcode());
  } else {
    delete edi;
  }
}

