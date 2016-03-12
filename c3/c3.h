#ifndef __C3_H__
#define __C3_H__

#include "common.h"
#include <vector>
#include <set>
#include <string>
#include <map>
#include <cassert>
using namespace std;

// the subject name
extern string subject;

// ********************** Driver Interface ***********************

// a disk operation
class DiskOp {
public:
  unsigned long offset;
  unsigned long nbytes;
  char *data;
 
  bool is_flush() { return flags & OP_FLUSH; }
  bool is_write() { return flags & OP_WRITE; }
  DiskOp(log_io *io): offset(io->offset), nbytes(io->nbytes),
                      data(io->data), flags(io->flags) {}
private:
  int flags;
};

// disk = initial snapshot + operations performed
struct Disk {
  unsigned long size;
  const char *data;
  vector<DiskOp*> ops;
};
extern Disk disk;

const int SECT_SIZE = 512;

// ********************** Expected Snapshot ***********************

class DiskImage;

// A snapshot of a disk file
struct File {
  File(DiskImage *img, const char *path);
  long size;
  string hashcode;
  char *content;
};

// A snapshot of a disk image, key-value mapping:
//    file-name -> file-content
class DiskImage {
public:
  // the mapping
  map<string, File*> files;
  // its overall hash code (to fast determine equivalence)
  string hashcode();
  int nbytes, bytes[256], incons_nbytes;
  string path;

  DiskImage();
  DiskImage(const char *);
  ~DiskImage();
  void show();

private:
  void dfs_collect(int level, const char *path, const char *relpath);
  string cached_hashcode;
};

// vector of distinct ESs
extern vector<DiskImage*> edis;
// hash of distinct ESs
extern set<string> edi_hash;
// the buggy disk image
extern DiskImage *buggy;

// ********************** Crash Sites ***********************

class CDIGenerator {
public:
  virtual DiskImage *next() = 0;
  DiskImage *mount_disk_img(vector<DiskOp*> ops);
};

class RandomGen: public CDIGenerator {
private:
  int remain;
public:
  RandomGen();
  DiskImage *next();
};

class SearchGen: public CDIGenerator {
private:
  void dfs(vector<vector<int>>&, int, int, int);
  void gen_plan(int);
  vector<int> tmp;
  map<int,vector<vector<int>>> plans;
  DiskImage *cons_ops(vector<DiskOp*>&, vector<DiskOp*>&, vector<int>&);
  int flush_id;
  int plan_id;
public:
  SearchGen();
  DiskImage *next();
};

// validates a CDI
bool validate(DiskImage *cdi);

const int nvalidate = 127;

// ***************** Instrument attached to ptrace ***************

// each run is an instrument pass
class Instrument {
public:
  virtual void init() = 0;
  virtual void finalize() = 0;
  virtual void do_before(int syscall, int arg1, int arg2, int arg3) = 0;
};

class Profiler: public Instrument {
  bool snapshot_next;
public:
  void init();
  void finalize();
  void do_before(int, int, int, int);
};

class Runner: public Instrument {
public:
  void init();
  void finalize();
  void do_before(int, int, int, int);
};

class Amplifier: public Instrument {
  bool snapshot_next;
public:
  void init();
  void finalize();
  void do_before(int, int, int, int);
};

void ptrace_program(Instrument *ins);
extern int nsleep;

// ********************** Utilities ***********************

void _system(const char*);
void start_io_log();
void fetch_io_log();
bool check_consistency(CDIGenerator *);
void get_edi();
void init_srand();
void prepare_run();   // cleanup & execute prepare script
char *subject_path(); 
const char *runner_path(); 
extern bool verbose;

class Stats {
  map<string,long> timer;
  int ncdi, nedi;

public:
  void edi();
  void cdi();
  void summary();
  void start(string);
  void end(string);
};
extern Stats stats;

#endif
