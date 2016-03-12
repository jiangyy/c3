#ifndef __COMMON_H__
#define __COMMON_H__

// This header is shared by the device driver and the application-level
// crash consistency validation program.

#define NR_DISK    1                  // # of virtual disks
#define DISK_CAP   (16 * 1024 * 1024) // capacity in bytes
#define NR_SECTORS (DISK_CAP / 512)   // capacity in sectors

// magic numbers!
#define IOCTL_TXBEGIN  5288
#define IOCTL_TXEND    6088

#define OP_WRITE 1
#define OP_FLUSH 2

struct log_disk {
  unsigned long nbytes;
  char *data;
  struct log_io *log;
};

struct log_io {
  int flags;
  unsigned long offset, nbytes;
  char *data;
  struct log_io *next;
};

#endif
