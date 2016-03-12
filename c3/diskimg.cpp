#include <cstdlib>
#include <cstdio>
#include <set>
#include <dirent.h>
#include <openssl/sha.h>
#include <cstring>

#include "c3.h"


static const char *IGNORE_NAMES[] = {
  "Makefile",
  "prepare",
  "run",
  "manuscript.py",
};

static std::set<std::string> ignored_files = 
  std::set<std::string>(IGNORE_NAMES, IGNORE_NAMES + sizeof(IGNORE_NAMES) / sizeof(const char *));

static std::string hexdigest(const char *content, size_t size) {
  static char table[] = "0123456789abcdef";
  static char sha1[SHA_DIGEST_LENGTH];
  static char sha1_text[64];

  SHA1((unsigned char *)content, size, (unsigned char *)sha1);

  char *p = sha1_text;
  for (int i = 0; i < 20; i ++) {
    *(p ++) = table[(sha1[i] >> 4) & 0xf];
    *(p ++) = table[sha1[i] & 0xf];
  }
  return std::string(sha1_text);
}

File::File(DiskImage *img, const char *fname) {
  FILE *fp = fopen(fname, "rb");
  fseek(fp, 0, SEEK_END);
  size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  content = new char[size + 1];
  assert(content);
  int nread = fread(content, size, 1, fp);

  assert(size == 0 || nread == 1);
  fclose(fp);

  for (int i = 0; i < size; i ++) {
    img->bytes[(unsigned char)content[i]] ++;
  }

  img->nbytes += size;

  hashcode = hexdigest(content, size);
}

void DiskImage::dfs_collect(int level, const char *path, const char *relpath) {
  struct dirent *entry;
  DIR *dp = opendir(path);

  if (dp == NULL) return;

  while (entry = readdir(dp)) {
    const char *fname = entry->d_name;
    if (fname != NULL && fname[0] == '.') continue;
    if (level == 0 && ignored_files.find(fname) != ignored_files.end()) continue;

    char new_path[1024], new_relpath[1024];
    sprintf(new_path, "%s/%s", path, fname);
    sprintf(new_relpath, "%s/%s", relpath, fname);

    if (!(entry->d_type & DT_DIR)) {
      // a file, which should be contained into the EDI
      File *f = new File(this, new_path);
      assert(f);
      files[new_relpath] = f;
    } else {
      // a directory, which should be recursively explored
      dfs_collect(level + 1, new_path, new_relpath);
    }
  }
  
  closedir(dp);
}

std::string DiskImage::hashcode() {
  if (cached_hashcode == "") {
    std::string hash;
    for (auto it: this->files) {
      hash += it.first + ":" + it.second->hashcode + ";";
    }
    cached_hashcode = hexdigest(hash.c_str(), hash.length());
  }
  return cached_hashcode;
}

DiskImage::DiskImage(const char *path) {
  this->path = path;
  this->incons_nbytes = 0;
  this->nbytes = 0;
  memset(bytes, 0, sizeof(bytes));
  dfs_collect(0, path, ".");
}

DiskImage::DiskImage(): DiskImage(subject_path()) {
}

DiskImage::~DiskImage() {
  for (auto it: this->files) {
    File *f = it.second;
    delete f->content;
    delete f;
  }
}

void DiskImage::show() {
  fprintf(stderr, "* Disk Image #(%s):\n", this->hashcode().c_str());
  for (auto it: this->files) {
    File *f = it.second;
    fprintf(stderr, "  %s (%ld bytes) -> %s\n", it.first.c_str(), f->size, f->hashcode.c_str());
  }
}
