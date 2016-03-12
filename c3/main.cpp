#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <unistd.h>
#include "c3.h"

std::string subject;

static void check() {
  SearchGen gen;
  check_consistency(&gen);
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: fuzz subject_name");
    return 1;
  }

  init_srand();
  subject = argv[1];

  set<string> args;
  for (int i = 2; i < argc; i ++) {
    args.insert(argv[i]);
  }

  bool noam = args.find("--noam") != args.end();
  bool notr = args.find("--notr") != args.end();

  fprintf(stderr, "[C3] Start validation of %s\n", subject.c_str());
  Profiler profiler;

  fprintf(stderr, "[C3] Profile running... "); fflush(stderr);
  ptrace_program(&profiler);
  fprintf(stderr, "Done\n");

  if (notr) {
  } else {
    Runner runner;
    fprintf(stderr, "[C3] Test running... "); fflush(stderr);
    ptrace_program(&runner);
    fprintf(stderr, "Done\n");
    fprintf(stderr, "[C3] Validating... "); fflush(stderr);
    check();
    fprintf(stderr, "Done\n");
  }

  if (noam) {
  } else {
    Amplifier amplifier;
    fprintf(stderr, "[C3] Amplification running... "); fflush(stderr);
    ptrace_program(&amplifier);
    fprintf(stderr, "Done\n");
    fprintf(stderr, "[C3] Validating... "); fflush(stderr);
    check();
    fprintf(stderr, "Done\n");
  }

  if (buggy == NULL) {
    fprintf(stderr, "* OK\n");
  } else {
    fprintf(stderr, "* ERROR (%d bytes cannot be aligned)\n", buggy->incons_nbytes);
    buggy->show();
  }
  return 0;
}
