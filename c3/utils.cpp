#include "c3.h"
#include <string>
#include <cstring>

int nsleep = 60;

void prepare_run() {
  std::string cmd;

  // cleanup file-system
  _system("rm -rf /tmp/disk.img /mnt/c3disk/*");
  cmd = "cp -r subjects/" + subject + " /mnt/c3disk/";
  _system(cmd.c_str());

  // execute prepare script
  cmd = std::string("python ") + runner_path() + " " + subject_path() + " prepare";
  _system(cmd.c_str());
}

char *subject_path() {
  static char *path = NULL;
  if (path == NULL) {
    std::string str = "/mnt/c3disk/" + subject;
    path = new char [str.length() + 1];
    strcpy(path, str.c_str());
  }
  return path;
}

const char *runner_path() {
  return "c3/runner.py";
}

void _system(const char *cmd) {
  int ret = system(cmd);
}
