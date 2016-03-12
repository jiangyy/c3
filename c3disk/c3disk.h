#ifndef __C3DISK_H__
#define __C3DISK_H__

#include <linux/fs.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/vmalloc.h>

#include "../c3/common.h"

// adversarial disk
struct c3disk_dev {
  int size; // # sectors
  u8* data; // the actural disk

  spinlock_t queue_lock;
  spinlock_t mutex;

  bool in_tx; // are we subject to recording disk operations?
  u8* snapshot; // snapshot of transaction begin

  struct gendisk *gd;
  struct request_queue *queue;
  struct list_head rob_queue;
};
extern struct c3disk_dev disks[NR_DISK];

// disk operations
int c3disk_open (struct block_device *, fmode_t);
void c3disk_release (struct gendisk*, fmode_t);
int c3disk_ioctl (struct block_device*, fmode_t, unsigned int, unsigned long);

// queue operation
void c3disk_request_fn(struct request_queue *q);

struct disk_op {
  int flags;

  struct c3disk_dev *dev;
  off_t offset;
  size_t nbytes;
  u8 *data;

  struct list_head list; // link a queue of disk operations
};

void rob_push(struct disk_op *);
void rob_flush(struct c3disk_dev *);
void rob_txbegin(struct c3disk_dev *, long);
void rob_txend(struct c3disk_dev *, long);

#endif
