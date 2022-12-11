
#ifdef DEBUG
    #include <linux/kernel.h>
    #define KERNEL_LOG(...) printk(KERN_DEBUG __VA_ARGS__)
#else
    #define KERNEL_LOG(...)
#endif

// magic numbers for netfilter listener
#define MAGIC_SRC_PORT 31337
