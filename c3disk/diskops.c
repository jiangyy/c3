#include "c3disk.h"

int c3disk_open(struct block_device *n, fmode_t mode) {
  printk(KERN_ALERT "disk open\n");
  return 0;
}

void c3disk_release(struct gendisk *n, fmode_t mode) {
  printk(KERN_ALERT "disk release\n");
}

// TODO: support nested TXs
int c3disk_ioctl (struct block_device *bd, fmode_t mode, unsigned int cmd, unsigned long arg) {
  struct c3disk_dev *dev = bd->bd_disk->private_data;
  spin_lock(&dev->mutex);
  switch (cmd) {
    case IOCTL_TXBEGIN:
      dev->in_tx = true;
      rob_txbegin(dev, arg);
      break;
    case IOCTL_TXEND:
      dev->in_tx = false;
      rob_txend(dev, arg);
      break;
  }
  spin_unlock(&dev->mutex);
  return 0;
}

// TODO: concurrency control, two threads concurrently write same block
static void c3disk_transfer(struct c3disk_dev *dev, off_t offset, unsigned long nbytes, u8 *buffer, int iswrite) {
  if ((offset + nbytes) <= dev->size) {
    // TODO: wierd, only GFP_ATOMIC allocation worked here
    // other flags, or vmalloc(), would cause system hang with no log.
    // Works fine for now.
    printk(KERN_ALERT "  %s [#%ld] %ld\n", iswrite ? "WRITE" : "READ", offset / 512, nbytes);
    
    if (iswrite) {
      struct disk_op *op = kmalloc(sizeof(struct disk_op), GFP_ATOMIC);
      op->flags = OP_WRITE;
      op->dev = dev;
      op->offset = offset;
      op->nbytes = nbytes;
      op->data = kmalloc(nbytes, GFP_ATOMIC);
      if (op->data) {
        memcpy(op->data, buffer, nbytes); // for reordering
      } else {
        printk(KERN_ALERT " [ERROR] allocation shadow block failed!\n");
      }
      rob_push(op);

      // this is the non-reordered disk image (ground truth).
      // and is only used for responding to read requests
      memcpy(dev->data + offset, buffer, nbytes); // for a consistent view
    } else {
      memcpy(buffer, dev->data + offset, nbytes);
    }
  } else {
    printk(KERN_ALERT "  %ld[+%ld] out of range.\n", offset, nbytes);
  }
}

static void c3disk_flush(struct c3disk_dev *dev) {
  rob_flush(dev);
}


// Kernel Document says:
//   The REQ_FLUSH flag can be OR ed into the r/w flags of a bio submitted from
//   the filesystem and will make sure the volatile cache of the storage device
//   has been flushed before the actual I/O operation is started.
// This is the only guarantee provied by c3disk: REQ_FLUSH enforces barrier
void c3disk_request_fn(struct request_queue *q) {
  struct request *req;
  struct c3disk_dev *dev = q->queuedata;
  pid_t pid = task_pid_nr(current);

  spin_lock(&dev->mutex);
  while ( (req = blk_fetch_request(q)) != NULL ) {
    unsigned long flags = req->cmd_flags;
    struct bio_vec bv;
    struct req_iterator it;
    sector_t start_sector = blk_rq_pos(req);
    sector_t sector_offset = 0;
    int iswrite = rq_data_dir(req) == WRITE;
    int isflush = !!(flags & REQ_FLUSH);
    int isfua   = !!(flags & REQ_FUA);

    if (isflush) {
      c3disk_flush(dev);
      printk(KERN_ALERT "--------------------\n");
    }

    printk(KERN_ALERT "[%d] disk request: %lx\n", pid, flags);
    rq_for_each_segment(bv, req, it) {
      u8 *buffer = page_address(bv.bv_page) + bv.bv_offset;
      int sectors = bv.bv_len / 512;
      unsigned long offset = (start_sector + sector_offset) * 512;
      unsigned long nbytes = sectors * 512;

      c3disk_transfer(dev, offset, nbytes, buffer, iswrite);
      sector_offset += sectors;
    }
    __blk_end_request_all(req, 0);

    if (isfua) {
      c3disk_flush(dev);
      printk(KERN_ALERT "====================\n");
    }
  }
  spin_unlock(&dev->mutex);
}
