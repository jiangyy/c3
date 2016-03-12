#include "c3disk.h"

// reorder-buffer
// not really used for reordering, rather, to keep faithful log of I/O operations

void rob_push(struct disk_op *op) {
  struct c3disk_dev *dev = op->dev;
  if (dev->in_tx) {
    list_add_tail(&op->list, &dev->rob_queue);
  } else {
    if (op->data) {
      kfree(op->data);
    }
    kfree(op);
  }
}

void rob_flush(struct c3disk_dev *dev) {
  struct disk_op *op = kmalloc(sizeof(struct disk_op), GFP_ATOMIC);
  if (op) {
    op->flags = OP_FLUSH;
    op->dev = dev;
    op->offset = 0;
    op->nbytes = 0;
    op->data = NULL;
    rob_push(op);
  }
}

void rob_txbegin(struct c3disk_dev *dev, long arg) {
  rob_txend(dev, 0);
  printk(KERN_ALERT "--- TXBEGIN ---");
  memcpy(dev->snapshot, dev->data, dev->size);
}


static void rob_parse_log(struct c3disk_dev *dev, u8 *uptr) {
  struct list_head *pos;
  list_for_each(pos, &dev->rob_queue) {
    struct disk_op *op = list_entry(pos, struct disk_op, list);
    if (op->flags & OP_FLUSH) {
      printk(KERN_ALERT " > --------------------\n");
    } else {
      printk(KERN_ALERT " > [+%ld] x %d %02x%02x%02x...\n", op->offset / 512, op->nbytes / 512, op->data[0] & 0xff, op->data[1] & 0xff, op->data[2] & 0xff);
    }
  }

  if (uptr != NULL) {
    // dump everything to user space!
    struct log_disk ld;
    u8 *snapshot_addr = uptr + sizeof(struct log_disk);
    u8 *first_log_addr = snapshot_addr + dev->size;
    u8 *uptr_old = uptr;

    ld.nbytes = dev->size;
    ld.data = snapshot_addr;
    ld.log = (void*)first_log_addr;

    if (uptr) copy_to_user(uptr, &ld, sizeof(struct log_disk));
    if (uptr) copy_to_user(snapshot_addr, dev->snapshot, dev->size);
    uptr = first_log_addr;
    list_for_each(pos, &dev->rob_queue) {
      struct disk_op *op = list_entry(pos, struct disk_op, list);
      struct log_io io;
      io.flags = op->flags;
      io.offset = op->offset;
      io.nbytes = op->nbytes;
      io.data = uptr + sizeof(struct log_io);
      if (op->list.next == &dev->rob_queue) {
        io.next = NULL;
      } else {
        io.next = (struct io_request*)(io.data + io.nbytes);
      }
      if (uptr) copy_to_user(uptr, &io, sizeof(struct log_io));
      uptr += sizeof(struct log_io);
      if (uptr) copy_to_user(uptr, op->data, op->nbytes);
      uptr += op->nbytes;
    }
    printk(KERN_ALERT " DUMP SIZE: %d\n", uptr - uptr_old);
  }
}

void rob_txend(struct c3disk_dev *dev, long arg) {
  struct list_head *pos;
  u8 *uptr = (u8*)arg;

  // arg is a pointer to hold the entire log
  rob_parse_log(dev, uptr);

  // free each disk_op's content
  list_for_each(pos, &dev->rob_queue) {
    struct disk_op *op = list_entry(pos, struct disk_op, list);
   if (op->data) {
      kfree(op->data);
    }
  }

  // free the disk_op structures
  while (!list_empty(&dev->rob_queue)) {
    struct disk_op *op = list_entry(dev->rob_queue.next, struct disk_op, list);
    list_del(&op->list);
    kfree(op);
  }

  printk(KERN_ALERT "--- TXEND ---");
}
