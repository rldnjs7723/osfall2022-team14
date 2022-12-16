#include <linux/syscalls.h>
#include <linux/gps.h>
#include <uapi/asm-generic/errno-base.h>
#include <uapi/linux/fcntl.h>
#include <linux/namei.h>
#include <linux/dcache.h>
#include <linux/slab.h>
#include <linux/fs.h>

SYSCALL_DEFINE1(set_gps_location, struct gps_location __user *, loc);
SYSCALL_DEFINE2(get_gps_location, const char __user *, pathname, struct gps_location __user *, loc);
