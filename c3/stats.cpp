#include "c3.h"
#include <sys/time.h>

void Stats::cdi() {
  ncdi ++;
}

void Stats::edi() {
  nedi ++;
}

static long tsc() {
  struct timeval tp;
  gettimeofday(&tp, NULL);
  return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

void Stats::start(string key) {
  timer[key] = tsc();
}

void Stats::end(string key) {
  long st = timer[key];
  long ed = tsc();
  timer[key] = ed - st;
}

void Stats::summary() {
  printf("[Stats] #ES: %d\n", nedi);
  printf("[Stats] #CS: %d\n", ncdi);
  for (auto &kv: timer) {
    printf("[Stats] %s: %.2lf minutes\n", kv.first.c_str(), kv.second / 1000.0 / 60);
  }
}

Stats stats;
