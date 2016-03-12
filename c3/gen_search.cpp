#include "c3.h"

int nchoose(int n, int k) {
  int ans = 1;
  double _ans = 1.0;
  for (int i = n - k + 1; i <= n; i ++) {
    ans *= i;
    _ans *= i;
    if (_ans > 1e7) return -1;
  }
  for (int i = 1; i <= k; i ++) {
    ans /= i;
  }
  return ans;
}

void SearchGen::dfs(vector<vector<int>> &pl, int n, int cnt, int rem) {
  if (rem == 0) {
    pl.push_back(tmp);
    return;
  }
  for (int i = cnt; i < n; i ++) {
    tmp.push_back(i);
    dfs(pl, n, i + 1, rem - 1);
    tmp.pop_back();
  }
}

void SearchGen::gen_plan(int n) {
  int tot = 0;
  if (plans.find(n) != plans.end()) return;


  vector<vector<int>> pl;
  for (int k = 0; k <= n; k ++) {
    int c = nchoose(n, k);
    if (k > 4) {
      break;
    }
    if (k > 1 && (c == -1 || tot + c > nvalidate)) {
      break;
    }
    dfs(pl, n, 0, k);
    tot += c;
    assert(tot == pl.size());
  }

  plans[n] = pl;
}

SearchGen::SearchGen() {
  vector<DiskOp*> cnt;

  for (DiskOp *io: disk.ops) {
    if (io->is_flush()) {
      gen_plan(cnt.size());
      cnt.clear();
    } else {
      cnt.push_back(io);
    }
  }
  gen_plan(cnt.size());

  flush_id = 0;
  plan_id = 0;
}

DiskImage *SearchGen::cons_ops(vector<DiskOp*> &ops, vector<DiskOp*> &cnt, vector<int> &plan) {
  set<int> S;
  for (int i: plan) {
    S.insert(i);
  }

  for (int i = 0; i < cnt.size(); i ++) {
    if (S.find(i) == S.end()) {
      ops.push_back(cnt[i]);
    }
  }
  DiskImage *img = mount_disk_img(ops);
  assert(img);
  return img;
}

DiskImage *SearchGen::next() {
  int i = 0;

  vector<int> plan;
  vector<DiskOp*> ops, cnt;
  for (DiskOp *io: disk.ops) {
    if (io->is_flush()) {
      if (i == flush_id) {
        vector<vector<int>>&pl = plans[cnt.size()];
        assert(pl.size() > 0);
        plan = pl[plan_id ++];
        if (pl.size() == plan_id) {
          flush_id ++;
          plan_id = 0;
        }
        return cons_ops(ops, cnt, plan);
      }
      for (DiskOp *op: cnt) ops.push_back(op);
      i ++;
      cnt.clear();
    } else {
      cnt.push_back(io);
    }
  }

  vector<vector<int>>&pl = plans[cnt.size()];
  if (plan_id >= pl.size()) return NULL;
  plan = pl[plan_id ++];
  return cons_ops(ops, cnt, plan);
}
