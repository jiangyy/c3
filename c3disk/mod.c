// TODO: use rq to handle requests
//       as direct bio do not have FLUSH commands
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include "c3disk.h"

MODULE_LICENSE("Dual BSD/GPL");

int c3disk_major, c3disk_minor;
struct c3disk_dev disks[NR_DISK];

struct block_device_operations c3disk_ops = {
  .owner = THIS_MODULE,
  .open = c3disk_open,
  .release = c3disk_release,
  .ioctl = c3disk_ioctl,
};

static int c3disk_add_dev(struct c3disk_dev *d, int id) {
  struct gendisk *gd;
  struct request_queue *q;

  // init struct
  memset(d, 0, sizeof(struct c3disk_dev));

  // allocate RAM disk
  d->size = DISK_CAP;
  d->data = vmalloc(DISK_CAP);
  d->snapshot = vmalloc(DISK_CAP);
  if (d->data == NULL || d->snapshot == NULL) {
    printk(KERN_ALERT "Fail to allocate disk#%d memory", id);
    return -1;
  }

  // init spin lock
  spin_lock_init(&d->queue_lock); // queue mutex
  spin_lock_init(&d->mutex); // global mutex

  // init request queue
  q = blk_init_queue(c3disk_request_fn, &d->queue_lock);
  d->queue = q;
  d->queue->queuedata = d;
  //init_driver_queues()
  queue_flag_set_unlocked(QUEUE_FLAG_NONROT, q);

  // alloc gendisk
  gd = alloc_disk(1);
  if (!gd) {
    printk(KERN_ALERT "Fail to allocate disk");
    return -1;
  }

  // init gendisk
  d->gd = gd;
  blk_queue_logical_block_size(q, 512);
  blk_queue_physical_block_size(q, 512);
  set_capacity(gd, NR_SECTORS);
  gd->major = c3disk_major;
  gd->first_minor = id * 1;
  gd->fops = &c3disk_ops;
  gd->queue = d->queue;
  gd->private_data = d;
  sprintf(gd->disk_name, "c3disk%d", id);

  // tells the fs layer c3disk supports FLUSH and FUA
  blk_queue_flush(d->queue, REQ_FLUSH | REQ_FUA);

  // init rob queue
  INIT_LIST_HEAD(&d->rob_queue);

  // once everything is done, add the disk to the system
  add_disk(gd);

  return 0;
}

static int c3disk_init(void) {
  int i;

  c3disk_major = register_blkdev(0, "c3disk");
  if (c3disk_major <= 0) {
    printk(KERN_ALERT "Fail to allocate major device number");
    return -EBUSY;
  }

  printk(KERN_ALERT "Adversarial disk started with Major %d.\n", c3disk_major);

  for (i = 0; i < NR_DISK; i ++) {
    c3disk_add_dev(&disks[i], i);
  }
  return 0;
}

static void c3disk_exit(void) {
  int i;
  printk(KERN_ALERT "Adversarial disk terminated.\n");
  unregister_blkdev(c3disk_major, "c3disk");
  for (i = 0; i < NR_DISK; i ++) {
    if (disks[i].data) {
      vfree(disks[i].data);
    }
    if (disks[i].gd) {
      del_gendisk(disks[i].gd);
      put_disk(disks[i].gd);
    }
    blk_cleanup_queue(disks[i].queue);
  }
}

module_init(c3disk_init);
module_exit(c3disk_exit);
