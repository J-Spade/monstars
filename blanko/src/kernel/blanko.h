#include <linux/init.h>
#include <linux/module.h>

MODULE_AUTHOR("monstars");
MODULE_DESCRIPTION("nothin but net");
MODULE_LICENSE("GPL");

// *** magic value to stamp over ***
static const char c_user_exe_path[255] = "BASKETBALLJONES";

int __init init_module(void);
void __exit cleanup_module(void);